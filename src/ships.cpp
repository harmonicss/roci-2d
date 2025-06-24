#include "../include/components.hpp"
#include "../include/ecs.hpp"

class ShipFactory {
public:
  ShipFactory(Coordinator &ecs, sf::Texture &shipTexture) :
    ecs(ecs), 
    shipTexture(shipTexture) {
    // std::cout << "ShipFactory created" << std::endl;
  }

  virtual ~ShipFactory() = default;

protected:
  Coordinator &ecs;
  sf::Texture &shipTexture;
};

// shold I make this a singleton?
class PlayerShipFactory : public ShipFactory {
public:
  PlayerShipFactory(Coordinator &ecs, sf::Texture &shipTexture) : ShipFactory(ecs, shipTexture) {}
 
  ~PlayerShipFactory() override = default;

  Entity createPlayerShip(const std::string &name) {
    Entity player = ecs.createEntity(name);
    ecs.addComponent(player, Position{{0,0}});
    ecs.addComponent(player, Velocity{{0.f, 0.f}});
    ecs.addComponent(player, Rotation{0.f});
    ecs.addComponent(player, Health{100});
    ecs.addComponent(player, Acceleration{{0.f, 0.f}});
 
    SpriteComponent sc{sf::Sprite(shipTexture)};
    sf::Vector2f shipOrigin(shipTexture.getSize().x / 2.f,
                            shipTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(shipOrigin);
    ecs.addComponent(player, sc);

    ecs.addComponent(player, Collision{player, ShapeType::AABB,
                                       static_cast<float>(shipTexture.getSize().x) / 2 - 45,
                                       static_cast<float>(shipTexture.getSize().y) / 2 - 45, 0.f});

    ecs.addComponent(player, TorpedoLauncher1{});
    ecs.addComponent(player, TorpedoLauncher2{});

    return player;
  }
};

class BelterFrigateShipFactory : public ShipFactory {

public:
  BelterFrigateShipFactory(Coordinator &ecs, sf::Texture &shipTexture) : ShipFactory(ecs, shipTexture) {}
 
  ~BelterFrigateShipFactory() override = default;

  Entity createBelterFrigateShip(const std::string &name, const sf::Vector2f position,
                                 const sf::Vector2f velocity, const float rotation, const uint32_t health) {

    Entity player = ecs.createEntity(name);
    ecs.addComponent(player, Position{position});
    ecs.addComponent(player, Velocity{velocity});
    ecs.addComponent(player, Rotation{rotation});
    ecs.addComponent(player, Health{health});
    ecs.addComponent(player, Acceleration{{0.f, 0.f}});
 
    SpriteComponent sc{sf::Sprite(shipTexture)};
    sf::Vector2f shipOrigin(shipTexture.getSize().x / 2.f,
                            shipTexture.getSize().y / 2.f);
    sc.sprite.setOrigin(shipOrigin);
    ecs.addComponent(player, sc);

    ecs.addComponent(player, Collision{player, ShapeType::AABB,
                                       static_cast<float>(shipTexture.getSize().x) / 2 - 60,
                                       static_cast<float>(shipTexture.getSize().y) / 2 - 60, 0.f});

    ecs.addComponent(player, TorpedoLauncher1{});
    ecs.addComponent(player, TorpedoLauncher2{});

    return player;
  }

};
