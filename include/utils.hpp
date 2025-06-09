#pragma once

#include <SFML/System/Vector2.hpp>
#include <cmath>

// check if an angle is within a range, considering wrap-around
inline bool isInRange(float angle, float minAngle, float maxAngle) {
  if (minAngle > maxAngle) {
    // wrap around case
    return angle >= minAngle || angle <= maxAngle;
  }
  return angle >= minAngle && angle <= maxAngle;
}

// Helper function to normalize angles to [-180, 180]
inline float normalizeAngle(float angle) {
  while (angle >= 180.f) angle -= 360.f;
  while (angle < -180.f) angle += 360.f;
  return angle;
};

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
