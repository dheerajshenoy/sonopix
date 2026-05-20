#include "MainWindow.hpp"

#include "lua/init.cpp"
#include "utils.hpp"

#include <SFML/Window/ContextSettings.hpp>
#include <print>

MainWindow::MainWindow() : m_sprite(m_tex)
{
    m_context_settings.antiAliasingLevel = 8;
    m_context_settings.depthBits         = 24;
    m_sonifier     = std::make_unique<sonify::SonifyEngine>();
    m_audio_engine = std::make_unique<AudioEngine>();
}

MainWindow::~MainWindow() = default;

void
MainWindow::create_window() noexcept
{
    m_window = sf::RenderWindow(sf::VideoMode(m_window_size), m_window_title,
                                sf::Style::Default, sf::State::Windowed,
                                m_context_settings);
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
        m_verbose = true;
        assert(0 && "Verbose logging not implemented yet");
    }

    if (parser.is_used("script"))
    {
        try
        {
            std::string script_file = parser.get<std::string>("script");
            init_lua(script_file);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            exit(1);
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

        else
            throw std::runtime_error("Invalid direction: " + dir_str);

        m_direction = direction;
        m_sonifier->set_direction(direction);
    }

    if (parser.is_used("cursor-width"))
    {
        m_cursor_width = parser.get<float>("cursor-width");
    }
}

void
MainWindow::load_image(sf::Image &image, const std::string &filename)
{
    std::vector<std::uint8_t> data;

    std::string fixed_filename = filename;
    if (!fixed_filename.empty() && fixed_filename[0] == '~')
    {
        const char *home = std::getenv("HOME");
        if (home)
            fixed_filename.replace(0, 1, home);
    }

    if (!image.loadFromFile(fixed_filename))
    {
        throw std::runtime_error("Failed to load image from file: " + filename);
    }
}

void
MainWindow::open_file(const std::string &filename)
{
    sf::Image img;
    load_image(img, filename);
    m_tex = sf::Texture(img);
    m_sprite.setTexture(m_tex, true);

    m_win_size  = m_window.getSize();
    m_tex_size  = img.getSize();
    const int w = (int)m_tex_size.x;
    const int h = (int)m_tex_size.y;

    if (w <= 0 || h <= 0)
    {
        throw std::runtime_error("SFML: invalid image dimensions");
    }

    float scale = rescale_recenter_image();
    init_cursor(scale);

    const std::uint8_t *data = img.getPixelsPtr(); // RGBA8, size = w*h*4
    if (!data)
        throw std::runtime_error("SFML: getPixelsPtr() returned null");

    int channels  = 4;
    auto img_data = sonify::normalize_u8_data(
        data, w * h * channels); // normalized to [0 .. 1]

    m_sonifier->set_raw_image(w, h, channels, w * 4, std::move(img_data));
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

    m_sonify_future = std::async(std::launch::async, [this]
    {
        m_sonifier->sonify();
        auto audio_data = m_sonifier->take_audio();
        if (!audio_data.empty())
            m_audio_engine->set_data(std::move(audio_data),
                                     m_sonifier->sample_rate());
    });

    return true;
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
    const float scaleX
        = static_cast<float>(m_win_size.x - m_win_size.x * 0.25) / m_tex_size.x;
    const float scaleY = static_cast<float>(m_win_size.y) / m_tex_size.y;
    const float scale  = std::min(scaleX, scaleY);

    m_sprite.setScale({scale, scale});
    m_sprite.setPosition({(m_win_size.x - m_tex_size.x * scale) * 0.5f,
                          (m_win_size.y - m_tex_size.y * scale) * 0.5f});
    return scale;
};

void
MainWindow::init_cursor(float scale, sf::Vector2<float> position) noexcept
{
    switch (m_direction)
    {
        case sonify::Direction::LEFT_TO_RIGHT:
        case sonify::Direction::RIGHT_TO_LEFT:
        {
            auto rect = std::make_unique<sf::RectangleShape>();
            rect->setFillColor(m_cursor_color);
            rect->setSize({m_cursor_width, m_tex_size.y * scale});
            if (position == sf::Vector2<float>{})
                position = m_sprite.getPosition();
            rect->setPosition(position);
            m_cursor = std::move(rect);
        }
        break;

        case sonify::Direction::TOP_TO_BOTTOM:
        case sonify::Direction::BOTTOM_TO_TOP:
        {
            auto rect = std::make_unique<sf::RectangleShape>();
            rect->setFillColor(m_cursor_color);
            rect->setSize({m_tex_size.x * scale, m_cursor_width});
            if (position == sf::Vector2<float>{})
                position = m_sprite.getPosition();
            rect->setPosition(position);
            m_cursor = std::move(rect);
        }
        break;

        case sonify::Direction::CIRCLE_OUTWARDS:
        case sonify::Direction::CIRCLE_INWARDS:
        {
            auto circle = std::make_unique<sf::CircleShape>();
            circle->setFillColor(sf::Color::Transparent);
            circle->setOutlineColor(m_cursor_color);
            circle->setOutlineThickness(m_cursor_width);
            circle->setRadius(0.f);
            const sf::Vector2f spritePos = m_sprite.getPosition();
            circle->setPosition({spritePos.x + (m_tex_size.x * scale) * 0.5f,
                                 spritePos.y + (m_tex_size.y * scale) * 0.5f});
            m_cursor = std::move(circle);
        }
        break;
    }
}

void
MainWindow::main_loop()
{
    create_window();
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

    m_window.draw(m_sprite);
    if (m_cursor)
        m_window.draw(*m_cursor);

    m_window.display();
}

void
MainWindow::update() noexcept
{
    if (m_sonify_future.valid()
        && m_sonify_future.wait_for(std::chrono::seconds(0))
               == std::future_status::ready)
    {
        m_sonify_future = {};
        m_window.setTitle(m_window_title);

        if (!m_output_file.empty())
        {
            save_audio(m_output_file);
            m_window.close();
        }
    }

    move_cursor();
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

    switch (m_direction)
    {
        case sonify::Direction::LEFT_TO_RIGHT:
        case sonify::Direction::RIGHT_TO_LEFT:
        {
            const int spu
                = std::max(1, static_cast<int>(m_sonifier->sample_rate()
                                               * m_sonifier->secs_per_unit()));
            const int w = static_cast<int>(m_tex_size.x);
            const int col
                = static_cast<int>(sample_idx / static_cast<std::size_t>(spu));
            const float scale = m_sprite.getScale().x;
            const float x     = m_direction == sonify::Direction::RIGHT_TO_LEFT
                                    ? m_sprite.getPosition().x
                                          + static_cast<float>(w - col) * scale
                                    : m_sprite.getPosition().x
                                          + static_cast<float>(col) * scale;

            auto *rect = static_cast<sf::RectangleShape *>(m_cursor.get());
            rect->setSize(
                {m_cursor_width, static_cast<float>(m_tex_size.y) * scale});
            rect->setPosition({x, m_sprite.getPosition().y});
        }
        break;

        case sonify::Direction::TOP_TO_BOTTOM:
        case sonify::Direction::BOTTOM_TO_TOP:
        {
            const int spu
                = std::max(1, static_cast<int>(m_sonifier->sample_rate()
                                               * m_sonifier->secs_per_unit()));
            const int h = static_cast<int>(m_tex_size.y);
            const int row
                = static_cast<int>(sample_idx / static_cast<std::size_t>(spu));
            const float scale = m_sprite.getScale().y;
            const float y     = m_direction == sonify::Direction::BOTTOM_TO_TOP
                                    ? m_sprite.getPosition().y
                                          + static_cast<float>(h - row) * scale
                                    : m_sprite.getPosition().y
                                          + static_cast<float>(row) * scale;

            auto *rect = static_cast<sf::RectangleShape *>(m_cursor.get());
            rect->setSize(
                {static_cast<float>(m_tex_size.x) * scale, m_cursor_width});
            rect->setPosition({m_sprite.getPosition().x, y});
        }
        break;

        case sonify::Direction::CIRCLE_OUTWARDS:
        case sonify::Direction::CIRCLE_INWARDS:
        {
            const int spu
                = std::max(1, static_cast<int>(m_sonifier->sample_rate()
                                               * m_sonifier->secs_per_unit()));
            const float scale  = m_sprite.getScale().x;
            const int w        = static_cast<int>(m_tex_size.x);
            const int h        = static_cast<int>(m_tex_size.y);
            const float cx_img = (w - 1) * 0.5f;
            const float cy_img = (h - 1) * 0.5f;
            const int max_r
                = static_cast<int>(std::sqrt(cx_img * cx_img + cy_img * cy_img))
                  + 1;

            const int ring
                = static_cast<int>(sample_idx / static_cast<std::size_t>(spu));
            const int r = m_direction == sonify::Direction::CIRCLE_INWARDS
                              ? std::max(0, max_r - ring)
                              : std::min(ring, max_r);

            const float radius           = r * scale;
            const sf::Vector2f spritePos = m_sprite.getPosition();
            const float cx_disp          = spritePos.x + cx_img * scale;
            const float cy_disp          = spritePos.y + cy_img * scale;

            // Subtract half stroke width so the outline is centred on the
            // ring boundary rather than hanging fully outside it.
            const float inner_r = std::max(0.f, radius - m_cursor_width * 0.5f);

            // One point per ~1.5 display pixels of circumference, min 60,
            // so the circle stays smooth at all sizes.
            const auto points = static_cast<std::size_t>(
                std::max(60.f, 2.f * 3.14159265f * radius / 1.5f));

            auto *circle = static_cast<sf::CircleShape *>(m_cursor.get());
            circle->setPointCount(points);
            circle->setOutlineColor(m_cursor_color);
            circle->setOutlineThickness(m_cursor_width);
            circle->setRadius(inner_r);
            circle->setPosition({cx_disp - inner_r, cy_disp - inner_r});
        }
        break;
    }
}

void
MainWindow::set_cursor_width(float w) noexcept
{
    m_cursor_width = w;
    if (!m_cursor)
        return;
    if (m_direction == sonify::Direction::CIRCLE_OUTWARDS
        || m_direction == sonify::Direction::CIRCLE_INWARDS)
    {
        auto *circle = static_cast<sf::CircleShape *>(m_cursor.get());
        circle->setOutlineThickness(w);
    }
    else
    {
        auto *rect = static_cast<sf::RectangleShape *>(m_cursor.get());
        const sf::Vector2f size = rect->getSize();
        if (m_direction == sonify::Direction::LEFT_TO_RIGHT
            || m_direction == sonify::Direction::RIGHT_TO_LEFT)
            rect->setSize({w, size.y});
        else
            rect->setSize({size.x, w});
    }
}

std::string
MainWindow::cursor_color() const noexcept
{
    char buf[10];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", m_cursor_color.r,
                  m_cursor_color.g, m_cursor_color.b, m_cursor_color.a);
    return std::string(buf);
}

void
MainWindow::set_cursor_color(const std::string &color_str) noexcept
{
    std::string hex = color_str.substr(1);
    if (hex.length() == 6)
        hex += "FF";
    m_cursor_color = sf::Color(std::stoul(hex, nullptr, 16));
    if (!m_cursor)
        return;
    if (m_direction == sonify::Direction::CIRCLE_OUTWARDS
        || m_direction == sonify::Direction::CIRCLE_INWARDS)
        static_cast<sf::CircleShape *>(m_cursor.get())
            ->setOutlineColor(m_cursor_color);
    else
        m_cursor->setFillColor(m_cursor_color);
}
