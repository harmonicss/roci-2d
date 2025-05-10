#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>

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

struct Weapon {
  float cooldown = 0.5f;
  float timeSinceFired = 0.f;
  float projectileSpeed = 600.f;
};

