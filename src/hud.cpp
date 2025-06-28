#include "../include/components.hpp"
#include "../include/ecs.hpp"
#include "../include/utils.hpp"
#include "../include/hud.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <string>
#include <sys/types.h>



static void DrawVector(sf::RenderWindow& window, Coordinator& ecs, Entity e, sf::Vector2f start,
                sf::Vector2f end, sf::Vector2f cameraOffset, sf::Color color, float zoomFactor, float thickness);


HUD::HUD(Coordinator& ecs, Entity player) :
    ecs(ecs), player(player) {

    // Load the font
    if (!font.openFromFile("../assets/fonts/FiraCodeNerdFont-Medium.ttf")) {
      throw std::runtime_error("Failed to load font");
    }
  }

// TODO: move from fixed enemy entities
void HUD::DrawHUD(sf::RenderWindow &window, Entity enemy1, Entity enemy2, float zoomFactor) {

  screenWidth = window.getSize().x;
  screenHeight = window.getSize().y;
  screenCentre = {screenWidth / 2.f, screenHeight / 2.f};

  // Sidebar
  sf::RectangleShape sidebar({180.f, static_cast<float>(screenHeight)});
  sf::Vector2f sidebarPosition = {0.f, 0.f};
  sidebar.setFillColor(sf::Color(10,40,50));
  sidebar.setPosition(sidebarPosition);
  window.draw(sidebar);

  DrawSidebarText(window, player);
  DrawSidebarText(window, enemy1);
  DrawShipNames(window, enemy1, zoomFactor);
  DrawShipNames(window, enemy2, zoomFactor);
  DrawShipNames(window, player, zoomFactor);
  DrawTorpedoOverlay(window, zoomFactor);
  DrawPlayerPdcOverlay(window, player, zoomFactor);
  DrawVectorOverlay(window, player, zoomFactor);
  DrawVectorOverlay(window, enemy1, zoomFactor);
  DrawVectorOverlay(window, enemy2, zoomFactor);

}

void HUD::DrawSidebarText(sf::RenderWindow& window, Entity e) {

  // display the enemy data at the top, player at the bottom
  float yOffset = 0;

  // Sidebar Text
  char bufx[32];
  char bufy[32];

  // entity 0 is the player
  if (e == player) {
    yOffset = 800.f;
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
  // auto &pdcEntities = ecs.getComponent<PdcMounts>(e).pdcEntities;
  for (Entity pdcEntity : ecs.getComponent<PdcMounts>(e).pdcEntities) {
    auto &pdc = ecs.getComponent<Pdc>(pdcEntity);
    std::snprintf(bufx, sizeof(bufx), "%i", pdc.rounds);
    sf::String pdcString = ecs.getEntityName(pdcEntity) + ": " + bufx;
    pdctext.setString(pdcString);
    pdctext.setCharacterSize(10);
    pdctext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));
    sf::Vector2f pdcTextPosition = { 10.f, yOffset };
    yOffset += 20.f;
    pdctext.setPosition(pdcTextPosition);
    window.draw(pdctext);
  }

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

void HUD::DrawShipNames (sf::RenderWindow& window, Entity e, float zoomFactor) {

  // draw the name on the gui
  sf::Text nametext(font);
  nametext.setString(ecs.getEntityName(e));
  nametext.setCharacterSize(13);
  nametext.setFillColor(sf::Color(0x81, 0xb6, 0xbe));

  auto &ppos = ecs.getComponent<Position>(player);
  auto &epos = ecs.getComponent<Position>(e);

  sf::Vector2f cameraOffset = screenCentre - (ppos.value / zoomFactor);
  sf::Vector2f eposRelative = epos.value / zoomFactor;

  // offset the name to the left of the ship
  nametext.setPosition(eposRelative + cameraOffset - sf::Vector2f(500.f / zoomFactor, 0.f));
  window.draw(nametext);
}

// draws the vector on the ship
void HUD::DrawVectorOverlay (sf::RenderWindow& window, Entity e, float zoomFactor) {

  auto &ppos = ecs.getComponent<Position>(player);
  auto &epos = ecs.getComponent<Position>(e);
  auto &evel = ecs.getComponent<Velocity>(e);

  sf::Vector2f cameraOffset = screenCentre - (ppos.value / zoomFactor);

  DrawVector(window, ecs, e, epos.value, evel.value, cameraOffset, sf::Color::Green, zoomFactor, 40.f);
}

// draws the angles of the pdc targeting on the player
void HUD::DrawPlayerPdcOverlay (sf::RenderWindow& window, Entity e, float zoomFactor) {

  auto &ppos = ecs.getComponent<Position>(e);
  auto prot = ecs.getComponent<Rotation>(e);

  sf::Vector2f cameraOffset = screenCentre - (ppos.value / zoomFactor);

  auto &pdcEntities = ecs.getComponent<PdcMounts>(e).pdcEntities;
  auto &pdc1 = ecs.getComponent<Pdc>(pdcEntities[0]);
  auto &pdc2 = ecs.getComponent<Pdc>(pdcEntities[1]);
  auto &pdc3 = ecs.getComponent<Pdc>(pdcEntities[2]);
  auto &pdc4 = ecs.getComponent<Pdc>(pdcEntities[3]);
  auto &pdc5 = ecs.getComponent<Pdc>(pdcEntities[4]);
  auto &pdc6 = ecs.getComponent<Pdc>(pdcEntities[5]);

  // firing angle is as absolute, not rotataed angle
  // create a vector for the pdc1 firing angle
  sf::Vector2f pdc1Vector = {static_cast<float>(std::cos((pdc1.firingAngle) * (M_PI / 180.f)) * 200.f),
                             static_cast<float>(std::sin((pdc1.firingAngle) * (M_PI / 180.f)) * 200.f)};

  sf::Vector2f pdc2Vector = {static_cast<float>(std::cos((pdc2.firingAngle) * (M_PI / 180.f)) * 200.f),
                             static_cast<float>(std::sin((pdc2.firingAngle) * (M_PI / 180.f)) * 200.f)};

  sf::Vector2f pdc3Vector = {static_cast<float>(std::cos((pdc3.firingAngle) * (M_PI / 180.f)) * 200.f),
                             static_cast<float>(std::sin((pdc3.firingAngle) * (M_PI / 180.f)) * 200.f)};

  sf::Vector2f pdc4Vector = {static_cast<float>(std::cos((pdc4.firingAngle) * (M_PI / 180.f)) * 200.f),
                             static_cast<float>(std::sin((pdc4.firingAngle) * (M_PI / 180.f)) * 200.f)};

  sf::Vector2f pdc5Vector = {static_cast<float>(std::cos((pdc5.firingAngle) * (M_PI / 180.f)) * 200.f),
                             static_cast<float>(std::sin((pdc5.firingAngle) * (M_PI / 180.f)) * 200.f)};

  sf::Vector2f pdc6Vector = {static_cast<float>(std::cos((pdc6.firingAngle) * (M_PI / 180.f)) * 200.f),
                             static_cast<float>(std::sin((pdc6.firingAngle) * (M_PI / 180.f)) * 200.f)};

  // fire from the actual pdc, not the centre of the ship
  sf::Vector2f pdc1Offset = rotateVector({pdc1.positionx, pdc1.positiony}, prot.angle);
  sf::Vector2f pdc2Offset = rotateVector({pdc2.positionx, pdc2.positiony}, prot.angle);
  sf::Vector2f pdc3Offset = rotateVector({pdc3.positionx, pdc3.positiony}, prot.angle);
  sf::Vector2f pdc4Offset = rotateVector({pdc4.positionx, pdc4.positiony}, prot.angle);
  sf::Vector2f pdc5Offset = rotateVector({pdc5.positionx, pdc5.positiony}, prot.angle);
  sf::Vector2f pdc6Offset = rotateVector({pdc6.positionx, pdc6.positiony}, prot.angle);

  DrawVector(window, ecs, e, ppos.value + pdc1Offset, pdc1Vector, cameraOffset, sf::Color::Red, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value + pdc2Offset, pdc2Vector, cameraOffset, sf::Color::Green, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value + pdc3Offset, pdc3Vector, cameraOffset, sf::Color::Red, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value + pdc4Offset, pdc4Vector, cameraOffset, sf::Color::Green, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value + pdc5Offset, pdc5Vector, cameraOffset, sf::Color::Blue, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value + pdc6Offset, pdc6Vector, cameraOffset, sf::Color::Cyan, zoomFactor, 10.f);

#if 0 // firing arcs
  sf::Color darkRed(156, 0, 0);
  sf::Color lightRed(150, 110, 110);
  sf::Color darkGreen(52, 156, 0);
  sf::Color lightGreen(123, 150, 110);

  // draw the min and max angles for the pdc1
  // Normalize angles
  float headingAngle = normalizeAngle(prot.angle);

  // calculate angular difference relative to the front of the ship
  float absoluteFiringAngle = normalizeAngle(pdc1.firingAngle);

  float rotatedMinAngle = normalizeAngle(pdc1.minFiringAngle + prot.angle);
  float rotatedMaxAngle = normalizeAngle(pdc1.maxFiringAngle + prot.angle);

  sf::Vector2f pdc1MinVector = {static_cast<float>(std::cos((rotatedMinAngle) * (M_PI / 180.f)) * 250.f),
                                static_cast<float>(std::sin((rotatedMinAngle) * (M_PI / 180.f)) * 250.f)};

  sf::Vector2f pdc1MaxVector = {static_cast<float>(std::cos((rotatedMaxAngle) * (M_PI / 180.f)) * 250.f),
                                static_cast<float>(std::sin((rotatedMaxAngle) * (M_PI / 180.f)) * 250.f)};

  DrawVector(window, ecs, e, ppos.value, pdc1MinVector, cameraOffset, lightRed, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value, pdc1MaxVector, cameraOffset, darkRed, zoomFactor, 10.f);

  // pdc2 min and max angles
  // NOTE: this does show pdc2 min and max backwards, but live with this for now

  rotatedMinAngle = normalizeAngle(pdc2.minFiringAngle + prot.angle);
  rotatedMaxAngle = normalizeAngle(pdc2.maxFiringAngle + prot.angle);

  sf::Vector2f pdc2MinVector = {static_cast<float>(std::cos((rotatedMinAngle) * (M_PI / 180.f)) * 250.f),
                                static_cast<float>(std::sin((rotatedMinAngle) * (M_PI / 180.f)) * 250.f)};

  sf::Vector2f pdc2MaxVector = {static_cast<float>(std::cos((rotatedMaxAngle) * (M_PI / 180.f)) * 250.f),
                                static_cast<float>(std::sin((rotatedMaxAngle) * (M_PI / 180.f)) * 250.f)};

  DrawVector(window, ecs, e, ppos.value, pdc2MinVector, cameraOffset, lightGreen, zoomFactor, 10.f);
  DrawVector(window, ecs, e, ppos.value, pdc2MaxVector, cameraOffset, darkGreen, zoomFactor, 10.f);
#endif
}

void HUD::DrawTorpedoOverlay (sf::RenderWindow& window, float zoomFactor) {

  // change the size of the circle based on the zoom factor
  float radius = 0.5f + (80.f / zoomFactor);

  // need to get all the missiles
  std::vector<Entity> torpedos = ecs.getEntitiesByName("Torpedo");

  for (auto e : torpedos) {

    auto &ppos = ecs.getComponent<Position>(player);
    auto &tpos = ecs.getComponent<Position>(e);
    auto &trot = ecs.getComponent<Rotation>(e);
    auto &ttgt = ecs.getComponent<TorpedoTarget>(e);

    //std::cout << "DrawTorpedoOverlay Torpedo Position: " << tpos.value.x << ", " << tpos.value.y << "\n";

    ///////////////////////////////////////////////////////////////////////////////
    // draw a circle around the missile
    ///////////////////////////////////////////////////////////////////////////////
    sf::CircleShape circle(radius);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineThickness(1.f);
    if (ttgt.target == player)
      circle.setOutlineColor(sf::Color(sf::Color::Red));
    else
      circle.setOutlineColor(sf::Color(sf::Color::Blue));

    circle.setOrigin(sf::Vector2f(radius, radius));

    // zoomFactor is needed here, as I am drawing this on a seperate HUD view
    sf::Vector2f cameraOffset = screenCentre - (ppos.value / zoomFactor);
    sf::Vector2f tposRelative = tpos.value / zoomFactor;

    circle.setPosition(tposRelative + cameraOffset);
    window.draw(circle);

    ///////////////////////////////////////////////////////////////////////////////
    // draw the vector of the acceleration, velocity and rotation
    ///////////////////////////////////////////////////////////////////////////////
    DrawVector(window, ecs, e, tpos.value, ecs.getComponent<Acceleration>(e).value, cameraOffset, sf::Color::Red, zoomFactor, 20.f);
    DrawVector(window, ecs, e, tpos.value, ecs.getComponent<Velocity>(e).value, cameraOffset, sf::Color::Green, zoomFactor, 20.f);

    float radians = trot.angle * (M_PI / 180.f);
 
    DrawVector(window, ecs, e, tpos.value,
               sf::Vector2f{std::cos(radians) * 10000.f, std::sin(radians) * 10000.f}, 
               cameraOffset, sf::Color::Blue, zoomFactor, 30.f);
  }
}

static void DrawVector(sf::RenderWindow& window, Coordinator& ecs, Entity e, sf::Vector2f start,
                sf::Vector2f end, sf::Vector2f cameraOffset, sf::Color color, float zoomFactor, float thickness) {

    // std::cout << "\nDrawVectorStart: " << start.x << ", " << start.y << "\n";
    // std::cout << "DrawVectorEnd: " << end.x << ", " << end.y << "\n";
    // std::cout << "DrawVector Camera Offset: " << cameraOffset.x << ", " << cameraOffset.y << "\n";

    // compute the direction and length of the arrow shaft

    float length = std::hypot(end.x, end.y);
    // std::cout << "DrawVector Length: " << length << "\n";

    length = length / zoomFactor; // scale length with zoom factor
    length *= 2;                  // easier to see with zoom

    if (length < 1.f)
      return; // don't draw if the length is too small

    thickness = thickness / zoomFactor; // make it thinner with zoom

    if (thickness < 1.f)
      thickness = 1.f; // don't let it get too thin
  
    // create a rectangle of size (length x thickness)
    sf::RectangleShape arrow(sf::Vector2f(length, thickness));

    arrow.setFillColor(color);

    // centre it vertically
    arrow.setOrigin(sf::Vector2f(0.f, thickness / 2.f));

    // position the arrow at the start position
    arrow.setPosition((start / zoomFactor) + cameraOffset);

    // rotate to match the direction of the arrow
    float angle = std::atan2(end.y, end.x) * (180.f / M_PI);

    arrow.setRotation(sf::degrees(angle));

    window.draw(arrow);
}
