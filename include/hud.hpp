#pragma once

#include "../include/ecs.hpp"
#include <SFML/Graphics.hpp>

class HUD {
public:
  HUD(Coordinator& ecs, Entity player);

  ~HUD() = default;

  void DrawHUD(sf::RenderWindow& window, Entity enemy1, Entity enemy2, float zoomFactor);


private:
  sf::Font font;
  Coordinator& ecs;
  Entity player;
 
  u_int16_t screenWidth;
  u_int16_t screenHeight;
  sf::Vector2f screenCentre;

  void DrawSidebarText(sf::RenderWindow& window, Entity e);
  void DrawShipNames (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawTorpedoOverlay (sf::RenderWindow& window, float zoomFactor);
  void DrawVectorOverlay (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawPlayerPdcOverlay (sf::RenderWindow& window, Entity e, float zoomFactor);
};
