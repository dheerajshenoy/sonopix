#include "MainWindow.hpp"

#include "Effects.hpp"
#include "lua/init.cpp"
#include "shaders/image_effects.hpp"
#include "utils.hpp"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <cmath>
#include <fstream>
#include <print>
#include <sys/inotify.h>
#include <unistd.h>
#include <webp/decode.h>

MainWindow::MainWindow() : m_sprite(m_tex)
{
    m_config.window.antiAliasingLevel = 8;
    m_config.window.depthBits         = 24;
    m_sonifier     = std::make_unique<sonify::SonifyEngine>();
    m_audio_engine = std::make_unique<AudioEngine>();
}

MainWindow::~MainWindow()
{
    stop_script_watcher();
    if (m_L)
    {
        clear_event_listeners();
        lua_close(m_L);
    }
}

void
MainWindow::create_window() noexcept
{
    m_window = sf::RenderWindow(sf::VideoMode(m_window_size), m_window_title,
                                sf::Style::Default, sf::State::Windowed,
                                m_config.window);
    m_window.setFramerateLimit(m_config.fps_limit);
}

void
MainWindow::read_args(const argparse::ArgumentParser &parser)
{
    if (parser.is_used("version"))
    {
        std::print("%s - version %s", APP_NAME, APP_VERSION);
        exit(0);
    }

    if (parser.is_used("help"))
    {
        std::println("{}", parser.help().str());
        exit(0);
    }

    if (parser.is_used("verbose"))
    {
        m_config.verbose = true;
        assert(0 && "Verbose logging not implemented yet");
    }

    if (parser.is_used("script"))
    {
        m_script_file = parser.get<std::string>("script");
        try
        {
            init_lua(m_script_file);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        if (parser.is_used("hot-reload"))
        {
            m_hot_reload_script = true;
            start_script_watcher(m_script_file);
        }
    }

    if (parser.is_used("secs-per-unit"))
    {
        m_sonifier->set_secs_per_unit(parser.get<float>("secs-per-unit"));
    }

    if (parser.is_used("sample-rate"))
    {
        m_sonifier->set_sample_rate(parser.get<float>("sample-rate"));
    }

    if (parser.is_used("channels"))
    {
        int channels = parser.get<int>("channels");
        assert(0 && "Channels is not supported currently");
    }

    if (parser.is_used("freq-scale"))
    {
        std::string scale_str = parser.get<std::string>("freq-scale");
        sonify::FreqScale scale;
        if (scale_str == "linear")
            scale = sonify::FreqScale::LINEAR;
        else if (scale_str == "log")
            scale = sonify::FreqScale::LOG;
        else if (scale_str == "exponential")
            scale = sonify::FreqScale::EXPONENTIAL;
        else
            throw std::runtime_error("Invalid frequency scale: " + scale_str);

        m_sonifier->set_freq_scale(scale);
    }

    if (parser.is_used("frequency"))
    {
        auto [fmin, fmax] = parser.get<std::pair<float, float>>("frequency");
        m_sonifier->set_freq_range(fmin, fmax);
    }

    if (parser.is_used("input"))
    {
        open_file(parser.get<std::string>("input"));
    }

    if (parser.is_used("output"))
    {
        m_output_file = parser.get<std::string>("output");
        if (m_output_file.find('.') == std::string::npos)
            m_output_file += ".wav";
    }

    if (parser.is_used("direction"))
    {
        const std::string dir_str = parser.get<std::string>("direction");
        sonify::Direction direction;

        if (dir_str == "left-to-right")
            direction = sonify::Direction::LEFT_TO_RIGHT;

        else if (dir_str == "right-to-left")
            direction = sonify::Direction::RIGHT_TO_LEFT;

        else if (dir_str == "top-to-bottom")
            direction = sonify::Direction::TOP_TO_BOTTOM;

        else if (dir_str == "bottom-to-top")
            direction = sonify::Direction::BOTTOM_TO_TOP;

        else if (dir_str == "circle-outwards")
            direction = sonify::Direction::CIRCLE_OUTWARDS;

        else if (dir_str == "circle-inwards")
            direction = sonify::Direction::CIRCLE_INWARDS;

        else if (dir_str == "rotate-cw")
            direction = sonify::Direction::ROTATE_CW;

        else if (dir_str == "rotate-ccw")
            direction = sonify::Direction::ROTATE_CCW;

        else
            throw std::runtime_error("Invalid direction: " + dir_str);

        m_config.direction = direction;
        m_sonifier->set_direction(direction);
    }

    if (parser.is_used("cursor-width"))
    {
        m_config.cursor.width = parser.get<float>("cursor-width");
    }
}

void
MainWindow::load_image(sf::Image &image, const std::string &filename)
{
    std::string fixed_filename = filename;
    if (!fixed_filename.empty() && fixed_filename[0] == '~')
    {
        const char *home = std::getenv("HOME");
        if (home)
            fixed_filename.replace(0, 1, home);
    }

    const bool is_webp = fixed_filename.size() >= 5
        && fixed_filename.substr(fixed_filename.size() - 5) == ".webp";

    if (is_webp)
    {
        std::ifstream f(fixed_filename, std::ios::binary);
        if (!f)
            throw std::runtime_error("Failed to open WebP file: " + filename);
        std::vector<std::uint8_t> raw(
            (std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());

        int w = 0, h = 0;
        std::uint8_t *pixels = WebPDecodeRGBA(raw.data(), raw.size(), &w, &h);
        if (!pixels)
            throw std::runtime_error("Failed to decode WebP file: " + filename);

        image = sf::Image({static_cast<unsigned>(w), static_cast<unsigned>(h)},
                          pixels);
        WebPFree(pixels);
        return;
    }

    if (!image.loadFromFile(fixed_filename))
        throw std::runtime_error("Failed to load image from file: " + filename);
}

void
MainWindow::open_file(const std::string &filename)
{
    sf::Image img;
    load_image(img, filename);
    m_tex = sf::Texture(img);
    m_sprite.setTexture(m_tex, true);

    m_win_size  = m_window.isOpen() ? m_window.getSize() : m_window_size;
    m_tex_size  = img.getSize();
    const int w = (int)m_tex_size.x;
    const int h = (int)m_tex_size.y;

    if (w <= 0 || h <= 0)
    {
        throw std::runtime_error("SFML: invalid image dimensions");
    }

    float scale = rescale_recenter_image();
    init_cursor(scale);
    init_playback_bar();
    init_waveform();
    init_oscilloscope();

    const std::uint8_t *data = img.getPixelsPtr(); // RGBA8, size = w*h*4
    if (!data)
        throw std::runtime_error("SFML: getPixelsPtr() returned null");

    int channels  = 4;
    auto img_data = sonify::normalize_u8_data(
        data, w * h * channels); // normalized to [0 .. 1]

    m_sonifier->set_raw_image(w, h, channels, w * 4, std::move(img_data));
    fire_event("file_loaded");
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
        else if (const auto *mouseEvent
                 = e->getIf<sf::Event::MouseButtonPressed>())
            handle_mouse_press_event(mouseEvent);
        else if (const auto *mouseEvent
                 = e->getIf<sf::Event::MouseButtonReleased>())
            handle_mouse_release_event(mouseEvent);
        else if (const auto *mouseEvent = e->getIf<sf::Event::MouseMoved>())
            handle_mouse_move_event(mouseEvent);
    }
}

void
MainWindow::handle_resize_event(const sf::Event::Resized *e) noexcept
{
    sf::Rect<float> visibleArea({0, 0}, {(float)e->size.x, (float)e->size.y});
    m_window.setView(sf::View(visibleArea));

    m_tex_size = m_tex.getSize();
    m_win_size = m_window.getSize();

    // Save cursor offset relative to old sprite origin before rescaling
    const float oldScale         = m_sprite.getScale().x;
    const sf::Vector2f oldOrigin = m_sprite.getPosition();
    const sf::Vector2f cursorPos
        = m_cursor ? m_cursor->getPosition() : sf::Vector2f{};
    const sf::Vector2f relOffset
        = oldScale > 0.f ? (cursorPos - oldOrigin) / oldScale : sf::Vector2f{};

    float scale = rescale_recenter_image();

    // Reapply cursor at same image-space offset under new scale
    const sf::Vector2f newOrigin = m_sprite.getPosition();
    init_cursor(scale, newOrigin + relOffset * scale);
    init_playback_bar();
    init_waveform();
    init_oscilloscope();
    if (!m_audio_engine->dataf().empty())
        build_waveform();
    if (m_shader_active)
        sync_shader(); // refresh u_texel_size for new image
}

void
MainWindow::seek_waveform(float mx) noexcept
{
    const std::size_t total = m_audio_engine->dataf().size();
    if (total == 0 || !m_waveform_bg)
        return;

    const float wave_x     = m_waveform_bg->getPosition().x;
    const float wave_width = m_waveform_bg->getSize().x;
    const float progress   = std::clamp((mx - wave_x) / wave_width, 0.f, 1.f);
    const auto target
        = static_cast<std::size_t>(progress * static_cast<float>(total));

    m_audio_engine->seek_to_sample(target);
    m_audio_engine->play_sample_at(target);
    m_last_sample_index = target;
}

void
MainWindow::seek_relative(float fraction) noexcept
{
    const std::size_t total = m_audio_engine->dataf().size();
    if (total == 0)
        return;

    const auto delta
        = static_cast<std::ptrdiff_t>(fraction * static_cast<float>(total));
    const auto target = static_cast<std::size_t>(
        std::clamp(static_cast<std::ptrdiff_t>(m_last_sample_index) + delta,
                   std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(total - 1)));

    m_audio_engine->seek_to_sample(target);
    m_last_sample_index = target;
}

void
MainWindow::handle_mouse_press_event(
    const sf::Event::MouseButtonPressed *e) noexcept
{
    if (e->button != sf::Mouse::Button::Left)
        return;
    if (!m_config.waveform.visible || !m_waveform_bg)
        return;

    const float mx     = static_cast<float>(e->position.x);
    const float my     = static_cast<float>(e->position.y);
    const float wave_x = m_waveform_bg->getPosition().x;
    const float wave_y = m_waveform_bg->getPosition().y;
    const float wave_w = m_waveform_bg->getSize().x;
    const float wave_h = m_waveform_bg->getSize().y;

    if (mx < wave_x || mx > wave_x + wave_w || my < wave_y
        || my > wave_y + wave_h)
        return;

    m_seeking = true;
    seek_waveform(mx);
}

void
MainWindow::handle_mouse_release_event(
    const sf::Event::MouseButtonReleased *e) noexcept
{
    if (e->button == sf::Mouse::Button::Left)
        m_seeking = false;
}

void
MainWindow::handle_mouse_move_event(const sf::Event::MouseMoved *e) noexcept
{
    if (!m_seeking)
        return;
    seek_waveform(static_cast<float>(e->position.x));
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
            sonify();
            break;
        case sf::Keyboard::Key::L:
            set_loop(!m_config.loop);
            break;
        case sf::Keyboard::Key::Left:
            seek_relative(e->shift ? -0.10f : -0.02f);
            break;
        case sf::Keyboard::Key::Right:
            seek_relative(e->shift ? 0.10f : 0.02f);
            break;
        default:
            break;
    }
}

bool
MainWindow::sonify()
{
    if (m_sonify_future.valid()
        && m_sonify_future.wait_for(std::chrono::seconds(0))
               != std::future_status::ready)
        return false; // already running

    m_audio_engine->stop();
    m_last_sample_index = 0;
    m_window.setTitle(m_window_title + " [sonifying...]");

    // Bake current image effects into the sonifier's pixel buffer so that
    // grayscale, brightness, contrast, etc. are reflected in the audio.
    snapshot_shaded_image();

    collect_traversal_pixels(); // runs on main thread (Lua not thread-safe);
                                // fills m_traversal_pixels and
                                // m_using_custom_traversal

    // Reinitialize cursor shape for current mode (custom → point; built-in →
    // line/circle)
    init_cursor(m_sprite.getScale().x);

    m_sonify_future
        = std::async(std::launch::async, [this, amp = m_config.amplitude,
                                          ae = m_config.audio_effects]
    {
        if (!m_using_custom_traversal)
            m_sonifier->sonify();
        else
            m_sonifier->sonify_with_pixels(m_traversal_pixels);

        auto audio_data = m_sonifier->take_audio();
        if (audio_data.empty())
            return;

        const float sr = m_sonifier->sample_rate();

        // Amplitude (legacy) + gain
        const float total_gain = amp * ae.gain;
        if (total_gain != 1.0f)
            Effects::Gain(audio_data, total_gain);

        // Distortion before reverb/delay so it feeds into the wet signal
        if (ae.distortion_mix > 0.f)
            audio_data = Effects::Distortion(audio_data, ae.distortion_drive,
                                             ae.distortion_mix);

        if (ae.reverb_mix > 0.f)
            audio_data = Effects::Reverb(audio_data, sr, ae.reverb_room,
                                         ae.reverb_damping, ae.reverb_mix);

        if (ae.delay_mix > 0.f)
            audio_data = Effects::Delay(audio_data, sr, ae.delay_time,
                                        ae.delay_feedback, ae.delay_mix);

        if (ae.has_process_func)
            this->apply_audio_process_func(audio_data, sr);

        m_audio_engine->set_data(std::move(audio_data), sr);
    });

    return true;
}

void
MainWindow::collect_traversal_pixels() noexcept
{
    m_traversal_pixels.clear();
    m_using_custom_traversal = false;

    if (!m_L)
        return;

    lua_getfield(m_L, LUA_REGISTRYINDEX, "sonopix_traversal_func");
    if (lua_isnil(m_L, -1))
    {
        lua_pop(m_L, 1);
        return;
    }

    const auto &img = m_sonifier->raw_image();
    if (img.data.empty())
    {
        lua_pop(m_L, 1);
        return;
    }

    const int w     = img.width;
    const int h     = img.height;
    const int total = w * h;

    // Function sits at this absolute stack index throughout the loop.
    const int func_idx = lua_gettop(m_L);
    m_traversal_pixels.reserve(static_cast<std::size_t>(total));

    for (int i = 0; i < total; ++i)
    {
        lua_pushvalue(m_L, func_idx); // function copy
        lua_pushinteger(m_L, i);      // strip_index
        lua_pushinteger(m_L, total);  // total
        lua_pushinteger(m_L, w);      // width
        lua_pushinteger(m_L, h);      // height

        if (lua_pcall(m_L, 4, 2, 0) != LUA_OK)
        {
            fprintf(stderr, "traversal_func error at strip %d: %s\n", i,
                    lua_tostring(m_L, -1));
            lua_pop(m_L, 1); // pop error
            m_traversal_pixels.clear();
            lua_pop(m_L, 1); // pop function
            return;
        }

        const int x = static_cast<int>(lua_tointeger(m_L, -2));
        const int y = static_cast<int>(lua_tointeger(m_L, -1));
        lua_pop(m_L, 2);

        if (x >= 0 && x < w && y >= 0 && y < h)
            m_traversal_pixels.emplace_back(x, y);
    }

    lua_pop(m_L, 1); // pop function
    m_using_custom_traversal = !m_traversal_pixels.empty();
}

void
MainWindow::apply_audio_process_func(std::vector<float> &audio_data,
                                     float sample_rate) noexcept
{
    if (!m_L)
        return;

    lua_getfield(m_L, LUA_REGISTRYINDEX, "sonopix_audio_process_func");
    if (!lua_isfunction(m_L, -1))
    {
        lua_pop(m_L, 1);
        return;
    }

    // Build Lua table from audio_data
    lua_createtable(m_L, static_cast<int>(audio_data.size()), 0);
    for (std::size_t i = 0; i < audio_data.size(); ++i)
    {
        lua_pushnumber(m_L, audio_data[i]);
        lua_rawseti(m_L, -2, static_cast<int>(i + 1));
    }

    // Push sample_rate as second argument
    lua_pushnumber(m_L, sample_rate);

    if (lua_pcall(m_L, 2, 1, 0) != LUA_OK)
    {
        fprintf(stderr, "audio_effects.process_func error: %s\n",
                lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return;
    }

    if (lua_istable(m_L, -1))
    {
        const int n = static_cast<int>(audio_data.size());
        audio_data.resize(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i)
        {
            lua_rawgeti(m_L, -1, i + 1);
            audio_data[static_cast<std::size_t>(i)]
                = static_cast<float>(lua_tonumber(m_L, -1));
            lua_pop(m_L, 1);
        }
    }

    lua_pop(m_L, 1); // pop result
}

bool
MainWindow::save_audio(const std::string &filename) noexcept
{
    if (m_sonify_future.valid())
        m_sonify_future.wait();
    return m_audio_engine->save(filename);
}

float
MainWindow::rescale_recenter_image() noexcept
{
    const float win_h = static_cast<float>(m_win_size.y);
    auto ph = [&](float frac) { return frac * win_h; };

    const float reserved_bottom
        = (m_config.waveform.visible    ? ph(m_config.waveform.height)     : 0.f)
        + (m_config.oscilloscope.visible ? ph(m_config.oscilloscope.height) : 0.f)
        + (m_config.progress_bar.visible ? ph(m_config.progress_bar.height) : 0.f);
    const float available_h
        = std::max(0.f, static_cast<float>(m_win_size.y) - reserved_bottom);
    // Compute the axis-aligned bounding box of the rotated texture so the
    // entire image remains visible regardless of rotation angle.
    const float rad    = m_config.image_rotation * (3.14159265f / 180.f);
    const float cosA   = std::abs(std::cos(rad));
    const float sinA   = std::abs(std::sin(rad));
    const float bbox_w = m_tex_size.x * cosA + m_tex_size.y * sinA;
    const float bbox_h = m_tex_size.x * sinA + m_tex_size.y * cosA;
    const float scaleX = static_cast<float>(m_win_size.x) / bbox_w;
    const float scaleY = available_h / bbox_h;
    const float scale  = std::max(0.f, std::min(scaleX, scaleY));

    m_sprite.setScale({scale, scale});
    m_sprite.setOrigin({m_tex_size.x * 0.5f, m_tex_size.y * 0.5f});
    m_sprite.setRotation(sf::degrees(m_config.image_rotation));
    // Center the sprite in the available area; origin is at texture center
    const float cx = m_win_size.x * 0.5f;
    const float cy = available_h * 0.5f;
    m_sprite.setPosition({cx, cy});
    return scale;
};

void
MainWindow::init_waveform() noexcept
{
    const float win_h       = static_cast<float>(m_win_size.y);
    auto ph = [&](float frac) { return frac * win_h; };
    const float wave_height = ph(m_config.waveform.height);
    const float scale       = m_sprite.getScale().x;
    const float wave_width  = m_tex_size.x * scale;
    const float wave_x      = m_sprite.getPosition().x - wave_width * 0.5f;
    const float osc_height  = m_config.oscilloscope.visible ? ph(m_config.oscilloscope.height) : 0.f;
    const float bar_height  = m_config.progress_bar.visible ? ph(m_config.progress_bar.height) : 0.f;
    const float wave_y      = win_h - wave_height - osc_height - bar_height;

    m_waveform_bg = std::make_unique<sf::RectangleShape>();
    m_waveform_bg->setFillColor(sf::Color(0, 0, 0, 150));
    m_waveform_bg->setSize({wave_width, wave_height});
    m_waveform_bg->setPosition({wave_x, wave_y});

    m_waveform = std::make_unique<sf::VertexArray>(sf::PrimitiveType::Lines, 0);

    m_waveform_playhead
        = std::make_unique<sf::VertexArray>(sf::PrimitiveType::Lines, 2);
    (*m_waveform_playhead)[0].position = {wave_x, wave_y};
    (*m_waveform_playhead)[0].color    = sf::Color(255, 255, 255, 220);
    (*m_waveform_playhead)[1].position = {wave_x, wave_y + wave_height};
    (*m_waveform_playhead)[1].color    = sf::Color(255, 255, 255, 220);
}

void
MainWindow::build_waveform() noexcept
{
    if (!m_config.waveform.visible || !m_waveform)
        return;

    const auto &samples     = m_audio_engine->dataf();
    const std::size_t total = samples.size();
    if (total == 0)
        return;

    const float win_h       = static_cast<float>(m_win_size.y);
    auto ph = [&](float frac) { return frac * win_h; };
    const float wave_height = ph(m_config.waveform.height);
    const float scale       = m_sprite.getScale().x;
    const float wave_width  = m_tex_size.x * scale;
    const float wave_x      = m_sprite.getPosition().x - wave_width * 0.5f;
    const float osc_height  = m_config.oscilloscope.visible ? ph(m_config.oscilloscope.height) : 0.f;
    const float bar_height  = m_config.progress_bar.visible ? ph(m_config.progress_bar.height) : 0.f;
    const float wave_y      = win_h - wave_height - osc_height - bar_height;
    const float center_y = wave_y + wave_height * 0.5f;
    const float half     = wave_height * 0.5f;
    const int num_cols   = static_cast<int>(wave_width);

    m_waveform->resize(static_cast<std::size_t>(num_cols) * 2);

    for (int col = 0; col < num_cols; ++col)
    {
        const auto s0 = static_cast<std::size_t>(
            static_cast<float>(col) / wave_width * static_cast<float>(total));
        const auto s1 = std::min(
            static_cast<std::size_t>(static_cast<float>(col + 1) / wave_width
                                     * static_cast<float>(total)),
            total);
        const std::size_t end = s1 > s0 ? s1 : std::min(s0 + 1, total);

        float sum = 0.f;
        for (std::size_t s = s0; s < end; ++s)
            sum += samples[s] * samples[s];
        const float rms = std::sqrt(sum / static_cast<float>(end - s0));

        const float x                       = wave_x + static_cast<float>(col);
        (*m_waveform)[col * 2].position     = {x, center_y - rms * half};
        (*m_waveform)[col * 2].color        = m_config.waveform.color;
        (*m_waveform)[col * 2 + 1].position = {x, center_y + rms * half};
        (*m_waveform)[col * 2 + 1].color    = m_config.waveform.color;
    }

    if (m_waveform_playhead)
    {
        (*m_waveform_playhead)[0].position = {wave_x, wave_y};
        (*m_waveform_playhead)[1].position = {wave_x, wave_y + wave_height};
    }
}

void
MainWindow::update_waveform() noexcept
{
    if (!m_config.waveform.visible || !m_waveform_playhead)
        return;

    const std::size_t total = m_audio_engine->dataf().size();
    if (total == 0)
        return;

    const float wave_width = m_tex_size.x * m_sprite.getScale().x;
    const float wave_x     = m_sprite.getPosition().x - wave_width * 0.5f;
    const float progress
        = static_cast<float>(m_last_sample_index) / static_cast<float>(total);
    const float x                        = wave_x + progress * wave_width;
    (*m_waveform_playhead)[0].position.x = x;
    (*m_waveform_playhead)[1].position.x = x;
}

void
MainWindow::init_oscilloscope() noexcept
{
    const float win_h      = static_cast<float>(m_win_size.y);
    auto ph = [&](float frac) { return frac * win_h; };
    const float osc_height = ph(m_config.oscilloscope.height);
    const float osc_width  = static_cast<float>(m_win_size.x);
    const float bar_height = m_config.progress_bar.visible ? ph(m_config.progress_bar.height) : 0.f;
    const float osc_y      = win_h - osc_height - bar_height;

    m_oscilloscope_bg = std::make_unique<sf::RectangleShape>();
    m_oscilloscope_bg->setFillColor(sf::Color(0, 0, 0, 180));
    m_oscilloscope_bg->setSize({osc_width, osc_height});
    m_oscilloscope_bg->setPosition({0.f, osc_y});

    const int num_pts = static_cast<int>(osc_width);
    m_oscilloscope    = std::make_unique<sf::VertexArray>(
        sf::PrimitiveType::LineStrip, num_pts);
    for (int i = 0; i < num_pts; ++i)
    {
        (*m_oscilloscope)[i].position
            = {static_cast<float>(i), osc_y + osc_height * 0.5f};
        (*m_oscilloscope)[i].color = m_config.oscilloscope.color;
    }
}

void
MainWindow::update_oscilloscope() noexcept
{
    if (!m_config.oscilloscope.visible || !m_oscilloscope)
        return;

    const auto &samples     = m_audio_engine->dataf();
    const std::size_t total = samples.size();
    if (total == 0)
        return;

    const float win_h      = static_cast<float>(m_win_size.y);
    auto ph = [&](float frac) { return frac * win_h; };
    const float osc_height = ph(m_config.oscilloscope.height);
    const float osc_width  = static_cast<float>(m_win_size.x);
    const float bar_height = m_config.progress_bar.visible ? ph(m_config.progress_bar.height) : 0.f;
    const float osc_y      = win_h - osc_height - bar_height;
    const float center_y = osc_y + osc_height * 0.5f;
    const float half     = osc_height * 0.5f;

    const int num_pts     = static_cast<int>(osc_width);
    const int win_samples = m_config.oscilloscope.window_samples;
    const auto cur        = static_cast<std::ptrdiff_t>(m_last_sample_index);
    const auto start      = std::max<std::ptrdiff_t>(0, cur - win_samples / 2);

    m_oscilloscope->resize(num_pts);
    for (int i = 0; i < num_pts; ++i)
    {
        const auto idx = static_cast<std::size_t>(
            start + static_cast<std::ptrdiff_t>(i) * win_samples / num_pts);
        const float v = (idx < total) ? samples[idx] : 0.f;
        (*m_oscilloscope)[i].position
            = {static_cast<float>(i), center_y - v * half};
        (*m_oscilloscope)[i].color = m_config.oscilloscope.color;
    }
}

void
MainWindow::init_playback_bar() noexcept
{
    const float win_h      = static_cast<float>(m_win_size.y);
    const float bar_height = m_config.progress_bar.height * win_h;
    const float bar_width  = static_cast<float>(m_win_size.x);
    const float bar_y      = win_h - bar_height;

    m_playback_bar = std::make_unique<sf::RectangleShape>();
    m_playback_bar->setFillColor(sf::Color(0, 0, 0, 120));
    m_playback_bar->setSize({bar_width, bar_height});
    m_playback_bar->setPosition({0.f, bar_y});

    m_playback_fill = std::make_unique<sf::RectangleShape>();
    m_playback_fill->setFillColor(m_config.progress_bar.color);
    m_playback_fill->setSize({0.f, bar_height});
    m_playback_fill->setPosition({0.f, bar_y});
}

void
MainWindow::update_playback_bar() noexcept
{
    if (!m_config.progress_bar.visible || !m_playback_bar || !m_playback_fill)
        return;

    const std::size_t total = m_audio_engine->sound_buffer().getSampleCount();
    if (total == 0)
        return;

    const float progress
        = static_cast<float>(m_last_sample_index) / static_cast<float>(total);
    const float full_w = m_playback_bar->getSize().x;
    m_playback_fill->setSize(
        {std::max(0.f, progress * full_w), m_playback_fill->getSize().y});
}

void
MainWindow::init_cursor(float scale, sf::Vector2<float> position) noexcept
{
    // Sprite origin is at texture center; use the transform to map
    // texture-space points to screen space so cursor positions are correct even
    // when the image is rotated.
    (void)position; // unused — kept in signature for call-site compatibility

    if (m_using_custom_traversal)
    {
        auto rect = std::make_unique<sf::RectangleShape>();
        rect->setFillColor(m_config.cursor.color);
        rect->setSize({m_config.cursor.width, m_config.cursor.width});
        rect->setPosition(m_sprite.getTransform().transformPoint({0.f, 0.f}));
        m_cursor = std::move(rect);
        return;
    }

    switch (m_config.direction)
    {
        case sonify::Direction::LEFT_TO_RIGHT:
        case sonify::Direction::RIGHT_TO_LEFT:
        {
            auto rect = std::make_unique<sf::RectangleShape>();
            rect->setFillColor(m_config.cursor.color);
            rect->setSize({m_config.cursor.width, m_tex_size.y * scale});
            rect->setPosition(
                m_sprite.getTransform().transformPoint({0.f, 0.f}));
            rect->setRotation(sf::degrees(m_config.image_rotation));
            m_cursor = std::move(rect);
        }
        break;

        case sonify::Direction::TOP_TO_BOTTOM:
        case sonify::Direction::BOTTOM_TO_TOP:
        {
            auto rect = std::make_unique<sf::RectangleShape>();
            rect->setFillColor(m_config.cursor.color);
            rect->setSize({m_tex_size.x * scale, m_config.cursor.width});
            rect->setPosition(
                m_sprite.getTransform().transformPoint({0.f, 0.f}));
            rect->setRotation(sf::degrees(m_config.image_rotation));
            m_cursor = std::move(rect);
        }
        break;

        case sonify::Direction::CIRCLE_OUTWARDS:
        case sonify::Direction::CIRCLE_INWARDS:
        {
            auto circle = std::make_unique<sf::CircleShape>();
            circle->setFillColor(sf::Color::Transparent);
            circle->setOutlineColor(m_config.cursor.color);
            circle->setOutlineThickness(m_config.cursor.width);
            circle->setRadius(0.f);
            // Sprite position IS the image center (origin is at texture center)
            circle->setPosition(m_sprite.getPosition());
            m_cursor = std::move(circle);
        }
        break;

        case sonify::Direction::ROTATE_CW:
        case sonify::Direction::ROTATE_CCW:
        {
            const float cx_img = m_tex_size.x * 0.5f;
            const float cy_img = m_tex_size.y * 0.5f;
            const float max_r
                = std::sqrt(cx_img * cx_img + cy_img * cy_img) * scale;
            auto rect = std::make_unique<sf::RectangleShape>();
            rect->setFillColor(m_config.cursor.color);
            rect->setSize({m_config.cursor.width, max_r});
            rect->setOrigin(
                {m_config.cursor.width * 0.5f, max_r}); // pivot at image center
            // Sprite position IS the image center
            rect->setPosition(m_sprite.getPosition());
            m_cursor = std::move(rect);
        }
        break;

        default:
            break;
    }
}

void
MainWindow::init_image_shader() noexcept
{
    if (!sf::Shader::isAvailable())
    {
        std::cerr << "warning: shaders not supported; image effects disabled\n";
        return;
    }
    if (!m_image_shader.loadFromMemory(k_image_frag_shader,
                                       sf::Shader::Type::Fragment))
    {
        std::cerr
            << "warning: failed to compile image shader; effects disabled\n";
        return;
    }
    m_image_shader.setUniform("texture", sf::Shader::CurrentTexture);
    sync_shader();
    m_shader_active = true;
}

void
MainWindow::sync_shader() noexcept
{
    const auto &e         = m_config.image_effects;
    const sf::Vector2f ts = m_tex_size.x > 0 ? sf::Vector2f{1.f / m_tex_size.x,
                                                            1.f / m_tex_size.y}
                                             : sf::Vector2f{1.f, 1.f};
    m_image_shader.setUniform("u_texel_size", ts);
    m_image_shader.setUniform("u_grayscale", e.grayscale ? 1.f : 0.f);
    m_image_shader.setUniform("u_brightness", e.brightness);
    m_image_shader.setUniform("u_saturation", e.saturation);
    m_image_shader.setUniform("u_contrast", e.contrast);
    m_image_shader.setUniform("u_hue", e.hue);
    m_image_shader.setUniform("u_blur", e.blur);
    m_image_shader.setUniform("u_sharpen", e.sharpen);
    m_image_shader.setUniform("u_threshold", e.threshold);
    m_image_shader.setUniform("u_invert", e.invert ? 1 : 0);
}

// Render the image through the current shader into an offscreen texture, read
// the pixels back to CPU, and update the sonifier's raw image so that image
// effects are reflected in the next sonification.
void
MainWindow::snapshot_shaded_image() noexcept
{
    const int w = static_cast<int>(m_tex_size.x);
    const int h = static_cast<int>(m_tex_size.y);
    if (w <= 0 || h <= 0 || m_tex.getSize().x == 0)
        return;

    sf::RenderTexture rt;
    if (!rt.resize({static_cast<unsigned>(w), static_cast<unsigned>(h)}))
        return;

    rt.clear(sf::Color::Transparent);

    // Render at native 1:1 size so pixel indices match the sonifier's grid.
    sf::Sprite tmp(m_tex);
    if (m_shader_active)
    {
        sf::RenderStates states;
        states.shader = &m_image_shader;
        rt.draw(tmp, states);
    }
    else
        rt.draw(tmp);

    rt.display();

    const sf::Image shaded    = rt.getTexture().copyToImage();
    const std::uint8_t *data  = shaded.getPixelsPtr();
    if (!data)
        return;

    constexpr int channels = 4;
    auto img_data = sonify::normalize_u8_data(data, w * h * channels);
    m_sonifier->set_raw_image(w, h, channels, w * 4, std::move(img_data));
}

// Reset all script-controlled state to defaults before a hot-reload so that
// removing a line from the script is equivalent to reverting that setting.
void
MainWindow::reset_script_state() noexcept
{
    // Reset config structs to their default-constructed values.
    m_config.image_effects  = ImageEffectsOpts{};
    m_config.audio_effects  = AudioEffectsOpts{};
    m_config.cursor         = CursorOpts{};
    m_config.progress_bar   = ProgressBarOpts{};
    m_config.waveform       = WaveformOpts{};
    m_config.oscilloscope   = OscilloscopeOpts{};
    m_config.amplitude      = 1.0f;
    m_config.direction      = sonify::Direction::LEFT_TO_RIGHT;
    m_config.image_rotation = 0.f;
    m_config.loop           = false;
    m_config.fps_limit      = 60;

    // Propagate to subsystems.
    m_sonifier->set_sonify_func(sonify::sonify_functions::sine());
    m_sonifier->set_direction(sonify::Direction::LEFT_TO_RIGHT);
    m_sonifier->set_channel_count(1);
    m_audio_engine->set_channel_count(1);
    m_audio_engine->set_looping(false);
    m_audio_engine->set_volume(100.f);
    m_sprite.setRotation(sf::degrees(0.f));
    m_window.setFramerateLimit(m_config.fps_limit);

    if (m_shader_active)
        sync_shader();
}

void
MainWindow::set_effect(const char *name, float value) noexcept
{
    auto &e = m_config.image_effects;
    if (strcmp(name, "brightness") == 0)
        e.brightness = value;
    else if (strcmp(name, "saturation") == 0)
        e.saturation = value;
    else if (strcmp(name, "contrast") == 0)
        e.contrast = value;
    else if (strcmp(name, "hue") == 0)
        e.hue = value;
    else if (strcmp(name, "blur") == 0)
        e.blur = value;
    else if (strcmp(name, "sharpen") == 0)
        e.sharpen = value;
    else if (strcmp(name, "threshold") == 0)
        e.threshold = value;
    if (m_shader_active)
        sync_shader();
}

void
MainWindow::set_effect_invert(bool value) noexcept
{
    m_config.image_effects.invert = value;
    if (m_shader_active)
        sync_shader();
}

void
MainWindow::set_effect_grayscale(bool value) noexcept
{
    m_config.image_effects.grayscale = value;
    if (m_shader_active)
        sync_shader();
}

void
MainWindow::main_loop()
{
    create_window();
    init_image_shader();

    if (!m_output_file.empty())
    {
        if (m_sonifier->raw_image().data.empty())
        {
            std::cerr << "error: --output requires --input\n";
            return;
        }
        sonify(); // batch mode: sonify immediately, save when done, then close
    }

    while (m_window.isOpen())
    {
        handle_events();
        render();
        update();
    }
}

void
MainWindow::render() noexcept
{
    m_window.clear();

    if (m_shader_active)
    {
        sf::RenderStates states;
        states.shader = &m_image_shader;
        m_window.draw(m_sprite, states);
    }
    else
        m_window.draw(m_sprite);
    if (m_cursor)
        m_window.draw(*m_cursor);
    if (m_config.waveform.visible)
    {
        if (m_waveform_bg)
            m_window.draw(*m_waveform_bg);
        if (m_waveform && m_waveform->getVertexCount() > 0)
            m_window.draw(*m_waveform);
        if (m_waveform_playhead && !m_audio_engine->dataf().empty())
            m_window.draw(*m_waveform_playhead);
    }
    if (m_config.oscilloscope.visible)
    {
        if (m_oscilloscope_bg)
            m_window.draw(*m_oscilloscope_bg);
        if (m_oscilloscope)
            m_window.draw(*m_oscilloscope);
    }
    if (m_config.progress_bar.visible && m_playback_bar)
    {
        m_window.draw(*m_playback_bar);
        if (m_playback_fill)
            m_window.draw(*m_playback_fill);
    }

    m_window.display();
}

void
MainWindow::update() noexcept
{
    if (m_hot_reload_script
        && m_script_changed.exchange(false, std::memory_order_relaxed))
    {
        if (m_L)
        {
            clear_event_listeners();
            lua_close(m_L);
            m_L = nullptr;
        }
        reset_script_state();
        try
        {
            init_lua(m_script_file);
        }
        catch (const std::exception &e)
        {
            std::cerr << "hot-reload error: " << e.what() << '\n';
        }
    }

    if (m_sonify_future.valid()
        && m_sonify_future.wait_for(std::chrono::seconds(0))
               == std::future_status::ready)
    {
        m_sonify_future = {};
        m_window.setTitle(m_window_title);
        build_waveform();
        fire_event("sonify_complete");

        if (!m_output_file.empty())
        {
            save_audio(m_output_file);
            m_window.close();
        }
    }

    const bool now_playing = m_audio_engine->is_playing();
    if (m_was_playing && !now_playing)
        fire_event("playback_end");
    m_was_playing = now_playing;

    move_cursor();
    update_playback_bar();
    update_waveform();
    update_oscilloscope();
}

void
MainWindow::play() noexcept
{
    m_audio_engine->play();
}

void
MainWindow::pause() noexcept
{
    m_audio_engine->pause();
}

void
MainWindow::stop() noexcept
{
    m_audio_engine->stop();
}

void
MainWindow::fire_event(const std::string &name) noexcept
{
    if (!m_L)
        return;
    auto it = m_event_listeners.find(name);
    if (it == m_event_listeners.end())
        return;
    for (int ref : it->second)
    {
        lua_rawgeti(m_L, LUA_REGISTRYINDEX, ref);
        if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
        {
            std::cerr << "event \"" << name << "\" error: "
                      << lua_tostring(m_L, -1) << '\n';
            lua_pop(m_L, 1);
        }
    }
}

void
MainWindow::clear_event_listeners() noexcept
{
    if (!m_L)
        return;
    for (auto &[name, refs] : m_event_listeners)
        for (int ref : refs)
            luaL_unref(m_L, LUA_REGISTRYINDEX, ref);
    m_event_listeners.clear();
}

void
MainWindow::start_script_watcher(const std::string &path) noexcept
{
    stop_script_watcher();

    m_inotify_fd = inotify_init1(IN_NONBLOCK);
    if (m_inotify_fd < 0)
        return;

    if (inotify_add_watch(m_inotify_fd, path.c_str(), IN_CLOSE_WRITE) < 0)
    {
        close(m_inotify_fd);
        m_inotify_fd = -1;
        return;
    }

    m_watcher_stop = false;
    m_watcher_thread = std::thread([this]() {
        constexpr std::size_t BUF_LEN = sizeof(inotify_event) + NAME_MAX + 1;
        char buf[BUF_LEN];
        while (!m_watcher_stop.load(std::memory_order_relaxed))
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(m_inotify_fd, &fds);
            timeval tv{0, 100'000}; // 100 ms timeout so we can check stop flag
            if (select(m_inotify_fd + 1, &fds, nullptr, nullptr, &tv) > 0)
            {
                if (read(m_inotify_fd, buf, BUF_LEN) > 0)
                    m_script_changed.store(true, std::memory_order_relaxed);
            }
        }
    });
}

void
MainWindow::stop_script_watcher() noexcept
{
    m_watcher_stop = true;
    if (m_watcher_thread.joinable())
        m_watcher_thread.join();
    if (m_inotify_fd >= 0)
    {
        close(m_inotify_fd);
        m_inotify_fd = -1;
    }
}

void
MainWindow::toggle_pause() noexcept
{
    if (m_audio_engine->is_playing())
        pause();
    else
        play();
}

void
MainWindow::move_cursor() noexcept
{
    if (!m_audio_engine)
        return;

    if (!m_audio_engine->is_stopped())
        m_last_sample_index = m_audio_engine->sample_index();
    else if (m_last_sample_index == 0)
        return;

    const std::size_t sample_idx = m_last_sample_index;
    const int spu
        = std::max(1, static_cast<int>(m_sonifier->sample_rate()
                                       * m_sonifier->secs_per_unit()));
    const int strip
        = static_cast<int>(sample_idx / static_cast<std::size_t>(
              spu * m_sonifier->channel_count()));

    // Custom traversal: look up pixel from m_traversal_pixels
    if (m_using_custom_traversal)
    {
        if (strip < 0 || strip >= static_cast<int>(m_traversal_pixels.size()))
            return;
        const auto [px, py] = m_traversal_pixels[strip];
        auto *rect          = static_cast<sf::RectangleShape *>(m_cursor.get());
        rect->setSize({m_config.cursor.width, m_config.cursor.width});
        rect->setPosition(m_sprite.getTransform().transformPoint(
            {static_cast<float>(px), static_cast<float>(py)}));
        return;
    }

    switch (m_config.direction)
    {
        case sonify::Direction::LEFT_TO_RIGHT:
        case sonify::Direction::RIGHT_TO_LEFT:
        {
            const int w       = static_cast<int>(m_tex_size.x);
            const float scale = m_sprite.getScale().x;
            const float col
                = m_config.direction == sonify::Direction::RIGHT_TO_LEFT
                      ? static_cast<float>(w - strip)
                      : static_cast<float>(strip);
            auto *rect = static_cast<sf::RectangleShape *>(m_cursor.get());
            rect->setSize({m_config.cursor.width,
                           static_cast<float>(m_tex_size.y) * scale});
            rect->setPosition(
                m_sprite.getTransform().transformPoint({col, 0.f}));
            rect->setRotation(sf::degrees(m_config.image_rotation));
        }
        break;

        case sonify::Direction::TOP_TO_BOTTOM:
        case sonify::Direction::BOTTOM_TO_TOP:
        {
            const int h       = static_cast<int>(m_tex_size.y);
            const float scale = m_sprite.getScale().y;
            const float row
                = m_config.direction == sonify::Direction::BOTTOM_TO_TOP
                      ? static_cast<float>(h - strip)
                      : static_cast<float>(strip);
            auto *rect = static_cast<sf::RectangleShape *>(m_cursor.get());
            rect->setSize({static_cast<float>(m_tex_size.x) * scale,
                           m_config.cursor.width});
            rect->setPosition(
                m_sprite.getTransform().transformPoint({0.f, row}));
            rect->setRotation(sf::degrees(m_config.image_rotation));
        }
        break;

        case sonify::Direction::CIRCLE_OUTWARDS:
        case sonify::Direction::CIRCLE_INWARDS:
        {
            const float scale  = m_sprite.getScale().x;
            const int w        = static_cast<int>(m_tex_size.x);
            const int h        = static_cast<int>(m_tex_size.y);
            const float cx_img = (w - 1) * 0.5f;
            const float cy_img = (h - 1) * 0.5f;
            const int max_r
                = static_cast<int>(std::sqrt(cx_img * cx_img + cy_img * cy_img))
                  + 1;

            const int r
                = m_config.direction == sonify::Direction::CIRCLE_INWARDS
                      ? std::max(0, max_r - strip)
                      : std::min(strip, max_r);

            const float radius = r * scale;
            // Sprite position IS the image center (origin is at texture center)
            const sf::Vector2f center = m_sprite.getPosition();
            const float cx_disp       = center.x;
            const float cy_disp       = center.y;

            // Subtract half stroke width so the outline is centred on the
            // ring boundary rather than hanging fully outside it.
            const float inner_r
                = std::max(0.f, radius - m_config.cursor.width * 0.5f);

            // One point per ~1.5 display pixels of circumference, min 60,
            // so the circle stays smooth at all sizes.
            const auto points = static_cast<std::size_t>(
                std::max(60.f, 2.f * 3.14159265f * radius / 1.5f));

            auto *circle = static_cast<sf::CircleShape *>(m_cursor.get());
            circle->setPointCount(points);
            circle->setOutlineColor(m_config.cursor.color);
            circle->setOutlineThickness(m_config.cursor.width);
            circle->setRadius(inner_r);
            circle->setPosition({cx_disp - inner_r, cy_disp - inner_r});
        }
        break;

        case sonify::Direction::ROTATE_CW:
        case sonify::Direction::ROTATE_CCW:
        {
            const float scale  = m_sprite.getScale().x;
            const float cx_img = m_tex_size.x * 0.5f;
            const float cy_img = m_tex_size.y * 0.5f;
            const float max_r
                = std::sqrt(cx_img * cx_img + cy_img * cy_img) * scale;
            const int num_strips  = std::max(static_cast<int>(m_tex_size.x),
                                             static_cast<int>(m_tex_size.y));
            const float angle_deg = static_cast<float>(strip) * 360.0f
                                    / static_cast<float>(num_strips);

            auto *rect = static_cast<sf::RectangleShape *>(m_cursor.get());
            rect->setSize({m_config.cursor.width, max_r});
            rect->setOrigin({m_config.cursor.width * 0.5f, max_r});
            // Sprite position IS the image center
            rect->setPosition(m_sprite.getPosition());
            rect->setRotation(
                sf::degrees(m_config.direction == sonify::Direction::ROTATE_CW
                                ? angle_deg
                                : -angle_deg));
        }
        break;
    }
}

void
MainWindow::set_cursor_width(float w) noexcept
{
    m_config.cursor.width = w;
    if (!m_cursor)
        return;
    if (m_config.direction == sonify::Direction::CIRCLE_OUTWARDS
        || m_config.direction == sonify::Direction::CIRCLE_INWARDS)
    {
        static_cast<sf::CircleShape *>(m_cursor.get())->setOutlineThickness(w);
        return;
    }
    auto *rect              = static_cast<sf::RectangleShape *>(m_cursor.get());
    const sf::Vector2f size = rect->getSize();
    if (m_using_custom_traversal)
        rect->setSize({w, w});
    else if (m_config.direction == sonify::Direction::LEFT_TO_RIGHT
             || m_config.direction == sonify::Direction::RIGHT_TO_LEFT
             || m_config.direction == sonify::Direction::ROTATE_CW
             || m_config.direction == sonify::Direction::ROTATE_CCW)
        rect->setSize({w, size.y}); // w = thin dimension, keep length
    else
        rect->setSize({size.x, w});
}

std::string
MainWindow::cursor_color() const noexcept
{
    char buf[10];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X",
                  m_config.cursor.color.r, m_config.cursor.color.g,
                  m_config.cursor.color.b, m_config.cursor.color.a);
    return std::string(buf);
}

void
MainWindow::set_cursor_color(const std::string &color_str) noexcept
{
    std::string hex = color_str.substr(1);
    if (hex.length() == 6)
        hex += "FF";
    m_config.cursor.color = sf::Color(std::stoul(hex, nullptr, 16));
    if (!m_cursor)
        return;
    if (m_config.direction == sonify::Direction::CIRCLE_OUTWARDS
        || m_config.direction == sonify::Direction::CIRCLE_INWARDS)
        static_cast<sf::CircleShape *>(m_cursor.get())
            ->setOutlineColor(m_config.cursor.color);
    else
        m_cursor->setFillColor(m_config.cursor.color);
}
