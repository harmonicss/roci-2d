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
  EnemyAI(Coordinator &ecs, Entity enemy, BulletFactory bulletFactory,
          TorpedoFactory torpedoFactory, sf::Sound pdcFireSoundPlayer) :
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
    auto &shipControl = ecs.getComponent<ShipControl>(enemy);
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyVel = ecs.getComponent<Velocity>(enemy);
    auto &enemyAcc = ecs.getComponent<Acceleration>(enemy);
    auto &enemyRot = ecs.getComponent<Rotation>(enemy);
    auto &playerPos = ecs.getComponent<Position>(0);

    float dist = distance(enemyPos.value, playerPos.value);
    float atp = angleToTarget(enemyPos.value, playerPos.value);

    auto &t1rounds = ecs.getComponent<TorpedoLauncher1>(enemy).rounds;
    auto &t2rounds = ecs.getComponent<TorpedoLauncher2>(enemy).rounds;

    // just get pdc1 rounds for now
    auto &pdcMounts = ecs.getComponent<PdcMounts>(enemy).pdcEntities;
    auto &pdc1 = ecs.getComponent<Pdc>(pdcMounts[0]);
    auto &pdc1rounds = pdc1.rounds; 

    // std::cout << "EnemyAI distance to player: " << dist << std::endl;
    // std::cout << "\nEnemyAI angle to player: " << atp << std::endl;

    ///////////////////////////////////////////////////////////////////////////////
    // - Ship State Machine -
    ///////////////////////////////////////////////////////////////////////////////
    if (pdcTarget.pdcTorpedoThreatDetect() && pdc1rounds > 0) {
      state = State::DEFENCE_PDC;
      std::cout << "EnemyAI state: DEFENCE_PDC" << std::endl;
    }
    else if (pdc1rounds < 30 && t1rounds == 0 && t2rounds == 0) {
      // if we have no PDCs or torpedos left, switch to FLEE
      state = State::FLEE;
      // std::cout << "EnemyAI state: FLEE (no PDCs or torpedos left)" << std::endl;
    }
    else
    {
      if (state == State::DEFENCE_PDC) {
        // if we are in defence mode, we want to stop defending if there are no threats
        std::cout << "EnemyAI state: no threats, switching to IDLE" << std::endl;
        state = State::IDLE;
      }
      else if (state == State::CLOSE) {
        if (dist <= attack_torpedo_distance && t1rounds > 0 && t2rounds > 0) {
          state = State::ATTACK_TORPEDO;
          std::cout << "EnemyAI state: ATTACK_TORPEDO" << std::endl;
        }
        else if (dist < attack_pdc_distance) {
          state = State::ATTACK_PDC;
          std::cout << "EnemyAI state: ATTACK_PDC" << std::endl;
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
        else if (t1rounds == 0 && t2rounds == 0) {
          // if we have no torpedos left, switch to close
          state = State::CLOSE;
          std::cout << "EnemyAI state: CLOSE (no torpedos left)" << std::endl;
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
        startTurn(ecs, shipControl, enemy, atp);
      }

      // aquire the nearest torpedo and fire the PDCs
      pdcTarget.pdcDefendTorpedo(tt, dt);
    }
    else if (state == State::CLOSE) {
      shipControl.targetPosition = playerPos.value;
      //shipControl.targetAcceleration = enemyAcc.value;
     
      // only want to turn the ship if we are not already turning, prevents jittering
      if (shipControl.turning == false) {
        startTurn(ecs, shipControl, enemy, atp);
      }

      // accelerate towards the player, about 3G atm
      // TODO: update to change acceleration based on distance
      enemyAcc.value.y = std::sin((enemyRot.angle) * (M_PI / 180.f)) * 300.f;
      enemyAcc.value.x = std::cos((enemyRot.angle) * (M_PI / 180.f)) * 300.f;
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
        startTurn(ecs, shipControl, enemy, atp);
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

      diff = normalizeAngle(diff);

      // playing with setting the heading to an offsest for pdc fire and to avoid collisions
      // +/- 45 degrees will miss if the player is moving, so try 35
      // probably need to take into account target velocity instead for targeting
      if (shipControl.turning == false) {
        if (diff > 0.f) {
          startTurn(ecs, shipControl, enemy, atp - 50.f);
        } else {
          startTurn(ecs, shipControl, enemy, atp + 50.f);
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
      // commented out keeping the existing velocity
      // enemyVel.value.y = 0.f;
      // enemyVel.value.x = 0.f;
#endif

      // for now, attack the player. 
      // Could additional evasive maneuvers later.
      pdcTarget.pdcAttack<FriendlyShipTarget>(tt);
    }
    else if (state == State::FLEE) {
      // set accel to 5G
      enemyAcc.value.y = std::sin((enemyRot.angle) * (M_PI / 180.f)) * 500.f;
      enemyAcc.value.x = std::cos((enemyRot.angle) * (M_PI / 180.f)) * 500.f;

      // only want to turn the ship if we are not already turning, prevents jittering
      if (shipControl.turning == false) {
        startTurn(ecs, shipControl, enemy, atp + 180.f); // turn away from the player
      }
    }

    avoidAsteroids(dt);

    // perform the turn
    if (shipControl.turning) {
      performTurn(ecs, shipControl, enemy);
    }
  }

 private:
  Coordinator &ecs;
  Entity enemy;
  BulletFactory bulletFactory;
  TorpedoFactory torpedoFactory;
  sf::Sound pdcFireSoundPlayer;
  PdcTarget pdcTarget;

  const float close_distance          = 500000.f;
  const float attack_pdc_distance     = 8000.f;
  const float attack_torpedo_distance = 200000.f;

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

  void avoidAsteroids(float dt) {

    const float lookAheadDistance = 10000.f; // distance to look ahead for asteroids
    const float avoidanceForce = 5000.f * dt; // force to apply to avoid asteroids
 
    auto &enemyPos = ecs.getComponent<Position>(enemy);
    auto &enemyVel = ecs.getComponent<Velocity>(enemy);

    sf::Vector2f forward = normalizeVector(enemyVel.value);
    sf::Vector2f lookAheadPos = enemyPos.value + forward * lookAheadDistance;

    for (auto &e : ecs.view<Position, Collision>()) {

      auto &collision = ecs.getComponent<Collision>(e);

      if (collision.ctype != CollisionType::ASTEROID) {
        continue; // skip non-asteroid entities
      }

      auto &asteroidPos = ecs.getComponent<Position>(e).value;

      float dist = distance(lookAheadPos, asteroidPos);

      // this will strafe
      // TODO: change direction at a greater range
      if (dist < 6000.f) { // if an asteroid is too close
        sf::Vector2f avoidanceDir = normalizeVector(lookAheadPos - asteroidPos);
        sf::Vector2f avoidanceVector = avoidanceDir * avoidanceForce;
        std::cout << "Avoidance vector: " << avoidanceVector.x << ", " << avoidanceVector.y << std::endl;
        enemyVel.value += avoidanceVector; // apply avoidance force
      }
    }

  }

};

