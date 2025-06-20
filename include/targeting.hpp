#pragma once
#include "ecs.hpp"
#include "ballistics.hpp"
#include "components.hpp"
#include "utils.hpp"
#include <cmath>
#include <SFML/Audio.hpp>
#include <optional>

#undef PDCTARGET_AI_DEBUG

#if !defined(PDCTARGET_AI_DEBUG)
// a streambuf that does nothing, used to redirect cout to a null stream
struct PdcNullBuffer : std::streambuf {
  int overflow(int c) override { return c; } // do nothing
};

static PdcNullBuffer pdcNullBuffer; // global null buffer to use for cout
static std::ostream pdcNullStream(&pdcNullBuffer); // global null stream to use for cout

#define PDCTARGET_DEBUG pdcNullStream

#else

#define PDCTARGET_DEBUG std::cout

#endif

const Entity INVALID_TARGET_ID = 0xFFFF; // used to indicate no target

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
    PDCTARGET_DEBUG << "PdcTarget created for entity: " << e << std::endl;
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
      // PDCTARGET_DEBUG << "PdcTarget no torpedos in range\n";
      return false;
    }

    PDCTARGET_DEBUG << "pdcThreatDetected nearest torpedo: " << nearestTorpedo << "\n";
    return true; // there is a torpedo to target
  }

  void pdcAttack(Entity target, float tt) {
    clearTargets(); // clear the targets before adding new ones
    addTarget(target);
    aquireTargets(true); // re-aquire targets for the PDCs, to update targeting, use prediction
    firePdcBursts(e, tt, 1.0f);
  }

  // target incoming torpedos
  void pdcDefendTorpedo(float tt, float dt) {
    Entity nearestTorpedo = 0;  // might confuse with the player
    float nearestTorpedoDist = std::numeric_limits<float>::max();

    torpedoTargetDistances.clear(); // clear the map of torpedo distances

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

      PDCTARGET_DEBUG << "\npdcDefendTorpedo added torpedo: " << torpedo << " distance: " << dist << "\n";

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
      // PDCTARGET_DEBUG << "PdcTarget no torpedos in range\n";
      return;
    }

    PDCTARGET_DEBUG << "pdcDefendTorpedo nearest torpedo: " << nearestTorpedo << "\n";
    PDCTARGET_DEBUG << "pdcDefendTorpedo nearest torpedo distance: " << nearestTorpedoDist << "\n";

    clearTargets(); // clear the targets before adding new ones

    // get the closest four torpedos
    int count = 0;
    for (auto &torpedo : torpedoTargetDistances) {
      if (count >= 4) {
        break; 
      }

      // get the torpedo entity
      Entity tt = torpedo.second;
      PDCTARGET_DEBUG << "pdcDefendTorpedo adding target index: " << count << " torpedo entity: " << tt << "\n";

      // set the target for the PDCs
      addTarget(tt);
      count++;
    }

    aquireTargets(false); // dont use prediction, they move to fast better to aim straight at them
    firePdcBursts(e, tt, 5.0f); // larger burstSpread to hit torps
  }

  void clearTargets() {
    for (auto &t : pdcTargets) {
      t.reset();
    }

    for (auto &pdcEntity : ecs.getComponent<PdcMounts>(e).pdcEntities) {
      auto &pdcMount = ecs.getComponent<Pdc>(pdcEntity);
      pdcMount.target = INVALID_TARGET_ID; // reset the target for the PDC
    }
  }

  // add a target to the targeting list and assign the target to the appropriate PDCs
  void addTarget(Entity target) {

    auto &entityRot = ecs.getComponent<Rotation>(e);
    auto &entityPos = ecs.getComponent<Position>(e);
    auto &targetPos = ecs.getComponent<Position>(target); 
    auto &mounts = ecs.getComponent<PdcMounts>(e);

    // get the angle to the target
    float att = angleToTarget(entityPos.value, targetPos.value);

    // assign the target to all PDCs that are within firing arc of the target
    for (Entity pdcEntity : mounts.pdcEntities) {

      auto &pdc = ecs.getComponent<Pdc>(pdcEntity);

      // work out which pdc to use

      // if the pdcTargets are full, and this is a different target, then we need to
      // remove the oldest target and add the new one
#if 0
      if (pdcTargets[0].has_value() && pdcTargets[1].has_value()) {
        if (pdcTargets[0] != target && pdcTargets[1] != target) {
          // remove the oldest target, which is the first one
          PDCTARGET_DEBUG << "addTarget " << ecs.getEntityName(pdcEntity) 
                    << " removing oldest target: " << pdcTargets[0].value() << "\n";
          pdcTargets[0].reset();
        }
      }
#endif
      bool emptySlotFound = false;
      for (auto &t : pdcTargets) {
        if (!t.has_value()) {
          emptySlotFound = true;
          continue; 
        }
      }

      if (emptySlotFound == false) {
        // go through the targets again, and check if this is a new target
        for (auto &t : pdcTargets) {
          if (t.has_value() && t.value() == target) {
            // target already assigned, no need to add it again
            PDCTARGET_DEBUG << "addTarget " << ecs.getEntityName(pdcEntity) 
                            << " target already assigned: " << target << "\n";
            return;
          }
        }

        // all slots are full, so we need to remove the oldest target
        PDCTARGET_DEBUG << "addTarget " << ecs.getEntityName(pdcEntity) 
                        << " removing oldest target: " << pdcTargets[0].value() << "\n";
        pdcTargets[0].reset();
      }

      // calculate angular difference relative to the front of the ship
      // all other angles are relative to the front of the ship aswell, this keeps
      // all angles in relation to the ship's rotation
      float pdcRelativeFiringAngle = normalizeAngle(att - entityRot.angle);

      // check if the target is within the firing angle of the PDCs
      if (isInRange(pdcRelativeFiringAngle, pdc.minFiringAngle, pdc.maxFiringAngle)) {

        // Check if the target is already assigned
        // TODO: not sure if this is correct, what if I need to assign to a different pdc?
        // will it be ok as I clear the pdcTarget list beforehand?
        // what if I want all pdcs to target the same target. This will automatically assign all 
        // pdcTargets to the same target. 
        // If there are different targets, this should also be taken care of as the firing arcs will 
        // split the targeting. 

        // Find first empty target slot
        for (auto &t : pdcTargets) {
          if (!t.has_value()) {
            t = target; // assign the target to the first empty slot
 
            // assign the pdc target to the PDC
            pdc.target = target;

            PDCTARGET_DEBUG << "\naddTarget " << ecs.getEntityName(pdcEntity) 
                      << " added target: " << target << "\n";
            break;
          }
        }
      }
    }
  }

  // Target the PDCs at target, optionally taking velocity and position into account
  void aquireTargets (bool predict = true) {

    auto &mounts = ecs.getComponent<PdcMounts>(e);

    // aquire the targets for the PDCs
    for (Entity pdcEntity : mounts.pdcEntities) {
      auto &pdc = ecs.getComponent<Pdc>(pdcEntity);

      if (pdc.target == INVALID_TARGET_ID) {
        PDCTARGET_DEBUG << "PdcTarget no target for : " << ecs.getEntityName(pdcEntity) << "\n";
        continue; // no target assigned to this PDC
      }

      PDCTARGET_DEBUG << "\non " << ecs.getEntityName(e)
                << " aquireTargets aquiring target for "
                << ecs.getEntityName(pdcEntity)
                << ": " << pdc.target << "\n";

      auto &targetPos = ecs.getComponent<Position>(pdc.target); 
      auto &targetVel = ecs.getComponent<Velocity>(pdc.target);
      auto &entityPos = ecs.getComponent<Position>(e);
      auto &entityRot = ecs.getComponent<Rotation>(e);
      auto &entityVel = ecs.getComponent<Velocity>(e);

      sf::Vector2f pdcOffset = rotateVector({pdc.positionx, pdc.positiony}, entityRot.angle);

      // get the angle to the target, adjust for the position of the pdc
      float att = angleToTarget(entityPos.value + pdcOffset, targetPos.value);
      PDCTARGET_DEBUG << "PdcTarget angle to target: " << att << "\n";

      // using velocity to predict the target's future position, not very good when the
      // target is moving fast.
      if (predict) {
        // estimate a targeting angle based on the target's velocity
        // use a simple prediction based on the target's velocity and distance
        sf::Vector2f distanceVector = targetPos.value - (entityPos.value + pdcOffset);

        float distance = distanceVector.length();

        // estimate the time to impact based on the distance and target velocity
        float timeToImpact = distance / pdc.projectileSpeed;

        // fudge factor for time to impact, as the change in angle is minimal
        timeToImpact *= 1.00f;

        PDCTARGET_DEBUG << "PdcTarget time to impact: " << timeToImpact << "\n";

        // for a fast moving target, we need to set to a maximum time to impact,
        // or else for a torpedo the vector will pass through us and our guess
        // angle will flip. There is a comprimise here as a higher clip is more
        // accurate for targeting, but will lead to incorrect torpedo targeting.
        timeToImpact = std::clamp(timeToImpact, 0.f, 1.5f);

        // Predict the target's position based on our relative velocity
        // and the time to impact

        // compute relative velocity
        sf::Vector2f relativeVel = targetVel.value - entityVel.value;
        PDCTARGET_DEBUG << "PdcTarget relative velocity: " << relativeVel.x << ", " << relativeVel.y << "\n";

        sf::Vector2f predictedTargetPos = distanceVector + (relativeVel * timeToImpact);
        PDCTARGET_DEBUG << "PdcTarget target position: " 
                  << targetPos.value.x << ", " << targetPos.value.y << "\n";

        // aim at a guessed future position
        sf::Vector2f guessDir = predictedTargetPos.normalized();

        float guessAngle = guessDir.angle().asDegrees();
        PDCTARGET_DEBUG << "PdcTarget guess angle: " << guessAngle << "\n";

        att = normalizeAngle(guessAngle);
        PDCTARGET_DEBUG << "PdcTarget final absolute angle: " << att << "\n";
      }

      pdc.firingAngle = att;
    }
  }


  // fire all the PDCs if it is ready and target in arc
  void firePdcBursts(Entity source, float tt, float burstSpread)  {

    auto &mounts = ecs.getComponent<PdcMounts>(e);

    // fire all the PDCs
    for (Entity pdcEntity : mounts.pdcEntities) {
      auto &pdc = ecs.getComponent<Pdc>(pdcEntity);
      auto &entityRot = ecs.getComponent<Rotation>(source);

      // calculate angular difference relative to the front of the ship
      // all other angles are relative to the front of the ship aswell, this keeps
      // all angles in relation to the ship's rotation
      float relativeFiringAngle = normalizeAngle(pdc.firingAngle - entityRot.angle);

      // spread the burst fire angle (beter chance of hitting torpedos and accelerating targets)

      // cycle the burst through +/- burstSpread,
      // introduce some randomness for one aft pdc.
      if (ecs.getEntityName(pdcEntity) == "PDC5") {
        pdc.burstSpreadAngle = burstSpread * std::cos(tt * 10.f); // oscillate between -burstSpread and +burstSpread
      }
      else {
        pdc.burstSpreadAngle = burstSpread * std::sin(tt * 10.f); // oscillate between -burstSpread and +burstSpread
      }

      // dont worry about a few degrees past the min or max
      pdc.firingAngle += pdc.burstSpreadAngle;

      PDCTARGET_DEBUG << ecs.getEntityName(pdcEntity)
        << " relative firing angle: " << relativeFiringAngle
        << " absolute firing angle: " << pdc.firingAngle
        << " entity rotation angle: " << entityRot.angle
        << " relative min: " << pdc.minFiringAngle
        << " relative max: " << pdc.maxFiringAngle
        << " source: " << source 
        << " burst spread angle: " << pdc.burstSpreadAngle << "\n";

      // check that we have a valid target and the target is within the firing angle of the PDCs
      if (pdc.target != INVALID_TARGET_ID && isInRange(relativeFiringAngle, pdc.minFiringAngle, pdc.maxFiringAngle)) {

        if (pdc.timeSinceBurst == 0 || tt > pdc.timeSinceBurst + pdc.pdcBurstCooldown) {
          pdc.timeSinceBurst = tt;
          pdc.pdcBurst = pdc.maxPdcBurst;
        }

        if (pdc.pdcBurst > 0 && pdc.rounds) {
          if (tt > pdc.timeSinceFired + pdc.cooldown) {
            pdc.timeSinceFired = tt;
            bulletFactory.fireone(source, pdcEntity, tt);
            pdc.rounds--;
            pdc.pdcBurst--;
            pdcFireSoundPlayer.play();
            PDCTARGET_DEBUG << ecs.getEntityName(pdcEntity) << " FIRING AT " << pdc.target << "\n";
          }
        }
      }
      else {
        PDCTARGET_DEBUG << ecs.getEntityName(pdcEntity)
          << " not firing, relative angle: " << relativeFiringAngle 
          << " is out or range. min: " << pdc.minFiringAngle 
          << " max: " << pdc.maxFiringAngle << "\n";

        // set the firing angle to one that is out of range for display purposes
        // this has the unfortunate side effect of firing the PDCs, so we check in the main 
        // if statement for valid target.
        pdc.firingAngle = pdc.minFiringAngle;
      }
    }
  }


private:
  Coordinator &ecs;
  Entity e;        // player or enemy that is using the pdcs
  BulletFactory bulletFactory;
  sf::Sound pdcFireSoundPlayer;
 
  std::map<float, Entity> torpedoTargetDistances; // map of targets and their distances

  std::array<std::optional<Entity>, 4> pdcTargets; // list of 4 current targets 
 
  // distance in pixels to consider a torpedo a threat.
  // has to be close enough for pdcs to track it.
  float torpedoThreatRange = 45000.f;
};
