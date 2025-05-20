#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <sys/types.h>

// 2D Vector Alias
using Vec2 = sf::Vector2f;

struct Position {
  Vec2 value{0.f, 0.f};
};

struct Velocity {
  Vec2 value{0.f, 0.f};
};

struct Acceleration {
  Vec2 value{0.f, 0.f};
};

struct Rotation {
  float angle = 0.f; // in degrees
};

struct SpriteComponent {
  sf::Sprite sprite;
};

struct Health {
  unsigned int value = 100;
};

// Point Defense Cannon
struct Pdc1 {
  float firingAngle = -45.f;
  float cooldown = 0.10f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 2000.f;
  u_int32_t rounds = 100;
};

struct Pdc2 {
  float firingAngle = +45.f;
  float cooldown = 0.10f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 2000.f;
  u_int32_t rounds = 100;
};

enum class ShapeType { AABB, Circle};

// Describes the collision shape of an entity
struct Collision {
  ShapeType type;
  float halfWidth = 0.f;  // for AABB
  float halfHeight = 0.f; // for AABB
  float radius = 0.f;     // for Circle
};
