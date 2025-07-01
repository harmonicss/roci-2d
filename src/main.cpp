#include "../include/collision.hpp"
#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include "../include/ballistics.hpp"
#include "../include/enemyai.hpp"
#include "../include/torpedoai.hpp"
#include "../include/targeting.hpp"
#include "../include/explosion.hpp"
#include "../include/hud.hpp"
#include "ships.cpp"
#include "../include/asteroids.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sys/types.h>


void destroyEntity(Coordinator &ecs, Entity e);
void DisplayWorldThreatRings(sf::RenderWindow& window, sf::Vector2f screenCentre, float zoomFactor);
void DrawWorldRangeRings(sf::RenderWindow& window, sf::Vector2f centerWorldPos, float zoomFactor, int ringCount = 6);

// use this to control the pdc targeting
enum class State {
  IDLE,
  ATTACK_PDC,
  DEFENCE_PDC,
};


int main() {

  auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "Rocinante",
                                 sf::Style::None);
  window.setFramerateLimit(60);

  State state = State::IDLE;

  std::vector<Explosion> explosions;

  ///////////////////////////////////////////////////////////////////////////////
  // - ECS Setup -
  ///////////////////////////////////////////////////////////////////////////////
  Coordinator ecs;
  ecs.registerComponent<Position>();
  ecs.registerComponent<Velocity>();
  ecs.registerComponent<Acceleration>();
  ecs.registerComponent<Rotation>();
  ecs.registerComponent<SpriteComponent>();
  ecs.registerComponent<Health>();
  ecs.registerComponent<Pdc>();
  ecs.registerComponent<TorpedoLauncher1>();
  ecs.registerComponent<TorpedoLauncher2>();
  ecs.registerComponent<Collision>();
  ecs.registerComponent<TorpedoTarget>();
  ecs.registerComponent<TimeFired>();
  ecs.registerComponent<PdcMounts>();
  ecs.registerComponent<TorpedoControl>();
  ecs.registerComponent<EnemyShipTarget>();
  ecs.registerComponent<FriendlyShipTarget>();
  ecs.registerComponent<ShipControl>();
  ecs.registerComponent<DrivePlume>();

  ///////////////////////////////////////////////////////////////////////////////
  // - Load Textures -
  ///////////////////////////////////////////////////////////////////////////////
  sf::Texture rociTexture, belterFrigateTexture, bulletTexture, torpedoTexture, 
              explosionTexture, driveTexture;

  if (!rociTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }
  std::cout << "Roci x " << rociTexture.getSize().x << " y "
            << rociTexture.getSize().y << "\n";

  // roci fighting itself for now
  if (!belterFrigateTexture.loadFromFile("../assets/textures/bashi-bazouk.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  if (!bulletTexture.loadFromFile("../assets/textures/pdc-bullet.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  if (!torpedoTexture.loadFromFile("../assets/textures/torpedo.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  if (!explosionTexture.loadFromFile("../assets/textures/explosion_sheet.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  if (!driveTexture.loadFromFile("../assets/textures/drive.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // - Load Sounds -
  ///////////////////////////////////////////////////////////////////////////////
  sf::SoundBuffer pdcFireSoundBuffer;
  if (!pdcFireSoundBuffer.loadFromFile("../assets/sounds/pdc.wav")) {
    std::cout << "Error loading sound" << std::endl;
    return -1;
  }
  sf::Sound pdcFireSoundPlayer(pdcFireSoundBuffer);

  sf::SoundBuffer pdcHitSoundBuffer;
  if (!pdcHitSoundBuffer.loadFromFile("../assets/sounds/pdc-hit.wav")) {
    std::cout << "Error loading sound" << std::endl;
    return -1;
  }
  sf::Sound pdcHitSoundPlayer(pdcHitSoundBuffer);

  sf::SoundBuffer explosionSoundBuffer;
  if (!explosionSoundBuffer.loadFromFile("../assets/sounds/explosion.wav")) {
    std::cout << "Error loading sound" << std::endl;
    return -1;
  }
  sf::Sound explosionSoundPlayer(explosionSoundBuffer);

  ///////////////////////////////////////////////////////////////////////////////
  // - Create Ship Entities -
  ///////////////////////////////////////////////////////////////////////////////

  PlayerShipFactory playerShipFactory(ecs, rociTexture, driveTexture);
  Entity player = playerShipFactory.createPlayerShip("Rocinante");

  BelterFrigateShipFactory belterShipFactory(ecs, belterFrigateTexture, driveTexture);
  Entity enemy1 = belterShipFactory.createBelterFrigateShip(
      "Bashi Bazouk", {0.f, -180000.f}, {0.f, 0.f}, 90.f, 50);

  Entity enemy2 = belterShipFactory.createBelterFrigateShip(
      "Behemoth", {14000.f, -180000.f}, {0.f, 0.f}, 90.f, 50);

  ///////////////////////////////////////////////////////////////////////////////
  // Create Ballistics Factory
  ///////////////////////////////////////////////////////////////////////////////
  BulletFactory bulletFactory(ecs, bulletTexture);
  TorpedoFactory torpedoFactory(ecs, torpedoTexture);

  ///////////////////////////////////////////////////////////////////////////////
  // Create Enemy and Torpedo AIs
  ///////////////////////////////////////////////////////////////////////////////
  EnemyAI enemy1AI(ecs, enemy1, bulletFactory, torpedoFactory, pdcFireSoundPlayer);
  EnemyAI enemy2AI(ecs, enemy2, bulletFactory, torpedoFactory, pdcFireSoundPlayer);
  TorpedoAI torpedoAI(ecs);

  // Create PDC Targeting System for player
  PdcTarget pdcTarget(ecs, player, bulletFactory, pdcFireSoundPlayer);

  // Set up worldview
  sf::FloatRect viewRect({0.f, 0.f}, {1920.f, 1080.f});
  sf::View worldview(viewRect);
  window.setView(worldview);

  // zoom out by default and then display
  float zoomFactor = 30.f;
  worldview.setSize({1920 * zoomFactor, 1080 * zoomFactor});

  ///////////////////////////////////////////////////////////////////////////////
  // create Asteroids
  ///////////////////////////////////////////////////////////////////////////////
  AsteroidFactory asteroidFactory(ecs);
  asteroidFactory.createInitialAsteroids();

  ///////////////////////////////////////////////////////////////////////////////
  // Create Collision System
  ///////////////////////////////////////////////////////////////////////////////
  CollisionSystem collisionSystem(ecs, pdcHitSoundPlayer, explosionSoundPlayer,
                                  explosions, explosionTexture, asteroidFactory);

  ///////////////////////////////////////////////////////////////////////////////
  // create the HUD object
  ///////////////////////////////////////////////////////////////////////////////
  HUD hud(ecs, player);


  sf::Clock clock;
  float tt = 0; // total time for weapon cooldown

  while (window.isOpen()) {
    float dt = clock.restart().asSeconds();
    tt += dt;

    // use this as a temporary fix to stop bullets colliding with the ship that
    // fired it means the bullets start away from the ship, but live for this
    // for now
    const uint32_t bullet_launch_distance = 500;

    // this is the main space window. Render this first, then render the HUD
    window.setView(worldview);

    u_int16_t screenWidth = window.getSize().x;
    u_int16_t screenHeight = window.getSize().y;
    sf::Vector2f screenCentre = {screenWidth / 2.f, screenHeight / 2.f};

    // main ship control structure
    auto &shipControl = ecs.getComponent<ShipControl>(player);

    ///////////////////////////////////////////////////////////////////////////////
    // - Events -
    ///////////////////////////////////////////////////////////////////////////////
    while (const std::optional event = window.pollEvent()) {

      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto *keyPressed =
                     event->getIf<sf::Event::KeyPressed>()) {

        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
          window.close();
        }
      } else if (event->is<sf::Event::MouseWheelScrolled>()) {
        auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>();

        if (scroll->delta < 0) {
          zoomFactor *= 1.3f;
        } else {
          zoomFactor /= 1.3f;
        }

        // min zoom factor
        if (zoomFactor < 1.f) {
          zoomFactor = 1.f;
        }

        std::cout << "Zoom Factor: " << zoomFactor << "\n";

        worldview.setSize({1920 * zoomFactor, 1080 * zoomFactor});
        window.setView(worldview);
      }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // - Physics: A->V->P -
  ///////////////////////////////////////////////////////////////////////////////
    for (auto e : ecs.view<Velocity, Acceleration>()) {
      auto &vel = ecs.getComponent<Velocity>(e);
      auto &acc = ecs.getComponent<Acceleration>(e);

      // update velocity
      vel.value += acc.value * dt;
    }

    for (auto e : ecs.view<Position, Velocity>()) {
      auto &vel = ecs.getComponent<Velocity>(e);
      auto &pos = ecs.getComponent<Position>(e);

      // update position
      pos.value += vel.value * dt;
    }

    for (auto e : ecs.view<Rotation>()) {

      auto &rot = ecs.getComponent<Rotation>(e);

      // update rotating objects
      rot.angle = rot.angle + rot.angularVelocity * dt;
    }

    // Collision System - check for collisions
    // check again later as sometimes only one of two missiles will hit
    collisionSystem.Update();

    ///////////////////////////////////////////////////////////////////////////////
    // Keyboard and flip control
    // dont use events for the keyboard, check if currently pressed.
    ///////////////////////////////////////////////////////////////////////////////
    if (shipControl.burning == true) {
      // burn deceleration to 0
      auto &acc = ecs.getComponent<Acceleration>(player);
      auto &vel = ecs.getComponent<Velocity>(player);

      accelerateToMax(ecs, player, 10.f, dt);

      // approaching 0 velocity
      if ((vel.value.length() < 100.f) && 
          (vel.value.length() > -100.f)) {
        shipControl.burning = false;
        acc.value.x = 0.f;
        acc.value.y = 0.f;
        vel.value.x = 0.f;
        vel.value.y = 0.f;
      }
    }
    else if (shipControl.flipping == false && shipControl.burning == false) {

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)) {
        // rotate left
        auto &rot = ecs.getComponent<Rotation>(player);
        //rot.angle -= (window.getSize().x / 500.f);
        rot.angle -= 90.f * dt;
        rot.angle = normalizeAngle(rot.angle);
      }
      else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
        // rotate right
        auto &rot = ecs.getComponent<Rotation>(player);
        // rot.angle += (window.getSize().x / 500.f);
        rot.angle += 90.f * dt;
        rot.angle = normalizeAngle(rot.angle);
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
        // accelerate
        auto &acc = ecs.getComponent<Acceleration>(player);
        auto &rot = ecs.getComponent<Rotation>(player);
        acc.value.x += std::cos((rot.angle) * (M_PI / 180.f)) * 500.f * dt;
        acc.value.y += std::sin((rot.angle) * (M_PI / 180.f)) * 500.f * dt;
 
        // limit the acceleration to a maximum value, I am assuming 100 = 1G
        // this is a bit arbitrary, but it is a good starting point, as I am limiting 
        // the maximum accel in x and y, rather than limiting the length of the vector
        acc.value.x = std::clamp(acc.value.x, -1000.f, 1000.f);
        acc.value.y = std::clamp(acc.value.y, -1000.f, 1000.f);
      }
      else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        ///////////////////////////////////////////////////////////////////////////////
        // Use the left mouse key to set rotation and acceleration
        ///////////////////////////////////////////////////////////////////////////////

        sf::Vector2i clickPosition = sf::Mouse::getPosition(window);
        sf::Vector2f clickPositionf(static_cast<float>(clickPosition.x),
                                    static_cast<float>(clickPosition.y));

        // convert to world coordinates, doesnt seem to work very well
        // sf::Vector2f worldPosition = window.mapPixelToCoords(clickPosition);

        auto &acc = ecs.getComponent<Acceleration>(player);
        auto &vel = ecs.getComponent<Velocity>(player);
        auto &rot = ecs.getComponent<Rotation>(player);

        sf::Vector2f playerPos = ecs.getComponent<Position>(player).value;

        sf::Vector2f cameraOffset = screenCentre - playerPos;
        sf::Vector2f newVector = clickPositionf - playerPos - cameraOffset;

        // std::cout << "new vector length " << newVector.length()
        //           << " angle: " << newVector.angle().asDegrees() << "\n";

        // turn towards the vector
        if (newVector.length() > 0.f) {
          // std::cout << "Turning towards vector: " << newVector.angle().asDegrees() << "\n";
          startTurn(ecs, shipControl, player, newVector.angle().asDegrees()); 

          // use the distance from the mouse click to set acceleration, max 10 Gs
          accelerateToMax(ecs, player, 10.f, dt);
        }
      }
      else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        ///////////////////////////////////////////////////////////////////////////////
        // Use the right mouse key to strafe
        ///////////////////////////////////////////////////////////////////////////////

        sf::Vector2i clickPosition = sf::Mouse::getPosition(window);
        sf::Vector2f clickPositionf(static_cast<float>(clickPosition.x),
                                    static_cast<float>(clickPosition.y));

        auto &acc = ecs.getComponent<Acceleration>(player);
        auto &vel = ecs.getComponent<Velocity>(player);
        auto &rot = ecs.getComponent<Rotation>(player);

        sf::Vector2f playerPos = ecs.getComponent<Position>(player).value;

        sf::Vector2f cameraOffset = screenCentre - playerPos;
        sf::Vector2f newVector = clickPositionf - playerPos - cameraOffset;

        // std::cout << "new vector length " << newVector.length()
        //           << " angle: " << newVector.angle().asDegrees() << "\n";

        // strafe towards the vector
        if (newVector.length() > 0.f) {
          // take this vector and strafe towards it

          // vel.value.x += std::cos((newVector.angle().asRadians())) * 1000.f * dt;
          // vel.value.y += std::sin((newVector.angle().asRadians())) * 1000.f * dt;
          vel.value += newVector.normalized() * 500.f * dt;
        }
      }
      else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        // flip and decelerate
        // debounce the flip
        // rotate to burn and reduce velocity, may not be 180 if there is 
        // some lateral movement
        if (tt > shipControl.timeSinceFlipped + shipControl.cooldown) {
          // std::cout << "Starting Flipping!\n";
          shipControl.timeSinceFlipped = tt;
          shipControl.flipping = true;
          auto &rot = ecs.getComponent<Rotation>(player);
          auto &vel = ecs.getComponent<Velocity>(player).value;

          // cannot calculate the angle from a zero vector
          if (vel.length() == 0.f) {
            // std::cout << "Flip correctionAngle: 0\n";
            shipControl.targetAngle = rot.angle + 180.f;
            shipControl.targetAngle = normalizeAngle(shipControl.targetAngle);
            shipControl.rotationDir = ShipControl::RotationDirection::CLOCKWISE;
          }
          else {
            // get the angle of the velocity vector
            sf::Angle correctionAngle = vel.angle();
            // std::cout << "Flip correctionAngle: " << correctionAngle.asDegrees() << "\n";
            shipControl.targetAngle = correctionAngle.asDegrees() - 180.f;
            float diff = std::abs(shipControl.targetAngle - rot.angle);
            diff = normalizeAngle(diff);

            if (diff > 0.f) {
              shipControl.rotationDir = ShipControl::RotationDirection::CLOCKWISE;
            } else {
              shipControl.rotationDir = ShipControl::RotationDirection::COUNTERCLOCKWISE;
            }

            shipControl.targetAngle = normalizeAngle(shipControl.targetAngle);
            // std::cout << "Flip target angle: " << shipControl.targetAngle << "\n";
          }
        }
      }
      else {
        // turn off acceleration if a acceleration key is not pressed
        auto &acc = ecs.getComponent<Acceleration>(player);
        acc.value.x = 0.f;
        acc.value.y = 0.f;
      }
    }
 
    ///////////////////////////////////////////////////////////////////////////////
    // Animate the flip
    ///////////////////////////////////////////////////////////////////////////////
    if (shipControl.flipping) {
      auto &rot = ecs.getComponent<Rotation>(player);
      float diff = std::abs(shipControl.targetAngle - rot.angle);
      rot.angle = normalizeAngle(rot.angle);

      // std::cout << "\ntargetAngle:   " << shipControl.targetAngle << "\n";
      // std::cout << "Current angle: " << rot.angle << "\n";
      // std::cout << "Flipping diff: " << diff << "\n";

      if (diff < 20.f) {
        rot.angle = shipControl.targetAngle;
      } 
      else if (shipControl.rotationDir == ShipControl::RotationDirection::CLOCKWISE) {
        rot.angle -= 15.f; //(window.getSize().x / 100.f);
      }
      else {
        rot.angle += 15.f; //(window.getSize().x / 100.f);
      }

      // TODO: wrap with Angle.wrapUnsigned
      rot.angle = normalizeAngle(rot.angle);

      // std::cout << "New angle:     " << rot.angle << "\n";
      if (rot.angle == shipControl.targetAngle) {
        std::cout << "Flipped to target angle: " << shipControl.targetAngle << "\n";
        shipControl.flipping = false;
        shipControl.targetAngle = 0.f;
        shipControl.burning = true;
      }
    }
    else if (shipControl.turning) {
      // perform the turn
      performTurn(ecs, shipControl, player);
    }


    ///////////////////////////////////////////////////////////////////////////////
    // Fire! Attacking PDCs
    ///////////////////////////////////////////////////////////////////////////////
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
      state = State::ATTACK_PDC;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Fire! Defensive PDCs Change to defence state
    ///////////////////////////////////////////////////////////////////////////////
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
      state = State::DEFENCE_PDC;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // simple state machine 
    // target all pdcs if attacking enemy, this updates the display of pdc target heading
    ///////////////////////////////////////////////////////////////////////////////
    if (state == State::ATTACK_PDC) {
      // target the enemy
      pdcTarget.pdcAttack<EnemyShipTarget>(tt);
    }
    else if (state == State::DEFENCE_PDC) {
      // target the nearest torpedo
      pdcTarget.pdcDefendTorpedo(tt, dt);
    }

    state = State::IDLE;

    ///////////////////////////////////////////////////////////////////////////////
    // Fire! Torpedo 1 & 2
    ///////////////////////////////////////////////////////////////////////////////
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {

      auto &launcher1 = ecs.getComponent<TorpedoLauncher1>(player);
      auto &launcher2 = ecs.getComponent<TorpedoLauncher2>(player);

      if ((launcher1.timeSinceFired == 0.f || tt > launcher1.timeSinceFired + launcher1.cooldown) && launcher1.rounds) {
        launcher1.timeSinceFired = tt;
        //TODO: need some way to target enemies
        torpedoFactory.fireone<TorpedoLauncher1>(player, enemy1);
        // TODO: add torpedo sound
        pdcFireSoundPlayer.play();
        launcher1.rounds--;
      }
      if ((launcher2.timeSinceFired == 0.f || tt > launcher2.timeSinceFired + launcher2.cooldown) && launcher2.rounds) {
        launcher2.timeSinceFired = tt;
        torpedoFactory.fireone<TorpedoLauncher2>(player, enemy2);
        pdcFireSoundPlayer.play();
        launcher2.rounds--;
      }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // - Update everying else -
    ///////////////////////////////////////////////////////////////////////////////
    // Enemy & Torpedo AIs
    enemy1AI.Update(tt, dt);
    enemy2AI.Update(tt, dt);
    torpedoAI.Update(tt, dt);
    bulletFactory.Update(tt); // remove bullets that have been fired for too long

    // Collision System - check for collisions again
    // it is possible that two missiles have collided near a target, so we need to check again
    collisionSystem.Update();


    ///////////////////////////////////////////////////////////////////////////////
    // - Render -
    ///////////////////////////////////////////////////////////////////////////////
    window.clear(sf::Color(7, 5, 8));

    // prefer the world view
    DisplayWorldThreatRings(window, screenCentre, zoomFactor);
    DrawWorldRangeRings(window, screenCentre, zoomFactor);

    ///////////////////////////////////////////////////////////////////////////////
    // draw all the sprites
    ///////////////////////////////////////////////////////////////////////////////
    for (auto e : ecs.view<Position, Rotation, SpriteComponent>()) {

      auto &pos = ecs.getComponent<Position>(e);
      auto &rot = ecs.getComponent<Rotation>(e);
      auto &sc = ecs.getComponent<SpriteComponent>(e);

      sf::Angle angle = sf::degrees(rot.angle);
      sc.sprite.setRotation(angle);

      // center the screen for the player
      if (e == player) {
        sc.sprite.setPosition(screenCentre);
      } else {
        auto &playerpos = ecs.getComponent<Position>(player);

        sf::Vector2f cameraOffset = screenCentre - playerpos.value;
        sc.sprite.setPosition(pos.value + cameraOffset);
      }

      window.draw(sc.sprite);

      // check if we have a drive plume
      if (ecs.hasComponent<DrivePlume>(e)) {
        auto &acc = ecs.getComponent<Acceleration>(e);
        auto &dp = ecs.getComponent<DrivePlume>(e);
        float accelLength = acc.value.length();
 
        dp.sprite.setRotation(sf::degrees(rot.angle));

        // adjust the drive plume sprite based on the acceleration length
        if (accelLength > 0.f) {
          dp.sprite.setScale(sf::Vector2f{5.f + (accelLength / 100.f), 5.f + (accelLength / 500.f)});
        } else {
          dp.sprite.setScale(sf::Vector2f{0.f, 0.f});
        }

        // center the screen for the player
        if (e == player) {
          sf::Vector2f drivePlumePosition = rotateVector(dp.offset, rot.angle);
          dp.sprite.setPosition(screenCentre + drivePlumePosition);
        } else {
          sf::Vector2f drivePlumePosition = rotateVector(dp.offset, rot.angle);
          auto &playerpos = ecs.getComponent<Position>(player);

          sf::Vector2f cameraOffset = screenCentre - playerpos.value;
          dp.sprite.setPosition(pos.value + cameraOffset + drivePlumePosition);
        }

        window.draw(dp.sprite);
      }
    }


    // Draw the explosions
    for (auto &explosion : explosions) explosion.Update(dt);

    // Remove all finished explosions
    //
    // std::remove_if : This rearranges the vector so that all unwanted elements 
    //                  (e.finished == true) are moved to the end. It returns an
    //                  iterator pointing to the new logical end of the "kept" elements.
    //
    // The lambda [](Explosion& e) { return e.finished; } is the predicate.
    // If it returns true, the element is considered removed.
    // So: it marks explosions where e.finished == true.
    //
    // explosions.erase(...)
    // This actually erases elements from the container, using the iterator returned by remove_if.
    //
    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
                       [](const Explosion &e) { return e.finished; }),
        explosions.end()
    );

    for (auto &explosion : explosions) explosion.Draw(window, ecs);


    ///////////////////////////////////////////////////////////////////////////////
    // Use a seperate HUD view, note that zoomFactor is needed here as it is 
    // not part of the zoomable world view.
    ///////////////////////////////////////////////////////////////////////////////
    sf::View hudView = window.getDefaultView();
    window.setView(hudView);
    hud.DrawHUD(window, enemy1, enemy2, zoomFactor);
    window.display();
  }
}


void DisplayWorldThreatRings(sf::RenderWindow& window, sf::Vector2f screenCentre, float zoomFactor) {
    // GUI outer radius is torpedo threat range
    // middle is pdc thread range
    // inner is best pdc range
    std::array<int, 3> radius{8000, 16000, 45000};
 
    // Circles
    for (auto r : radius) {
      sf::CircleShape shape(r);
      shape.setPointCount(100);
      shape.setFillColor(sf::Color::Transparent);
      shape.setOutlineThickness(1.5f * zoomFactor);
      // shape.setOutlineColor(sf::Color(75, 20, 26));
      shape.setOutlineColor(sf::Color(100, 61, 61)); // lighter pink for close ranges

      // move the origin to the centre of the circle
      shape.setOrigin(sf::Vector2f(r, r));
      shape.setPosition(screenCentre);

      window.draw(shape);
    }
}

void DrawWorldRangeRings(sf::RenderWindow& window, sf::Vector2f centerWorldPos, float zoomFactor, int ringCount) {

  float baseSpacing = 150.f; // base spacing between rings
  float spacing = baseSpacing * zoomFactor; // adjust spacing based on zoom factor

  // dont show the rings if we are zoomed in
  if (zoomFactor < 100.f) {
    ringCount = 0;
  }

  // std::cout << "Drawing " << ringCount << " range rings with spacing: " << spacing << "\n";

  for (size_t i = 1; i <= ringCount; ++i) {
    float radius = i * spacing; // calculate radius for each ring

    sf::CircleShape ring(radius);
    ring.setPointCount(100);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(1.2f * zoomFactor);

    // Fade with range
    float alpha = std::max(0.f, 240.f - i * 10.f);   // simple fade rule
    ring.setOutlineColor(sf::Color(75, 20, 26, static_cast<uint8_t>(alpha)));

    ring.setOrigin(sf::Vector2f{radius, radius});
    ring.setPosition(centerWorldPos);

    window.draw(ring);  // Let SFML view transform handle scaling
  }
}
