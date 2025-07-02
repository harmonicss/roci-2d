#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>

struct Explosion {
  sf::Sprite sprite;
  sf::Texture *texture; // Do not own the texture
  sf::Vector2f position;
  int frameWidth, frameHeight;
  int columns, rows;
  int currentFrame = 0;
  float frameTime = 0.04f;
  float elapsedTime = 0.f; // Time since last frame update
  bool finished = false;

  Explosion(sf::Texture *tex, sf::Vector2f position, int columns, int rows)
    : texture(tex), sprite(*tex), position(position), columns(columns), rows(rows) 
  {
    // std::cout << "Explosion created at position: " << position.x << ", " << position.y << std::endl;

    frameWidth = texture->getSize().x / columns;
    frameHeight = texture->getSize().y / rows;

    sprite.setTextureRect(sf::IntRect({0, 0}, {frameWidth, frameHeight}));
    sf::Vector2f origin(frameWidth / 2.f, frameHeight / 2.f);
    sprite.setOrigin(origin);
    // sprite.setPosition(position);
    sprite.setScale(sf::Vector2f{30.f, 30.f}); // Scale the sprite for visibility
 
    updateFrame();
  }

  void Update(float dt) {
    if (finished) return;

    elapsedTime += dt;

    if (elapsedTime >= frameTime) {
      elapsedTime = 0.f;
      ++currentFrame;

      if (currentFrame >= columns * rows) {
        finished = true; // Mark as finished after all frames are shown
        return;
      }

      updateFrame();
    }
  }

  void updateFrame() {
    int row = currentFrame / columns;
    int col = currentFrame % columns;

    sprite.setTextureRect(sf::IntRect({col * frameWidth, row * frameHeight},
                                      {frameWidth, frameHeight}));
  }

  void Draw(sf::RenderWindow &window, Coordinator &ecs) {
    if (!finished) {
      u_int16_t screenWidth = window.getSize().x;
      u_int16_t screenHeight = window.getSize().y;
      sf::Vector2f screenCentre = {screenWidth / 2.f, screenHeight / 2.f};

      auto &ppos = ecs.getComponent<Position>(0);

      // std::cout << "\nExplosion Position: " << position.x << ", " << position.y << std::endl;

      sf::Vector2f cameraOffset = screenCentre - ppos.value;
      // std::cout << "Camera Offset: " << cameraOffset.x << ", " << cameraOffset.y << std::endl;

      sf::Vector2f explosionPosRelative = position; 
      // std::cout << "Drawing explosion at: " << explosionPosRelative.x << ", " << explosionPosRelative.y << std::endl;

      sprite.setPosition(explosionPosRelative + cameraOffset);
      // std::cout << "Explosion Sprite Position: " << sprite.getPosition().x << ", " << sprite.getPosition().y << std::endl;

      window.draw(sprite);
    }
  }
};
