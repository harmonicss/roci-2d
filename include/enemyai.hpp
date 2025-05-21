#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>

class EnemyAI {

public:
  EnemyAI(Coordinator &ecs, Entity enemy) : ecs(ecs), enemy(enemy) {
    std::cout << "EnemyAI created" << std::endl;
  }
  ~EnemyAI() = default;

  void Update(float dt) {
    // update state based on player distance
    //
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyAcc = ecs.getComponent<Acceleration>(enemy);
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);
    auto &playerPos = ecs.getComponent<Position>(0);

    float dist = distance(enemyPos.value, playerPos.value);

    float atp = angleToPlayer(enemyPos.value, playerPos.value);

    // std::cout << "EnemyAI distance to player: " << dist << std::endl;
    std::cout << "\nEnemyAI angle to player: " << atp << std::endl;

    if (state == State::CLOSE) {
      if (dist <= 5000.f) {
        state = State::ATTACK_PDC;
        std::cout << "EnemyAI state: ATTACK" << std::endl;
      }
    } else if (state == State::IDLE) {
      if (dist < 20000.f) {
        state = State::CLOSE;
        std::cout << "EnemyAI state: CLOSE" << std::endl;
      }
    } else if (state == State::ATTACK_PDC) {
      if (dist > 5000.f) {
        state = State::CLOSE;
        std::cout << "EnemyAI state: CLOSE" << std::endl;
      }
    }

    if (state == State::CLOSE) {
      shipControl.targetPosition = playerPos.value;
      //shipControl.targetAcceleration = enemyAcc.value;
     
      // only want to turn the ship if we are not already turning, prevents jittering
      if (shipControl.turning == false) {
        startTurn(atp);
      }

      // accelerate towards the player, about 1G atm
      enemyAcc.value.y = std::sin((enemyRot.angle) * (M_PI / 180.f)) * 5000.f * dt;
      enemyAcc.value.x = std::cos((enemyRot.angle) * (M_PI / 180.f)) * 5000.f * dt;
    }
    else if (state == State::IDLE) {
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f;
    }
    else if (state == State::ATTACK_PDC) {

      // playing with setting the heading to an offsest for pdc fire and to avoic collisions
      if (shipControl.turning == false) {
        if (atp > 0.f) {
          startTurn(atp - 45.f);
        } else {
          startTurn(atp + 45.f);
        }
      }

      // set accel to 0
      // probably want to set a target veolcity instead
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f; 
    }
 
    // perform the turn
    if (shipControl.turning) {
      performTurn();
    }
  }

 private:
  Coordinator &ecs;
  Entity enemy;

  enum class State {
    IDLE,
    CLOSE,
    ATTACK_PDC,
    ATTACK_MISSILE,
    EVADE,
    FLEE
  };
 
  State state = State::CLOSE;

  struct ShipControl {
    bool turning = false;
    bool burning = false;
    float targetAngle = 0.f;
    sf::Vector2f targetPosition;
    sf::Vector2f targetAcceleration;
    enum class RotationDirection { CLOCKWISE, COUNTERCLOCKWISE };
    RotationDirection rotationDir = RotationDirection::CLOCKWISE;
  };
  
  ShipControl shipControl;

  void startTurn(float angle) {
    shipControl.targetAngle = angle;
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);

    // TODO: wrap with Angle.wrapUnsigned
    if (enemyRot.angle >= 180.f) {
      enemyRot.angle -= 180.f;
    } else if (enemyRot.angle < -180.f) {
      enemyRot.angle += 180.f;
    }

    std::cout << "Starting Turn to " << angle << "\n";

    float diff = shipControl.targetAngle - enemyRot.angle;

    // take into account the wrap around to get the shortest distance
    // TODO this doesnt really work after a collision
    if (diff < -180.f) {
      diff += 360.f;
    } else if (diff > 180.f) {
      diff -= 360.f;
    }

    if (diff > 0.f) {
      shipControl.rotationDir = ShipControl::RotationDirection::CLOCKWISE;
      shipControl.turning = true;
    } 
    else {
      shipControl.rotationDir = ShipControl::RotationDirection::COUNTERCLOCKWISE;
      shipControl.turning = true;
    }
  }

  void performTurn() {
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);
    float diff = shipControl.targetAngle - enemyRot.angle;

    // take into account the wrap around to get the shortest distance
    if (diff < -180.f) {
      diff += 360.f;
    } else if (diff > 180.f) {
      diff -= 360.f;
    }
 
    std::cout << "Performing Turn to " << shipControl.targetAngle << ", Diff " << diff << " current angle " << enemyRot.angle << "\n";

    // some leeway to ensure we stop
    if (diff >= -20.f && diff <= 20.f) {
      enemyRot.angle = shipControl.targetAngle;
      shipControl.turning = false;
      std::cout << "Turn complete to " << shipControl.targetAngle << "\n";
    } 
    else if (shipControl.rotationDir == ShipControl::RotationDirection::CLOCKWISE) {
      enemyRot.angle += 15.f; //(window.getSize().x / 100.f);
      std::cout << "Clockwise turn to " << enemyRot.angle << "\n";
    }
    else {
      enemyRot.angle -= 15.f; //(window.getSize().x / 100.f);
      std::cout << "CounterClockwise turn to " << enemyRot.angle << "\n";
    }

    // TODO: wrap with Angle.wrapUnsigned
    if (enemyRot.angle >= 180.f) {
      enemyRot.angle -= 180.f;
    } else if (enemyRot.angle < -180.f) {
      enemyRot.angle += 180.f;
    }
  }

  inline float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
  }

  inline float distance(sf::Vector2f source, sf::Vector2f target) {
    return length(target - source);
  }

  inline float angleToPlayer(sf::Vector2f source, sf::Vector2f target) {
    float radians = std::atan2(target.y - source.y, target.x - source.x);

    return (radians * 180.f) / M_PI;
  }
};

