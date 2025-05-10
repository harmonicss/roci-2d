#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <cmath>
#include "../include/ecs.hpp"
#include "../include/components.hpp"

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
  sf::Texture rociTexture, enemyTexture;

  if (!rociTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  // roci fighting itself for now
  if (!enemyTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  // - Create Player Entity -
  Entity player = ecs.createEntity();
  ecs.addComponent(player, Position{{960, 540}});
  ecs.addComponent(player, Velocity {{0.f, 0.f}});
  ecs.addComponent(player, Rotation{0.f});
  ecs.addComponent(player, Acceleration {{0.f, 0.f}});
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
  ecs.addComponent(enemy, Velocity {{1.f, 0.f}});
  ecs.addComponent(enemy, Rotation{130.f});
  ecs.addComponent(enemy, Acceleration {{1.f, 0.f}});
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
      std::cout << "rotationAngle: " << rot.angle << std::endl;
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
      auto &rot = ecs.getComponent<Rotation>(player);
      rot.angle += (window.getSize().x / 1000.f);
      std::cout << "rotationAngle: " << rot.angle << std::endl;
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
      auto &acc = ecs.getComponent<Acceleration>(player);
      auto &rot = ecs.getComponent<Rotation>(player);
      // subtract 90 degs from the angle to get the correct direction
      acc.value.x += std::cos((rot.angle - 90.f) * (M_PI / 180.f)) * 100.f * dt;
      acc.value.y += std::sin((rot.angle - 90.f) * (M_PI / 180.f)) * 100.f * dt;
      std::cout << "Acceleration: " << acc.value.x << "," << acc.value.y << std::endl;
    }
    // remove this as there  is no deceleration, only flip and burn
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
      auto &acc = ecs.getComponent<Acceleration>(player);
      auto &rot = ecs.getComponent<Rotation>(player);
      // subtract 90 degs from the angle to get the correct direction
      acc.value.x -= std::cos((rot.angle - 90.f) * (M_PI / 180.f)) * 100.f * dt;
      acc.value.y -= std::sin((rot.angle - 90.f) * (M_PI / 180.f)) * 100.f * dt;
      std::cout << "Acceleration: " << acc.value.x << "," << acc.value.y << std::endl;
    }

    // - Render -
    window.clear(sf::Color(56,58,94));
    for (auto e : ecs.view<Position, Rotation, SpriteComponent>()) {
      auto &pos = ecs.getComponent<Position>(e);
      auto &rot = ecs.getComponent<Rotation>(e);
      auto &sc = ecs.getComponent<SpriteComponent>(e);

      sf::Angle angle = sf::degrees(rot.angle);
      sc.sprite.setRotation(angle);
      sc.sprite.setPosition(pos.value);
      window.draw(sc.sprite);
    }
    window.display();

  }
}
