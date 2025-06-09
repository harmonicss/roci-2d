#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <sys/types.h>
#include "../include/ecs.hpp"

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

enum class PdcFireMode { BURST, CONTINUOUS };

// Point Defense Cannon
struct Pdc1 {
  PdcFireMode fireMode = PdcFireMode::BURST;
  float firingAngle;                 // absolute, relative to the front of the ship and not rotatated
  float burstSpreadAngle;            // absolute small angle to spread the burst
  float minFiringAngle = -170.f;     // not quite behind the ship
  float maxFiringAngle = 10.f;
  float cooldown = 0.02f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 5000.f;
  u_int32_t projectileDamage = 2;
  u_int32_t rounds = 600;
  Entity target;                     // target for the PDC, can be a ship or a torpedo
  uint32_t pdcBurst = 0;             // number of rounds fired in the burst
  uint32_t maxPdcBurst = 20;
  float timeSinceBurst = 0.f;
  float pdcBurstCooldown = 1.f;
};

struct Pdc2 {
  PdcFireMode fireMode = PdcFireMode::BURST;
  float firingAngle;
  float burstSpreadAngle;            // absolute small angle to spread the burst
  float minFiringAngle = -10.f;      // just past the front of the ship 
  float maxFiringAngle = 170.f;
  float cooldown = 0.02f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 5000.f;
  u_int32_t projectileDamage = 2;
  u_int32_t rounds = 600;
  Entity target;                     // target for the PDC, can be a ship or a torpedo
  uint32_t pdcBurst = 0;             // number of rounds fired in the burst
  uint32_t maxPdcBurst = 20;
  float timeSinceBurst = 0.f;
  float pdcBurstCooldown = 1.f;
};

struct TorpedoLauncher1 {
  float firingAngle = -35.f;         // helps avoid collision
  float firingOffset = -100.f;       // to create a seperation distance between launchers
  float cooldown = 5.0;
  float timeSinceFired = 0.f;
  float projectileSpeed = 500.f; 
  float projectileAccel = 1000.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 8;
};

struct TorpedoLauncher2 {
  float firingAngle = 35.f;
  float firingOffset = 100.f;
  float cooldown = 5.0f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 500.f;
  float projectileAccel = 1000.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 0;
};

enum class ShapeType { AABB, Circle};

// Describes the collision shape of an entity
struct Collision {
  ShapeType type;
  float halfWidth = 0.f;  // for AABB
  float halfHeight = 0.f; // for AABB
  float radius = 0.f;     // for Circle
};

// get a torpedo to target a ship, or a pdc to target a torpedo.
struct Target {
  Entity target;
};
