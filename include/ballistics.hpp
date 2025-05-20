#pragma once
#include "ecs.hpp"
#include "components.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>


///////////////////////////////////////////////////////////////////////////////
// BALLISTICS FACTORY
///////////////////////////////////////////////////////////////////////////////
class BallisticsFactory {
public:
  // Constructor cannot be declared virtual 
  BallisticsFactory(Coordinator &ecs, sf::Texture texture) :
    ecs(ecs),
    texture(texture) {
    std::cout << "BallisticsFactory created" << std::endl;

  }
  virtual ~BallisticsFactory() = default;

protected:  
  Coordinator &ecs;
  sf::Texture texture;

private:

  // pure virtual method
  virtual void fire(Entity createdby) = 0; 
};

// use this as a temporary fix to stop bullets colliding with the ship that
// fired it means the bullets start away from the ship, but live for this
// for now
const uint32_t bullet_launch_distance = 500;

class BulletFactory : public BallisticsFactory {
public:
  BulletFactory(Coordinator &ecs, sf::Texture &texture) : BallisticsFactory(ecs, texture) {
    std::cout << "BulletFactory created" << std::endl;
  }
  ~BulletFactory() override = default;

  void fire(Entity firedby) override {
    std::cout << "Bullet fired by: " << firedby << std::endl;

    auto &pdc1 = ecs.getComponent<Pdc1>(firedby);

    if (pdc1.rounds == 0) {
      return;
    }

    Entity bullet = ecs.createEntity();
 
    auto pvel = ecs.getComponent<Velocity>(firedby);
    auto ppos = ecs.getComponent<Position>(firedby);
    auto prot = ecs.getComponent<Rotation>(firedby);

    // fire pdc 1 out at an angle, convert to radians
    float dx = std::cos((prot.angle + pdc1.firingAngle) * (M_PI / 180.f));
    float dy = std::sin((prot.angle + pdc1.firingAngle) * (M_PI / 180.f));

    ecs.addComponent(
        bullet, Velocity{{pvel.value.x + (dx * pdc1.projectileSpeed),
                          pvel.value.y + (dy * pdc1.projectileSpeed)}});
    ecs.addComponent(
        bullet, Position{{ppos.value.x + (dx * bullet_launch_distance),
                          ppos.value.y + (dy * bullet_launch_distance)}});
    ecs.addComponent(bullet, Rotation{prot.angle + pdc1.firingAngle});
    ecs.addComponent(bullet, Collision{ShapeType::AABB, 0.25f, 0.25f, 0.f});

    SpriteComponent sc{sf::Sprite(texture)};
    sf::Vector2f bulletOrigin(texture.getSize().x / 2.f,
                              texture.getSize().y / 2.f);
    sc.sprite.setOrigin(bulletOrigin);
    ecs.addComponent(bullet, sc);

    pdc1.rounds--;
  }
};

class MissileFactory : public BallisticsFactory {
public:
  MissileFactory(Coordinator &ecs, sf::Texture texture) : BallisticsFactory(ecs, texture) {
    std::cout << "MissileFactory created" << std::endl;
  }
  ~MissileFactory() override = default;

  void fire(Entity firedby) override {
    std::cout << "Missile fired" << std::endl;
  }
};



// remove?
class Ballistic {
public:
  virtual void create() = 0;
  virtual ~Ballistic() = default;
};

class Bullet : public Ballistic {
public:
  void create() {
    std::cout << "Bullet created" << std::endl;
  }
};

class Missile : public Ballistic {
public:
  void create() {
    std::cout << "Missile created" << std::endl;
  }
};

