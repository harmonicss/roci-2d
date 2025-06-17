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

struct TimeFired {
  float value = 0.f; // time in seconds
};

enum class PdcFireMode { BURST, CONTINUOUS };

// manage all the pdcs on a ship
struct PdcMounts {
  std::vector<Entity> pdcEntities;
};

// a generic PDC structure that can be used for different types of PDCs, and 
// different pdc mounts on a ship.
struct Pdc {
  PdcFireMode fireMode;
  float firingAngle;                 // absolute, relative to the front of the ship and not rotatated
  float burstSpreadAngle;            // absolute small angle to spread the burst
  float minFiringAngle;              // not quite behind the ship
  float maxFiringAngle;
  float cooldown;
  float timeSinceFired;
  float projectileSpeed;
  u_int32_t projectileDamage;
  u_int32_t rounds;
  Entity target;                     // target for the PDC, can be a ship or a torpedo
  uint32_t pdcBurst;                 // number of rounds fired in the burst
  uint32_t maxPdcBurst;
  float timeSinceBurst;
  float pdcBurstCooldown;
  float positionx;                   // position of the pdc on the ship
  float positiony;
};

struct TorpedoLauncher1 {
  float firingAngle = -35.f;         // helps avoid collision
  float firingOffset = -100.f;       // to create a seperation distance between launchers
  float cooldown = 5.0;
  float timeSinceFired = 0.f;
  float projectileSpeed = 100.f;
  float projectileAccel = 500.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 8;
};

struct TorpedoLauncher2 {
  float firingAngle = 35.f;
  float firingOffset = 100.f;
  float cooldown = 5.0f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 100.f;
  float projectileAccel = 500.f;
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

// get a torpedo to target a ship, or a pdc to target a torpedo.
struct Target {
  Entity target;
};
