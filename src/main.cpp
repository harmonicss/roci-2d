#include "../include/collision.hpp"
#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include "../include/ballistics.hpp"
#include "../include/enemyai.hpp"
#include "../include/torpedoai.hpp"
#include "../include/targeting.hpp"
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
  ATTACK_PDC, 
  DEFENCE_PDC,
};


int main() {

  auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "Rocinante",
                                 sf::Style::Default);
  window.setFramerateLimit(60);

  FlipBurnControl flipControl;

  State state = State::DEFENCE_PDC;

  // - ECS Setup -
  Coordinator ecs;
  ecs.registerComponent<Position>();
  ecs.registerComponent<Velocity>();
  ecs.registerComponent<Acceleration>();
  ecs.registerComponent<Rotation>();
  ecs.registerComponent<SpriteComponent>();
  ecs.registerComponent<Health>();
  ecs.registerComponent<Pdc1>();
  ecs.registerComponent<Pdc2>();
  ecs.registerComponent<TorpedoLauncher1>();
  ecs.registerComponent<TorpedoLauncher2>();
  ecs.registerComponent<Collision>();
  ecs.registerComponent<Target>();

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
  sf::Texture rociTexture, enemyTexture, bulletTexture, torpedoTexture;

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
  
  ecs.addComponent(player, Pdc1{});
  ecs.addComponent(player, Pdc2{});
  ecs.addComponent(player, TorpedoLauncher1{});
  ecs.addComponent(player, TorpedoLauncher2{});
  ecs.addComponent(
      player, Collision{ShapeType::AABB,
                        static_cast<float>(rociTexture.getSize().x) / 2 - 45,
                        static_cast<float>(rociTexture.getSize().y) / 2 - 45, 0.f});

  ///////////////////////////////////////////////////////////////////////////////
  // - Create Enemy Entity -
  ///////////////////////////////////////////////////////////////////////////////
  Entity enemy = ecs.createEntity("Enemy");
  ecs.addComponent(enemy, Position{{20000, 0}});
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
  ecs.addComponent(enemy, Pdc1{});
  ecs.addComponent(enemy, Pdc2{});
  ecs.addComponent(enemy, TorpedoLauncher1{});
  ecs.addComponent(enemy, TorpedoLauncher2{});
  ecs.addComponent(
      enemy, Collision{ShapeType::AABB,
                       static_cast<float>(enemyTexture.getSize().x) / 2 - 45,
                       static_cast<float>(enemyTexture.getSize().y) / 2 - 45, 0.f});

  ///////////////////////////////////////////////////////////////////////////////
  // Create Collision System, with lambda callback
  ///////////////////////////////////////////////////////////////////////////////
  CollisionSystem collisionSystem(ecs, pdcHitSoundPlayer, [&ecs, &pdcHitSoundPlayer](Entity e1, Entity e2) {
    std::string e1Name = ecs.getEntityName(e1);
    std::string e2Name = ecs.getEntityName(e2);

    // bullets cant collide with each other, so only destroy if they hit the player or enemy or torpedo
    if (e1Name == "Bullet" && e2Name == "Bullet") {
      return;
    }

    std::cout << "Collision detected between " << e1 << " and " << e2 << "\n";

    // Damage the health of the entities
    // cant capture player here, I know it is 0.
    if (e1 == 0 || e2 == 0) {
      auto &phealth = ecs.getComponent<Health>(0);
      if (e1Name == "Torpedo" || e2Name == "Torpedo") {
        phealth.value -= ecs.getComponent<TorpedoLauncher1>(0).projectileDamage;
      }
      else { // bullet, can add railgun later
        phealth.value -= ecs.getComponent<Pdc1>(0).projectileDamage;

        // TODO: add torpedo sound
        pdcHitSoundPlayer.play();
      }
    }
 
    // enemy is 1
    if (e1 == 1 || e2 == 1) {
      auto &ehealth = ecs.getComponent<Health>(1);
      if (e1Name == "Torpedo" || e2Name == "Torpedo") {
        ehealth.value -= ecs.getComponent<TorpedoLauncher1>(1).projectileDamage;
      }
      else { // bullet, can add railgun later
        ehealth.value -= ecs.getComponent<Pdc1>(1).projectileDamage;

        // TODO: add torpedo sound
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
      ecs.destroyEntity(e1);
    }

    if (e2 > 1) {
      ecs.removeComponent<Velocity>(e2);
      ecs.removeComponent<Position>(e2);
      ecs.removeComponent<Rotation>(e2);
      ecs.removeComponent<Collision>(e2);
      ecs.removeComponent<Target>(e2);
      ecs.removeComponent<SpriteComponent>(e2);
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
        if (zoomFactor < 5.f) {
          zoomFactor = 5.f;
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
        std::cout << "Starting Flipping!\n";
        flipControl.timeSinceFlipped = tt;
        flipControl.flipping = true;
        auto &rot = ecs.getComponent<Rotation>(player);
        auto &vel = ecs.getComponent<Velocity>(player).value;

        // cannot calculate the angle from a zero vector
        if (vel.length() == 0.f) {
          std::cout << "Flip correctionAngle: 0\n";
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
          std::cout << "Flip correctionAngle: " << correctionAngle.asDegrees() << "\n";
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
          std::cout << "Flip target angle: " << flipControl.targetAngle << "\n";
        }
      }
    }
    else if (flipControl.flipping == false) {
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        // rotate left
        auto &rot = ecs.getComponent<Rotation>(player);
        rot.angle -= (window.getSize().x / 500.f);
        if (rot.angle >= 180.f) {
          rot.angle -= 360.f;
        } else if (rot.angle < -180.f) {
          rot.angle += 360.f;
        }
      }
      else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
        // rotate right
        auto &rot = ecs.getComponent<Rotation>(player);
        rot.angle += (window.getSize().x / 500.f);
        if (rot.angle >= 180.f) {
          rot.angle -= 360.f;
        } else if (rot.angle < -180.f) {
          rot.angle += 360.f;
        }
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
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
    // Animate the flip
    ///////////////////////////////////////////////////////////////////////////////
    if (flipControl.flipping) {
      auto &rot = ecs.getComponent<Rotation>(player);
      float diff = std::abs(flipControl.targetAngle - rot.angle);

      std::cout << "\ntargetAngle:   " << flipControl.targetAngle << "\n";
      std::cout << "Current angle: " << rot.angle << "\n";
      std::cout << "Flipping diff: " << diff << "\n";

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
      std::cout << "New angle:     " << rot.angle << "\n";
      if (rot.angle == flipControl.targetAngle) {
        std::cout << "Flipped to target angle: " << flipControl.targetAngle << "\n";
        flipControl.flipping = false;
        flipControl.targetAngle = 0.f;
        flipControl.burning = true;
      }
    }

    // target all pdcs if attacking enemy
    if (state == State::ATTACK_PDC) {
      // target the player
      pdcTarget.aquireTargets(enemy);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Fire! Attacking PDCs
    ///////////////////////////////////////////////////////////////////////////////
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
      state = State::ATTACK_PDC;
      pdcTarget.pdcAttack(tt);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Fire! Defensive PDCs
    ///////////////////////////////////////////////////////////////////////////////
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
      state = State::DEFENCE_PDC;
      pdcTarget.pdcDefendTorpedo(tt, dt);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Fire! Torpedo 1 & 2
    ///////////////////////////////////////////////////////////////////////////////
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {

      auto &launcher1 = ecs.getComponent<TorpedoLauncher1>(player);
      auto &launcher2 = ecs.getComponent<TorpedoLauncher2>(player);

      if (launcher1.timeSinceFired == 0.f || tt > launcher1.timeSinceFired + launcher1.cooldown && launcher1.rounds) {
        launcher1.timeSinceFired = tt;
        torpedoFactory.fireone<TorpedoLauncher1>(player, enemy);
        // TODO: add torpedo sound
        pdcFireSoundPlayer.play();
        launcher1.rounds--;
      }
      if (launcher2.timeSinceFired == 0.f || tt > launcher2.timeSinceFired + launcher2.cooldown && launcher2.rounds) {
        launcher2.timeSinceFired = tt;
        torpedoFactory.fireone<TorpedoLauncher2>(player, enemy);
        pdcFireSoundPlayer.play();
        launcher2.rounds--;
      }
    }

    // Enemy & Torpedo AIs
    enemyAI.Update(tt, dt);
    torpedoAI.Update(tt, dt);


    // Collision System - check for collisions
    collisionSystem.Update();

    ///////////////////////////////////////////////////////////////////////////////
    // - Render -
    ///////////////////////////////////////////////////////////////////////////////
    window.clear(sf::Color(7, 5, 8));
    u_int16_t screenWidth = window.getSize().x;
    u_int16_t screenHeight = window.getSize().y;
    sf::Vector2f screenCentre = {screenWidth / 2.f, screenHeight / 2.f};

    // GUI
    std::array<int, 5> radius{2000, 3000, 4000, 8000, 16000};

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

    // Use a seperate HUD view
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
