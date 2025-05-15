#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include <SFML/System/Vector2.hpp>
#include <functional>
#include <iostream>
#include <cmath>

class CollisionSystem {
public:
  using CollisionCallback = std::function<void(Entity, Entity)>;

  CollisionSystem(Coordinator &ecs, CollisionCallback cb)
      : ecs(ecs), // Bind member variable to the passed in Coordinator
        onCollision(std::move(cb)) // Move callback into member
  {}

  void Update() {

    for (auto e : ecs.view<Collision>()) {
      auto &collision = ecs.getComponent<Collision>(e);
      auto &pos = ecs.getComponent<Position>(e);
      auto &rot = ecs.getComponent<Rotation>(e);

      // check for collisions with other entities
      for (auto other : ecs.view<Collision>()) {
        if (e == other)
          continue; // skip self collision

        auto& otherPos = ecs.getComponent<Position>(other);
        auto& otherCollision = ecs.getComponent<Collision>(other);
        auto& otherRot = ecs.getComponent<Rotation>(other);

        // Perform collision detection based on shape type
        if (collision.type      == ShapeType::AABB &&
            otherCollision.type == ShapeType::AABB) {
          if (AABBCollision(pos.value, rot.angle, collision.halfWidth, collision.halfHeight, 
                            otherPos.value, otherRot.angle, otherCollision.halfWidth, otherCollision.halfHeight)) {
            onCollision(e, other);
          }
        } else if (collision.type      == ShapeType::Circle &&
                   otherCollision.type == ShapeType::Circle) {
          if (CircleCollision(pos.value, collision.radius, 
                              otherPos.value, otherCollision.radius)) {
            onCollision(e, other);
          }
        }
      }
    }
  };


private:
  Coordinator &ecs;
  CollisionCallback onCollision;

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
    if (collision) {

      std::cout << 
        "AABB Collision detected " << "\n" << "pos1.x " << pos1.x << " pos1.y " << pos1.y << 
        " halfWidth1 " << halfWidth1 << " halfHeight1 " << halfHeight1 << "\n" <<
        "pos2.x " << pos2.x << " pos2.y " << pos2.y << 
        " halfWidth2 " << halfWidth2 << " halfHeight2 " << halfHeight2 << std::endl;

    }
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
