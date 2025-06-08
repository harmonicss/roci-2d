#pragma once
#include "ecs.hpp"
#include "ballistics.hpp"
#include "components.hpp"
#include <cmath>
#include <SFML/Audio.hpp>

// to target the pdcs and burst fire at incoming torpedos or enemy ships
class PdcTarget {
public:
  // Entity e is the player or enemy that is using the pdcs
  PdcTarget(Coordinator &ecs, Entity e, BulletFactory bulletFactory, sf::Sound pdcFireSoundPlayer) :
    ecs(ecs),
    e(e),
    bulletFactory(bulletFactory),
    pdcFireSoundPlayer(pdcFireSoundPlayer)
  {
    std::cout << "PdcTarget created for entity: " << e << std::endl;
  }
  ~PdcTarget() = default;

  bool pdcTorpedoThreatDetect() {
    Entity nearestTorpedo = 0;  // might confuse with the player
    float nearestTorpedoDist = std::numeric_limits<float>::max();

    // find target torpedos
    for (auto &torpedo : ecs.view<Target>()) {
      auto &torpedoTarget = ecs.getComponent<Target>(torpedo);

      if (torpedoTarget.target != e) {
        // skip if the torpedo is not targeting this entity
        continue;
      }

      // find the nearest torpedo to the player or enemy
      auto &torpedoPos = ecs.getComponent<Position>(torpedo);
      auto &myPos = ecs.getComponent<Position>(e);

      float dist = distance(myPos.value, torpedoPos.value);

      if (dist < nearestTorpedoDist) {
        nearestTorpedoDist = dist;
        nearestTorpedo = torpedo;
      }
    }

    // no torpedos to target
    if (nearestTorpedo == 0) {
      return false; 
    }

    // no torpedos in range
    if (nearestTorpedoDist > 10000.f) {
      std::cout << "PdcTarget no torpedos in range\n";
      return false; 
    }

    std::cout << "pdcThreatDetected nearest torpedo: " << nearestTorpedo << "\n";
    return true; // there is a torpedo to target
  }

  // targeting has alredy been done
  void pdcAttack(float tt) {
    firePdc1Burst(e, tt);
    firePdc2Burst(e, tt);
  }

  // target incoming torpedos
  void pdcDefendTorpedo(float tt, float dt) {
    Entity nearestTorpedo = 0;  // might confuse with the player
    float nearestTorpedoDist = std::numeric_limits<float>::max();

    // find target torpedos
    // currently find the nearest, need to get all and target individually
    for (auto &torpedo : ecs.view<Target>()) {
      auto &torpedoTarget = ecs.getComponent<Target>(torpedo);

      if (torpedoTarget.target != e) {
        // skip if the torpedo is not targeting this entity
        continue;
      }

      // find the nearest torpedo to the player or enemy
      auto &torpedoPos = ecs.getComponent<Position>(torpedo);
      auto &myPos = ecs.getComponent<Position>(e);

      float dist = distance(myPos.value, torpedoPos.value);

      if (dist < nearestTorpedoDist) {
        nearestTorpedoDist = dist;
        nearestTorpedo = torpedo;
      }
    }

    // no torpedos to target
    if (nearestTorpedo == 0) {
      return;
    }

    // no torpedos in range
    if (nearestTorpedoDist > 100000.f) {
      std::cout << "PdcTarget no torpedos in range\n";
      return; 
    }

    std::cout << "\nPdcTarget nearest torpedo: " << nearestTorpedo << "\n";
    std::cout << "PdcTarget nearest torpedo distance: " << nearestTorpedoDist << "\n";
 
    setTarget(nearestTorpedo);
    aquireTargets();
    firePdc1Burst(e, tt);
    firePdc2Burst(e, tt);
  }

  void setTarget(Entity target) {
    // set the target for the PDCs
    auto &pdc1 = ecs.getComponent<Pdc1>(e);
    auto &pdc2 = ecs.getComponent<Pdc2>(e);

    pdc1.target = target;
    pdc2.target = target;

    // std::cout << "PdcTarget set target: " << target << "\n";
  }

  // Target the PDCs at target
  // TODO: move the pdcs for spread
  void aquireTargets () {

    // update the target for the PDCs
    //TODO: investigate as this will get all pdc entities, not correct?
    // for (auto &pdc : ecs.view<Pdc1, Pdc2>()) {
      auto &pdc1 = ecs.getComponent<Pdc1>(e);
      auto &pdc2 = ecs.getComponent<Pdc2>(e);

      auto &targetPos = ecs.getComponent<Position>(pdc1.target); // what about pdc2 different target?
      auto &targetVel = ecs.getComponent<Velocity>(pdc1.target);
      auto &entityPos = ecs.getComponent<Position>(e);
      auto &entityRot = ecs.getComponent<Rotation>(e);
      auto &entityVel = ecs.getComponent<Velocity>(e);

      // get the angle to the nearest torpedo
      float att = angleToTarget(entityPos.value, targetPos.value);

      // get rotated firing angle based on the entity's rotation
      // float rotatedAngle = att - entityRot.angle;
      // if (rotatedAngle >= 180.f) {
      //   rotatedAngle -= 360.f;
      // } else if (rotatedAngle < -180.f) {
      //   rotatedAngle += 360.f;
      // }

      // std::cout << "\nPdcTarget angle to target: " << att << "\n";

      // estimate a targeting angle based on the target's velocity
      // use a simple prediction based on the target's velocity and distance
      // TODO: This doesnt work there is a sign wrong somewhere
      sf::Vector2f distanceVector = targetPos.value - entityPos.value;

      float distance = distanceVector.length();

      // estimate the time to impact based on the distance and target velocity
      // trim it down to prevent overshooting
      float timeToImpact = distance / pdc1.projectileSpeed;

      // fudge factor for time to impact, as the change in angle is minimal
      timeToImpact *= 1.00f;

      // std::cout << "PdcTarget time to impact: " << timeToImpact << "\n";

      // Predict the target's position based on our relative velocity
      // and the time to impact

      // compute relative velocity
      sf::Vector2f relativeVel = targetVel.value - entityVel.value;
      // std::cout << "PdcTarget relative velocity: " << relativeVel.x << ", " << relativeVel.y << "\n";
 
      sf::Vector2f predictedTargetPos = distanceVector + (relativeVel * timeToImpact);
      // std::cout << "PdcTarget target position: " 
      //           << targetPos.value.x << ", " << targetPos.value.y << "\n";

      // aim at a guessed future position
      // - entityPos.value for the enemy? Not for player 
      sf::Vector2f guessDir = predictedTargetPos.normalized();

      float guessAngle = guessDir.angle().asDegrees();

      // std::cout << "PdcTarget guess angle: " << guessAngle << "\n";

      // now that we have the guess angle, we need to adjust it based on the entity's rotation
      //att = guessAngle - entityRot.angle;
      att = guessAngle;

      if (att >= 180.f) {
        att -= 360.f;
      } else if (att < -180.f) {
        att += 360.f;
      }

      // std::cout << "PdcTarget final absolute angle: " << att << "\n";

      pdc1.firingAngle = att;
      pdc2.firingAngle = att;
  }

  // fire the PDC if it is ready and target in arc
  void firePdc1Burst(Entity source, float tt) {
    auto &pdc = ecs.getComponent<Pdc1>(source);
    auto &entityRot = ecs.getComponent<Rotation>(source);

    // rotate the firing angle based on the entity's rotation
    float rotatedMinAngle = pdc.minFiringAngle - entityRot.angle;
    float rotatedMaxAngle = pdc.maxFiringAngle - entityRot.angle;
    float rotatedFiringAngle = pdc.firingAngle - entityRot.angle;

    if (rotatedMinAngle >= 180.f) {
      rotatedMinAngle -= 360.f;
    } else if (rotatedMinAngle < -180.f) {
      rotatedMinAngle += 360.f;
    }

    if (rotatedMaxAngle >= 180.f) {
      rotatedMaxAngle -= 360.f;
    } else if (rotatedMaxAngle < -180.f) {
      rotatedMaxAngle += 360.f;
    }

    if (rotatedFiringAngle >= 180.f) {
      rotatedFiringAngle -= 360.f;
    } else if (rotatedFiringAngle < -180.f) {
      rotatedFiringAngle += 360.f;
    }

    // min and max angles could be the wrong way around, so check
    if (rotatedMinAngle > rotatedMaxAngle) {
      std::swap(rotatedMinAngle, rotatedMaxAngle);
    }

    std::cout << "PDC1 rotated firing angle: " << rotatedFiringAngle
              << " absolute firing angle: " << pdc.firingAngle
              << " entity rotation angle: " << entityRot.angle
              << " rotated min: " << rotatedMinAngle
              << " rotated max: " << rotatedMaxAngle
              << " source: " << source << "\n";

    // check if the torpedo is within the firing angle of the PDCs
    // TODO: would be good to rotate the pdcs over time
    if (isInRange(rotatedFiringAngle, rotatedMinAngle, rotatedMaxAngle)) {

      if (pdc.timeSinceBurst == 0 || tt > pdc.timeSinceBurst + pdc.pdcBurstCooldown) {
        pdc.timeSinceBurst = tt;
        pdc.pdcBurst = pdc.maxPdcBurst;
      }

      if (pdc.pdcBurst > 0 && pdc.rounds) {
        if (tt > pdc.timeSinceFired + pdc.cooldown) {
          pdc.timeSinceFired = tt;
          bulletFactory.fireone<Pdc1>(source);
          pdc.rounds--;
          pdc.pdcBurst--;
          pdcFireSoundPlayer.play();
          std::cout << "PDC1 FIRING!" << "\n";
        }
      }
    }
    else {
      std::cout << "PDC1 not firing, rotated angle out of range: " 
                << rotatedFiringAngle << " min: " << rotatedMinAngle 
                << " max: " << rotatedMaxAngle << "\n";
    }
  }

  void firePdc2Burst(Entity source, float tt) {
    auto &pdc = ecs.getComponent<Pdc2>(source);
    auto &entityRot = ecs.getComponent<Rotation>(source);

    // rotate the firing angle based on the entity's rotation
    float rotatedMinAngle = pdc.minFiringAngle - entityRot.angle;
    float rotatedMaxAngle = pdc.maxFiringAngle - entityRot.angle;
    float rotatedFiringAngle = pdc.firingAngle - entityRot.angle;

    if (rotatedMinAngle >= 180.f) {
      rotatedMinAngle -= 360.f;
    } else if (rotatedMinAngle < -180.f) {
      rotatedMinAngle += 360.f;
    }

    if (rotatedMaxAngle >= 180.f) {
      rotatedMaxAngle -= 360.f;
    } else if (rotatedMaxAngle < -180.f) {
      rotatedMaxAngle += 360.f;
    }

    if (rotatedFiringAngle >= 180.f) {
      rotatedFiringAngle -= 360.f;
    } else if (rotatedFiringAngle < -180.f) {
      rotatedFiringAngle += 360.f;
    }

    // min and max angles could be the wrong way around, so check
    if (rotatedMinAngle > rotatedMaxAngle) {
      std::swap(rotatedMinAngle, rotatedMaxAngle);
    }

    std::cout << "PDC2 rotated firing angle: " << rotatedFiringAngle
              << " absolute firing angle: " << pdc.firingAngle
              << " entity rotation angle: " << entityRot.angle
              << " rotated min: " << rotatedMinAngle
              << " rotated max: " << rotatedMaxAngle
              << " source:" << source << "\n";

    if (isInRange(rotatedFiringAngle, rotatedMinAngle, rotatedMaxAngle)) {

      if (pdc.timeSinceBurst == 0 || tt > pdc.timeSinceBurst + pdc.pdcBurstCooldown) {
        pdc.timeSinceBurst = tt;
        pdc.pdcBurst = pdc.maxPdcBurst;
      }

      if (pdc.pdcBurst > 0 && pdc.rounds) {
        if (tt > pdc.timeSinceFired + pdc.cooldown) {
          pdc.timeSinceFired = tt;
          bulletFactory.fireone<Pdc2>(source);
          pdc.rounds--;
          pdc.pdcBurst--;
          pdcFireSoundPlayer.play();
          std::cout << "PDC2 FIRING!" << "\n";
        }
      }
    }
    else {
      std::cout << "PDC2 not firing, rotated angle out of range: " 
                << rotatedFiringAngle << " min: " << rotatedMinAngle 
                << " max: " << rotatedMaxAngle << "\n";
    }
  }

private:
  Coordinator &ecs;
  Entity e;        // player or enemy that is using the pdcs
  BulletFactory bulletFactory;
  sf::Sound pdcFireSoundPlayer;

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

  inline bool isInRange(float angle, float minAngle, float maxAngle) {
    if (minAngle > maxAngle) {
      // wrap around case
      return angle >= minAngle || angle <= maxAngle;
    }
    return angle >= minAngle && angle <= maxAngle;
  }
};
