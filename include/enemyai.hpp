#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include "ballistics.hpp"
#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

class EnemyAI {

public:
  EnemyAI(Coordinator &ecs, Entity enemy, BulletFactory bulletFactory, TorpedoFactory torpedoFactory, sf::Sound pdcFireSoundPlayer ) : 
    ecs(ecs), 
    enemy(enemy),
    bulletFactory(bulletFactory),
    torpedoFactory(torpedoFactory),
    pdcFireSoundPlayer(pdcFireSoundPlayer) {
  
    std::cout << "EnemyAI created" << std::endl;
  }
  ~EnemyAI() = default;

  void Update(float tt, float dt)  {
    // update state based on player distance
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyAcc = ecs.getComponent<Acceleration>(enemy);
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);
    auto &playerPos = ecs.getComponent<Position>(0);

    float dist = distance(enemyPos.value, playerPos.value);

    float atp = angleToPlayer(enemyPos.value, playerPos.value);

    // std::cout << "EnemyAI distance to player: " << dist << std::endl;
    // std::cout << "\nEnemyAI angle to player: " << atp << std::endl;

    ///////////////////////////////////////////////////////////////////////////////
    // - Ship State Machine -
    ///////////////////////////////////////////////////////////////////////////////
    if (state == State::CLOSE) {
      if (dist <= attack_torpedo_distance) {
        state = State::ATTACK_TORPEDO;
        std::cout << "EnemyAI state: ATTACK_TORPEDO" << std::endl;
      }
    }
    else if (state == State::IDLE) {
      if (dist < close_distance) {
        state = State::CLOSE;
        std::cout << "EnemyAI state: CLOSE" << std::endl;
      }
    }
    else if (state == State::ATTACK_TORPEDO) {
      if (dist < attack_pdc_distance) {
        state = State::ATTACK_PDC;
        std::cout << "EnemyAI state: ATTACK_PDC" << std::endl;
      } 
      else if (dist > attack_torpedo_distance) {
        state = State::CLOSE;
        std::cout << "EnemyAI state: CLOSE" << std::endl;
      }
    }
    else if (state == State::ATTACK_PDC) {
      if (dist > attack_pdc_distance) {
        state = State::ATTACK_TORPEDO;
        std::cout << "EnemyAI state: ATTACK_TORPEDO" << std::endl;
      }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // - Ship Control -
    ///////////////////////////////////////////////////////////////////////////////
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
    else if (state == State::ATTACK_TORPEDO) {
      auto &launcher1 = ecs.getComponent<TorpedoLauncher1>(enemy);
      auto &launcher2 = ecs.getComponent<TorpedoLauncher2>(enemy);

      // set accel to 0
      // probably want to set a target velocity instead
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f;

      // only want to turn the ship if we are not already turning, prevents jittering
      if (shipControl.turning == false) {
        startTurn(atp);
      }

      auto &enemyRot = ecs.getComponent<Rotation>(enemy);
      float diff = atp - enemyRot.angle;

      if (diff >= -05.f && diff <= +05.f) {
        if (tt > launcher1.timeSinceFired + launcher1.cooldown && launcher1.rounds) {
          launcher1.timeSinceFired = tt;
          torpedoFactory.fire<TorpedoLauncher1>(enemy);
          launcher1.rounds--;
          std::cout << "EnemyAI firing TorpedoLauncher1" << std::endl;
        }
        if (tt > launcher2.timeSinceFired + launcher2.cooldown && launcher2.rounds) {
          launcher2.timeSinceFired = tt;
          torpedoFactory.fire<TorpedoLauncher2>(enemy);
          launcher2.rounds--;
          std::cout << "EnemyAI firing TorpedoLauncher2" << std::endl;
        }
      }
    }
    else if (state == State::ATTACK_PDC) {

      auto &enemyRot = ecs.getComponent<Rotation>(enemy);
      float diff = atp - enemyRot.angle;
      // playing with setting the heading to an offsest for pdc fire and to avoid collisions
      // +/- 45 degrees will miss if the player is moving, so try 35
      // probably need to take into account target velocity instead for targeting
      if (shipControl.turning == false) {
        if (diff > 0.f) {
          startTurn(atp - 50.f);
        } else {
          startTurn(atp + 50.f);
        }
      }

      // set accel to 0
      // probably want to set a target velocity instead
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f; 

      if (tt > shipControl.timeSinceBurst + shipControl.pdcBusrtCooldown) {
        shipControl.timeSinceBurst = tt;
        shipControl.pdcBurst = 15;
      }

      if (shipControl.pdcBurst > 0) {
        // fire pdc, in attacking range
        // std::cout << "EnemyAI firing diff: " << diff << std::endl;
        if (diff >= -55.f && diff <= -20.f) {
          auto &pdc1 = ecs.getComponent<Pdc1>(enemy);
          if (pdc1.rounds != 0 && shipControl.pdcBurst != 0) {
            if (tt > pdc1.timeSinceFired + pdc1.cooldown) {
              pdc1.timeSinceFired = tt;
              bulletFactory.fire<Pdc1>(enemy);
              pdc1.rounds--;
              shipControl.pdcBurst--;
              pdcFireSoundPlayer.play();
              std::cout << "EnemyAI firing PDC1" << std::endl;
            }
          }
        }
        else if (diff >= 20.f && diff <= 55.f) {
          auto &pdc2 = ecs.getComponent<Pdc2>(enemy);
          if (pdc2.rounds != 0 && shipControl.pdcBurst != 0) {
            if (tt > pdc2.timeSinceFired + pdc2.cooldown) {
              pdc2.timeSinceFired = tt;
              bulletFactory.fire<Pdc2>(enemy);
              pdc2.rounds--;
              shipControl.pdcBurst--;
              pdcFireSoundPlayer.play();
              std::cout << "EnemyAI firing PDC2" << std::endl;
            }
          }
        }
      }
    }
 
    // perform the turn
    if (shipControl.turning) {
      performTurn();
    }
  }

 private:
  Coordinator &ecs;
  Entity enemy;
  BulletFactory bulletFactory;
  TorpedoFactory torpedoFactory;
  sf::Sound pdcFireSoundPlayer;

  const float close_distance          = 100000.f;
  const float attack_pdc_distance     = 8000.f;
  const float attack_torpedo_distance = 50000.f;

  enum class State {
    IDLE,
    CLOSE,
    ATTACK_PDC,
    ATTACK_TORPEDO,
    EVADE,
    FLEE
  };
 
  State state = State::CLOSE;

  struct ShipControl {
    bool turning = false;
    bool burning = false;
    float targetAngle = 0.f;
    uint32_t pdcBurst = 0;
    float timeSinceBurst = 0.f;
    float pdcBusrtCooldown = 4.f;
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
      enemyRot.angle -= 360.f;
    } else if (enemyRot.angle < -180.f) {
      enemyRot.angle += 360.f;
    }

    // std::cout << "Starting Turn to " << angle << "\n";

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
 
    // std::cout << "Performing Turn to " << shipControl.targetAngle << ", Diff " << diff << " current angle " << enemyRot.angle << "\n";

    // some leeway to ensure we stop
    if (diff >= -20.f && diff <= 20.f) {
      enemyRot.angle = shipControl.targetAngle;
      shipControl.turning = false;
      // std::cout << "Turn complete to " << shipControl.targetAngle << "\n";
    } 
    else if (shipControl.rotationDir == ShipControl::RotationDirection::CLOCKWISE) {
      enemyRot.angle += 15.f; //(window.getSize().x / 100.f);
      // std::cout << "Clockwise turn to " << enemyRot.angle << "\n";
    }
    else {
      enemyRot.angle -= 15.f; //(window.getSize().x / 100.f);
      // std::cout << "CounterClockwise turn to " << enemyRot.angle << "\n";
    }

    // TODO: wrap with Angle.wrapUnsigned
    if (enemyRot.angle >= 180.f) {
      enemyRot.angle -= 360.f;
    } else if (enemyRot.angle < -180.f) {
      enemyRot.angle += 360.f;
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

