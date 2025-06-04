#pragma once
#include "ballistics.hpp"
#include "components.hpp"
#include "ecs.hpp"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
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
    // FIX: watch out for pdcs that have a target aswell. Maybe create pdcTarget?
    for (auto &torpedo :
         ecs.view<Position, Velocity, Acceleration, Rotation, Target>()) {

      auto &torpedoPos = ecs.getComponent<Position>(torpedo);
      auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);
      auto &torpedoVel = ecs.getComponent<Velocity>(torpedo).value;
      auto &torpedoAcc = ecs.getComponent<Acceleration>(torpedo).value;

      Entity target = ecs.getComponent<Target>(torpedo).target;
      auto &targetPos = ecs.getComponent<Position>(target);
      auto &targetVel = ecs.getComponent<Velocity>(target).value;
      auto &targetAcc = ecs.getComponent<Acceleration>(target).value;

      // get the angle to the target
      float att = angleToTarget(torpedoPos.value, targetPos.value);
 
      std::cout << "\nTorpedoAI torpedo position: " << torpedoPos.value.x << ", " << torpedoPos.value.y << "\n";
      std::cout << "TorpedoAI angle to target: " << att << "\n";
      std::cout << "TorpedoAI torpedo angle: " << torpedoRot.angle << "\n";

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

      float dist = distance(torpedoPos.value, targetPos.value);
      std::cout << "TorpedoAI distance to target: " << dist << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Use 2D proportional navigation to calculate the angle and acceleration needed 
      // to hit the target.
      ///////////////////////////////////////////////////////////////////////////////////
 
      // relative position
      sf::Vector2f p_rel = targetPos.value - torpedoPos.value;

      // relative velocity
      sf::Vector2f v_rel = targetVel - torpedoVel;
      std::cout << "TorpedoAI relative speed: " << v_rel.length() << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Line of sight (angle to target) angular rate is the derivative of the angle
      // used to calculate the lateral acceleration magnitude
      float dot_att = 0.f;

      // too close or division by zero guard
      if (dist > 1e-4)
        // direct kinematic control
        dot_att = (p_rel.x * v_rel.y - p_rel.y * v_rel.x) / (p_rel.x * p_rel.x + p_rel.y * p_rel.y);

      std::cout << "TorpedoAI dot_att: " << dot_att << "\n";


      ////////////////////////////////////////////////////////////////////////////////////
      // Compute closing velocity
      // Vc = -Vrt . unit vector in the direction of L, take the original vector and divide it by
      // its length to get the unit vector
      sf::Vector2f los_unit_vector = p_rel / length(p_rel);

      float Vc = -v_rel.x * los_unit_vector.x - v_rel.y * los_unit_vector.y;

      std::cout << "TorpedoAI closing speed: " << Vc << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Proportional navigation commanded lateral acceleration
      // torpedo applies a lateral acceleration (perpendicular to its velocity vector) of
      // magnitude Acc = N * Vc * |dot_atp|, where N is the proportional navigation constant
      // and Vc is the closing speed
      float Acc_N = 3.f * Vc * std::abs(dot_att); // N = 3 is a common value

      // apply a fudge factor for more acceleration
      //Acc_N *= 800.f;

      // as this is lateral (thruster) acceleration, we need to limit it to a reasonable value
      if (Acc_N > 5000.f) {
        Acc_N = 5000.f; // set a maximum acceleration 
      }
      else if (Acc_N < -5000.f) {
        Acc_N = -5000.f; // set a minimum acceleration
      }

      std::cout << "TorpedoAI commanded lateral acceleration: " << Acc_N << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // resolve the acceleration perpendicular to the velocity vector of the torpedo

      // lateral unit vector of torpedo, in radians
      sf::Vector2f u_t = { static_cast<float>(std::cos(torpedoRot.angle * (M_PI / 180.f))), 
                           static_cast<float>(std::sin(torpedoRot.angle * (M_PI / 180.f))) };

      // "left" normal (rotate 90 degrees CCW) perpendicular vector
      sf::Vector2f perp_t = {-u_t.y, u_t.x};

      // decide sign base on sign of dot_atp
      float sign = (dot_att < 0.f) ? -1.f : 1.f;

      // new acceleration command needed for lateral movement (thrusters)
      sf::Vector2f a_cmd = perp_t * (sign * Acc_N);

      std::cout << "TorpedoAI lateral acceleration length: " << a_cmd.length() << "\n";
      if (a_cmd.length() > 0.f) 
        std::cout << "TorpedoAI lateral acceleration angle: " << a_cmd.angle().asDegrees() << "\n";
      else 
        std::cout << "TorpedoAI lateral acceleration angle: 0\n";
 
      // apply continuous engine thrust, convert to radians
      sf::Vector2f a_engines = { static_cast<float>(std::cos(torpedoRot.angle * (M_PI / 180.f)) * 1000.f),
                                 static_cast<float>(std::sin(torpedoRot.angle * (M_PI / 180.f)) * 1000.f) };

      std::cout << "TorpedoAI engine acceleration length: " << a_engines.length() << "\n";
      if (a_engines.length() > 0.f) 
        std::cout << "TorpedoAI engine acceleration angle: " << a_engines.angle().asDegrees() << "\n";
      else 
        std::cout << "TorpedoAI engine acceleration angle: 0\n";

      // set the total acceleration
      torpedoAcc = a_engines + a_cmd;

      std::cout << "TorpedoAI commanded acceleration length: " << torpedoAcc.length() << "\n";
      if (torpedoAcc.length() > 0.f)
        std::cout << "TorpedoAI commanded acceleration angle: " << torpedoAcc.angle().asDegrees() << "\n";
      else 
        std::cout << "TorpedoAI commanded acceleration angle: 0\n";

      // turn towards the target
      if (torpedoControl.turning == false) {
        startTurn(att, torpedo, target);
      }

      // perform the turn
      if (torpedoControl.turning) {
        performTurn(torpedo, target);
      }

      // get the direction/ angle of this acceleration
      // float new_angle = std::atan2(torpedoAcc.y, torpedoAcc.x) * (180.f / M_PI);


      // accelerate towards the target 
      // TODO: update to change acceleration based on distance
      // TODO: not getting torpedo acc from launcher atm
      // torpedoAcc.y = std::sin((torpedoRot.angle) * (M_PI / 180.f)) * 1000.f;
      // torpedoAcc.x = std::cos((torpedoRot.angle) * (M_PI / 180.f)) * 1000.f;
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
 
    std::cout << "Performing Turn to " << torpedoControl.targetAngle << ", Diff " << diff << " current angle " << torpedoRot.angle << "\n";

    // some leeway to ensure we stop
    if (diff >= -20.f && diff <= 20.f) {
      torpedoRot.angle = torpedoControl.targetAngle;
      torpedoControl.turning = false;
      std::cout << "Turn complete to " << torpedoControl.targetAngle << "\n";
    } 
    else if (torpedoControl.rotationDir == TorpedoControl::RotationDirection::CLOCKWISE) {
      torpedoRot.angle += 15.f; //(window.getSize().x / 100.f);
      std::cout << "Clockwise turn to " << torpedoRot.angle << "\n";
    }
    else {
      torpedoRot.angle -= 15.f; //(window.getSize().x / 100.f);
      std::cout << "CounterClockwise turn to " << torpedoRot.angle << "\n";
    }

    // TODO: wrap with Angle.wrapUnsigned
    if (torpedoRot.angle >= 180.f) {
      torpedoRot.angle -= 360.f;
    } else if (torpedoRot.angle < -180.f) {
      torpedoRot.angle += 360.f;
    }
  }

  inline float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
  }

  inline float distance(sf::Vector2f source, sf::Vector2f target) {
    return length(target - source);
  }

  inline float angleToTarget(sf::Vector2f source, sf::Vector2f target) {
    float radians = std::atan2(target.y - source.y, target.x - source.x);

    return (radians * 180.f) / M_PI;
  }
};
