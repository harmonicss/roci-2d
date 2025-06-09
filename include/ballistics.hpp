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
  void fireone(Entity firedby) {

    Entity bullet = ecs.createEntity("Bullet");
 
    auto &pdc = ecs.getComponent<Weapon>(firedby);
    auto pvel = ecs.getComponent<Velocity>(firedby);
    auto ppos = ecs.getComponent<Position>(firedby);
    auto prot = ecs.getComponent<Rotation>(firedby);

    // fire pdc out at an angle, convert to radians
    // float dx = std::cos((prot.angle + pdc.firingAngle) * (M_PI / 180.f));
    // float dy = std::sin((prot.angle + pdc.firingAngle) * (M_PI / 180.f));

    // the firing angle is an abosolute angle, so we need to use it directly
    float dx = std::cos((pdc.firingAngle) * (M_PI / 180.f));
    float dy = std::sin((pdc.firingAngle) * (M_PI / 180.f));

    ecs.addComponent(
        bullet, Velocity{{pvel.value.x + (dx * pdc.projectileSpeed),
                          pvel.value.y + (dy * pdc.projectileSpeed)}});
    ecs.addComponent(
        bullet, Position{{ppos.value.x + (dx * launch_distance),
                          ppos.value.y + (dy * launch_distance)}});
    ecs.addComponent(bullet, Rotation{pdc.firingAngle});
    ecs.addComponent(bullet, Collision{ShapeType::AABB, 0.25f, 0.25f, 0.f});

    SpriteComponent sc{sf::Sprite(texture)};
    sf::Vector2f bulletOrigin(texture.getSize().x / 2.f,
                              texture.getSize().y / 2.f);
    sc.sprite.setOrigin(bulletOrigin);
    ecs.addComponent(bullet, sc);
  }
};

class TorpedoFactory : public BallisticsFactory {
public:
  TorpedoFactory(Coordinator &ecs, sf::Texture &texture) : BallisticsFactory(ecs, texture) {
    std::cout << "TorpedoFactory created" << std::endl;
  }
  ~TorpedoFactory() override = default;

  template<typename Weapon>
  void fireone(Entity firedby, Entity target) {
    Entity torpedo = ecs.createEntity("Torpedo");
 
    auto &launcher = ecs.getComponent<Weapon>(firedby);
    auto svel = ecs.getComponent<Velocity>(firedby); // s for source
    auto spos = ecs.getComponent<Position>(firedby);
    auto srot = ecs.getComponent<Rotation>(firedby);

    ecs.addComponent(torpedo, Target{target});

    // fire launcher out at an angle, convert to radians
    // add an offset to fire on the left or right of the ship
    float dx = std::cos((srot.angle + launcher.firingAngle) * (M_PI / 180.f));
    float dy = std::sin((srot.angle + launcher.firingAngle) * (M_PI / 180.f));

    // get a perpendicular vector to the ship. This is to create seperatation between the launchers. 
    // points to the ship's right relative to the firing direction
    float perp_dx = -dy;
    float perp_dy = dx;

    // adding a fudge factor to launch further away from the ship
    float wx = spos.value.x + dx * (launch_distance + 150.f) + perp_dx * launcher.firingOffset;
    float wy = spos.value.y + dy * (launch_distance + 150.f) + perp_dy * launcher.firingOffset;

    // the velocity of the torpedo is the velocity of the launcher plus the projectile speed
    ecs.addComponent(
        torpedo, Velocity{{svel.value.x + (dx * launcher.projectileSpeed),
                           svel.value.y + (dy * launcher.projectileSpeed)}});

    // want launcher1 to be seperated from launcher2
    ecs.addComponent(torpedo, Position{{wx, wy}}); 

    ecs.addComponent(torpedo, Rotation{srot.angle + launcher.firingAngle});

    // accelerate the torpedo out
    ecs.addComponent(torpedo, Acceleration{{(dx * launcher.projectileAccel),
                                            (dy * launcher.projectileAccel)}});

    // TODO: collision size is a guess atm
    ecs.addComponent(torpedo, Collision{ShapeType::AABB, 80.0f, 30.f, 0.f});

    SpriteComponent sc{sf::Sprite(texture)};
    sf::Vector2f torpedoOrigin(texture.getSize().x / 2.f,
                              texture.getSize().y / 2.f);
    sc.sprite.setOrigin(torpedoOrigin);
    ecs.addComponent(torpedo, sc);
  }
};

