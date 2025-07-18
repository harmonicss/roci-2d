#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include "utils.hpp"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <iostream>
#include <ostream>

#undef TORPEDO_AI_DEBUG

#if !defined(TORPEDO_AI_DEBUG)
// a streambuf that does nothing, used to redirect cout to a null stream
struct NullBuffer : std::streambuf {
  int overflow(int c) override { return c; } // do nothing
};

static NullBuffer nullBuffer; // global null buffer to use for cout
static std::ostream nullStream(&nullBuffer); // global null stream to use for cout

#define TORPEDO_DEBUG nullStream

#else

#define TORPEDO_DEBUG std::cout

#endif


class TorpedoAI {

public:
  TorpedoAI(Coordinator &ecs) : ecs(ecs) {
    TORPEDO_DEBUG << "TorpedoAI created" << std::endl;
  }
  ~TorpedoAI() = default;

  // Update Torpedo Targeting
  void Update(float tt, float dt) {

    float const max_lateral_accel = 1500.f; // maximum lateral acceleration (thrusters) for torpedos
 
    // need to get all the torpedos, find their targets and turn towards them
    for (auto &torpedo :
         ecs.view<Position, Velocity, Acceleration, Rotation, TorpedoTarget, TorpedoControl>()) {

      auto &torpedoPos = ecs.getComponent<Position>(torpedo);
      auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);
      auto &torpedoVel = ecs.getComponent<Velocity>(torpedo).value;
      auto &torpedoAcc = ecs.getComponent<Acceleration>(torpedo).value;
      auto &torpedoControl = ecs.getComponent<TorpedoControl>(torpedo);

      Entity target = ecs.getComponent<TorpedoTarget>(torpedo).target;

      // it is possible that the target has been destroyed
      if (ecs.isAlive(target) == false) {
        // if the target is not alive, remove the torpedo target component
        // TODO: will need some way to reaquire another target
        continue; // skip to the next torpedo
      }

      auto &targetPos = ecs.getComponent<Position>(target);
      auto &targetVel = ecs.getComponent<Velocity>(target).value;
      auto &targetAcc = ecs.getComponent<Acceleration>(target).value;

      // get the angle to the target
      float att = angleToTarget(torpedoPos.value, targetPos.value);

      TORPEDO_DEBUG << "\nTorpedoAI Entity: " << torpedo << "\n";
      TORPEDO_DEBUG << "TorpedoAI torpedo position: " << torpedoPos.value.x << ", " << torpedoPos.value.y << "\n";
      TORPEDO_DEBUG << "TorpedoAI angle to target: " << att << "\n";
      TORPEDO_DEBUG << "TorpedoAI torpedo angle: " << torpedoRot.angle << "\n";

      if (torpedoVel.x == 0.f && torpedoVel.y == 0.f) {
        TORPEDO_DEBUG << "Torpedo Velocity: " << 0 << "\n";
      }
      else {
        TORPEDO_DEBUG << "Torpedo Velocity: " << torpedoVel.length() << ", angle: " << torpedoVel.angle().asDegrees() << "\n";
      }

      if (torpedoAcc.x == 0.f && torpedoAcc.y == 0.f) {
        TORPEDO_DEBUG << "Torpedo Acc: " << 0 << "\n";
      }
      else {
        TORPEDO_DEBUG << "Torpedo Acc: " << torpedoAcc.length() << ", angle: " << torpedoAcc.angle().asDegrees() << "\n";
      }

      float dist = distance(torpedoPos.value, targetPos.value);
      TORPEDO_DEBUG << "TorpedoAI distance to target: " << dist << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Use 2D proportional navigation to calculate the angle and acceleration needed 
      // to hit the target.
      ///////////////////////////////////////////////////////////////////////////////////
 
      // relative position
      sf::Vector2f p_rel = targetPos.value - torpedoPos.value;

      // relative velocity
      sf::Vector2f v_rel = targetVel - torpedoVel;
      TORPEDO_DEBUG << "TorpedoAI relative speed: " << v_rel.length() << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Line of sight (angle to target) angular rate is the derivative of the angle
      // used to calculate the lateral acceleration magnitude
      float dot_att = 0.f;

      // too close or division by zero guard
      if (dist > 1e-4)
        // direct kinematic control
        dot_att = (p_rel.x * v_rel.y - p_rel.y * v_rel.x) / (p_rel.x * p_rel.x + p_rel.y * p_rel.y);

      TORPEDO_DEBUG << "TorpedoAI dot_att: " << dot_att << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Compute closing velocity
      // Vc = -Vrt . unit vector in the direction of L, take the original vector and divide it by
      // its length to get the unit vector
      sf::Vector2f los_unit_vector = p_rel / length(p_rel);

      float Vc = -v_rel.x * los_unit_vector.x - v_rel.y * los_unit_vector.y;

      TORPEDO_DEBUG << "TorpedoAI closing speed: " << Vc << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // Proportional navigation commanded lateral acceleration
      // torpedo applies a lateral acceleration (perpendicular to its velocity vector) of
      // magnitude Acc = N * Vc * |dot_atp|, where N is the proportional navigation constant
      // and Vc is the closing speed
      float Acc_N = 4.f * Vc * std::abs(dot_att); // N = 3 is a common value

      TORPEDO_DEBUG << "TorpedoAI commanded lateral acceleration: " << Acc_N << "\n";

      float Rem_Acc_N = 0.f;
 
      // as this is lateral (thruster) acceleration, we need to limit it to a reasonable value
      if (Acc_N > max_lateral_accel) {
        Rem_Acc_N = Acc_N - max_lateral_accel; 
        Rem_Acc_N = std::min(Rem_Acc_N, max_lateral_accel);
        Acc_N = max_lateral_accel;
      }
      else if (Acc_N < -max_lateral_accel) {
        Rem_Acc_N = Acc_N + max_lateral_accel;
        Rem_Acc_N = std::min(Rem_Acc_N,  -max_lateral_accel);
        Acc_N = -max_lateral_accel;
      }

      // convert the remaining lateral acceleration into a heading rate
      // the remaining lateral acceleration after thrust to be applied to the turn
      TORPEDO_DEBUG << "TorpedoAI remaining lateral acceleration: " << Rem_Acc_N << "\n";

      ////////////////////////////////////////////////////////////////////////////////////
      // resolve the acceleration perpendicular to the velocity vector of the torpedo

      // lateral unit vector of torpedo, in radians
      sf::Vector2f u_t = { static_cast<float>(std::cos(torpedoRot.angle * (M_PI / 180.f))), 
                           static_cast<float>(std::sin(torpedoRot.angle * (M_PI / 180.f))) };

      // "left" normal (rotate 90 degrees CCW) perpendicular vector
      sf::Vector2f perp_t = {-u_t.y, u_t.x};

      // decide sign base on sign of dot_atp
      float sign = (dot_att < 0.f) ? -1.f : 1.f;

      // new acceleration command needed for lateral movement (thrusters)
      sf::Vector2f a_cmd = perp_t * (sign * Acc_N);

      TORPEDO_DEBUG << "TorpedoAI lateral acceleration length: " << a_cmd.length() << "\n";
      if (a_cmd.length() > 0.f) 
        TORPEDO_DEBUG << "TorpedoAI lateral acceleration angle: " << a_cmd.angle().asDegrees() << "\n";
      else 
        TORPEDO_DEBUG << "TorpedoAI lateral acceleration angle: 0\n";

      float engine_accel = 0.f;

#if 0 // this doesnt really work
      // dont accelerate if we are facing the wrong way, this help prevent torpedo misses 
      // that have too much velocity from accelerating away from the target.
      // This also stops deceleration, but no way to get the torpedo to decelerate and then 
      // reengage. 
      if (isInRange(torpedoRot.angle,  att - 10.f, att + 10.f)) {
        // limit maximum velocity
        if (torpedoVel.length() > 10000.f) {
          // dont accelerate if we are going too fast
          engine_accel = 0.f;
        }
        else {
          // apply the engine acceleration

          // not sure how to get the launcher acceleration, so copy for now
          engine_accel = 2000.f;
        }
      }
      else {
        engine_accel = 0.f;
        std::cout << "TorpedoAI not accelerating, angle to target: " << att << ", torpedo angle: " << torpedoRot.angle << "\n";
      }
#else
      engine_accel = 2000.f;
#endif

      // apply continuous engine thrust, convert to radians
      sf::Vector2f a_engines = { static_cast<float>(std::cos(torpedoRot.angle * (M_PI / 180.f)) * engine_accel),
                                 static_cast<float>(std::sin(torpedoRot.angle * (M_PI / 180.f)) * engine_accel) };

      TORPEDO_DEBUG << "TorpedoAI engine acceleration length: " << a_engines.length() << "\n";
      if (a_engines.length() > 0.f) 
        TORPEDO_DEBUG << "TorpedoAI engine acceleration angle: " << a_engines.angle().asDegrees() << "\n";
      else 
        TORPEDO_DEBUG << "TorpedoAI engine acceleration angle: 0\n";


      // apply the sign from the dot lambda
      // Rem_Acc_N *= (dot_att < 0.f) ? -1.f : 1.f;

      // convert remaining lateral acceleration to a heading rate
      // hopefully the speed is always non-zero
      float omega = Rem_Acc_N / torpedoVel.length();

#if 0
      // apply the acceleration
      // the we have to perform a course change (as we are way out, probably just mssed), then stop the acceleration
      if (omega > 0.01f || omega < -0.01f) {
        // just use the thrusters 
        torpedoAcc = a_cmd;
        TORPEDO_DEBUG << "TorpedoAI using thrusters only, omega: " << omega << "\n";
      }
      else {
        // full power
        torpedoAcc = a_engines + a_cmd;
        TORPEDO_DEBUG << "TorpedoAI using engines and thrusters, omega: " << omega << "\n";
      }
#else
      torpedoAcc = a_engines + a_cmd;
#endif

      TORPEDO_DEBUG << "TorpedoAI commanded acceleration length: " << torpedoAcc.length() << "\n";
      if (torpedoAcc.length() > 0.f)
        TORPEDO_DEBUG << "TorpedoAI commanded acceleration angle: " << torpedoAcc.angle().asDegrees() << "\n";
      else 
        TORPEDO_DEBUG << "TorpedoAI commanded acceleration angle: 0\n";

      // convert to degrees per second
      omega = omega * (180.f / M_PI);
      omega *= 1.f;  // TODO: not sure about this

      // limit the heading rate to a maximum 
      // this might be causing some jumping round of the direction
      // TODO: not sure about this either
#if 0
      if (omega > 180.f) {
        omega = 180.f;
      } else if (omega < -180.f) {
        omega = -180.f;
      }
#endif
      TORPEDO_DEBUG << "TorpedoAI lateral heading rate: " << omega << "\n";
      std::cout << "TorpedoAI lateral heading rate: " << omega << "\n";

      // turn towards the target, also apply extra turn from the lateral thrust.
      if (torpedoControl.turning == false) {
        // startTurn(att, torpedo, target);
        // not really sure why subtraction works, maybe I get the sign wrong somewhere
        startTurn(att + omega, torpedo);
      }

      // perform the turn
      if (torpedoControl.turning) {
        performTurn(torpedo);
      }

      // get the direction/ angle of this acceleration
      // float new_angle = std::atan2(torpedoAcc.y, torpedoAcc.x) * (180.f / M_PI);


      // accelerate towards the target 
      // TODO: update to change acceleration based on distance
      // TODO: not getting torpedo acc from launcher atm
      // torpedoAcc.y = std::sin((torpedoRot.angle) * (M_PI / 180.f)) * 1000.f;
      // torpedoAcc.x = std::cos((torpedoRot.angle) * (M_PI / 180.f)) * 1000.f;
    }
  }

private:
  Coordinator &ecs;

  void startTurn(float atp, Entity torpedo) {
    auto &torpedoControl = ecs.getComponent<TorpedoControl>(torpedo);

    torpedoControl.targetAngle = atp;
    auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);

    TORPEDO_DEBUG << "Starting Turn to " << atp << "\n";

    float diff = torpedoControl.targetAngle - torpedoRot.angle;
    diff = normalizeAngle(diff);

    if (diff > 0.f) {
      torpedoControl.rotationDir = RotationDirection::CLOCKWISE;
      torpedoControl.turning = true;
    } 
    else {
      torpedoControl.rotationDir = RotationDirection::COUNTERCLOCKWISE;
      torpedoControl.turning = true;
    }
  }

  void performTurn(Entity torpedo) {
    auto &torpedoControl = ecs.getComponent<TorpedoControl>(torpedo);

    auto &torpedoRot = ecs.getComponent<Rotation>(torpedo);
    float diff = torpedoControl.targetAngle - torpedoRot.angle;
    diff = normalizeAngle(diff);

    TORPEDO_DEBUG << "Performing Turn to " << torpedoControl.targetAngle << ", Diff " << diff << " current angle " << torpedoRot.angle << "\n";

    // some leeway to ensure we stop
    if (diff >= -20.f && diff <= 20.f) {
      torpedoRot.angle = torpedoControl.targetAngle;
      torpedoControl.turning = false;
      TORPEDO_DEBUG << "Turn complete to " << torpedoControl.targetAngle << "\n";
    } 
    else if (torpedoControl.rotationDir == RotationDirection::CLOCKWISE) {
      torpedoRot.angle += 15.f; //(window.getSize().x / 100.f);
      TORPEDO_DEBUG << "Clockwise turn to " << torpedoRot.angle << "\n";
    }
    else {
      torpedoRot.angle -= 15.f; //(window.getSize().x / 100.f);
      TORPEDO_DEBUG << "CounterClockwise turn to " << torpedoRot.angle << "\n";
    }

    torpedoRot.angle = normalizeAngle(torpedoRot.angle);
  }
};
