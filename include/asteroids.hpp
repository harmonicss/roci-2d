
#pragma once

#include "ecs.hpp"
#include "components.hpp"
#include "utils.hpp"
#include <SFML/Graphics/Texture.hpp>

class AsteroidFactory {
public:
  AsteroidFactory(Coordinator &ecs) : ecs(ecs) {

    if (!mediumAsteroidTexture.loadFromFile("../assets/textures/asteroid-1.png")) {

      std::cout << "Error loading texture" << std::endl;
      std::exit(-1);
    }

    // seed random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
  }

  virtual ~AsteroidFactory() = default;

  void createAsteroids() {

    for (int a = 0; a < 100; ++a) {
      float size = randFloat(1.0f, 5.0f);
      float posX = randFloat(-1000000.f, 1000000.f);
      float posY = randFloat(-40000.f, -30000.f);  // so it wont be on the player
      float velX = randFloat(-50.f, 50.f);
      float velY = randFloat(-50.f, 50.f);
      float rot  = randFloat(0.f, 360.f);
      float av   = randFloat(0.f, 20.f);

      createAsteroid(mediumAsteroidTexture, "Asteroid", size,
                     {posX, posY},
                     {velX, velY},
                     rot,
                     av, 1000);

    }


    // createAsteroid(mediumAsteroidTexture, "Asteroid", 1.0f, {11100.f, 200.f}, {50.f, 0.f}, 100.1f, 1000);
    // createAsteroid(mediumAsteroidTexture, "Asteroid", 3.0f, {-11300.f, 400.f}, {-30.f, 20.f}, -0.05f, 1000);
    // createAsteroid(mediumAsteroidTexture, "Asteroid", 3.0f, {-11300.f, 400.f}, {-30.f, 20.f}, -0.05f, 1000);
    // createAsteroid(mediumAsteroidTexture, "Asteroid", 3.0f, {-5000.f, 2400.f}, {130.f, -20.f}, 1.00f, 1000);
    // createAsteroid(mediumAsteroidTexture, "Asteroid", 5.0f, {1500.f, -6600.f}, {0.f, -40.f}, 0.2f, 1000);
  }

private:
    Coordinator &ecs;
    sf::Texture mediumAsteroidTexture;

  // Create an asteroid entity
  Entity createAsteroid(sf::Texture &asteroidTexture, const std::string &name, float scale, sf::Vector2f position,
                        sf::Vector2f velocity, float rotation, float angularVelocity, unsigned int health) {
    Entity e = ecs.createEntity(name);
    ecs.addComponent(e, Position{position});
    ecs.addComponent(e, Velocity{velocity});
    ecs.addComponent(e, Rotation{rotation, angularVelocity}); // Initial rotation
    ecs.addComponent(e, Health{health});
    SpriteComponent sc{sf::Sprite(asteroidTexture)};
    sf::Vector2f asteroidOrigin(asteroidTexture.getSize().x / 2.f,
                                asteroidTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(asteroidOrigin);
    sc.sprite.setScale(sf::Vector2f{scale, scale});
    ecs.addComponent(e, sc);

    // scale is to make sure the collision box is slightly smaller than the sprite
    ecs.addComponent(e, Collision{e, ShapeType::AABB,
                                  CollisionType::ASTEROID,
                                  100, // damage
                                  static_cast<float>(asteroidTexture.getSize().x * scale * 0.75f) / 2,
                                  static_cast<float>(asteroidTexture.getSize().y * scale * 0.75f) / 2, 0.f});

    std::cout << "Created asteroid: " << name << " with texture size: "
              << asteroidTexture.getSize().x * scale << "x" << asteroidTexture.getSize().y * scale
              << " at position: " << position.x << ", " << position.y << std::endl;

    return e;
  }
};

