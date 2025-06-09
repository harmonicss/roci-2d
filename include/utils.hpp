#pragma once

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

