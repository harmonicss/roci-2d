#pragma once
#include "components.hpp"
#include "ecs.hpp"
#include "explosion.hpp"
#include "utils.hpp"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Texture.hpp>

class DamageSystem {
public:

  DamageSystem(Coordinator &ecs,
               sf::Sound &explosionSoundPlayer,
               std::vector<Explosion> &explosions,
               sf::Texture &explosionTexture)
  : ecs(ecs),
    explosionSoundPlayer(explosionSoundPlayer),
    explosions(explosions),
    explosionTexture(explosionTexture)
  {}

  void Update() {
    // Iterate through all entities with Health and Position components
    for (auto entity : ecs.view<Health, Position>()) {
      auto &health = ecs.getComponent<Health>(entity);
      auto &position = ecs.getComponent<Position>(entity);

      // Check if health is below or equal to zero
      if (health.value <= 0) {
        // std::cout << "Entity " << entity << " destroyed due to damage.\n";
        // Create an explosion at the entity's position
        explosions.emplace_back(&explosionTexture, position.value, 8, 7);
        explosionSoundPlayer.play();
        // disable for testing
        destroyEntity(ecs, entity); 
      }
    }
  }

private:
  Coordinator &ecs;
  sf::Sound &explosionSoundPlayer;
  std::vector<Explosion> &explosions;
  sf::Texture &explosionTexture;
};

