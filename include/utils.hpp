#pragma once

#include "components.hpp"
#include "ecs.hpp"
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

inline float randFloat(float min, float max) {
  return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

inline float randInt(int min, int max) {
  return static_cast<float>(min + rand() % (max - min + 1));
}

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

inline sf::Vector2f normalizeVector(sf::Vector2f v) {
  float len = length(v);
  if (len == 0.f) return {0.f, 0.f}; // avoid division by zero
  return {v / len};
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

  // only turn if we are idle
  if (shipControl.state != ControlState::IDLE) {
      return;
  }

  auto &eRot = ecs.getComponent<Rotation>(e);

  // already at target angle
  if (eRot.angle == angle) {
    // std::cout << "Already at target angle: " << angle << "\n";
    if (shipControl.flipAndBurnDistance > 0.f) {
      shipControl.state = ControlState::BURNING_ACCEL;
      std::cout << "Starting Burn! Distance: " << shipControl.flipAndBurnDistance << "\n";
    } 
    else {
      shipControl.state = ControlState::IDLE;
    }
    return;
  }

  shipControl.targetAngle = angle;

  float diff = shipControl.targetAngle - eRot.angle;
  diff = normalizeAngle(diff);

  if (diff > 0.f) {
    shipControl.rotationDir = RotationDirection::CLOCKWISE;
    shipControl.state = ControlState::TURNING;
  }
  else {
    shipControl.rotationDir = RotationDirection::COUNTERCLOCKWISE;
    shipControl.state = ControlState::TURNING;
  }
}

inline void performTurn(Coordinator &ecs, ShipControl &shipControl, Entity e) {
  if (shipControl.state == ControlState::TURNING) {
    auto &eRot = ecs.getComponent<Rotation>(e);
    float diff = shipControl.targetAngle - eRot.angle;

    // take into account the wrap around to get the shortest distance
    diff = normalizeAngle(diff);

    // std::cout << "Performing Turn to " << shipControl.targetAngle << ", Diff " << diff << " current angle " << enemyRot.angle << "\n";

    // some leeway to ensure we stop
    if (diff >= -20.f && diff <= 20.f) {
      eRot.angle = shipControl.targetAngle;

      // std::cout << "Turn complete to " << shipControl.targetAngle << "\n";
        // if we are flipping and burning, we need to start the burn
      if (shipControl.flipAndBurnDistance > 0.f) {
        shipControl.state = ControlState::BURNING_ACCEL;
        std::cout << "Starting Burn! Distance: " << shipControl.flipAndBurnDistance << "\n";
      } 
      else {
        // otherwise we are done turning
        shipControl.state = ControlState::IDLE;
      }
    }
    else if (shipControl.rotationDir == RotationDirection::CLOCKWISE) {
      eRot.angle += 15.f; //(window.getSize().x / 100.f);
      // std::cout << "Clockwise turn to " << enemyRot.angle << "\n";
    }
    else {
      eRot.angle -= 15.f; //(window.getSize().x / 100.f);
      // std::cout << "CounterClockwise turn to " << enemyRot.angle << "\n";
    }

    eRot.angle = normalizeAngle(eRot.angle);
  }
}

// set and limit acceleration based on the ship's rotation
inline void accelerateToMax(Coordinator &ecs, ShipControl &shipControl, Entity e, float maxAccGs, float dt) {

  auto &acc = ecs.getComponent<Acceleration>(e);
  auto &vel = ecs.getComponent<Velocity>(e);
  auto &rot = ecs.getComponent<Rotation>(e);

  acc.value.x += std::cos((rot.angle) * (M_PI / 180.f)) * 500.f * dt;
  acc.value.y += std::sin((rot.angle) * (M_PI / 180.f)) * 500.f * dt;

  // std::cout << "\nEntity: " << e << " new acceleration: " << acc.value.x << ", " << acc.value.y << "\n";

  float maxx = std::cos((rot.angle) * (M_PI / 180.f)) * maxAccGs * 100.f;
  float maxy = std::sin((rot.angle) * (M_PI / 180.f)) * maxAccGs * 100.f;

  // std::cout << "max acceleration: " << maxx << ", " << maxy << "\n";

  // rearrange as maxx/y can be negative.
  // note: using auto here caused issues as the return type uses structured
  // bindings which didnt always return the correct type. Fixed with explict type.
  std::pair<float, float> xlimits = std::minmax(-maxx, maxx);
  std::pair<float, float> ylimits = std::minmax(-maxy, maxy);

  float xmin = xlimits.first;
  float xmax = xlimits.second;
  float ymin = ylimits.first;
  float ymax = ylimits.second;

  // std::cout << "acceleration limits: " << xmin << ", " << xmax << ", " << ymin << ", " << ymax << "\n";

  // limit to Gs. TODO: make this ship specific at some point
  acc.value.x = std::clamp(acc.value.x, xmin, xmax);
  acc.value.y = std::clamp(acc.value.y, ymin, ymax);
 
  // std::cout << "Entity: " << e << " final acceleration: " << acc.value.x << ", " << acc.value.y << "\n";
}

inline void startAccelBurnAndFlip(Coordinator &ecs, ShipControl &shipControl, Entity e, float angle,
                      float maxAccGs, float distance, float dt) {

  startTurn(ecs, shipControl, e, angle);
  shipControl.flipAndBurnMaxAccGs = maxAccGs;
  shipControl.flipAndBurnDistance = distance;
  shipControl.distanceTraveled = 0.f;
  std::cout << "Entity: " << e << " Starting Burn! Distance: " << distance << " ControlState: " << static_cast<int>(shipControl.state) << "\n";
}

inline void startFlipAndStop(Coordinator &ecs, ShipControl &shipControl, Entity e, 
                             float maxAccGs, float tt) {

  if (shipControl.state != ControlState::BURNING_ACCEL &&
      shipControl.state != ControlState::IDLE) {
    // std::cout << "Cannot start flip and stop, not in BURNING_ACCEL or IDLE state!\n";
    return;
  }

  if (tt > shipControl.timeSinceFlipped + shipControl.flipCooldown) {
    std::cout << "Starting Flipping!\n";
    shipControl.timeSinceFlipped = tt;
    shipControl.flipAndBurnMaxAccGs = maxAccGs;
    shipControl.state = ControlState::FLIPPING;
    auto &rot = ecs.getComponent<Rotation>(e);
    auto &vel = ecs.getComponent<Velocity>(e).value;

    // cannot calculate the angle from a zero vector
    if (vel.length() == 0.f) {
      // std::cout << "Flip correctionAngle: 0\n";
      shipControl.targetAngle = rot.angle + 180.f;
      shipControl.targetAngle = normalizeAngle(shipControl.targetAngle);
      shipControl.rotationDir = RotationDirection::CLOCKWISE;
    }
    else {
      // get the angle of the velocity vector
      sf::Angle correctionAngle = vel.angle();
      // std::cout << "Flip correctionAngle: " << correctionAngle.asDegrees() << "\n";
      shipControl.targetAngle = correctionAngle.asDegrees() - 180.f;
      float diff = std::abs(shipControl.targetAngle - rot.angle);
      diff = normalizeAngle(diff);

      if (diff > 0.f) {
        shipControl.rotationDir = RotationDirection::CLOCKWISE;
      } else {
        shipControl.rotationDir = RotationDirection::COUNTERCLOCKWISE;
      }

      shipControl.targetAngle = normalizeAngle(shipControl.targetAngle);
      std::cout << "Flip target angle: " << shipControl.targetAngle << "\n";
    }
  }
}

inline void performFlip(Coordinator &ecs, ShipControl &shipControl, Entity e) {

    if (shipControl.state == ControlState::FLIPPING) {
      auto &rot = ecs.getComponent<Rotation>(e);
      float diff = std::abs(shipControl.targetAngle - rot.angle);
      diff = normalizeAngle(diff);

      // std::cout << "\ntargetAngle:   " << shipControl.targetAngle << "\n";
      // std::cout << "Current angle: " << rot.angle << "\n";
      // std::cout << "Flipping diff: " << diff << "\n";

      if (diff > -20.f && diff < 20.f) {
        rot.angle = shipControl.targetAngle;
      }
      else if (shipControl.rotationDir == RotationDirection::CLOCKWISE) {
        rot.angle -= 15.f; //(window.getSize().x / 100.f);
      }
      else {
        rot.angle += 15.f; //(window.getSize().x / 100.f);
      }

      rot.angle = normalizeAngle(rot.angle);

      // std::cout << "New angle:     " << rot.angle << "\n";
      if (rot.angle == shipControl.targetAngle) {
        std::cout << "Flipped to target angle: " << shipControl.targetAngle << "\n";
        shipControl.targetAngle = 0.f;
        shipControl.state = ControlState::BURNING_DECEL;
      }
    }
}

inline void performStop(Coordinator &ecs, ShipControl &shipControl, Entity e, float maxAccGs, float dt) {

  // std::cout << "Performing Stop\n";

  // burn deceleration to 0
  auto &acc = ecs.getComponent<Acceleration>(e);
  auto &vel = ecs.getComponent<Velocity>(e);
  auto &rot = ecs.getComponent<Rotation>(e);

  if (vel.value.length() == 0.f) {
    // not moving, nothing to do
    shipControl.state = ControlState::DONE;
    return;
  }

  // targetAngle should be facing away
  float diff = shipControl.targetAngle + 180.f - vel.value.angle().asDegrees();
  diff = normalizeAngle(diff);
  float diffabs = std::abs(diff);

  // if our stop vector has been changed due to collision avoidance, make small correctons here
  if (diffabs > 0.1f) {
    rot.angle = vel.value.angle().asDegrees() - 180.f;
    rot.angle = normalizeAngle(rot.angle);
    shipControl.targetAngle = rot.angle;
    std::cout << "Correcting rotation angle to: " << rot.angle << " diff: " << diff << "\n";
  }

  accelerateToMax(ecs, shipControl, e, maxAccGs, dt);

  // approaching 0 velocity
  if ((vel.value.length() < 100.f) &&
      (vel.value.length() > -100.f)) {
    shipControl.state = ControlState::DONE;
    shipControl.flipAndBurnDistance = 0.f;
    acc.value.x = 0.f;
    acc.value.y = 0.f;
    vel.value.x = 0.f;
    vel.value.y = 0.f;
  }
}

inline void updateControlState(Coordinator &ecs, ShipControl &shipControl, Entity e,
                               float tt, float dt) {

  // DONE -> IDLE is performed in the enemyAI, as an external signal to get of of 
  // enemyAI FLIP_AND_BURN state. 
  if (shipControl.state == ControlState::TURNING) {
    performTurn(ecs, shipControl, e);
  }
  else if (shipControl.state == ControlState::BURNING_ACCEL) {
    accelerateToMax(ecs, shipControl, e, shipControl.flipAndBurnMaxAccGs, dt);

    shipControl.distanceTraveled += ecs.getComponent<Velocity>(e).value.length() * dt;

    // std::cout << "Distance travelled : " << shipControl.distanceTraveled
    //   << ", flip at: "
    //   << shipControl.flipAndBurnDistance / 2 << " flipAndBurnDistance: "
    //   << shipControl.flipAndBurnDistance << "\n";

    if (shipControl.distanceTraveled > shipControl.flipAndBurnDistance / 2) {
      // we are done accelerating, start flipping
      startFlipAndStop(ecs, shipControl, e, shipControl.flipAndBurnMaxAccGs, tt);
      std::cout << "Starting Flip!\n";
    }
  }
  else if (shipControl.state == ControlState::FLIPPING) {
    performFlip(ecs, shipControl, e);
  }
  else if (shipControl.state == ControlState::BURNING_DECEL) {
   performStop(ecs, shipControl, e, shipControl.flipAndBurnMaxAccGs, dt);
  }
  else if (shipControl.state == ControlState::IDLE) {
    // nothing to do, just idle
  }
  else if (shipControl.state == ControlState::DONE) {
    // we are done, set to idle
    shipControl.state = ControlState::IDLE;
  }
  else {
    std::cerr << "Entity: " << e << " Unknown control state: " << static_cast<int>(shipControl.state) << "\n";
  }
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
