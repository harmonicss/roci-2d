#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <sys/types.h>

void DrawHUD(sf::RenderWindow& window, Coordinator& ecs, Entity e, sf::Font& font) {

  u_int16_t screenWidth = window.getSize().x;
  u_int16_t screenHeight = window.getSize().y;

  // Sidebar
  sf::RectangleShape sidebar({180.f, static_cast<float>(screenHeight)});
  sf::Vector2f sidebarPosition = {0.f, 0.f};
  sidebar.setFillColor(sf::Color(10,40,50));
  sidebar.setPosition(sidebarPosition);
  window.draw(sidebar);
}

void DrawSidebarText(sf::RenderWindow& window, Coordinator& ecs, Entity e, sf::Font& font) {

  // display the enemy data at the top, player at the bottom
  float yOffset = 0;

  // entity 0 is the player
  if (e == 0) {
    yOffset = 900.f;
  } else {
    yOffset = 50.f;
  }

  // Sidebar Text
  char bufx[32];
  char bufy[32];
   
  sf::Text veltext(font);

  std::snprintf(bufx, sizeof(bufx), "%.2f", ecs.getComponent<Velocity>(e).value.x);
  std::snprintf(bufy, sizeof(bufy), "%.2f", ecs.getComponent<Velocity>(e).value.y);

  sf::String velString = std::string("Vel: ") + bufx + "," + bufy;
  veltext.setString(velString);
  veltext.setCharacterSize(10);
  veltext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f velTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  veltext.setPosition(velTextPosition);
  window.draw(veltext);

  // Rotation
  sf::Text rottext(font);
  std::snprintf(bufx, sizeof(bufx), "%.1f", ecs.getComponent<Rotation>(e).angle);
  sf::String rotString = std::string("Rot: ") + bufx; 
  rottext.setString(rotString);
  rottext.setCharacterSize(10);
  rottext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f rotTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  rottext.setPosition(rotTextPosition);
  window.draw(rottext);

  // Position
  sf::Text postext(font);
  std::snprintf(bufx, sizeof(bufx), "%.0f", ecs.getComponent<Position>(e).value.x);
  std::snprintf(bufy, sizeof(bufy), "%.0f", ecs.getComponent<Position>(e).value.y);
  sf::String posString = std::string("Pos: ") + bufx + "," + bufy;
  postext.setString(posString);
  postext.setCharacterSize(10);
  postext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f posTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  postext.setPosition(posTextPosition);
  window.draw(postext);
}

void DrawShipNames (sf::RenderWindow& window, Coordinator& ecs, Entity e, sf::Font& font, float zoomFactor) {

  u_int16_t screenWidth = window.getSize().x;
  u_int16_t screenHeight = window.getSize().y;
  sf::Vector2f screenCentre = {screenWidth / 2.f, screenHeight / 2.f};
  
  // draw the name on the gui
  sf::Text nametext(font);
  nametext.setString(ecs.getEntityName(e));
  nametext.setCharacterSize(13);
  nametext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));

  // 0 == player
  auto &ppos = ecs.getComponent<Position>(0);
  auto &epos = ecs.getComponent<Position>(e);

  sf::Vector2f cameraOffset = screenCentre - (ppos.value / zoomFactor);
  sf::Vector2f eposRelative = epos.value / zoomFactor;

  nametext.setPosition(eposRelative + cameraOffset - sf::Vector2f(500.f / zoomFactor, 0.f));
  window.draw(nametext);
}
