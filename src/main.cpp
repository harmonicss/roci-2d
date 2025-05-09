#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
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
      else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {

        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
          window.close();
        }
      }
      else if (event->is<sf::Event::MouseWheelScrolled>()) {
        auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>();

        std::cout << "Scroll delta: " << scroll->delta << std::endl;

        if (scroll->delta < 0) {
          zoomFactor *= 1.1f;
        } else {
          zoomFactor /= 1.1f;
        }

        view.setSize({1920 * zoomFactor, 1080 * zoomFactor});
        window.setView(view);
      }

      // dont use events for the keyboard, check if currently pressed. 
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
          rotationAngle -= (window.getSize().x / 1000.f);
          sf::Angle angle = sf::degrees(rotationAngle);
          std::cout << "rotationAngle: " << rotationAngle << "angle: " << angle.asDegrees() << std::endl;
          rociSprite.setRotation(angle);
      }
      else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
          rotationAngle += (window.getSize().x / 1000.f);
          sf::Angle angle = sf::degrees(rotationAngle);
          std::cout << "rotationAngle: " << rotationAngle << "angle: " << angle.asDegrees() << std::endl;
          rociSprite.setRotation(angle);
      }

    }

    window.clear(sf::Color(56,58,94));
    rociSprite.setPosition(view.getCenter());
    window.draw(rociSprite);
    window.display();
  }
}
