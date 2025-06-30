#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include "explosion.hpp"
#include "utils.hpp"
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Audio.hpp>
#include <functional>
#include <cmath>

class CollisionSystem {
public:
  using CollisionCallback = std::function<void(Entity, Entity)>;
  using CollisionHandler  = std::function<void(Entity, Entity)>;

  CollisionSystem(Coordinator &ecs,
                  sf::Sound &pdcHitSoundPlayer,
                  sf::Sound &explosionSoundPlayer,
                  std::vector<Explosion> &explosions,
                  sf::Texture &explosionTexture,
                  CollisionCallback cb)
      : ecs(ecs), // Bind member variable to the passed in Coordinator
        pdcHitSoundPlayer(pdcHitSoundPlayer),
        explosionSoundPlayer(explosionSoundPlayer),
        explosions(explosions),
        explosionTexture(explosionTexture),
        onCollision(std::move(cb)) // Move callback into member
  {
    registerCollisionHandlers();
  }

  void Update() {

    for (auto e : ecs.view<Collision>()) {

      // it is possible that the entity has been destroyed by the callback in the meantime
      if (!ecs.isAlive(e))
        continue;

      auto &collision = ecs.getComponent<Collision>(e);
      auto &pos = ecs.getComponent<Position>(e);
      auto &rot = ecs.getComponent<Rotation>(e);

      // check for collisions with other entities
      for (auto other : ecs.view<Collision>()) {
        if (e == other)
          continue; // skip self collision

        // it is possible that the entity has been destroyed by the callback in the meantime
        if (!ecs.isAlive(e))
          continue;

        auto& otherCollision = ecs.getComponent<Collision>(other);
        auto& otherPos = ecs.getComponent<Position>(other);
        auto& otherRot = ecs.getComponent<Rotation>(other);

        // Perform collision detection based on shape type
        if (collision.type      == ShapeType::AABB &&
            otherCollision.type == ShapeType::AABB) {
          if (AABBCollision(pos.value, rot.angle, collision.halfWidth, collision.halfHeight, 
                            otherPos.value, otherRot.angle, otherCollision.halfWidth, otherCollision.halfHeight)) {
            // onCollision(e, other);
            handleCollision(e, other);
          }
        } else if (collision.type      == ShapeType::Circle &&
                   otherCollision.type == ShapeType::Circle) {
          if (CircleCollision(pos.value, collision.radius, 
                              otherPos.value, otherCollision.radius)) {
            // onCollision(e, other);
            handleCollision(e, other);
          }
        }
      }
    }
  };


private:
  Coordinator &ecs;
  sf::Sound &pdcHitSoundPlayer;
  sf::Sound &explosionSoundPlayer;
  CollisionCallback onCollision;
  std::vector<Explosion> &explosions; // to store explosions
  sf::Texture &explosionTexture; // texture for explosions

  inline static std::map<std::pair<CollisionType, CollisionType>, CollisionHandler> collisionHandlers;

  void registerCollisionHandlers() {

    // SHIP & ASTEROID
    collisionHandlers[{CollisionType::SHIP, CollisionType::ASTEROID}] = 
      [](Entity e1, Entity e2) {
        // damage will depend on the speed of both. 
        std::cout << "Ship vs Asteroid collision detected between " << e1 << " and " << e2 << "\n";
      };

    collisionHandlers[{CollisionType::ASTEROID, CollisionType::SHIP}] = 
      [](Entity e1, Entity e2) {
        std::cout << "Ship vs Asteroid collision detected between " << e1 << " and " << e2 << "\n";
      };
 
    // SHIP & PROJECTILE
    collisionHandlers[{CollisionType::SHIP, CollisionType::PROJECTILE}] = 
      [this](Entity e1, Entity e2) {
        // grab any pdc for now
        auto &mounts = ecs.getComponent<PdcMounts>(e1);
        auto &ehealth = ecs.getComponent<Health>(e1);
        ehealth.value -= ecs.getComponent<Pdc>(mounts.pdcEntities[0]).projectileDamage;
        pdcHitSoundPlayer.play();
        destroyEntity(ecs, e2);
      };

    collisionHandlers[{CollisionType::PROJECTILE, CollisionType::SHIP}] = 
      [this](Entity e1, Entity e2) {
        // grab any pdc for now
        auto &mounts = ecs.getComponent<PdcMounts>(e2);
        auto &ehealth = ecs.getComponent<Health>(e2);
        ehealth.value -= ecs.getComponent<Pdc>(mounts.pdcEntities[0]).projectileDamage;
        pdcHitSoundPlayer.play();
        destroyEntity(ecs, e1);
      };

    // SHIP & TORPEDO
    collisionHandlers[{CollisionType::SHIP, CollisionType::TORPEDO}] = 
      [this](Entity e1, Entity e2) {
        std::cout << "Ship vs Torpedo collision detected between " << e1 << " and " << e2 << "\n";
        auto &ehealth = ecs.getComponent<Health>(e1);
        ehealth.value -= ecs.getComponent<TorpedoLauncher1>(e1).projectileDamage;

        // trigger explosion
        auto &e2pos = ecs.getComponent<Position>(e2);
        explosions.emplace_back(&explosionTexture, e2pos.value, 8, 7);
        explosionSoundPlayer.play();
        destroyEntity(ecs, e2);
      };

    collisionHandlers[{CollisionType::TORPEDO, CollisionType::SHIP}] = 
      [this](Entity e1, Entity e2) {
        std::cout << "Ship vs Torpedo collision detected between " << e1 << " and " << e2 << "\n";
        auto &ehealth = ecs.getComponent<Health>(e2);
        ehealth.value -= ecs.getComponent<TorpedoLauncher1>(e2).projectileDamage;

        // trigger explosion
        auto &e1pos = ecs.getComponent<Position>(e1);
        explosions.emplace_back(&explosionTexture, e1pos.value, 8, 7);
        explosionSoundPlayer.play();
        destroyEntity(ecs, e1);
      };

    // PROJECTILE & TORPEDO
    collisionHandlers[{CollisionType::PROJECTILE, CollisionType::TORPEDO}] = 
      [this](Entity e1, Entity e2) {
        std::cout << "Projectile vs Torpedo collision detected between " << e1 << " and " << e2 << "\n";
        // trigger explosion
        auto &e2pos = ecs.getComponent<Position>(e2);
        explosions.emplace_back(&explosionTexture, e2pos.value, 8, 7);
        explosionSoundPlayer.play();
        destroyEntity(ecs, e1);
        destroyEntity(ecs, e2);
      };

    collisionHandlers[{CollisionType::TORPEDO, CollisionType::PROJECTILE}] = 
      [this](Entity e1, Entity e2) {
        std::cout << "Projectile vs Torpedo collision detected between " << e1 << " and " << e2 << "\n";
        // trigger explosion
        auto &e1pos = ecs.getComponent<Position>(e1);
        explosions.emplace_back(&explosionTexture, e1pos.value, 8, 7);
        explosionSoundPlayer.play();
        destroyEntity(ecs, e1);
        destroyEntity(ecs, e2);
      };

    // ASTEROID & PROJECTILE
    collisionHandlers[{CollisionType::ASTEROID, CollisionType::PROJECTILE}] = 
      [](Entity e1, Entity e2) {
        std::cout << "Asteroid vs Projectile collision detected between " << e1 << " and " << e2 << "\n";
      };

    collisionHandlers[{CollisionType::PROJECTILE, CollisionType::ASTEROID}] = 
      [](Entity e1, Entity e2) {
        std::cout << "Asteroid vs Projectile collision detected between " << e1 << " and " << e2 << "\n";
      };

    // ASTEROID & TORPEDO
    collisionHandlers[{CollisionType::ASTEROID, CollisionType::TORPEDO}] =
      [](Entity e1, Entity e2) {
        std::cout << "Asteroid vs Torpedo collision detected between " << e1 << " and " << e2 << "\n";
      };

    collisionHandlers[{CollisionType::TORPEDO, CollisionType::ASTEROID}] =
      [](Entity e1, Entity e2) {
        std::cout << "Asteroid vs Torpedo collision detected between " << e1 << " and " << e2 << "\n";
      };

    // SHIP & SHIP
    collisionHandlers[{CollisionType::SHIP, CollisionType::SHIP}] = 
      [](Entity e1, Entity e2) {
        // currently ships cant collide with each other, so do nothing.
      };

    // TORPEDO & TORPEDO
    collisionHandlers[{CollisionType::TORPEDO, CollisionType::TORPEDO}] = 
      [](Entity e1, Entity e2) {
        // torpedos cant collide with each other, so do nothing.
      };

    // PROJECTILE & PROJECTILE
    collisionHandlers[{CollisionType::PROJECTILE, CollisionType::PROJECTILE}] = 
      [](Entity e1, Entity e2) {
        // bullets cant collide with each other, so do nothing.
      };
  }
 
  void handleCollision(Entity e1, Entity e2) {
    // Get the collision types of the entities
    CollisionType type1 = ecs.getComponent<Collision>(e1).ctype;
    CollisionType type2 = ecs.getComponent<Collision>(e2).ctype;

    // prevent collision between the firer and the bullet or torpedo fired
    // this stops fire/launch collisions
    if (ecs.getComponent<Collision>(e1).firedBy == e2 ||
        ecs.getComponent<Collision>(e2).firedBy == e1) {
      // std::cout << "Collision between " << e1Name << " and " << e2Name
      //           << " prevented due to being fired by the other entity.\n";
      return;
    }

    // Check if a handler exists for this pair of collision types
    auto it = collisionHandlers.find({type1, type2});
    if (it != collisionHandlers.end()) {
      it->second(e1, e2);
    } else {
      std::cout << "No collision handler for " << static_cast<int>(type1) << " and " << static_cast<int>(type2) << "\n";
    }
  }


  // check for AABB collision
  // use a dynamic AABB rectangle, whish isnt the best solution as the rectangle gets bigger 
  // in certain situations, but will do for now. 
  bool AABBCollision (sf::Vector2f pos1, float rot1, float halfWidth1, float halfHeight1, 
                      sf::Vector2f pos2, float rot2, float halfWidth2, float halfHeight2) {

    // take into account the rotation of the object, if it is big enough to make a difference
    float hw1new = 0.f;
    float hh1new = 0.f;
    float hw2new = 0.f;
    float hh2new = 0.f;

    // compute absolute cos/sin
    if (halfWidth1 > 100.f || halfHeight1 > 100.f) {
      float ca1 = std::abs(std::cos(rot1 * (M_PI / 180.f)));
      float sa1 = std::abs(std::sin(rot1 * (M_PI / 180.f)));

      hw1new = halfWidth1 * ca1 + halfHeight1 * sa1;
      hh1new = halfWidth1 * sa1 + halfHeight1 * ca1;

      halfWidth1 = hw1new;
      halfHeight1 = hh1new;
    }

    if (halfWidth2 > 100.f || halfHeight2 > 100.f) {
      float ca2 = std::abs(std::cos(rot2 * (M_PI / 180.f)));
      float sa2 = std::abs(std::sin(rot2 * (M_PI / 180.f)));

      hw2new = halfWidth2 * ca2 + halfHeight2 * sa2;
      hh2new = halfWidth2 * sa2 + halfHeight2 * ca2;

      halfWidth2 = hw2new;
      halfHeight2 = hh2new;
    }

    bool collision = (pos1.x - halfWidth1 < pos2.x + halfWidth2 &&
            pos1.x + halfWidth1 > pos2.x - halfWidth2 &&
            pos1.y - halfHeight1 < pos2.y + halfHeight2 &&
            pos1.y + halfHeight1 > pos2.y - halfHeight2);
#if 0
    if (collision) {

      std::cout << 
        "AABB Collision detected " << "\n" << "pos1.x " << pos1.x << " pos1.y " << pos1.y << 
        " halfWidth1 " << halfWidth1 << " halfHeight1 " << halfHeight1 << "\n" <<
        "pos2.x " << pos2.x << " pos2.y " << pos2.y << 
        " halfWidth2 " << halfWidth2 << " halfHeight2 " << halfHeight2 << std::endl;

    }
#endif
    return collision;
  }

  bool CircleCollision (sf::Vector2f pos1, float radius1, 
                        sf::Vector2f pos2, float radius2) {
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = radius1 + radius2;
    return distanceSquared < (radiusSum * radiusSum);
  }

  #if 0
    bool CirclevsCircle(const TransformComponent& t1, const CollisionComponent& c1,
                        const TransformComponent& t2, const CollisionComponent& c2) const {
        float dx = t1.position.x - t2.position.x;
        float dy = t1.position.y - t2.position.y;
        float distSq = dx * dx + dy * dy;
        float radiusSum = c1.radius + c2.radius;
        return distSq <= (radiusSum * radiusSum);
    }

    bool AABBvsCircle(const TransformComponent& tA, const CollisionComponent& cA,
                      const TransformComponent& tC, const CollisionComponent& cC) const {
        float nearestX = std::max(tA.position.x - cA.halfWidth,
                                  std::min(tC.position.x, tA.position.x + cA.halfWidth));
        float nearestY = std::max(tA.position.y - cA.halfHeight,
                                  std::min(tC.position.y, tA.position.y + cA.halfHeight));
        float dx = tC.position.x - nearestX;
        float dy = tC.position.y - nearestY;
        return (dx * dx + dy * dy) <= (cC.radius * cC.radius);
    }
  #endif
};
