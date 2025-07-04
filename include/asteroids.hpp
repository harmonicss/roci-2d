
#pragma once

#include "ecs.hpp"
#include "components.hpp"
#include "utils.hpp"
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>

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

  void createInitialAsteroids() {

    for (int a = 0; a < 100; ++a) {
      float size = randFloat(1.0f, 5.0f);
      float posX = randFloat(-200000.f, 200000.f);
      float posY = randFloat(-20000.f, -40000.f);  // so it wont be on the player
      float velX = randFloat(-250.f, 250.f);
      float velY = randFloat(-250.f, 250.f);
      float rot  = randFloat(-180.f, 180.f);
      float av   = randFloat(-20.f, 20.f);

      createAsteroid(mediumAsteroidTexture, "Asteroid", size,
                     {posX, posY},
                     {velX, velY},
                     rot,
                     av, 2000);
    }
  }

  void createDebrisAsteroids(sf::Vector2f position) {

    for (int a = 0; a < randInt(1,3); ++a) {
      float size = randFloat(0.25f, 0.8f);
      auto newpos = position + sf::Vector2f{randFloat(-1000.f, 1000.f), randFloat(-1000.f, 1000.f)};

      float velY = randFloat(-5000.f, 5000.f);
      float velX = randFloat(-5000.f, 5000.f);
      float rot  = randFloat(-180.f, 180.f);
      float av   = randFloat(-140.f, 140.f);

      createAsteroid(mediumAsteroidTexture, "Asteroid", size,
                     newpos,
                     {velX, velY},
                     rot,
                     av, 2000);
    }
  }


private:
    Coordinator &ecs;
    sf::Texture mediumAsteroidTexture;

  // Create an asteroid entity
  Entity createAsteroid(sf::Texture &asteroidTexture, const std::string &name, float scale, sf::Vector2f position,
                        sf::Vector2f velocity, float rotation, float angularVelocity, int32_t health) {
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
              << " with velocity: " << velocity.x << ", " << velocity.y
              << " with rotation: " << rotation
              << " at position: " << position.x << ", " << position.y << std::endl;

    return e;
  }
};

