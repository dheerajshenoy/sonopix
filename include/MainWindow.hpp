#pragma once

#include "AudioEngine.hpp"
#include "sonify.hpp"
#include "thirdparty/argparse.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window.hpp>

class MainWindow
{

public:
    MainWindow();
    void main_loop();
    void read_args(const argparse::ArgumentParser &parser);

private:
    void load_image(sf::Image &img, const std::string &filename);
    /* Interactive methods */
    void open_file(const std::string &filename);
    void play() noexcept;
    void pause() noexcept;
    void stop() noexcept;
    void toggle_pause() noexcept;
    void sonify();

    /* Events */
    void handle_events() noexcept;
    void handle_resize_event(const sf::Event::Resized *e) noexcept;
    void handle_keypress_event(const sf::Event::KeyPressed *e) noexcept;

    void rescale_recenter_image() noexcept;
    void render() noexcept;
    void update() noexcept;
    void move_cursor() noexcept;

    void init_cursor(float scale = 1.0f) noexcept;

    AudioEngine *m_audio_engine{nullptr};
    std::string m_window_title{"SFML Window"}, m_input_file{""};

    sf::Vector2u m_window_size{800, 600};
    sf::Vector2u m_win_size;
    sf::Vector2u m_tex_size;
    sf::RenderWindow m_window;
    sf::Texture m_tex;
    sf::Sprite m_sprite;
    sf::RectangleShape m_cursor_rect;
    sf::Clock m_clock;
    float m_last_time{0.0f};

    sonify::Sonify m_sonifier;
    bool m_paused{true};
    bool m_verbose{false};
    float m_cursor_width{5.0};
    sonify::Direction m_direction{sonify::Direction::LEFT_TO_RIGHT};
};
