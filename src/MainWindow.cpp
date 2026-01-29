#include "MainWindow.hpp"

#include <print>

MainWindow::MainWindow()
{
    m_window = sf::RenderWindow(sf::VideoMode(m_window_size), m_window_title);
}

void
MainWindow::read_args(const argparse::ArgumentParser &parser)
{
    if (parser.is_used("version"))
    {
        std::print("%s - version %s", APP_NAME, APP_VERSION);
        return;
    }

    if (parser.is_used("input"))
    {
        open_file(parser.get<std::string>("input"));
    }
}

void
MainWindow::open_file(const std::string &filename)
{
    if (!m_texture.loadFromFile(filename))
    {
        std::print("Failed to load image from file: {}\n", filename);
        return;
    }

    m_image_rect.setSize({static_cast<float>(m_texture.getSize().x),
                          static_cast<float>(m_texture.getSize().y)});
    m_image_rect.setTexture(&m_texture);

    m_image_rect.setTexture(&m_texture);

    m_tex_size = m_texture.getSize();
    m_win_size = m_window.getSize();
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
            handle_keypress_event(keyEvent);
    }
}

void
MainWindow::handle_resize_event(const sf::Event::Resized *e) noexcept
{
    sf::Rect<float> visibleArea({0, 0}, {(float)e->size.x, (float)e->size.y});
    m_window.setView(sf::View(visibleArea));

    m_tex_size = m_texture.getSize();
    m_win_size = m_window.getSize();
    rescale_recenter_image();
}

void
MainWindow::handle_keypress_event(const sf::Event::KeyPressed *e) noexcept
{
    switch (e->code)
    {
        case sf::Keyboard::Key::Space:
            toggle_pause();
            break;
        case sf::Keyboard::Key::S:
            stop();
            break;
        default:
            break;
    }
}

void
MainWindow::rescale_recenter_image() noexcept
{
    const float scaleX
        = static_cast<float>(m_win_size.x - m_win_size.x * 0.25) / m_tex_size.x;
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

void
MainWindow::play() noexcept
{
}

void
MainWindow::pause() noexcept
{
    // Implementation for pause functionality
}

void
MainWindow::stop() noexcept
{
    // Implementation for stop functionality
}

void
MainWindow::toggle_pause() noexcept
{
    m_is_paused = !m_is_paused;
    if (m_is_paused)
    {
        pause();
    }
    else
    {
        play();
    }
}

void
MainWindow::move_cursor() noexcept
{
    float current_time = m_clock.getElapsedTime().asSeconds();
    float delta_time   = current_time - m_last_time;
    m_last_time        = current_time;

    m_cursor_rect.move({100.0f * delta_time, 0.0f});
}
