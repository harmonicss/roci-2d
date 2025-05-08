#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>

int main() {

  auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "Rocinante",
                                 sf::Style::Default);
  window.setFramerateLimit(60);

  sf::Texture rociTexture;

  if (!rociTexture.loadFromFile("../assets/textures/roci.png")) {
    std::cout << "Error loading texture" << std::endl;
    return -1;
  }

  sf::Sprite rociSprite(rociTexture);
  sf::Vector2f rociOrigin(rociTexture.getSize().x / 2.f,
                          rociTexture.getSize().y / 2.f);
  rociSprite.setOrigin(rociOrigin);

  sf::FloatRect viewRect({0.f, 0.f}, {1920.f, 1080.f});
  sf::View view(viewRect);
  window.setView(view);

  float zoomFactor = 1.f;
  float rotationAngle = 0.f;

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (event->is<sf::Event::MouseMoved>()) {
        auto *move = event->getIf<sf::Event::MouseMoved>();

        rotationAngle += move->position.x - (window.getSize().x / 0.1f);
        sf::Angle angle = sf::degrees(rotationAngle);
        rociSprite.setRotation(angle);
      }
      else if (event->is<sf::Event::MouseWheelScrolled>()) {
        auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>();

        std::cout << "Scroll delta: " << scroll->delta << std::endl;

        if (scroll->delta > 0) {
          zoomFactor *= 1.1f;
        } else {
          zoomFactor /= 1.1f;
        }

        view.setSize({1920 * zoomFactor, 1080 * zoomFactor});
        window.setView(view);
      }
    }

    window.clear(sf::Color::Black);
    rociSprite.setPosition(view.getCenter());
    window.draw(rociSprite);
    window.display();
  }
}
