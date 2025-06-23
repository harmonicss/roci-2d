#include "../include/collision.hpp"
#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include "../include/ballistics.hpp"
#include "../include/enemyai.hpp"
#include "../include/torpedoai.hpp"
#include "../include/targeting.hpp"
#include "../include/explosion.hpp"
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
extern void DrawPlayerOverlay(sf::RenderWindow &window, Coordinator &ecs,
                              sf::Font &font, float zoomFactor);


void createPdcs(Coordinator &ecs, Entity e);

// Flip 180 and burn to a stop
struct FlipBurnControl {
  bool flipping = false;
  bool burning = false;
  float timeSinceFlipped = 0.f;
  float cooldown = 0.5f;
  float targetAngle = 0.f;
  enum class RotationDirection { CLOCKWISE, COUNTERCLOCKWISE };
  RotationDirection rotationDir = RotationDirection::CLOCKWISE; 
};

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

  FlipBurnControl flipControl;

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
  ecs.registerComponent<Target>();
  ecs.registerComponent<TimeFired>();
  ecs.registerComponent<PdcMounts>();
  ecs.registerComponent<TorpedoControl>();

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
  sf::Texture rociTexture, enemyTexture, bulletTexture, torpedoTexture, explosionTexture;

  if (!rociTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }
  std::cout << "Roci x " << rociTexture.getSize().x << " y "
            << rociTexture.getSize().y << "\n";

  // roci fighting itself for now
  if (!enemyTexture.loadFromFile("../assets/textures/roci.png")) {
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
  // - Create Player Entity -
  ///////////////////////////////////////////////////////////////////////////////
  Entity player = ecs.createEntity("Rocinante");
  ecs.addComponent(player, Position{{0, 0}});
  ecs.addComponent(player, Velocity{{0.f, 0.f}});
  ecs.addComponent(player, Rotation{0.f});
  ecs.addComponent(player, Health{100});
  ecs.addComponent(player, Acceleration{{0.f, 0.f}});
  {
    SpriteComponent sc{sf::Sprite(rociTexture)};
    sf::Vector2f rociOrigin(rociTexture.getSize().x / 2.f,
                            rociTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(rociOrigin);
    ecs.addComponent(player, SpriteComponent{sc});
  }
 
  ecs.addComponent(player, TorpedoLauncher1{});
  ecs.addComponent(player, TorpedoLauncher2{});
  ecs.addComponent(
      player, Collision{player, ShapeType::AABB,
                        static_cast<float>(rociTexture.getSize().x) / 2 - 45,
                        static_cast<float>(rociTexture.getSize().y) / 2 - 45, 0.f});

  ///////////////////////////////////////////////////////////////////////////////
  // - Create Enemy Entity - be careful with the order of this, as I assume
  // enemy = 1 sometimes.
  ///////////////////////////////////////////////////////////////////////////////
  Entity enemy = ecs.createEntity("Enemy");
  ecs.addComponent(enemy, Position{{0, -180000.f}});
  ecs.addComponent(enemy, Velocity{{0.f, 0.f}});
  ecs.addComponent(enemy, Rotation{90.f});
  ecs.addComponent(enemy, Health{100});
  ecs.addComponent(enemy, Acceleration{{0.f, 0.f}});
  {
    SpriteComponent sc{sf::Sprite(enemyTexture)};
    sf::Vector2f enemyOrigin(enemyTexture.getSize().x / 2.f,
                             enemyTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(enemyOrigin);
    ecs.addComponent(enemy, sc);
  }
  ecs.addComponent(enemy, TorpedoLauncher1{});
  ecs.addComponent(enemy, TorpedoLauncher2{});
  ecs.addComponent(
      enemy, Collision{enemy,ShapeType::AABB,
                       static_cast<float>(enemyTexture.getSize().x) / 2 - 45,
                       static_cast<float>(enemyTexture.getSize().y) / 2 - 45, 0.f});

  ///////////////////////////////////////////////////////////////////////////////
  // create pdc mounts
  ///////////////////////////////////////////////////////////////////////////////
  createPdcs(ecs, player);
  createPdcs(ecs, enemy);

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
    // cant capture player here, I know it is 0.
    if (e1 == 0 || e2 == 0) {
      auto &phealth = ecs.getComponent<Health>(0);
      if (e1Name == "Torpedo" || e2Name == "Torpedo") {
        phealth.value -= ecs.getComponent<TorpedoLauncher1>(0).projectileDamage;

        // trigger explosion
        auto &e1pos = ecs.getComponent<Position>(e1);
        explosions.emplace_back(&explosionTexture, e1pos.value, 8, 7);

        explosionSoundPlayer.play();
      }
      else { // bullet, can add railgun later
        // just grab any pdc for now
        auto &mounts = ecs.getComponent<PdcMounts>(0);
        phealth.value -= ecs.getComponent<Pdc>(mounts.pdcEntities[0]).projectileDamage;

        pdcHitSoundPlayer.play();
      }
    }
 
    // enemy is 1
    if (e1 == 1 || e2 == 1) {
      auto &ehealth = ecs.getComponent<Health>(1);
      if (e1Name == "Torpedo" || e2Name == "Torpedo") {
        ehealth.value -= ecs.getComponent<TorpedoLauncher1>(1).projectileDamage;

        // trigger explosion
        auto &e1pos = ecs.getComponent<Position>(e1);
        explosions.emplace_back(&explosionTexture, e1pos.value, 8, 7);

        explosionSoundPlayer.play();
      }
      else { // bullet, can add railgun later
        // just grab any pdc for now
        auto &mounts = ecs.getComponent<PdcMounts>(1);
        ehealth.value -= ecs.getComponent<Pdc>(mounts.pdcEntities[0]).projectileDamage;

        pdcHitSoundPlayer.play();
      }
    }


    if (e1 > 1) {
      ecs.removeComponent<Velocity>(e1);
      ecs.removeComponent<Position>(e1);
      ecs.removeComponent<Rotation>(e1);
      ecs.removeComponent<Collision>(e1);
      ecs.removeComponent<Target>(e1);
      ecs.removeComponent<SpriteComponent>(e1);
      if (e1Name == "Bullet") 
        ecs.removeComponent<TimeFired>(e1);
      ecs.destroyEntity(e1);
    }

    if (e2 > 1) {
      ecs.removeComponent<Velocity>(e2);
      ecs.removeComponent<Position>(e2);
      ecs.removeComponent<Rotation>(e2);
      ecs.removeComponent<Collision>(e2);
      ecs.removeComponent<Target>(e2);
      ecs.removeComponent<SpriteComponent>(e2);
      if (e2Name == "Bullet") 
        ecs.removeComponent<TimeFired>(e2);
      ecs.destroyEntity(e2);
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
  EnemyAI enemyAI(ecs, enemy, bulletFactory, torpedoFactory, pdcFireSoundPlayer);
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
    if (flipControl.burning == true) {
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
        flipControl.burning = false;
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
      if (tt > flipControl.timeSinceFlipped + flipControl.cooldown) {
        // std::cout << "Starting Flipping!\n";
        flipControl.timeSinceFlipped = tt;
        flipControl.flipping = true;
        auto &rot = ecs.getComponent<Rotation>(player);
        auto &vel = ecs.getComponent<Velocity>(player).value;

        // cannot calculate the angle from a zero vector
        if (vel.length() == 0.f) {
          // std::cout << "Flip correctionAngle: 0\n";
          flipControl.targetAngle = rot.angle + 180.f;
          if (flipControl.targetAngle >= 180.f) {
            flipControl.targetAngle -= 360.f;
          } else if (flipControl.targetAngle < -180.f) {
            flipControl.targetAngle += 360.f;
          }
          flipControl.rotationDir = FlipBurnControl::RotationDirection::CLOCKWISE;
        }
        else {
          // get the angle of the velocity vector
          sf::Angle correctionAngle = vel.angle();
          // std::cout << "Flip correctionAngle: " << correctionAngle.asDegrees() << "\n";
          flipControl.targetAngle = correctionAngle.asDegrees() - 180.f;
          float diff = std::abs(flipControl.targetAngle - rot.angle);
          if (diff > 0.f) {
            flipControl.rotationDir = FlipBurnControl::RotationDirection::CLOCKWISE;
          } else {
            flipControl.rotationDir = FlipBurnControl::RotationDirection::COUNTERCLOCKWISE;
          }
          if (flipControl.targetAngle >= 180.f) {
            flipControl.targetAngle -= 360.f;
          } else if (flipControl.targetAngle < -180.f) {
            flipControl.targetAngle += 360.f;
          }
          // std::cout << "Flip target angle: " << flipControl.targetAngle << "\n";
        }
      }
    }
    else if (flipControl.flipping == false) {
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)) {
        // rotate left
        auto &rot = ecs.getComponent<Rotation>(player);
        //rot.angle -= (window.getSize().x / 500.f);
        rot.angle -= 90.f * dt;
        if (rot.angle >= 180.f) {
          rot.angle -= 360.f;
        } else if (rot.angle < -180.f) {
          rot.angle += 360.f;
        }
      }
      else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
        // rotate right
        auto &rot = ecs.getComponent<Rotation>(player);
        // rot.angle += (window.getSize().x / 500.f);
        rot.angle += 90.f * dt;
        if (rot.angle >= 180.f) {
          rot.angle -= 360.f;
        } else if (rot.angle < -180.f) {
          rot.angle += 360.f;
        }
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
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

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

      // turn towards the vector, instant turn for now
      rot.angle = newVector.angle().asDegrees();

      // start accelerating, using dt doesnt work very well if there are lots of bullets
      // use the distance from the mouse click to set acceleration, max 10 Gs
      acc.value.x += std::cos((rot.angle) * (M_PI / 180.f)) * (newVector.length() * 2.0f);
      acc.value.y += std::sin((rot.angle) * (M_PI / 180.f)) * (newVector.length() * 2.0f);
    }
 
    ///////////////////////////////////////////////////////////////////////////////
    // Animate the flip
    ///////////////////////////////////////////////////////////////////////////////
    if (flipControl.flipping) {
      auto &rot = ecs.getComponent<Rotation>(player);
      float diff = std::abs(flipControl.targetAngle - rot.angle);

      // std::cout << "\ntargetAngle:   " << flipControl.targetAngle << "\n";
      // std::cout << "Current angle: " << rot.angle << "\n";
      // std::cout << "Flipping diff: " << diff << "\n";

      if (diff < 20.f) {
        rot.angle = flipControl.targetAngle;
      } 
      else if (flipControl.rotationDir == FlipBurnControl::RotationDirection::CLOCKWISE) {
        rot.angle -= 15.f; //(window.getSize().x / 100.f);
      }
      else {
        rot.angle += 15.f; //(window.getSize().x / 100.f);
      }

      // TODO: wrap with Angle.wrapUnsigned
      if (rot.angle >= 180.f) {
        rot.angle -= 360.f;
      } else if (rot.angle < -180.f) {
        rot.angle += 360.f;
      }
      // std::cout << "New angle:     " << rot.angle << "\n";
      if (rot.angle == flipControl.targetAngle) {
        // std::cout << "Flipped to target angle: " << flipControl.targetAngle << "\n";
        flipControl.flipping = false;
        flipControl.targetAngle = 0.f;
        flipControl.burning = true;
      }
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
      pdcTarget.pdcAttack(enemy, tt);
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
        torpedoFactory.fireone<TorpedoLauncher1>(player, enemy);
        // TODO: add torpedo sound
        pdcFireSoundPlayer.play();
        launcher1.rounds--;
      }
      if ((launcher2.timeSinceFired == 0.f || tt > launcher2.timeSinceFired + launcher2.cooldown) && launcher2.rounds) {
        launcher2.timeSinceFired = tt;
        torpedoFactory.fireone<TorpedoLauncher2>(player, enemy);
        pdcFireSoundPlayer.play();
        launcher2.rounds--;
      }
    }

    // Enemy & Torpedo AIs
    enemyAI.Update(tt, dt);
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
    std::array<int, 6> radius{2000, 3000, 4000, 8000, 16000, 45000};

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
    DrawSidebarText(window, ecs, enemy, font);
    DrawShipNames(window, ecs, enemy, font, zoomFactor);
    DrawShipNames(window, ecs, player, font, zoomFactor);
    DrawTorpedoOverlay(window, ecs, font, zoomFactor);
    DrawPlayerOverlay(window, ecs, font, zoomFactor);

    // display everything
    window.display();
  }
}

void createPdcs(Coordinator &ecs, Entity e) {

  const int PDC_ROUNDS = 600;

  std::vector<Entity> pdcEntities;
  Entity pdc1 = ecs.createEntity("PDC1");
  Entity pdc2 = ecs.createEntity("PDC2");
  Entity pdc3 = ecs.createEntity("PDC3"); 
  Entity pdc4 = ecs.createEntity("PDC4");
  Entity pdc5 = ecs.createEntity("PDC5"); // centre
  Entity pdc6 = ecs.createEntity("PDC6"); // centre
 
  // Fore Left
  ecs.addComponent(pdc1, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = -45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -170.f,
    .maxFiringAngle = 10.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 210.f,       // top left
    .positiony = -130.f,       // top left
  });
  pdcEntities.push_back(pdc1);

  // Fore Right
  ecs.addComponent(pdc2, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -10.f,
    .maxFiringAngle = 170.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 210.f,       // top right
    .positiony = 130.f,       // top right
  });
  pdcEntities.push_back(pdc2);

  // Mid Left
  ecs.addComponent(pdc3, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = -45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -170.f,
    .maxFiringAngle = -10.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 40.f,       // middle left
    .positiony = -180.f,
  });
  pdcEntities.push_back(pdc3);

  // Mid Right
  ecs.addComponent(pdc4, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = 10.f,
    .maxFiringAngle = 170.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 40.f,       // middle right
    .positiony = 180.f,
  });
  pdcEntities.push_back(pdc4);

  // Aft 1 360 in centre
  ecs.addComponent(pdc5, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +0.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -180.f,
    .maxFiringAngle = 180.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = -160.f,       // centre
    .positiony = 0.f,
  });
  pdcEntities.push_back(pdc5);

  // Aft 2 360 in centre
  ecs.addComponent(pdc6, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +0.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -180.f,
    .maxFiringAngle = 180.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = -168.f,       // centre, move slightly back to see the vector
    .positiony = 0.f,
  });

  pdcEntities.push_back(pdc6);
  ecs.addComponent(e, PdcMounts{pdcEntities});
}
