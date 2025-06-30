#pragma once

#include "components.hpp"
#include "ecs.hpp"
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

inline sf::Vector2f rotateVector(const sf::Vector2f& vec, float angleDegrees) {
    float radians = angleDegrees * (M_PI / 180.f);
    float cs = std::cos(radians);
    float sn = std::sin(radians);
    return {
        vec.x * cs - vec.y * sn,
        vec.x * sn + vec.y * cs
    };
}

inline void startTurn(Coordinator &ecs, ShipControl &shipControl, Entity e, float angle) {
  shipControl.targetAngle = angle;
  auto &eRot = ecs.getComponent<Rotation>(e);

  float diff = shipControl.targetAngle - eRot.angle;
  diff = normalizeAngle(diff);

  if (diff > 0.f) {
    shipControl.rotationDir = ShipControl::RotationDirection::CLOCKWISE;
    shipControl.turning = true;
  } 
  else {
    shipControl.rotationDir = ShipControl::RotationDirection::COUNTERCLOCKWISE;
    shipControl.turning = true;
  }
}

inline void performTurn(Coordinator &ecs, ShipControl &shipControl, Entity e) {
  auto &eRot = ecs.getComponent<Rotation>(e);
  float diff = shipControl.targetAngle - eRot.angle;

  // take into account the wrap around to get the shortest distance
  diff = normalizeAngle(diff);

  // std::cout << "Performing Turn to " << shipControl.targetAngle << ", Diff " << diff << " current angle " << enemyRot.angle << "\n";

  // some leeway to ensure we stop
  if (diff >= -20.f && diff <= 20.f) {
    eRot.angle = shipControl.targetAngle;
    shipControl.turning = false;
    // std::cout << "Turn complete to " << shipControl.targetAngle << "\n";
  } 
  else if (shipControl.rotationDir == ShipControl::RotationDirection::CLOCKWISE) {
    eRot.angle += 15.f; //(window.getSize().x / 100.f);
    // std::cout << "Clockwise turn to " << enemyRot.angle << "\n";
  }
  else {
    eRot.angle -= 15.f; //(window.getSize().x / 100.f);
    // std::cout << "CounterClockwise turn to " << enemyRot.angle << "\n";
  }

  eRot.angle = normalizeAngle(eRot.angle);
}

// TODO: Make sure that all components are removed here, 
// will need to add new components especially when destroying ships. 
inline void destroyEntity(Coordinator &ecs, Entity e) {
  ecs.removeComponent<Velocity>(e);
  ecs.removeComponent<Position>(e);
  ecs.removeComponent<Collision>(e);
  ecs.removeComponent<Rotation>(e);
  if (ecs.hasComponent<TorpedoTarget>(e)) {
    ecs.removeComponent<TorpedoTarget>(e);
  }
  if (ecs.hasComponent<TorpedoControl>(e)) {
    ecs.removeComponent<TorpedoControl>(e);
  }
  ecs.removeComponent<SpriteComponent>(e);
  if (ecs.hasComponent<TimeFired>(e)) {
    ecs.removeComponent<TimeFired>(e);
  }
  ecs.destroyEntity(e);
}
