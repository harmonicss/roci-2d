#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <string>
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

  // Sidebar Text
  char bufx[32];
  char bufy[32];

  // entity 0 is the player
  if (e == 0) {
    yOffset = 850.f;
  } else {
    yOffset = 50.f;
  }

  // Name
  sf::Text nametext(font);
  sf::String name = ecs.getEntityName(e);
  nametext.setString(name);
  nametext.setCharacterSize(16);
  nametext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f nameTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  nametext.setPosition(nameTextPosition);
  window.draw(nametext);

  // Health
  sf::Text healthtext(font);
  sf::String healthString = std::string("Health: ") + std::to_string(ecs.getComponent<Health>(e).value);
  healthtext.setString(healthString);
  healthtext.setCharacterSize(10);
  healthtext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f healthTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  healthtext.setPosition(healthTextPosition);
  window.draw(healthtext);

  // Acceleration
  sf::Text acctext(font);
  sf::Vector2f acc = ecs.getComponent<Acceleration>(e).value;
  // convert to Gs with a simple division
  std::snprintf(bufx, sizeof(bufx), "%.1f", acc.length() / 100);
  sf::String accString = std::string("Acc: ") + bufx + "Gs"; 
  acctext.setString(accString);
  acctext.setCharacterSize(10);
  acctext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f accTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  acctext.setPosition(accTextPosition);
  window.draw(acctext);

  // Velocity
  sf::Text veltext(font);
  sf::Vector2f vel = ecs.getComponent<Velocity>(e).value;
  if (vel.x == 0.f && vel.y == 0.f) {
    std::snprintf(bufx, sizeof(bufx), "0.0");
    std::snprintf(bufx, sizeof(bufx), "-");
  } else {
    std::snprintf(bufx, sizeof(bufx), "%.2f", ecs.getComponent<Velocity>(e).value.length());
    std::snprintf(bufy, sizeof(bufy), "%.2f", ecs.getComponent<Velocity>(e).value.angle().asDegrees());
  }
  sf::String velString = std::string("Speed: ") + bufx + ", Angle: " + bufy;
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

  // Pdc rounds
  sf::Text pdctext(font);
  std::snprintf(bufx, sizeof(bufx), "%i", ecs.getComponent<Pdc1>(e).rounds);
  std::snprintf(bufy, sizeof(bufy), "%i", ecs.getComponent<Pdc2>(e).rounds);
  sf::String pdcString = std::string("PDC1: ") + bufx + ", PDC2: " + bufy;
  pdctext.setString(pdcString);
  pdctext.setCharacterSize(10);
  pdctext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f pdcTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  pdctext.setPosition(pdcTextPosition);
  window.draw(pdctext);

  // torpedo rounds
  sf::Text torpedotext(font);
  std::snprintf(bufx, sizeof(bufx), "%i", ecs.getComponent<TorpedoLauncher1>(e).rounds);
  std::snprintf(bufy, sizeof(bufy), "%i", ecs.getComponent<TorpedoLauncher2>(e).rounds);
  sf::String torpedoString = std::string("Torp1: ") + bufx + ", Torp2: " + bufy;
  torpedotext.setString(torpedoString);
  torpedotext.setCharacterSize(10);
  torpedotext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
  sf::Vector2f torpedoTextPosition = { 10.f, yOffset };
  yOffset += 20.f;
  torpedotext.setPosition(torpedoTextPosition);
  window.draw(torpedotext);
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

  // offset the name to the left of the ship
  nametext.setPosition(eposRelative + cameraOffset - sf::Vector2f(500.f / zoomFactor, 0.f));
  window.draw(nametext);
}

void DrawTorpedoOverlay (sf::RenderWindow& window, Coordinator& ecs, sf::Font& font, float zoomFactor) {

  u_int16_t screenWidth = window.getSize().x;
  u_int16_t screenHeight = window.getSize().y;
  sf::Vector2f screenCentre = {screenWidth / 2.f, screenHeight / 2.f};

  // change the size of the circle based on the zoom factor
  float radius = 0.5f + (80.f / zoomFactor);

  // need to get all the missiles
  std::vector<Entity> torpedos = ecs.getEntitiesByName("Torpedo");

  for (auto e : torpedos) {

    auto &ppos = ecs.getComponent<Position>(0);
    auto &tpos = ecs.getComponent<Position>(e);

    // draw a circle around the missile
    sf::CircleShape circle(radius);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineThickness(1.f);
    circle.setOutlineColor(sf::Color(sf::Color::Red));
    circle.setOrigin(sf::Vector2f(radius, radius));

    sf::Vector2f cameraOffset = screenCentre - (ppos.value / zoomFactor);
    sf::Vector2f mposRelative = tpos.value / zoomFactor;

    circle.setPosition(mposRelative + cameraOffset);
    window.draw(circle);
  }
}
