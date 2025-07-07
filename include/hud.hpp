#pragma once

#include "ecs.hpp"
#include "torpedotarget.hpp"
#include <SFML/Graphics.hpp>

class HUD {
public:
  HUD(Coordinator& ecs, Entity player, TorpedoTargeting &torpedoTargeting);

  ~HUD() = default;

  void DrawHUD(sf::RenderWindow& window, Entity enemy1, Entity enemy2, Entity enemy3, float zoomFactor);


private:
  sf::Font font;
  Coordinator& ecs;
  Entity player;
  TorpedoTargeting &torpedoTargeting;
 
  u_int16_t screenWidth;
  u_int16_t screenHeight;
  sf::Vector2f screenCentre;

  enum class SidebarPosition {LEFT_TOP, RIGHT_TOP, LEFT_MIDDLE, RIGHT_MIDDLE, LEFT_BOTTOM, RIGHT_BOTTOM};

  void DrawTorpedoTargetingText(sf::RenderWindow & window);
  void DrawSidebarText(sf::RenderWindow& window, Entity e, SidebarPosition pos);
  void DrawShipNames (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawTorpedoOverlay (sf::RenderWindow& window, float zoomFactor);
  void DrawVectorOverlay (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawPlayerPdcOverlay (sf::RenderWindow& window, Entity e, float zoomFactor);
};
