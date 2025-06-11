#pragma once
#include "ecs.hpp"
#include "ballistics.hpp"
#include "components.hpp"
#include "utils.hpp"
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
    if (nearestTorpedoDist > torpedoThreatRange) {
      std::cout << "PdcTarget no torpedos in range\n";
      return false; 
    }

    std::cout << "pdcThreatDetected nearest torpedo: " << nearestTorpedo << "\n";
    return true; // there is a torpedo to target
  }

  // targeting has alredy been done
  void pdcAttack(float tt) {
    firePdc1Burst(e, tt, 0.25f);
    firePdc2Burst(e, tt, 0.25f);
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

      // add each torpedo into the map, which is ordered by distance
      torpedoTargetDistances[dist] = torpedo;

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
    if (nearestTorpedoDist > torpedoThreatRange) {
      // std::cout << "PdcTarget no torpedos in range\n";
      return;
    }

    std::cout << "\nPdcTarget nearest torpedo: " << nearestTorpedo << "\n";
    std::cout << "PdcTarget nearest torpedo distance: " << nearestTorpedoDist << "\n";
 
    // get the closest two (for now) torpedos
    int count = 0;
    for (auto &torpedo : torpedoTargetDistances) {
      if (count >= 2) {
        break; // only target the two closest torpedos
      }
      count++;

      // get the torpedo entity
      Entity tt = torpedo.second;
      std::cout << "PdcTarget targeting index : " << count << " torpedo entity: " << tt << "\n";

      // set the target for the PDCs
      addTarget(tt);
    }

    // setTarget(nearestTorpedo);
    aquireTargets();
    firePdc1Burst(e, tt, 2.0f); // larger burstSpread to hit torps
    firePdc2Burst(e, tt, 2.0f);
  }

  // add a target to the targeting list and assign the target to the appropriate PDCs
  // TODO: not sure if I maintain a simple list of the two targets, or try to assign them 
  // here. The problem is that I need to assign targets to the pdcs, if there is one target
  // then assign them both. But with two I need to assign them seperately. 
  // I really need a way to grab all the pdcs on a ship, as the Donnager has about 40 pdcs. 
  void addTarget(Entity target) {

    auto &entityRot = ecs.getComponent<Rotation>(e);
    auto &targetPos = ecs.getComponent<Position>(target); 

    // for now, we know we have 2 pds, will have to get all the pdcs on 
    // the entity later
    auto &pdc1 = ecs.getComponent<Pdc1>(e);
    auto &pdc2 = ecs.getComponent<Pdc2>(e);

    // work out which pdc to use

    // calculate angular difference relative to the front of the ship
    // all other angles are relative to the front of the ship aswell, this keeps
    // all angles in relation to the ship's rotation
    float pdc1RelativeFiringAngle = normalizeAngle(pdc1.firingAngle - entityRot.angle);
    float pdc2RelativeFiringAngle = normalizeAngle(pdc2.firingAngle - entityRot.angle);

    // check if the target is within the firing angle of the PDCs
    if (isInRange(pdc1RelativeFiringAngle, pdc1.minFiringAngle, pdc1.maxFiringAngle)) {

      // if we already have a target for this PDC, need to decide if we replace it, based
      // on if already targeted by another PDC.
 
      // add the target to the map of targets
      pdcTargets[1] = target; // 1 for pdc1
    }

    if (isInRange(pdc2RelativeFiringAngle, pdc2.minFiringAngle, pdc2.maxFiringAngle)) {
      // add the target to the map of targets
      pdcTargets[2] = target; // 2 for pdc2
    }
    
    pdc1.target = target;
    pdc2.target = target;

    // std::cout << "PdcTarget set target: " << target << "\n";
  }

  // Target the PDCs at target
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
    // std::cout << "\nPdcTarget angle to target: " << att << "\n";

    // estimate a targeting angle based on the target's velocity
    // use a simple prediction based on the target's velocity and distance
    sf::Vector2f distanceVector = targetPos.value - entityPos.value;

    float distance = distanceVector.length();

    // estimate the time to impact based on the distance and target velocity
    float timeToImpact = distance / pdc1.projectileSpeed;

    // fudge factor for time to impact, as the change in angle is minimal
    timeToImpact *= 1.00f;

    // std::cout << "PdcTarget time to impact: " << timeToImpact << "\n";

    // for a fast moving target, we need to set to a maximum time to impact,
    // or else for a torpedo the vector will pass through us and our guess
    // angle will flip
    timeToImpact = std::clamp(timeToImpact, 0.f, 0.5f);

    // Predict the target's position based on our relative velocity
    // and the time to impact

    // compute relative velocity
    sf::Vector2f relativeVel = targetVel.value - entityVel.value;
     std::cout << "PdcTarget relative velocity: " << relativeVel.x << ", " << relativeVel.y << "\n";
 
    sf::Vector2f predictedTargetPos = distanceVector + (relativeVel * timeToImpact);
    // std::cout << "PdcTarget target position: " 
    //           << targetPos.value.x << ", " << targetPos.value.y << "\n";

    // aim at a guessed future position
    sf::Vector2f guessDir = predictedTargetPos.normalized();

    float guessAngle = guessDir.angle().asDegrees();
    // std::cout << "PdcTarget guess angle: " << guessAngle << "\n";

    att = normalizeAngle(guessAngle);
    // std::cout << "PdcTarget final absolute angle: " << att << "\n";

    pdc1.firingAngle = att;
    pdc2.firingAngle = att;
  }

  void firePdc1Burst(Entity source, float tt, float burstSpread) {
    firePdcBurst<Pdc1>(source, tt, burstSpread, "PDC1");
  }

  void firePdc2Burst(Entity source, float tt, float burstSpread) {
    firePdcBurst<Pdc2>(source, tt, burstSpread, "PDC2");
  }

  // fire the PDC if it is ready and target in arc
  template<typename Pdc>
  void firePdcBurst(Entity source, float tt, float burstSpread, const std::string &pdcName) {
    auto &pdc = ecs.getComponent<Pdc>(source);
    auto &entityRot = ecs.getComponent<Rotation>(source);

    // calculate angular difference relative to the front of the ship
    // all other angles are relative to the front of the ship aswell, this keeps
    // all angles in relation to the ship's rotation
    float relativeFiringAngle = normalizeAngle(pdc.firingAngle - entityRot.angle);

    // spread the burst fire angle (beter chance of hitting torpedos and accelerating targets)

    // cycle the burst through +/- burstSpread
    pdc.burstSpreadAngle = burstSpread * std::sin(tt * 10.f); // oscillate between -burstSpread and +burstSpread
    std::cout << "PDC1 burst spread angle: " << pdc.burstSpreadAngle << "\n";

    // dont worry about a few degrees past the min or max
    pdc.firingAngle += pdc.burstSpreadAngle;

    std::cout << pdcName
              << " relative firing angle: " << relativeFiringAngle
              << " absolute firing angle: " << pdc.firingAngle
              << " entity rotation angle: " << entityRot.angle
              << " relative min: " << pdc.minFiringAngle
              << " relative max: " << pdc.maxFiringAngle
              << " source: " << source << "\n";

    // check if the target is within the firing angle of the PDCs
    if (isInRange(relativeFiringAngle, pdc.minFiringAngle, pdc.maxFiringAngle)) {

      if (pdc.timeSinceBurst == 0 || tt > pdc.timeSinceBurst + pdc.pdcBurstCooldown) {
        pdc.timeSinceBurst = tt;
        pdc.pdcBurst = pdc.maxPdcBurst;
      }

      if (pdc.pdcBurst > 0 && pdc.rounds) {
        if (tt > pdc.timeSinceFired + pdc.cooldown) {
          pdc.timeSinceFired = tt;
          bulletFactory.fireone<Pdc1>(source, tt);
          pdc.rounds--;
          pdc.pdcBurst--;
          pdcFireSoundPlayer.play();
          std::cout << pdcName << " FIRING!" << "\n";
        }
      }
    }
    else {
      std::cout << pdcName
                << " not firing, relative angle: " << relativeFiringAngle 
                << " is out or range. min: " << pdc.minFiringAngle 
                << " max: " << pdc.maxFiringAngle << "\n";
    }
  }


private:
  Coordinator &ecs;
  Entity e;        // player or enemy that is using the pdcs
  BulletFactory bulletFactory;
  sf::Sound pdcFireSoundPlayer;
 
  std::map<float, Entity> torpedoTargetDistances; // map of targets and their distances

  std::array<Entity, 2> pdcTargets; // list of 2 current targets 
 
  // distance in pixels to consider a torpedo a threat.
  // has to be close enough for pdcs to track it 
  float torpedoThreatRange = 16000.f;
};
