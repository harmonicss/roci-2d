#pragma once
#include "ecs.hpp"
#include "components.hpp"
#include <cmath>

class PdcTarget {
public:
  // Entity e is the player or enemy that is using the pdcs
  PdcTarget(Coordinator &ecs, Entity e) : ecs(ecs) {
    std::cout << "PdcTarget created for entity: " << e << std::endl;
  }
  ~PdcTarget() = default;

  void Update(float tt, float dt) {
    // target incoming torpedos or enemy ships
    Entity nearestTorpedo = 0;  // might confuse with the player
    float nearestTorpedoDist = std::numeric_limits<float>::max();

    // first priority is to target torpedos
    for (auto &torpedo : ecs.view<Target>()) {
      auto &torpedoTarget = ecs.getComponent<Target>(torpedo);

      if (torpedoTarget.target != e) {
        // skip if the torpedo is not targeting this entity
        continue;
      }

      // find the nearest torpedo to the player or enemy

      auto &torpedoPos = ecs.getComponent<Position>(torpedo);
      auto &torpedoVel = ecs.getComponent<Velocity>(torpedo).value;

      auto &myPos = ecs.getComponent<Position>(e);

      float dist = distance(myPos.value, torpedoPos.value);

      if (dist < nearestTorpedoDist) {
        nearestTorpedoDist = dist;
        nearestTorpedo = torpedo;
      }
    }

    if (nearestTorpedo == 0) {
      return; // no torpedos to target
    }

    std::cout << "PdcTarget nearest torpedo: " << nearestTorpedo << "\n";
    std::cout << "PdcTarget nearest torpedo distance: " << nearestTorpedoDist << "\n";

    // update the target for the PDCs
    for (auto &pdc : ecs.view<Pdc1, Pdc2>()) {
      auto &pdc1 = ecs.getComponent<Pdc1>(pdc);
      auto &pdc2 = ecs.getComponent<Pdc2>(pdc);

      auto &torpedoPos = ecs.getComponent<Position>(nearestTorpedo);
      auto &entityPos = ecs.getComponent<Position>(e);
      auto &entityRot = ecs.getComponent<Rotation>(e);

      // get the angle to the nearest torpedo
      float att = angleToTarget(entityPos.value, torpedoPos.value);

      // get rotated firing angle based on the entity's rotation
      float rotatedAngle = att - entityRot.angle;
      std::cout << "PdcTarget rotated angle to nearest torpedo: " << rotatedAngle << "\n";

      // check if the torpedo is within the firing angle of the PDCs
      if (rotatedAngle >= pdc1.minFiringAngle && rotatedAngle <= pdc1.maxFiringAngle) {
        pdc1.firingAngle = rotatedAngle;
        pdc1.target = nearestTorpedo; // set the target for the PDC
        std::cout << "PdcTarget PDC1 FIRING angle: " << pdc1.firingAngle << "\n";
        std::cout << "PdcTarget PDC1 target: " << pdc1.target << "\n";
      } else {
        pdc1.firingAngle = 0.f; // reset firing angle if not in range
        std::cout << "PdcTarget PDC1 not firing \n";
      }

      if (rotatedAngle >= pdc2.minFiringAngle && rotatedAngle <= pdc2.maxFiringAngle) {
        pdc2.firingAngle = rotatedAngle;
        pdc2.target = nearestTorpedo; // set the target for the PDC
        std::cout << "PdcTarget PDC2 FIRING angle: " << pdc1.firingAngle << "\n";
        std::cout << "PdcTarget PDC2 target: " << pdc1.target << "\n";
      } else {
        pdc2.firingAngle = 0.f; // reset firing angle if not in range
        std::cout << "PdcTarget PDC2 not firing\n";
      }
    }

    // then target enemy ships


  }


private:
  Coordinator &ecs;
  Entity e;  // player or enemy that is using the pdcs


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
