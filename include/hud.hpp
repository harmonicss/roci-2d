#pragma once

#include "ecs.hpp"
#include "torpedotarget.hpp"
#include <SFML/Graphics.hpp>

class HUD {
public:
  HUD(Coordinator& ecs, Entity player, TorpedoTargeting &torpedoTargeting);

  ~HUD() = default;

  void DrawHUD(sf::RenderWindow& window, Entity enemy1, Entity enemy2, Entity enemy3, float zoomFactor);

  void toggleOverlay();

private:
  sf::Font font;
  Coordinator& ecs;
  Entity player;
  TorpedoTargeting &torpedoTargeting;
 
  u_int16_t screenWidth;
  u_int16_t screenHeight;
  sf::Vector2f screenCentre;

  // distance in pixels to consider a torpedo a threat for warning 
  const float torpedoThreatRange = 500000.f;

  bool displayOverlay = true; 

  enum class SidebarPosition {LEFT_TOP, RIGHT_TOP, LEFT_MIDDLE, RIGHT_MIDDLE, LEFT_BOTTOM, RIGHT_BOTTOM};

  void DrawTorpedoThreat(sf::RenderWindow & window);
  void DrawTorpedoTargetingText(sf::RenderWindow & window);
  void DrawSidebarText(sf::RenderWindow& window, Entity e, SidebarPosition pos);
  void DrawShipNames (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawTorpedoOverlay (sf::RenderWindow& window, float zoomFactor);
  void DrawEnemyShipOverlay (sf::RenderWindow& window, float zoomFactor);
  void DrawVectorOverlay (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawPlayerPdcOverlay (sf::RenderWindow& window, Entity e, float zoomFactor);
  void DrawVector(sf::RenderWindow& window, Coordinator& ecs, Entity e, sf::Vector2f start,
                  sf::Vector2f end, sf::Vector2f cameraOffset, sf::Color color, float zoomFactor, float thickness);
};
