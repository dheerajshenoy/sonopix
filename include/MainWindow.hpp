#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window.hpp>

class MainWindow
{

public:
    MainWindow();
    void main_loop();

private:
    void handle_events() noexcept;
    void handle_resize_event(const sf::Event::Resized *e) noexcept;
    void rescale_recenter_image() noexcept;
    void render() noexcept;

    sf::Vector2u m_window_size{800, 600};
    sf::Vector2u m_win_size;
    sf::Vector2u m_tex_size;
    std::string m_window_title{"SFML Window"};
    sf::RenderWindow m_window;
    sf::Texture m_texture;
    sf::RectangleShape m_image_rect;
    sf::RectangleShape m_cursor_rect;

};
