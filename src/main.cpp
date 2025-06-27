#include "../include/collision.hpp"
#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include "../include/ballistics.hpp"
#include "../include/enemyai.hpp"
#include "../include/torpedoai.hpp"
#include "../include/targeting.hpp"
#include "../include/explosion.hpp"
#include "ships.cpp"
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

extern void DrawHUD(sf::RenderWindow &window, Coordinator &ecs, Entity player,
                    sf::Font &font);
extern void DrawSidebarText(sf::RenderWindow &window, Coordinator &ecs,
                            Entity player, sf::Font &font);
extern void DrawShipNames(sf::RenderWindow &window, Coordinator &ecs, Entity e,
                          sf::Font &font, float zoomFactor);
extern void DrawTorpedoOverlay(sf::RenderWindow &window, Coordinator &ecs,
                               sf::Font &font, float zoomFactor);
extern void DrawVectorOverlay(sf::RenderWindow &window, Coordinator &ecs, Entity e,
                              sf::Font &font, float zoomFactor);


void destroyEntity(Coordinator &ecs, Entity e);

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

  // - ECS Setup -
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

  ///////////////////////////////////////////////////////////////////////////////
  // - Load Fonts -
  ///////////////////////////////////////////////////////////////////////////////
  sf::Font font;
  if (!font.openFromFile("../assets/fonts/FiraCodeNerdFont-Medium.ttf")) {
    std::cout << "Error loading font" << std::endl;
    return -1;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // - Load Textures -
  ///////////////////////////////////////////////////////////////////////////////
  sf::Texture rociTexture, belterFrigateTexture, bulletTexture, torpedoTexture, explosionTexture;

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

  PlayerShipFactory playerShipFactory(ecs, rociTexture);
  Entity player = playerShipFactory.createPlayerShip("Rocinante");

  BelterFrigateShipFactory belterShipFactory(ecs, belterFrigateTexture);
  Entity enemy1 = belterShipFactory.createBelterFrigateShip(
      "Bashi Bazouk", {0.f, -180000.f}, {0.f, 0.f}, 90.f, 50);

  Entity enemy2 = belterShipFactory.createBelterFrigateShip(
      "Behemoth", {14000.f, -180000.f}, {0.f, 0.f}, 90.f, 50);

  ///////////////////////////////////////////////////////////////////////////////
  // Create Collision System, with lambda callback
  ///////////////////////////////////////////////////////////////////////////////
  CollisionSystem collisionSystem(ecs, pdcHitSoundPlayer, explosionSoundPlayer, explosions, explosionTexture,
                                  [&ecs, &pdcHitSoundPlayer, &explosionSoundPlayer, &explosions, &explosionTexture](Entity e1, Entity e2) {
    std::string e1Name = ecs.getEntityName(e1);
    std::string e2Name = ecs.getEntityName(e2);

    // bullets cant collide with each other, so only destroy if they hit the player or enemy or torpedo
    if (e1Name == "Bullet" && e2Name == "Bullet") {
      return;
    }

    // torpedos cant collide with each other, so only destroy if they hit the player or enemy or bullet
    if (e1Name == "Torpedo" && e2Name == "Torpedo") {
      // std::cout << "Collision between two torpedos detected, but not handled.\n";
      return;
    }

    // prevent collision between the firer and the bullet or torpedo fired
    // this stops fire/launch collisions
    if (ecs.getComponent<Collision>(e1).firedBy == e2 ||
        ecs.getComponent<Collision>(e2).firedBy == e1) {
      // std::cout << "Collision between " << e1Name << " and " << e2Name
      //           << " prevented due to being fired by the other entity.\n";
      return;
    }

    // std::cout << "Collision detected between " << e1 << " and " << e2 << "\n";

    // Damage the health of the entities
    if (e1Name == "Bullet") {

      // torpedo doesnt have any health
      if (e2Name != "Torpedo") {
        auto &ehealth = ecs.getComponent<Health>(e2);

        // just grab any pdc for now
        auto &mounts = ecs.getComponent<PdcMounts>(e2);
        ehealth.value -= ecs.getComponent<Pdc>(mounts.pdcEntities[0]).projectileDamage;
      }
      pdcHitSoundPlayer.play();
      destroyEntity(ecs, e1);
    }
    else if (e2Name == "Bullet") {

      // torpedo doesnt have any health
      if (e1Name != "Torpedo") {
        auto &ehealth = ecs.getComponent<Health>(e1);

        // just grab any pdc for now
        auto &mounts = ecs.getComponent<PdcMounts>(e1);
        ehealth.value -= ecs.getComponent<Pdc>(mounts.pdcEntities[0]).projectileDamage;
      }
      pdcHitSoundPlayer.play();
      destroyEntity(ecs, e2);
    }

    if (e1Name == "Torpedo") {

      if (e2Name != "Bullet") {
        auto &ehealth = ecs.getComponent<Health>(e2);
        ehealth.value -= ecs.getComponent<TorpedoLauncher1>(e2).projectileDamage;
      }
      // trigger explosion
      auto &e1pos = ecs.getComponent<Position>(e1);
      explosions.emplace_back(&explosionTexture, e1pos.value, 8, 7);

      explosionSoundPlayer.play();
 
      destroyEntity(ecs, e1);
    }
    else if (e2Name == "Torpedo") {
      if (e1Name != "Bullet") {
        auto &ehealth = ecs.getComponent<Health>(e1);
        ehealth.value -= ecs.getComponent<TorpedoLauncher1>(e1).projectileDamage;
      }
      // trigger explosion
      auto &e2pos = ecs.getComponent<Position>(e2);
      explosions.emplace_back(&explosionTexture, e2pos.value, 8, 7);

      explosionSoundPlayer.play();

      destroyEntity(ecs, e2);
    }
  });


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
      auto &rot = ecs.getComponent<Rotation>(player);
      acc.value.x += std::cos((rot.angle) * (M_PI / 180.f)) * 500.f * dt;
      acc.value.y += std::sin((rot.angle) * (M_PI / 180.f)) * 500.f * dt;

      // std::cout << "Burn velocity: " << vel.value.length() << "\n";

      // approaching 0 velocity
      if ((vel.value.length() < 50.f) && 
          (vel.value.length() > -50.f)) {
        shipControl.burning = false;
        acc.value.x = 0.f;
        acc.value.y = 0.f;
        vel.value.x = 0.f;
        vel.value.y = 0.f;
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
    else if (shipControl.flipping == false) {
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
      else {
        // turn off acceleration if a acceleration key is not pressed
        auto &acc = ecs.getComponent<Acceleration>(player);
        acc.value.x = 0.f;
        acc.value.y = 0.f;
      }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Use the mouse to set rotation and acceleration
    ///////////////////////////////////////////////////////////////////////////////
    if (!shipControl.flipping && !shipControl.burning && 
      sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

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
      startTurn(ecs, shipControl, player, newVector.angle().asDegrees()); 

      // start accelerating, using dt doesnt work very well if there are lots of bullets
      // use the distance from the mouse click to set acceleration, max 10 Gs
      acc.value.x += std::cos((rot.angle) * (M_PI / 180.f)) * (newVector.length() * 2.0f);
      acc.value.y += std::sin((rot.angle) * (M_PI / 180.f)) * (newVector.length() * 2.0f);
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
        // std::cout << "Flipped to target angle: " << shipControl.targetAngle << "\n";
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

    // GUI outer radius is torpedo threat range
    std::array<int, 6> radius{8000, 16000, 45000};

    // Circles
    for (auto r : radius) {
      sf::CircleShape shape(r);
      shape.setPointCount(100);
      shape.setFillColor(sf::Color::Transparent);
      shape.setOutlineThickness(1.2f * zoomFactor);
      shape.setOutlineColor(sf::Color(75, 20, 26));

      // move the origin to the centre of the circle
      shape.setOrigin(sf::Vector2f(r, r));
      shape.setPosition(screenCentre);

      window.draw(shape);
    }

    // draw all the sprites
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

        // not sure if zoomFactor is needed here, seems to work without
        sf::Vector2f cameraOffset = screenCentre - playerpos.value;
        // sf::Vector2f cameraOffset = screenCentre - (playerpos.value / zoomFactor);
        sc.sprite.setPosition(pos.value + cameraOffset);
      }

      window.draw(sc.sprite);
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

    DrawHUD(window, ecs, player, font);
    DrawSidebarText(window, ecs, player, font);
    DrawSidebarText(window, ecs, enemy1, font);
    DrawShipNames(window, ecs, enemy1, font, zoomFactor);
    DrawShipNames(window, ecs, enemy2, font, zoomFactor);
    DrawShipNames(window, ecs, player, font, zoomFactor);
    DrawTorpedoOverlay(window, ecs, font, zoomFactor);
    DrawVectorOverlay(window, ecs, player, font, zoomFactor);
    DrawVectorOverlay(window, ecs, enemy1, font, zoomFactor);
    DrawVectorOverlay(window, ecs, enemy2, font, zoomFactor);

    // display everything
    window.display();
  }
}

  void destroyEntity(Coordinator &ecs, Entity e) {
    ecs.removeComponent<Velocity>(e);
    ecs.removeComponent<Position>(e);
    ecs.removeComponent<Rotation>(e);
    ecs.removeComponent<Collision>(e);
    if (ecs.hasComponent<TorpedoTarget>(e)) {
      ecs.removeComponent<TorpedoTarget>(e);
    }
    ecs.removeComponent<SpriteComponent>(e);
    if (ecs.hasComponent<TimeFired>(e)) {
      ecs.removeComponent<TimeFired>(e);
    }
    ecs.destroyEntity(e);
  }
