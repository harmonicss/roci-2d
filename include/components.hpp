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
  float angularVelocity = 0.f; // in degrees per second
};

struct SpriteComponent {
  sf::Sprite sprite;
};

struct Health {
  signed int value = 100;
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
  float cooldown = 2.0f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 100.f;
  float projectileAccel = 500.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 8;
  u_int32_t barrageRounds = 2;       // number of rounds to fire in a barrage. Only use in enemyAI
  u_int32_t barrageCount = 0; 
  float timeSinceBarrage = 0.f; 
  float barrageCooldown = 120.f;
};

struct TorpedoLauncher2 {
  float firingAngle = 35.f;
  float firingOffset = 100.f;
  float cooldown = 2.0f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 100.f;
  float projectileAccel = 500.f;
  u_int32_t projectileDamage = 200;
  u_int32_t rounds = 8;
  u_int32_t barrageRounds = 2; 
  u_int32_t barrageCount = 0; 
  float timeSinceBarrage = 0.f; 
  float barrageCooldown = 120.f;
};

enum class ControlState { IDLE, TURNING, BURNING_ACCEL, FLIPPING, BURNING_DECEL, DONE };

enum class RotationDirection { CLOCKWISE, COUNTERCLOCKWISE };

struct TorpedoControl {
  bool turning = false;
  bool burning = false;
  bool flipping = false;
  float targetAngle = 0.f;
  sf::Vector2f targetPosition;
  sf::Vector2f targetAcceleration;
  RotationDirection rotationDir = RotationDirection::CLOCKWISE;
};

struct ShipControl {
  ControlState state = ControlState::IDLE;
  float targetAngle = 0.f;
  float timeSinceFlipped = 0.f;
  float flipCooldown = 0.5f;
  float flipAndBurnDistance = 0.f; // the total distance the ship will travel while accelerating, flipping and burning
  float distanceTraveled = 0.f;    // distance traveled while flipping and burning
  float flipAndBurnMaxAccGs = 0.f; // max acceleration in Gs for the flip and burn
  sf::Vector2f targetPosition;     // unused atm
  sf::Vector2f targetAcceleration; // unused atm
  RotationDirection rotationDir = RotationDirection::CLOCKWISE;
};

enum class CollisionType { SHIP, PROJECTILE, TORPEDO, ASTEROID };

enum class ShapeType { AABB, Circle};

// Describes the collision shape of an entity
struct Collision {
  Entity firedBy; // the entity that fired this projectile, used for preventing collisions from the entity that fired it
  ShapeType type;
  CollisionType ctype;
  u_int32_t damage = 0; // damage dealt by this collision, used for projectiles and torpedos
  float halfWidth = 0.f;  // for AABB
  float halfHeight = 0.f; // for AABB
  float radius = 0.f;     // for Circle
};

// get a torpedo to target a ship
struct TorpedoTarget {
  Entity target;  // the target of the torpedo
};

// get a ship to target a ship
// targeting is handles seperately so just want to know if they are an enemy ship
struct EnemyShipTarget {

  // This is abit redundant as the entity that owns this component will have the same id,
  // but allows me to get a list of enemy ships.
  Entity ship;  // the id of the enemy ship.
};

struct FriendlyShipTarget {

  // again, redundant
  Entity ship;  // the id of the friendly ship.
};

struct DrivePlume {
  // this is used to create a drive plume for the ship
  // it is a sprite that is positioned behind the ship
  // and is scaled based on the speed of the ship
  sf::Sprite sprite;
  sf::Vector2f offset{0.f, 0.f}; // position of the drive plume relative to the ship
};
