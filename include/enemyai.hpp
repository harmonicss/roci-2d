#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include "ballistics.hpp"
#include "pdctarget.hpp"
#include "utils.hpp"
#include <cstdint>
#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unistd.h>

class EnemyAI {

public:
  EnemyAI(Coordinator &ecs, Entity enemy, BulletFactory bulletFactory,
          TorpedoFactory torpedoFactory, sf::Sound pdcFireSoundPlayer) :
    ecs(ecs),
    enemy(enemy),
    bulletFactory(bulletFactory),
    torpedoFactory(torpedoFactory),
    pdcFireSoundPlayer(pdcFireSoundPlayer),
    pdcTargeting(ecs, enemy, bulletFactory, pdcFireSoundPlayer) {

    // allow immediate torpedo barrage launch
 
    auto &launcher1 = ecs.getComponent<TorpedoLauncher1>(enemy);
    auto &launcher2 = ecs.getComponent<TorpedoLauncher2>(enemy);

    launcher1.timeSinceBarrage = -launcher1.barrageCooldown;
    launcher2.timeSinceBarrage = -launcher2.barrageCooldown;

    std::cout << "EnemyAI created" << std::endl;
  }
  ~EnemyAI() = default;

  void Update(float tt, float dt)  {
    // update state based on player distance
    auto &shipControl = ecs.getComponent<ShipControl>(enemy);
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyVel = ecs.getComponent<Velocity>(enemy);
    auto &enemyAcc = ecs.getComponent<Acceleration>(enemy);
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);
    auto &enemyHealth = ecs.getComponent<Health>(enemy);
    auto &playerPos = ecs.getComponent<Position>(0);
    auto &playerVel = ecs.getComponent<Velocity>(0);

    float dist = distance(enemyPos.value, playerPos.value);
    float atp = angleToTarget(enemyPos.value, playerPos.value);

    auto &launcher1 = ecs.getComponent<TorpedoLauncher1>(enemy);
    auto &launcher2 = ecs.getComponent<TorpedoLauncher2>(enemy);

    auto &t1rounds = launcher1.rounds;
    auto &t2rounds = launcher2.rounds;

    // just get pdc1 rounds for now
    auto &pdcMounts = ecs.getComponent<PdcMounts>(enemy).pdcEntities;
    auto &pdc1 = ecs.getComponent<Pdc>(pdcMounts[0]);
    auto &pdc1rounds = pdc1.rounds; 

    // std::cout << "EnemyAI distance to player: " << dist << std::endl;
    // std::cout << "\nEnemyAI angle to player: " << atp << std::endl;

    ///////////////////////////////////////////////////////////////////////////////
    // - Ship State Machine -
    ///////////////////////////////////////////////////////////////////////////////
    if (enemyHealth.value <= 0) {
      state = State::DISABLED;
      std::cout << "EnemyAI: " << enemy << "EnemyAI state: DISABLED (health <= 0)" << std::endl;
    }
    else if (pdcTargeting.pdcTorpedoThreatDetect() && pdc1rounds > 0) {
      state = State::DEFENCE_PDC;
      // std::cout << "EnemyAI state: DEFENCE_PDC" << std::endl;
    }
    else if ((pdc1rounds < 30 && t1rounds == 0 && t2rounds == 0) ||
             enemyHealth.value <= 50) {
      // if we have no PDCs or torpedos left, or damaged, switch to FLEE
      state = State::FLEE;
      std::cout << "EnemyAI: " << enemy << "state: FLEE (damaged or no PDCs or torpedos left)" << std::endl;
    }
    else
    {
      if (state == State::DEFENCE_PDC) {
        // if we are in defence mode, we want to stop defending if there are no threats
        std::cout << "EnemyAI: " << enemy << " state: no threats, switching to CLOSE" << std::endl;
        state = State::CLOSE;
      }
      else if (state == State::CLOSE) {

        // std::cout << "EnemyAI: " << enemy << " tt: " << tt 
        //   << " launcher1.timeSinceBarrage: " << launcher1.timeSinceBarrage
        //   << " launcher2.timeSinceBarrage: " << launcher2.timeSinceBarrage
        //   << std::endl;

        if (dist < attack_pdc_distance) {
          state = State::ATTACK_PDC;
          // std::cout << "EnemyAI state: ATTACK_PDC" << std::endl;
        }
        else if (dist <= attack_torpedo_distance &&
            t1rounds > 0 && t2rounds > 0    &&
            tt > launcher1.timeSinceBarrage + launcher1.barrageCooldown &&
            tt > launcher2.timeSinceBarrage + launcher2.barrageCooldown)
        {
          state = State::ATTACK_TORPEDO;
          std::cout << "EnemyAI: " << enemy << " state: ATTACK_TORPEDO" << std::endl;
        }
        else if (dist > close_distance && playerVel.value.length() < 1000.f) {
          // ony flip and burn if the player is not moving too fast
          state = State::FLIP_AND_BURN;
          std::cout << "EnemyAI: " << enemy << " state: FLIP_AND_BURN" << std::endl;
        }
        else if (dist > close_distance && playerVel.value.length() >= 1000.f) {
          // if the player is moving fast, chase
          state = State::CHASE;
          std::cout << "EnemyAI: " << enemy << " state: CHASE (player moving fast)" << std::endl;
        }
      }
      else if (state == State::CHASE) {
        if (dist < close_distance) {
          state = State::CLOSE;
          std::cout << "EnemyAI: " << enemy << " state: CLOSE" << std::endl;
        }
        else if (dist > close_distance && playerVel.value.length() < 1000.f) {
          // ony flip and burn if the player is not moving too fast
          state = State::FLIP_AND_BURN;
          std::cout << "EnemyAI: " << enemy << " state: FLIP_AND_BURN" << std::endl;
        }
      }
      else if (state == State::IDLE) {
        if (dist < close_distance) {
          state = State::CLOSE;
          std::cout << "EnemyAI: " << enemy << " state: CLOSE" << std::endl;
        }
      }
      else if (state == State::ATTACK_TORPEDO) {

        if (dist < attack_pdc_distance) {
          state = State::ATTACK_PDC;
          // std::cout << "EnemyAI state: ATTACK_PDC" << std::endl;
        } 
        else if (t1rounds == 0 && t2rounds == 0) {
          // if we have no torpedos left, switch to close
          state = State::CLOSE;
          std::cout << "EnemyAI: " << enemy << " state: CLOSE (no torpedos left)" << std::endl;
        }
        else if (dist > attack_torpedo_distance) {
          state = State::CLOSE;
          std::cout << "EnemyAI: " << enemy << " state: CLOSE" << std::endl;
        }
        else if (tt < launcher1.timeSinceBarrage + launcher1.barrageCooldown &&
                 tt < launcher2.timeSinceBarrage + launcher2.barrageCooldown) {
          state = State::CLOSE;
          std::cout << "EnemyAI: " << enemy << " state: CLOSE (barrage complete)" << std::endl;
        }
      }
      else if (state == State::ATTACK_PDC) {
        if (dist > attack_pdc_distance) {
          state = State::ATTACK_TORPEDO;
          std::cout << "EnemyAI: " << enemy << " state: ATTACK_TORPEDO" << std::endl;
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

      // turn to player for now
      startTurn(ecs, shipControl, enemy, atp);

      // aquire the nearest torpedo and fire the PDCs
      pdcTargeting.pdcDefendTorpedo(tt, dt);
    }
    else if (state == State::CLOSE) {

      float chaseForce = 1000.f * dt;

      sf::Vector2f interceptDir = normalizeVector(playerPos.value - enemyPos.value + playerVel.value);
      sf::Vector2f interceptVec = interceptDir * chaseForce;

      float turnAngle = angleToTarget(enemyPos.value, interceptVec);
      // TODO None of this really works, maybe use proportional navigation instead?
      //startTurn(ecs, shipControl, enemy, turnAngle);
      startTurn(ecs, shipControl, enemy, atp);

      // accelerate towards the player, about 3G atm
      // TODO: update to change acceleration based on distance
      accelerateToMax(ecs, shipControl, enemy, 1.f, dt);
      // enemyVel.value += interceptVec;
    }
    else if (state == State::IDLE) {
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f;
    }
    else if (state == State::ATTACK_TORPEDO) {

      // set accel to 0
      // probably want to set a target velocity instead
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f;

      // only want to turn the ship if we are not already turning, prevents jittering
      startTurn(ecs, shipControl, enemy, atp);

      auto &enemyRot = ecs.getComponent<Rotation>(enemy);
      float diff = normalizeAngle(atp - enemyRot.angle);

      // fire when facing the player, +/- 5 degrees
      if (diff >= -05.f && diff <= +05.f) {
        if (tt > launcher1.timeSinceFired + launcher1.cooldown && launcher1.rounds) {
          launcher1.timeSinceFired = tt;
          torpedoFactory.fireone<TorpedoLauncher1>(enemy, 0);
          launcher1.rounds--;
          launcher1.barrageCount++;
          std::cout << "EnemyAI firing TorpedoLauncher1" << std::endl;
        }
        if (tt > launcher2.timeSinceFired + launcher2.cooldown && launcher2.rounds) {
          launcher2.timeSinceFired = tt;
          torpedoFactory.fireone<TorpedoLauncher2>(enemy, 0);
          launcher2.rounds--;
          launcher2.barrageCount++;
          std::cout << "EnemyAI firing TorpedoLauncher2" << std::endl;
        }
      }

      if (launcher1.barrageCount >= launcher1.barrageRounds &&
          launcher2.barrageCount >= launcher2.barrageRounds) {

        std::cout << "EnemyAI: " << enemy << " Barrage complete" << std::endl;
        launcher1.timeSinceBarrage = tt; // reset the barrage timer
        launcher2.timeSinceBarrage = tt; // reset the barrage timer
        launcher1.barrageCount = 0;
        launcher2.barrageCount = 0;
      }
    }
    else if (state == State::ATTACK_PDC) {

      auto &enemyRot = ecs.getComponent<Rotation>(enemy);
      float diff = atp - enemyRot.angle;

      diff = normalizeAngle(diff);

      // playing with setting the heading to an offsest for pdc fire and to avoid collisions
      // +/- 45 degrees will miss if the player is moving, so try 35
      // probably need to take into account target velocity instead for targeting
      if (diff > 0.f) {
        startTurn(ecs, shipControl, enemy, atp - 50.f);
      } else {
        startTurn(ecs, shipControl, enemy, atp + 50.f);
      }

      // set accel to 0
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f; 

#if 0
      // set a target velocity instead
      enemyVel.value.y = std::sin((enemyRot.angle) * (M_PI / 180.f)) * 1000.f;
      enemyVel.value.x = std::cos((enemyRot.angle) * (M_PI / 180.f)) * 1000.f; 
#else 
      // commented out keeping the existing velocity
      // enemyVel.value.y = 0.f;
      // enemyVel.value.x = 0.f;
#endif

      // for now, attack the player. 
      // Could additional evasive maneuvers later.
      pdcTargeting.pdcAttack<FriendlyShipTarget>(tt);
    }
    else if (state == State::FLEE) {
      // set accel to 5G
      startTurn(ecs, shipControl, enemy, atp + 180.f); // turn away from the player
      accelerateToMax(ecs, shipControl, enemy, 5.f, dt);
    }
    else if (state == State::CHASE) {
      // set accel to 5G
      startTurn(ecs, shipControl, enemy, atp); // turn towards the player
      accelerateToMax(ecs, shipControl, enemy, 5.f, dt);
    }
    else if (state == State::FLIP_AND_BURN) {

      // this is a full accelerate, flip and burn maneuver

      if (shipControl.state == ControlState::IDLE) {
        // turn and burn towards the player, comfortably within close distance
        startAccelBurnAndFlip(ecs, shipControl, enemy, atp, 3.0f,
                              dist - (close_distance / 2), dt);
      }
      else if (shipControl.state == ControlState::DONE) {
        state = State::CLOSE; // if we are done, switch to close
        // the ControlState machine should reset to idle, and we should catch the DONE first
        // shipControl.state = ControlState::IDLE; // reset the state
        std::cout << "EnemyAI: " << enemy << " state CLOSE (done flipping and burning)" << std::endl;
      }
      else if (dist < close_distance) {
        // if the player is too close, stop whatever we are doing and stop
        // it is likely the avoidance code has messed up the vector. 
        startFlipAndStop(ecs, shipControl, enemy, 6.0f, tt);
        // std::cout << "EnemyAI: " << enemy << " state FLIP_AND_BURN (player close so startFlipAndStop)" << std::endl;
      }
    }
    else if (state == State::DISABLED) {
      // do nothing, the ship is disabled
      enemyAcc.value.x = 0.f;
      enemyAcc.value.y = 0.f;
      // slow spin for disabled ship
      startTurn(ecs, shipControl, enemy, enemyRot.angle + 1.0f); 
      std::cout << "EnemyAI: " << enemy << " state DISABLED" << std::endl;
    }
    else {
      std::cout << "EnemyAI: " << enemy << " state UNKNOWN" << std::endl;
    }

    // perform the turn (if needed)
    updateControlState(ecs, shipControl, enemy, tt, dt);

    avoidCollisions(dt);
  }

 private:
  Coordinator &ecs;
  Entity enemy;
  BulletFactory bulletFactory;
  TorpedoFactory torpedoFactory;
  sf::Sound pdcFireSoundPlayer;
  PdcTargeting pdcTargeting;

  const float close_distance          = 50000.f;  // will close rarther than flip and burn
  const float attack_torpedo_distance = 500000.f;
  const float attack_pdc_distance     = 8000.f;

  enum class State {
    IDLE,
    CLOSE,
    CHASE,
    FLIP_AND_BURN,
    ATTACK_PDC,
    DEFENCE_PDC,
    ATTACK_TORPEDO,
    EVADE,
    FLEE,
    DISABLED
  };
 
  State state = State::CLOSE;

  void avoidCollisions(float dt) {

    const float lookAheadDistance = 10000.f; // distance to look ahead for collisions
    const float avoidanceForce = 1000.f * dt; // force to apply to avoid collisions
 
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyVel = ecs.getComponent<Velocity>(enemy);

    sf::Vector2f forward = normalizeVector(enemyVel.value);
    sf::Vector2f lookAheadPos = enemyPos.value + forward * lookAheadDistance;

    for (auto &ec : ecs.view<Position, Collision>()) {

      auto &collision = ecs.getComponent<Collision>(ec);

      if (collision.ctype != CollisionType::ASTEROID &&
          collision.ctype != CollisionType::SHIP) {
        continue; // skip non-asteroid entities
      }

      auto &collisionPos = ecs.getComponent<Position>(ec).value;

      float dist = distance(lookAheadPos, collisionPos);

      // this will strafe
      // TODO: change direction at a greater range
      if (dist < 6000.f) { // if an object is too close
        sf::Vector2f avoidanceDir = normalizeVector(lookAheadPos - collisionPos);
        sf::Vector2f avoidanceVector = avoidanceDir * avoidanceForce;
        // std::cout << "Avoidance vector: " << avoidanceVector.x << ", " << avoidanceVector.y << std::endl;
        enemyVel.value += avoidanceVector; // apply avoidance force
      }
    }

  }

};

