#pragma once
#include "ballistics.hpp"
#include "components.hpp"
#include "ecs.hpp"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>

class TorpedoAI {

public:
  TorpedoAI(Coordinator &ecs) : ecs(ecs) {
    std::cout << "TorpedoAI created" << std::endl;
  }
  ~TorpedoAI() = default;

  void Update(float tt, float dt) {

    // need to get all the torpedos, find their targets and turn towards them
    // FIX: watch out for pdcs that have a target aswell. 
    for (auto &torpedo :
         ecs.view<Position, Velocity, Acceleration, Rotation, Target>()) {

      auto &torpedoAcc = ecs.getComponent<Acceleration>(torpedo).value;
      auto &torpedoVel = ecs.getComponent<Velocity>(torpedo).value;
      auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);
      auto &torpedoPos = ecs.getComponent<Position>(torpedo);
      Entity target = ecs.getComponent<Target>(torpedo).target;
      auto &targetPos  = ecs.getComponent<Position>(target);

      // get the angle to the target
      float atp = angleToPlayer(torpedoPos.value, targetPos.value);

      std::cout << "TorpedoAI angle to target: " << atp << std::endl;

      if (torpedoVel.x == 0.f && torpedoVel.y == 0.f) {
        std::cout << "Torpedo Velocity: " << 0 << "\n";
      }
      else {
        std::cout << "Torpedo Velocity: " << torpedoVel.length() << ", angle: " << torpedoVel.angle().asDegrees() << "\n";
      }

      if (torpedoAcc.x == 0.f && torpedoVel.y == 0.f) {
        std::cout << "Torpedo Acc: " << 0 << "\n";
      }
      else {
        std::cout << "Torpedo Acc: " << torpedoAcc.length() << ", angle: " << torpedoAcc.angle().asDegrees() << "\n";
      }

      if (torpedoControl.turning == false) {
        startTurn(atp, torpedo, target);
      }

      // perform the turn
      if (torpedoControl.turning) {
        performTurn(torpedo, target);
      }

      // accelerate towards the target 
      // TODO: update to change acceleration based on distance
      // TODO: not getting torpedo acc from launcher atm
      torpedoAcc.y = std::sin((torpedoRot.angle) * (M_PI / 180.f)) * 1000.f;
      torpedoAcc.x = std::cos((torpedoRot.angle) * (M_PI / 180.f)) * 1000.f;
    }
  }

private:
  Coordinator &ecs;

  struct TorpedoControl {
    bool turning = false;
    bool burning = false;
    float targetAngle = 0.f;
    sf::Vector2f targetPosition;
    sf::Vector2f targetAcceleration;
    enum class RotationDirection { CLOCKWISE, COUNTERCLOCKWISE };
    RotationDirection rotationDir = RotationDirection::CLOCKWISE;
  };

  TorpedoControl torpedoControl;

  void startTurn(float atp, Entity torpedo, Entity target) {
    torpedoControl.targetAngle = atp;
    auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);

    // TODO: wrap with Angle.wrapUnsigned
    if (torpedoRot.angle >= 180.f) {
      torpedoRot.angle -= 360.f;
    } else if (torpedoRot.angle < -180.f) {
      torpedoRot.angle += 360.f;
    }

    std::cout << "Starting Turn to " << atp << "\n";

    float diff = torpedoControl.targetAngle - torpedoRot.angle;

    // take into account the wrap around to get the shortest distance
    // TODO this doesnt really work after a collision
    if (diff < -180.f) {
      diff += 360.f;
    } else if (diff > 180.f) {
      diff -= 360.f;
    }

    if (diff > 0.f) {
      torpedoControl.rotationDir = TorpedoControl::RotationDirection::CLOCKWISE;
      torpedoControl.turning = true;
    } 
    else {
      torpedoControl.rotationDir = TorpedoControl::RotationDirection::COUNTERCLOCKWISE;
      torpedoControl.turning = true;
    }
  }

  void performTurn(Entity torpedo, Entity target) {
    auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);
    float diff = torpedoControl.targetAngle - torpedoRot.angle;

    // take into account the wrap around to get the shortest distance
    if (diff < -180.f) {
      diff += 360.f;
    } else if (diff > 180.f) {
      diff -= 360.f;
    }
 
    // std::cout << "Performing Turn to " << torpedoControl.targetAngle << ", Diff " << diff << " current angle " << torpedoRot.angle << "\n";

    // some leeway to ensure we stop
    if (diff >= -20.f && diff <= 20.f) {
      torpedoRot.angle = torpedoControl.targetAngle;
      torpedoControl.turning = false;
      // std::cout << "Turn complete to " << torpedoControl.targetAngle << "\n";
    } 
    else if (torpedoControl.rotationDir == TorpedoControl::RotationDirection::CLOCKWISE) {
      torpedoRot.angle += 15.f; //(window.getSize().x / 100.f);
      // std::cout << "Clockwise turn to " << torpedoRot.angle << "\n";
    }
    else {
      torpedoRot.angle -= 15.f; //(window.getSize().x / 100.f);
      // std::cout << "CounterClockwise turn to " << torpedoRot.angle << "\n";
    }

    // TODO: wrap with Angle.wrapUnsigned
    if (torpedoRot.angle >= 180.f) {
      torpedoRot.angle -= 360.f;
    } else if (torpedoRot.angle < -180.f) {
      torpedoRot.angle += 360.f;
    }
  }

  inline float angleToPlayer(sf::Vector2f source, sf::Vector2f target) {
    float radians = std::atan2(target.y - source.y, target.x - source.x);

    return (radians * 180.f) / M_PI;
  }
};
