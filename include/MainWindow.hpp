#pragma once

#include "AudioEngine.hpp"
#include "Config.hpp"
#include "SonifyEngine.hpp"
#include "thirdparty/argparse.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <future>
#include <lua.hpp>
#include <memory>
#include <utility>
#include <vector>

class MainWindow
{

public:
    MainWindow();
    ~MainWindow();

    inline sonify::SonifyEngine *sonifier() noexcept
    {
        return m_sonifier.get();
    }

    inline AudioEngine *audio_engine() noexcept
    {
        return m_audio_engine.get();
    }

    inline void set_direction(sonify::Direction dir) noexcept
    {
        m_config.direction = dir;
        m_sonifier->set_direction(dir);
    }

    inline sonify::Direction direction() const noexcept
    {
        return m_config.direction;
    }

    inline void set_amplitude(float amp) noexcept
    {
        m_config.amplitude = amp;
    }

    inline float amplitude() const noexcept
    {
        return m_config.amplitude;
    }

    inline float cursor_width() const noexcept
    {
        return m_config.cursor.width;
    }

    inline void set_show_progress_bar(bool show) noexcept
    {
        m_config.progress_bar.visible = show;
    }

    inline bool show_progress_bar() const noexcept
    {
        return m_config.progress_bar.visible;
    }

    inline void set_loop(bool loop) noexcept
    {
        m_config.loop = loop;
        m_audio_engine->set_looping(loop);
    }

    inline bool loop() const noexcept { return m_config.loop; }

    inline void set_antialiasing_level(unsigned int level) noexcept
    {
        m_config.window.antiAliasingLevel = level;
    }

    inline unsigned int antialiasing_level() const noexcept
    {
        return m_config.window.antiAliasingLevel;
    }

    inline std::string file_path() const noexcept
    {
        return m_input_file;
    }

    inline void set_window_title(const std::string &title) noexcept
    {
        m_window_title = title;
        if (m_window.isOpen())
            m_window.setTitle(m_window_title);
    }

    inline std::string window_title() const noexcept
    {
        return m_window_title;
    }

    inline sf::Vector2u window_size() const noexcept
    {
        return m_window_size;
    }

    inline void set_window_size(unsigned int width,
                                unsigned int height) noexcept
    {
        m_window_size = {width, height};
        if (m_window.isOpen())
            m_window.setSize(m_window_size);
    }

    void main_loop();
    void read_args(const argparse::ArgumentParser &parser);
    void set_cursor_width(float w) noexcept;
    void set_cursor_color(const std::string &color_str) noexcept;
    std::string cursor_color() const noexcept;
    bool save_audio(const std::string &filename) noexcept;

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
    void collect_traversal_pixels() noexcept;

    /* Events */
    void handle_events() noexcept;
    void handle_resize_event(const sf::Event::Resized *e) noexcept;
    void handle_keypress_event(const sf::Event::KeyPressed *e) noexcept;
    void handle_mouse_press_event(const sf::Event::MouseButtonPressed *e) noexcept;
    void handle_mouse_release_event(const sf::Event::MouseButtonReleased *e) noexcept;
    void handle_mouse_move_event(const sf::Event::MouseMoved *e) noexcept;
    void seek_waveform(float mx) noexcept;
    void seek_relative(float seconds) noexcept;

    float rescale_recenter_image() noexcept;
    void render() noexcept;
    void update() noexcept;
    void move_cursor() noexcept;

    void init_cursor(float scale                 = 1.0f,
                     sf::Vector2<float> position = {}) noexcept;
    void init_playback_bar() noexcept;
    void init_waveform() noexcept;
    void build_waveform() noexcept;
    void update_waveform() noexcept;
    void init_oscilloscope() noexcept;
    void update_oscilloscope() noexcept;
    void update_playback_bar() noexcept;
    void create_window() noexcept;

    std::unique_ptr<AudioEngine> m_audio_engine;
    std::unique_ptr<sonify::SonifyEngine> m_sonifier;
    std::string m_window_title = "Sonopix";
    std::string m_input_file   = "";
    std::string m_output_file  = "";
    sf::Vector2u m_window_size = {800, 600};
    sf::Vector2u m_win_size;
    sf::Vector2u m_tex_size;
    sf::RenderWindow m_window;
    sf::Texture m_tex;
    sf::Sprite m_sprite;
    std::unique_ptr<sf::Shape> m_cursor;
    std::unique_ptr<sf::RectangleShape> m_playback_bar;
    std::unique_ptr<sf::RectangleShape> m_playback_fill;
    std::unique_ptr<sf::RectangleShape> m_waveform_bg;
    std::unique_ptr<sf::VertexArray> m_waveform;
    std::unique_ptr<sf::VertexArray> m_waveform_playhead;
    std::unique_ptr<sf::RectangleShape> m_oscilloscope_bg;
    std::unique_ptr<sf::VertexArray> m_oscilloscope;
    sf::Clock m_clock;

    Config m_config;
    std::future<void> m_sonify_future;
    std::size_t m_last_sample_index = 0;
    std::vector<std::pair<int, int>> m_traversal_pixels;
    bool m_using_custom_traversal = false;
    bool m_seeking                = false;
    lua_State *m_L                = nullptr;
};
