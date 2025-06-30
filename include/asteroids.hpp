
#pragma once

#include "../include/ecs.hpp"
#include "../include/components.hpp"
#include <SFML/Graphics/Texture.hpp>

class AsteroidFactory {
public:
  AsteroidFactory(Coordinator &ecs) : ecs(ecs) {

    if (!mediumAsteroidTexture.loadFromFile("../assets/textures/asteroid-1.png")) {

      std::cout << "Error loading texture" << std::endl;
      std::exit(-1);
    }
  }

virtual ~AsteroidFactory() = default;


  void createAsteroids() {
    createAsteroid(mediumAsteroidTexture, "Asteroid", 1.0f, {11100.f, 200.f}, {50.f, 0.f}, 100.1f, 1000);
    createAsteroid(mediumAsteroidTexture, "Asteroid", 3.0f, {-11300.f, 400.f}, {-30.f, 20.f}, -0.05f, 1000);
    createAsteroid(mediumAsteroidTexture, "Asteroid", 5.0f, {1500.f, -6600.f}, {0.f, -40.f}, 0.2f, 1000);
  }

private:
    Coordinator &ecs;
    sf::Texture mediumAsteroidTexture;

  // Create an asteroid entity
  Entity createAsteroid(sf::Texture &asteroidTexture, const std::string &name, float scale, sf::Vector2f position,
                        sf::Vector2f velocity, float angularVelocity, unsigned int health) {
    Entity e = ecs.createEntity(name);
    ecs.addComponent(e, Position{position});
    ecs.addComponent(e, Velocity{velocity});
    ecs.addComponent(e, Rotation{0.f, angularVelocity}); // Initial rotation
    ecs.addComponent(e, Health{health});
    SpriteComponent sc{sf::Sprite(asteroidTexture)};
    sf::Vector2f asteroidOrigin(asteroidTexture.getSize().x / 2.f,
                                asteroidTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(asteroidOrigin);
    sc.sprite.setScale(sf::Vector2f{scale, scale});
    ecs.addComponent(e, sc);
    ecs.addComponent(e, Collision{e, ShapeType::AABB,
                                  CollisionType::ASTEROID,
                                  100, // damage
                                  static_cast<float>(asteroidTexture.getSize().x * scale) / 2,
                                  static_cast<float>(asteroidTexture.getSize().y * scale) / 2, 0.f});

    std::cout << "Created asteroid: " << name << " with texture size: "
              << asteroidTexture.getSize().x * scale << "x" << asteroidTexture.getSize().y * scale
              << " at position: " << position.x << ", " << position.y << std::endl;

    return e;
  }
};

