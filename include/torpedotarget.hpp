#pragma once
#include "ecs.hpp"
#include "ballistics.hpp"
#include "components.hpp"
#include "utils.hpp"
#include <cmath>
#include <SFML/Audio.hpp>

#undef TORPEDOTARGET_AI_DEBUG

#if !defined(TORPEDOTARGET_AI_DEBUG)
// a streambuf that does nothing, used to redirect cout to a null stream
struct TorpedoNullBuffer : std::streambuf {
  int overflow(int c) override { return c; } // do nothing
};

static TorpedoNullBuffer torpedoNullBuffer; // global null buffer to use for cout
static std::ostream torpedoNullStream(&torpedoNullBuffer); // global null stream to use for cout

#define TORPEDOTARGET_DEBUG torpedoNullStream

#else

#define PDCTARGET_DEBUG std::cout

#endif


// to target the torpedoes
class TorpedoTargeting {
public:
  // Entity e is the player or enemy that is using the pdcs
  TorpedoTargeting(Coordinator &ecs, Entity e, TorpedoFactory &torpedoFactory) :
    ecs(ecs),
    e(e),
    torpedoFactory(torpedoFactory)
  {
    TORPEDOTARGET_DEBUG << "PdcTarget created for entity: " << e << std::endl;
  }
  ~TorpedoTargeting() = default;


  template<typename TargetType> // should be EnemyShipTarget or FriendlyShipTarget
  void Update() {
    static_assert(std::is_same_v<TargetType, EnemyShipTarget> ||
                  std::is_same_v<TargetType, FriendlyShipTarget>,
                  "TargetType must be EnemyShipTarget or FriendlyShipTarget");

    Entity nearestShip = INVALID_TARGET_ID;  // so as not to confuse with the player 
    float nearestShipDist = std::numeric_limits<float>::max();

    shipTargetDistances.clear(); // clear the map of torpedo distances

    // find target ships
    // find the nearest for range, and add to the shipTargetDistances
    for (auto &ship : ecs.view<TargetType>()) {

      // find the nearest ship to the player or enemy
      auto &enemyPos = ecs.getComponent<Position>(ship);
      auto &myPos = ecs.getComponent<Position>(e);

      float dist = distance(myPos.value, enemyPos.value);

      // add each torpedo into the map, which is ordered by distance
      shipTargetDistances[dist] = ship;

      TORPEDOTARGET_DEBUG << "\nTorpedo aquireTargets added enemy ship: " << ship << " distance: " << dist << "\n";

      if (dist < nearestShipDist) {
        nearestShipDist = dist;
        nearestShip = ship;
      }
    }

    // no enemy ships to target
    if (nearestShip == INVALID_TARGET_ID) {
      return;
    }

    // no enemy ships in range
    if (nearestShipDist > shipThreatRange) {
      // TORPEDOTARGET_DEBUG << "Torpedo Update no enemy ship in range\n";
      return;
    }

    TORPEDOTARGET_DEBUG << "Torpedo aquireTargets nearest enemy ship: " << nearestShip << "\n";
    TORPEDOTARGET_DEBUG << "Torpedo aquireTargets nearest enemy ship distance: " << nearestShipDist << "\n";
  }

  // take the next target on the shipTargetDistance map and display on the torpedo window
  void selectNextTarget() {
    selectTargetIndex++;

    if (!shipTargetDistances.empty()) {
      if (selectTargetIndex >= shipTargetDistances.size()) {
        selectTargetIndex = 0; // reset to the first target
      }
    }
    else {
      selectTargetIndex = 0; // reset to the first target
    }
  }

  // called from hud
  sf::String getTarget() {
    if (!shipTargetDistances.empty()) {
      auto it = shipTargetDistances.begin();
      std::advance(it, selectTargetIndex);  // get the next one
      if (ecs.isAlive(it->second)) {
        return ecs.getEntityName(it->second);
      }
    }

    return "No Target";
  }

  Entity getTargetEntity() {
    if (!shipTargetDistances.empty()) {
      auto it = shipTargetDistances.begin();
      std::advance(it, selectTargetIndex);  // get the next one
      return (it->second);
    }

    return INVALID_TARGET_ID; // no target
  }

  void setLauncher1Target(Entity target) {
    launcher1Target = target;
    TORPEDOTARGET_DEBUG << "Torpedo setLauncher1Target: " << target << "\n";
  }

  void setLauncher2Target(Entity target) {
    launcher2Target = target;
    TORPEDOTARGET_DEBUG << "Torpedo setLauncher2Target: " << target << "\n";
  }

  sf::String getLauncher1Target() {
    if (launcher1Target != INVALID_TARGET_ID && ecs.isAlive(launcher1Target)) {
      return ecs.getEntityName(launcher1Target);
    }
    else 
      return "No Target";
  }

  sf::String getLauncher2Target() {
    if (launcher2Target != INVALID_TARGET_ID && ecs.isAlive(launcher2Target)) {
      return ecs.getEntityName(launcher2Target);
    }
    else 
      return "No Target";
  }

  void fireBoth (float tt) {
    auto &launcher1 = ecs.getComponent<TorpedoLauncher1>(e);
    auto &launcher2 = ecs.getComponent<TorpedoLauncher2>(e);

    if (launcher1Target != INVALID_TARGET_ID) {
      if ((launcher1.timeSinceFired == 0.f || tt > launcher1.timeSinceFired + launcher1.cooldown) && 
        launcher1.rounds) {
        launcher1.timeSinceFired = tt;
        torpedoFactory.fireone<TorpedoLauncher1>(e, launcher1Target);
        TORPEDOTARGET_DEBUG << "Torpedo fireLauncher1: " << launcher1Target << "\n";
        // TODO: add torpedo sound
        launcher1.rounds--;
      }
    }

    if (launcher2Target != INVALID_TARGET_ID) {
      if ((launcher2.timeSinceFired == 0.f || tt > launcher2.timeSinceFired + launcher2.cooldown) && 
        launcher2.rounds) {
        launcher2.timeSinceFired = tt;
        torpedoFactory.fireone<TorpedoLauncher2>(e, launcher2Target);
        TORPEDOTARGET_DEBUG << "Torpedo fireLauncher2: " << launcher2Target << "\n";
        // TODO: add torpedo sound
        launcher2.rounds--;
      }
    }
  }

private:
  const Entity INVALID_TARGET_ID = 0xFFFF; // used to indicate no target
  Coordinator &ecs;
  Entity e;        // player or enemy that is using the torpedos
  TorpedoFactory &torpedoFactory;

  Entity launcher1Target = INVALID_TARGET_ID; // target for launcher 1
  Entity launcher2Target = INVALID_TARGET_ID; // target for launcher 2

  u_int32_t selectTargetIndex = 0;

  float shipThreatRange = 1000000.f; // distance in pixels to consider an ship a threat for torpedo targeting

  std::map<float, Entity> shipTargetDistances;         // map of ships and their distances
};

