#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include "ballistics.hpp"
#include "targeting.hpp"
#include "utils.hpp"
#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

class EnemyAI {

public:
  EnemyAI(Coordinator &ecs, Entity enemy, BulletFactory bulletFactory, TorpedoFactory torpedoFactory, sf::Sound pdcFireSoundPlayer) :
    ecs(ecs), 
    enemy(enemy),
    bulletFactory(bulletFactory),
    torpedoFactory(torpedoFactory),
    pdcFireSoundPlayer(pdcFireSoundPlayer),
    pdcTarget(ecs, enemy, bulletFactory, pdcFireSoundPlayer) {

    std::cout << "EnemyAI created" << std::endl;
  }
  ~EnemyAI() = default;

  void Update(float tt, float dt)  {
    // update state based on player distance
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyVel = ecs.getComponent<Velocity>(enemy);
    auto &enemyAcc = ecs.getComponent<Acceleration>(enemy);
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);
    auto &playerPos = ecs.getComponent<Position>(0);

    float dist = distance(enemyPos.value, playerPos.value);

    float atp = angleToTarget(enemyPos.value, playerPos.value);

    // std::cout << "EnemyAI distance to player: " << dist << std::endl;
    // std::cout << "\nEnemyAI angle to player: " << atp << std::endl;



    ///////////////////////////////////////////////////////////////////////////////
    // - Ship State Machine -
    ///////////////////////////////////////////////////////////////////////////////
    if (pdcTarget.pdcTorpedoThreatDetect()) {
      state = State::DEFENCE_PDC;
      std::cout << "EnemyAI state: DEFENCE_PDC" << std::endl;
    }
    else
    {
      if (state == State::DEFENCE_PDC) {
        // if we are in defence mode, we want to stop defending if there are no threats
        std::cout << "EnemyAI state: no threats, switching to IDLE" << std::endl;
        state = State::IDLE;
      }
      else if (state == State::CLOSE) {
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
    }

    ///////////////////////////////////////////////////////////////////////////////
    // - Ship Control -
    ///////////////////////////////////////////////////////////////////////////////
    if (state == State::DEFENCE_PDC) {
      // set accel to 0
      // probably want to set a target velocity instead
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f;

      // only want to turn the ship if we are not already turning, prevents jittering
      // turn to player for now
      if (shipControl.turning == false) {
        startTurn(atp);
      }

      // aquire the nearest torpedo and fire the PDCs
      pdcTarget.pdcDefendTorpedo(tt, dt);
    }
    else if (state == State::CLOSE) {
      shipControl.targetPosition = playerPos.value;
      //shipControl.targetAcceleration = enemyAcc.value;
     
      // only want to turn the ship if we are not already turning, prevents jittering
      if (shipControl.turning == false) {
        startTurn(atp);
      }

      // accelerate towards the player, about 1G atm
      // TODO: update to change acceleration based on distance
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
      float diff = normalizeAngle(atp - enemyRot.angle);

      // fire when facing the player, +/- 5 degrees
      if (diff >= -05.f && diff <= +05.f) {
        if (tt > launcher1.timeSinceFired + launcher1.cooldown && launcher1.rounds) {
          launcher1.timeSinceFired = tt;
          torpedoFactory.fireone<TorpedoLauncher1>(enemy, 0);
          launcher1.rounds--;
          std::cout << "EnemyAI firing TorpedoLauncher1" << std::endl;
        }
        if (tt > launcher2.timeSinceFired + launcher2.cooldown && launcher2.rounds) {
          launcher2.timeSinceFired = tt;
          torpedoFactory.fireone<TorpedoLauncher2>(enemy, 0);
          launcher2.rounds--;
          std::cout << "EnemyAI firing TorpedoLauncher2" << std::endl;
        }
      }
    }
    else if (state == State::ATTACK_PDC) {

      auto &enemyRot = ecs.getComponent<Rotation>(enemy);
      float diff = atp - enemyRot.angle;

      if (diff >= 180.f) {
        diff -= 360.f;
      } else if (diff < -180.f) {
        diff += 360.f;
      }

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
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f; 
      
#if 0
      // set a target velocity instead
      enemyVel.value.y = std::sin((enemyRot.angle) * (M_PI / 180.f)) * 1000.f;
      enemyVel.value.x = std::cos((enemyRot.angle) * (M_PI / 180.f)) * 1000.f; 
#else 
      enemyVel.value.y = 0.f;
      enemyVel.value.x = 0.f;
#endif

      // for now, attack the player. 
      // Could additional evasive maneuvers later.
      pdcTarget.setTarget(0);
      pdcTarget.aquireTargets();
      pdcTarget.pdcAttack(tt);
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
  PdcTarget pdcTarget;

  const float close_distance          = 1000000.f;
  const float attack_pdc_distance     = 8000.f;
  const float attack_torpedo_distance = 900000.f;

  enum class State {
    IDLE,
    CLOSE,
    ATTACK_PDC,
    DEFENCE_PDC,
    ATTACK_TORPEDO,
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
};

