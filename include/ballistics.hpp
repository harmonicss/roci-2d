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

  // pure virtual method, removed as not allowed with fire template
  //virtual void fire(Entity firedby) = 0; 

protected:  
  Coordinator &ecs;
  sf::Texture texture;
};

// use this as a temporary fix to stop bullets colliding with the ship that
// fired it means the bullets start away from the ship, but live for this
// for now
const uint32_t launch_distance = 500;

class BulletFactory : public BallisticsFactory {
public:
  BulletFactory(Coordinator &ecs, sf::Texture &texture) : BallisticsFactory(ecs, texture) {
    std::cout << "BulletFactory created" << std::endl;
  }
  ~BulletFactory() override = default;

  template<typename Weapon>
  void fire(Entity firedby) {

    Entity bullet = ecs.createEntity("Bullet");
 
    auto &pdc = ecs.getComponent<Weapon>(firedby);
    auto pvel = ecs.getComponent<Velocity>(firedby);
    auto ppos = ecs.getComponent<Position>(firedby);
    auto prot = ecs.getComponent<Rotation>(firedby);

    // fire pdc out at an angle, convert to radians
    float dx = std::cos((prot.angle + pdc.firingAngle) * (M_PI / 180.f));
    float dy = std::sin((prot.angle + pdc.firingAngle) * (M_PI / 180.f));

    ecs.addComponent(
        bullet, Velocity{{pvel.value.x + (dx * pdc.projectileSpeed),
                          pvel.value.y + (dy * pdc.projectileSpeed)}});
    ecs.addComponent(
        bullet, Position{{ppos.value.x + (dx * launch_distance),
                          ppos.value.y + (dy * launch_distance)}});
    ecs.addComponent(bullet, Rotation{prot.angle + pdc.firingAngle});
    ecs.addComponent(bullet, Collision{ShapeType::AABB, 0.25f, 0.25f, 0.f});

    SpriteComponent sc{sf::Sprite(texture)};
    sf::Vector2f bulletOrigin(texture.getSize().x / 2.f,
                              texture.getSize().y / 2.f);
    sc.sprite.setOrigin(bulletOrigin);
    ecs.addComponent(bullet, sc);
  }
};

class MissileFactory : public BallisticsFactory {
public:
  MissileFactory(Coordinator &ecs, sf::Texture &texture) : BallisticsFactory(ecs, texture) {
    std::cout << "MissileFactory created" << std::endl;
  }
  ~MissileFactory() override = default;

  template<typename Weapon>
  void fire(Entity firedby) {
    std::cout << "Missile fired" << std::endl;
    Entity missile = ecs.createEntity("Missile");
 
    auto &launcher = ecs.getComponent<Weapon>(firedby);
    auto pvel = ecs.getComponent<Velocity>(firedby);
    auto ppos = ecs.getComponent<Position>(firedby);
    auto prot = ecs.getComponent<Rotation>(firedby);

    // fire launcher out at an angle, convert to radians
    float dx = std::cos((prot.angle + launcher.firingAngle) * (M_PI / 180.f));
    float dy = std::sin((prot.angle + launcher.firingAngle) * (M_PI / 180.f));

    ecs.addComponent(
        missile, Velocity{{pvel.value.x + (dx * launcher.projectileSpeed),
                          pvel.value.y + (dy * launcher.projectileSpeed)}});
    ecs.addComponent(
        missile, Position{{ppos.value.x + (dx * launch_distance),
                           ppos.value.y + (dy * launch_distance)}});
    ecs.addComponent(missile, Rotation{prot.angle + launcher.firingAngle});

    // missile has good acceleration, about 200Gs in the Expanse.
    // plus the acceleration of the ship
    ecs.addComponent(missile, Acceleration{{(dx * launcher.projectileAccel),
                                            (dy * launcher.projectileAccel)}});

    ecs.addComponent(missile, Collision{ShapeType::AABB, 1.25f, 1.25f, 0.f});

    SpriteComponent sc{sf::Sprite(texture)};
    sf::Vector2f missileOrigin(texture.getSize().x / 2.f,
                              texture.getSize().y / 2.f);
    sc.sprite.setOrigin(missileOrigin);
    ecs.addComponent(missile, sc);
  }
};

