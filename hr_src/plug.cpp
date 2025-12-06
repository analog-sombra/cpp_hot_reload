#include "plug.hpp"
#include <stdio.h>

void plug_init(PlugState* state)
{
    printf("Plug init called - Creating SFML window\n");
    
    // Create the SFML window
    state->window = new sf::RenderWindow(sf::VideoMode({800, 600}), "Hot Reload Demo");
    state->window->setFramerateLimit(60);
}

void plug_update(PlugState* state)
{    
    if (!state->window || !state->window->isOpen())
        return;
    
    // Clear window with dark gray
    state->window->clear(sf::Color(50, 50, 50));

    
    // Create and draw a circle
    sf::CircleShape circle(50.f);
    circle.setFillColor(sf::Color::Green);
    circle.setPosition({200.f, 250.f});
    state->window->draw(circle);
    
    // Create and draw a square (rectangle with equal sides)
    sf::RectangleShape square({300.f, 200.f});
    square.setFillColor(sf::Color::Red);
    square.setPosition({500.f, 250.f});
    state->window->draw(square);
    
    // Display the rendered frame
    state->window->display();
}