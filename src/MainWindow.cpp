#include "MainWindow.hpp"

#include <SFML/Graphics/Sprite.hpp>

MainWindow::MainWindow()
{
    m_window  = sf::RenderWindow(sf::VideoMode(m_window_size), m_window_title);
    m_texture = sf::Texture("wall.jpg");
    m_image_rect = sf::RectangleShape();
    m_image_rect.setSize({static_cast<float>(m_texture.getSize().x),
                          static_cast<float>(m_texture.getSize().y)});
    m_image_rect.setTexture(&m_texture);

    m_cursor_rect = sf::RectangleShape();
    m_cursor_rect.setFillColor(sf::Color::Red);
    rescale_recenter_image();
}

void
MainWindow::handle_events() noexcept
{
    while (const auto e = m_window.pollEvent())
    {

        // Close window: exit
        if (e->is<sf::Event::Closed>())
            m_window.close();
        else if (const auto *resizeEvent = e->getIf<sf::Event::Resized>())
            handle_resize_event(resizeEvent);
        else if (const auto *keyEvent = e->getIf<sf::Event::KeyPressed>())
        {
            if (keyEvent->code == sf::Keyboard::Key::Space) {
            }
        }
    }
}

void
MainWindow::handle_resize_event(const sf::Event::Resized *e) noexcept
{
    sf::Rect<float> visibleArea(
        {0, 0}, {(float)e->size.x, (float)e->size.y});
    m_window.setView(sf::View(visibleArea));

    m_tex_size = m_texture.getSize();
    m_win_size = m_window.getSize();
    rescale_recenter_image();
}

void
MainWindow::rescale_recenter_image() noexcept
{
    const float scaleX = static_cast<float>(m_win_size.x) / m_tex_size.x;
    const float scaleY = static_cast<float>(m_win_size.y) / m_tex_size.y;
    const float scale  = std::min(scaleX, scaleY);

    m_image_rect.setScale({scale, scale});
    m_image_rect.setPosition({(m_win_size.x - m_tex_size.x * scale) * 0.5f,
                              (m_win_size.y - m_tex_size.y * scale) * 0.5f});

    // Set cursor to the left edge of the image starting at the top-left corner
    m_cursor_rect.setSize({5.0f, m_tex_size.y * scale});
    m_cursor_rect.setPosition(
        {m_image_rect.getPosition().x, m_image_rect.getPosition().y});
};

void
MainWindow::main_loop()
{
    while (m_window.isOpen())
    {
        handle_events();
        render();
    }
}

void
MainWindow::render() noexcept
{
    m_window.clear();
    m_window.draw(m_image_rect);
    m_window.draw(m_cursor_rect);
    m_window.display();
}
