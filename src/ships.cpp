#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include "../include/targeting.hpp"

void createMcrnPdcs6(Coordinator &ecs, Entity e);
void createBelterPdcs1(Coordinator &ecs, Entity e);

class ShipFactory {
public:
  ShipFactory(Coordinator &ecs, sf::Texture &shipTexture, sf::Texture &driveTexture) :
    ecs(ecs),
    shipTexture(shipTexture),
    driveTexture(driveTexture) {
    // std::cout << "ShipFactory created" << std::endl;
  }

  virtual ~ShipFactory() = default;

protected:
  Coordinator &ecs;
  sf::Texture &shipTexture;
  sf::Texture &driveTexture;
};

// shold I make this a singleton?
class PlayerShipFactory : public ShipFactory {
public:
  PlayerShipFactory(Coordinator &ecs, sf::Texture &shipTexture, sf::Texture &driveTexture) :
    ShipFactory(ecs, shipTexture, driveTexture) {}
 
  ~PlayerShipFactory() override = default;

  Entity createPlayerShip(const std::string &name) {
    Entity e = ecs.createEntity(name);
    ecs.addComponent(e, Position{{0,0}});
    ecs.addComponent(e, Velocity{{0.f, 0.f}});
    ecs.addComponent(e, Rotation{0.f});
    ecs.addComponent(e, Health{100});
    ecs.addComponent(e, Acceleration{{0.f, 0.f}});
 
    SpriteComponent sc{sf::Sprite(shipTexture)};
    sf::Vector2f shipOrigin(shipTexture.getSize().x / 2.f,
                            shipTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(shipOrigin);
    ecs.addComponent(e, sc);

    ecs.addComponent(e, Collision{e, ShapeType::AABB,
                                       static_cast<float>(shipTexture.getSize().x) / 2 - 45,
                                       static_cast<float>(shipTexture.getSize().y) / 2 - 45, 0.f});

    ecs.addComponent(e, TorpedoLauncher1{});
    ecs.addComponent(e, TorpedoLauncher2{});

    // as this is a friendly ship, add the component to identify it as a friendly
    ecs.addComponent(e, FriendlyShipTarget{e});
    ecs.addComponent(e, ShipControl{});

    createMcrnPdcs6(ecs, e);
    createPlayerDrivePlume(e);

    return e;
  }



private:

  // always has to be called after the player has been created
  void createPlayerDrivePlume(Entity player) {
    SpriteComponent dc{sf::Sprite(driveTexture)};
    sf::Vector2f driveOrigin(driveTexture.getSize().x - 40.f, // the is whitespace at the front of the png
                             driveTexture.getSize().y / 2.f);
    dc.sprite.setOrigin(driveOrigin);
    dc.sprite.setScale(sf::Vector2f{0.f, 0.f});

    // add the sprite to the player entity
    ecs.addComponent(player, DrivePlume{dc.sprite, sf::Vector2f{-500.f, 0.f}});
  };
};

class BelterFrigateShipFactory : public ShipFactory {

public:
  BelterFrigateShipFactory(Coordinator &ecs, sf::Texture &shipTexture, sf::Texture &driveTexture) : ShipFactory(ecs, shipTexture, driveTexture) {}
 
  ~BelterFrigateShipFactory() override = default;

  Entity createBelterFrigateShip(const std::string &name, const sf::Vector2f position,
                                 const sf::Vector2f velocity, const float rotation, const uint32_t health) {

    Entity e = ecs.createEntity(name);
    ecs.addComponent(e, Position{position});
    ecs.addComponent(e, Velocity{velocity});
    ecs.addComponent(e, Rotation{rotation});
    ecs.addComponent(e, Health{health});
    ecs.addComponent(e, Acceleration{{0.f, 0.f}});
 
    SpriteComponent sc{sf::Sprite(shipTexture)};
    sf::Vector2f shipOrigin(shipTexture.getSize().x / 2.f,
                            shipTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(shipOrigin);
    ecs.addComponent(e, sc);

    ecs.addComponent(e, Collision{e, ShapeType::AABB,
                                     static_cast<float>(shipTexture.getSize().x) / 2 - 60,
                                     static_cast<float>(shipTexture.getSize().y) / 2 - 60, 0.f});

    ecs.addComponent(e, TorpedoLauncher1{.rounds = 4});
    ecs.addComponent(e, TorpedoLauncher2{.rounds = 4});

    // as this is an enemy ship, add the component to identify it as a target
    ecs.addComponent(e, EnemyShipTarget{e});
    ecs.addComponent(e, ShipControl{});

    createBelterPdcs1(ecs, e);
    createBelterDrivePlume(e);

    return e;
  }

private:
  void createBelterDrivePlume(Entity belter) {
    SpriteComponent dc{sf::Sprite(driveTexture)};
    sf::Vector2f driveOrigin(driveTexture.getSize().x - 40.f, // the is whitespace at the front of the png
                             driveTexture.getSize().y / 2.f);
    dc.sprite.setOrigin(driveOrigin);
    dc.sprite.setScale(sf::Vector2f{0.f, 0.f});
    ecs.addComponent(belter, DrivePlume{dc.sprite, sf::Vector2f{-800.f, 0.f}});
  }
};

void createMcrnPdcs6(Coordinator &ecs, Entity e) {

  const int PDC_ROUNDS = 600;

  std::vector<Entity> pdcEntities;
  Entity pdc1 = ecs.createEntity("PDC1");
  Entity pdc2 = ecs.createEntity("PDC2");
  Entity pdc3 = ecs.createEntity("PDC3"); 
  Entity pdc4 = ecs.createEntity("PDC4");
  Entity pdc5 = ecs.createEntity("PDC5"); // centre
  Entity pdc6 = ecs.createEntity("PDC6"); // centre
 
  // Fore Left
  ecs.addComponent(pdc1, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = -45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -170.f,
    .maxFiringAngle = 10.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 210.f,       // top left
    .positiony = -130.f,       // top left
  });
  pdcEntities.push_back(pdc1);

  // Fore Right
  ecs.addComponent(pdc2, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -10.f,
    .maxFiringAngle = 170.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 210.f,       // top right
    .positiony = 130.f,       // top right
  });
  pdcEntities.push_back(pdc2);

  // Mid Left
  ecs.addComponent(pdc3, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = -45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -170.f,
    .maxFiringAngle = -10.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 40.f,       // middle left
    .positiony = -180.f,
  });
  pdcEntities.push_back(pdc3);

  // Mid Right
  ecs.addComponent(pdc4, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +45.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = 10.f,
    .maxFiringAngle = 170.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = 40.f,       // middle right
    .positiony = 180.f,
  });
  pdcEntities.push_back(pdc4);

  // Aft 1 360 in centre
  ecs.addComponent(pdc5, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +0.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -180.f,
    .maxFiringAngle = 180.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = -160.f,       // centre
    .positiony = 0.f,
  });
  pdcEntities.push_back(pdc5);

  // Aft 2 360 in centre
  ecs.addComponent(pdc6, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +0.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -180.f,
    .maxFiringAngle = 180.f,
    .cooldown = 0.01f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 5000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 30,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 1.f,
    .positionx = -168.f,       // centre, move slightly back to see the vector
    .positiony = 0.f,
  });

  pdcEntities.push_back(pdc6);
  ecs.addComponent(e, PdcMounts{pdcEntities});
}


void createBelterPdcs1(Coordinator &ecs, Entity e) {

  const int PDC_ROUNDS = 300;

  std::vector<Entity> pdcEntities;
  Entity pdc1 = ecs.createEntity("PDC1");
 
  // Aft 1 360 in centre
  ecs.addComponent(pdc1, Pdc{
    .fireMode = PdcFireMode::BURST,
    .firingAngle = +0.f,
    .burstSpreadAngle = 5.f,
    .minFiringAngle = -180.f,
    .maxFiringAngle = 180.f,
    .cooldown = 0.03f,
    .timeSinceFired = 0.f,
    .projectileSpeed = 4000.f,
    .projectileDamage = 2,
    .rounds = PDC_ROUNDS,
    .target = INVALID_TARGET_ID,
    .pdcBurst = 0,
    .maxPdcBurst = 20,
    .timeSinceBurst = 0.f,
    .pdcBurstCooldown = 2.f,
    .positionx = -160.f,       // centre
    .positiony = 0.f,
  });
  pdcEntities.push_back(pdc1);

  ecs.addComponent(e, PdcMounts{pdcEntities});
}
