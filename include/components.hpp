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
  float firingAngle;
  float cooldown = 0.10f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 2000.f;
  u_int32_t projectileDamage = 2;
  u_int32_t rounds = 100;
};

struct Pdc2 {
  float firingAngle;
  float cooldown = 0.10f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 2000.f;
  u_int32_t projectileDamage = 2;
  u_int32_t rounds = 100;
};

struct TorpedoLauncher1 {
  float firingAngle = 0;
  float firingOffset = -100.f; // to create a seperation distance between launchers
  float cooldown = 5.0;
  float timeSinceFired = 0.f;
  float projectileSpeed = 100.f;
  float projectileAccel = 1000.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 8;
};

struct TorpedoLauncher2 {
  float firingAngle = 0;
  float firingOffset = 100.f;
  float cooldown = 5.0f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 100.f;
  float projectileAccel = 1000.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 8;
};

enum class ShapeType { AABB, Circle};

// Describes the collision shape of an entity
struct Collision {
  ShapeType type;
  float halfWidth = 0.f;  // for AABB
  float halfHeight = 0.f; // for AABB
  float radius = 0.f;     // for Circle
};
