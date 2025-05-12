#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <iostream>
#include <sys/types.h>

int main() {

  auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "Rocinante",
                                 sf::Style::Default);
  window.setFramerateLimit(60);

  // - ECS Setup -
  Coordinator ecs;
  ecs.registerComponent<Position>();
  ecs.registerComponent<Velocity>();
  ecs.registerComponent<Acceleration>();
  ecs.registerComponent<Rotation>();
  ecs.registerComponent<SpriteComponent>();
  ecs.registerComponent<Weapon>();

  // - Load Textures -
  sf::Texture rociTexture, enemyTexture, bulletTexture;

  if (!rociTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  // roci fighting itself for now
  if (!enemyTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }
 
  if (!bulletTexture.loadFromFile("../assets/textures/pdc-bullet.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }
 
  // - Create Player Entity -
  Entity player = ecs.createEntity();
  ecs.addComponent(player, Position{{960, 540}});
  ecs.addComponent(player, Velocity{{0.f, 0.f}});
  ecs.addComponent(player, Rotation{0.f});
  ecs.addComponent(player, Acceleration{{0.f, 0.f}});
  {
    SpriteComponent sc { sf::Sprite(rociTexture) };
    sf::Vector2f rociOrigin(rociTexture.getSize().x / 2.f,
                            rociTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(rociOrigin);
    ecs.addComponent(player, SpriteComponent{sc});
  }
  ecs.addComponent(player, Weapon{});

  // - Create Enemy Entity -
  Entity enemy = ecs.createEntity();
  ecs.addComponent(enemy, Position{{300, -10000}});
  ecs.addComponent(enemy, Velocity{{1.f, 0.f}});
  ecs.addComponent(enemy, Rotation{130.f});
  ecs.addComponent(enemy, Acceleration{{1.f, 0.f}});
  {
    SpriteComponent sc { sf::Sprite(enemyTexture) };
    sf::Vector2f enemyOrigin(enemyTexture.getSize().x / 2.f,
                             enemyTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(enemyOrigin);
    ecs.addComponent(enemy, sc);
  }
  ecs.addComponent(enemy, Weapon{});

  sf::FloatRect viewRect({0.f, 0.f}, {1920.f, 1080.f});
  sf::View view(viewRect);
  window.setView(view);

  float zoomFactor = 1.f;

  sf::Clock clock;
  while (window.isOpen()) {
    float dt = clock.restart().asSeconds();

    // - Events -
    while (const std::optional event = window.pollEvent()) {

      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
      else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {

        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
          window.close();
        }
      }
      else if (event->is<sf::Event::MouseWheelScrolled>()) {
        auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>();

        std::cout << "Scroll delta: " << scroll->delta << std::endl;

        if (scroll->delta < 0) {
          zoomFactor *= 1.1f;
        } else {
          zoomFactor /= 1.1f;
        }

        view.setSize({1920 * zoomFactor, 1080 * zoomFactor});
        window.setView(view);
      }
    }

    // - Physics: A->V->P -
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

    // dont use events for the keyboard, check if currently pressed.
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
      auto &rot = ecs.getComponent<Rotation>(player);
      rot.angle -= (window.getSize().x / 1000.f);
      if (rot.angle >= 360.f) {
        rot.angle -= 360.f;
      } else if (rot.angle < 0.f) {
        rot.angle += 360.f;
      }
      std::cout << "rotationAngle: " << rot.angle << std::endl;
    } 
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
      auto &rot = ecs.getComponent<Rotation>(player);
      rot.angle += (window.getSize().x / 1000.f);
      if (rot.angle >= 360.f) {
        rot.angle -= 360.f;
      } else if (rot.angle < 0.f) {
        rot.angle += 360.f;
      }
      std::cout << "rotationAngle: " << rot.angle << std::endl;
    } 
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
      auto &acc = ecs.getComponent<Acceleration>(player);
      auto &rot = ecs.getComponent<Rotation>(player);
      // subtract 90 degs from the angle to get the correct direction
      acc.value.x += std::cos((rot.angle - 90.f) * (M_PI / 180.f)) * 500.f * dt;
      acc.value.y += std::sin((rot.angle - 90.f) * (M_PI / 180.f)) * 500.f * dt;
      std::cout << "Acceleration: " << acc.value.x << "," << acc.value.y
                << std::endl;
    }
    // remove this as there is no deceleration, only flip and burn
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
      auto &acc = ecs.getComponent<Acceleration>(player);
      auto &rot = ecs.getComponent<Rotation>(player);
      // subtract 90 degs from the angle to get the correct direction
      acc.value.x -= std::cos((rot.angle - 90.f) * (M_PI / 180.f)) * 500.f * dt;
      acc.value.y -= std::sin((rot.angle - 90.f) * (M_PI / 180.f)) * 500.f * dt;
      std::cout << "Acceleration: " << acc.value.x << "," << acc.value.y
                << std::endl;
    }
    // Fire!
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
      // get the vel of the player
      auto pvel = ecs.getComponent<Velocity>(player);
      auto ppos = ecs.getComponent<Position>(player);
      auto prot = ecs.getComponent<Rotation>(player);

      // try to get pdc 1 out at an angle
      float dx = std::cos((prot.angle - 90.f - 45.f) * (M_PI / 180.f));
      float dy = std::sin((prot.angle - 90.f - 45.f) * (M_PI / 180.f));

      // create a pdc bullet entity
      Entity bullet = ecs.createEntity();
      ecs.addComponent(bullet, Velocity{{pvel.value.x + (dx * 100000.f * dt), pvel.value.y + (dy * 100000.f * dt)}});
      ecs.addComponent(bullet, Position{{ppos.value.x + (dx * 400.f), ppos.value.y + (dy * 400.f)}});
      ecs.addComponent(bullet, Rotation{prot.angle - 45.f});

      SpriteComponent sc{sf::Sprite(bulletTexture)};
      sf::Vector2f bulletOrigin(bulletTexture.getSize().x / 2.f,
                                bulletTexture.getSize().y / 2.f);
      sc.sprite.setOrigin(bulletOrigin);
      ecs.addComponent(bullet, sc);

      auto bvel = ecs.getComponent<Velocity>(bullet);
      auto bpos = ecs.getComponent<Position>(bullet);
      auto brot = ecs.getComponent<Rotation>(bullet);

      std::cout << "dx " << dx << ", dy " << dy << "\n";
      std::cout << "Bullet Velocity: " << bvel.value.x << "," << bvel.value.y << "\n";
      std::cout << "Bullet Rotation: " << brot.angle << "," << brot.angle << "\n";
      std::cout << "Bullet Position: " << bpos.value.x << "," << bpos.value.y << "\n";
    }
    // print for now
    else {
      auto &pacc = ecs.getComponent<Acceleration>(player);
      auto &pvel = ecs.getComponent<Velocity>(player);
      auto &ppos = ecs.getComponent<Position>(player);
      // std::cout << "Player Acceleration: " << pacc.value.x << "," << pacc.value.y << "\n";
      // std::cout << "Player Velocity: " << pvel.value.x << "," << pvel.value.y << "\n";
      // std::cout << "Player Position: " << ppos.value.x << "," << ppos.value.y << "\n";

      auto &eacc = ecs.getComponent<Acceleration>(enemy);
      auto &evel = ecs.getComponent<Velocity>(enemy);
      auto &epos = ecs.getComponent<Position>(enemy);
      // std::cout << "Enemy Acceleration: " << eacc.value.x << "," << eacc.value.y << "\n";
      // std::cout << "Enemy Velocity: " << evel.value.x << "," << evel.value.y << "\n";
      // std::cout << "Enemy Position: " << epos.value.x << "," << epos.value.y << "\n";

      // turn off the acceleration when not pressing W or S
      pacc.value.x = 0;
      pacc.value.y = 0;
    }

    // - Render -
    window.clear(sf::Color(56, 58, 94));
    u_int16_t screenWidth = window.getSize().x;
    u_int16_t screenHeight = window.getSize().y;
    sf::Vector2f screenCentre = {screenWidth / 2.f, screenHeight / 2.f};

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
        sf::Vector2f camerOffset = screenCentre - playerpos.value;
        sc.sprite.setPosition(pos.value + camerOffset);
      }

      window.draw(sc.sprite);
    }
    window.display();
  }
}
