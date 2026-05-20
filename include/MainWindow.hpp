#pragma once

#include "AudioEngine.hpp"
#include "SonifyEngine.hpp"
#include "thirdparty/argparse.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window.hpp>
#include <lua.hpp>

class MainWindow
{

public:
    MainWindow();
    ~MainWindow();
    void main_loop();
    void read_args(const argparse::ArgumentParser &parser);

    inline sonify::SonifyEngine *sonifier() noexcept
    {
        return m_sonifier;
    }

    inline void set_direction(sonify::Direction direction) noexcept
    {
        m_direction = direction;
        m_sonifier->set_direction(direction);
    }

    inline sonify::Direction direction() const noexcept
    {
        return m_direction;
    }

    inline void set_cursor_width(float w) noexcept
    {
        m_cursor_width = w;
    }

    inline float cursor_width() const noexcept
    {
        return m_cursor_width;
    }

    void set_cursor_color(const std::string &color_str) noexcept;
    std::string cursor_color() const noexcept;

private:
    void load_image(sf::Image &img, const std::string &filename);
    /* Interactive methods */
    void open_file(const std::string &filename);
    void play() noexcept;
    void pause() noexcept;
    void stop() noexcept;
    void toggle_pause() noexcept;
    bool sonify();

    // Lua integration
    void init_lua(const std::string &script_file);
    void init_lua_sonopix() noexcept;
    void init_lua_sonopix_opts() noexcept;

    /* Events */
    void handle_events() noexcept;
    void handle_resize_event(const sf::Event::Resized *e) noexcept;
    void handle_keypress_event(const sf::Event::KeyPressed *e) noexcept;

    float rescale_recenter_image() noexcept;
    void render() noexcept;
    void update() noexcept;
    void move_cursor() noexcept;

    void init_cursor(float scale                 = 1.0f,
                     sf::Vector2<float> position = {}) noexcept;

    AudioEngine *m_audio_engine = nullptr;
    std::string m_window_title  = "Sonopix";
    std::string m_input_file    = "";
    sf::Vector2u m_window_size  = {800, 600};
    sf::Vector2u m_win_size;
    sf::Vector2u m_tex_size;
    sf::RenderWindow m_window;
    sf::Texture m_tex;
    sf::Sprite m_sprite;
    sf::RectangleShape m_cursor_rect;
    sf::Clock m_clock;

    sonify::SonifyEngine *m_sonifier = nullptr;
    bool m_paused                    = true;
    bool m_verbose                   = false;
    float m_cursor_width             = 5.0;
    sf::Color m_cursor_color         = sf::Color(255, 0, 0, 128);
    sonify::Direction m_direction    = sonify::Direction::LEFT_TO_RIGHT;
    lua_State *m_L                   = nullptr;
};
