#include "MainWindow.hpp"

#include "sonify.hpp"

#include <print>

MainWindow::MainWindow() : m_sprite(m_tex)
{
    m_window = sf::RenderWindow(sf::VideoMode(m_window_size), m_window_title);
    m_audio_engine = new AudioEngine(m_sonifier.sample_rate());
}

void
MainWindow::read_args(const argparse::ArgumentParser &parser)
{
    if (parser.is_used("version"))
    {
        std::print("%s - version %s", APP_NAME, APP_VERSION);
        return;
    }

    if (parser.is_used("verbose"))
    {
        m_verbose = true;
        assert(0 && "Verbose logging not implemented yet");
    }

    if (parser.is_used("secs-per-unit"))
    {
        m_sonifier.set_secs_per_unit(parser.get<float>("secs-per-unit"));
    }

    if (parser.is_used("sample-rate"))
    {
        m_sonifier.set_sample_rate(parser.get<float>("sample-rate"));
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

        m_sonifier.set_freq_scale(scale);
    }

    if (parser.is_used("frequency"))
    {
        auto [fmin, fmax] = parser.get<std::pair<float, float>>("frequency");
        m_sonifier.set_freq_range(fmin, fmax);
    }

    if (parser.is_used("input"))
    {
        open_file(parser.get<std::string>("input"));
    }

    if (parser.is_used("output"))
    {
        assert(0 && "Output file argument is not implemented yet");
        std::string output_file = parser.get<std::string>("output");
        std::print("Output file argument provided: {}\n", output_file);
        std::print("Not implemented yet, but would save sonified audio to this "
                   "file instead of playing it.\n");
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
        m_sonifier.set_direction(direction);
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
    try
    {
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Failed to load image: ")
                                 + e.what());
    }

    // if (!image.loadFromMemory(data.data(), data.size()))
    // {
    //     throw std::runtime_error("Failed to load image from memory");
    // }
    if (!image.loadFromFile(filename))
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

    rescale_recenter_image();

    const std::uint8_t *data = img.getPixelsPtr(); // RGBA8, size = w*h*4
    if (!data)
        throw std::runtime_error("SFML: getPixelsPtr() returned null");

    int channels  = 4;
    auto img_data = sonify::normalize_u8_data(
        data, w * h * channels); // normalized to [0 .. 1]

    m_sonifier.set_raw_image(w, h, channels, w * 4, std::move(img_data));
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
            sonify();
            break;
        default:
            break;
    }
}

void
MainWindow::sonify()
{
    m_sonifier.sonify();

    auto audio_data = m_sonifier.const_audio();
    if (audio_data.empty())
        throw std::runtime_error("Audio data is empty!");

    m_audio_engine->set_data(std::move(audio_data));
    sonify::log("Sonification complete");
}

void
MainWindow::rescale_recenter_image() noexcept
{
    const float scaleX
        = static_cast<float>(m_win_size.x - m_win_size.x * 0.25) / m_tex_size.x;
    const float scaleY = static_cast<float>(m_win_size.y) / m_tex_size.y;
    const float scale  = std::min(scaleX, scaleY);

    m_sprite.setScale({scale, scale});
    m_sprite.setPosition({(m_win_size.x - m_tex_size.x * scale) * 0.5f,
                          (m_win_size.y - m_tex_size.y * scale) * 0.5f});

    init_cursor(scale);
};

void
MainWindow::init_cursor(float scale) noexcept
{
    m_cursor_rect.setFillColor(sf::Color(255, 0, 0, 128));

    switch (m_direction)
    {
        case sonify::Direction::LEFT_TO_RIGHT:
        case sonify::Direction::RIGHT_TO_LEFT:
            m_cursor_rect.setSize({m_cursor_width, m_tex_size.y * scale});
            m_cursor_rect.setPosition(
                {m_sprite.getPosition().x, m_sprite.getPosition().y});
            break;

        case sonify::Direction::TOP_TO_BOTTOM:
        case sonify::Direction::BOTTOM_TO_TOP:
            m_cursor_rect.setSize({m_tex_size.x * scale, m_cursor_width});
            m_cursor_rect.setPosition(
                {m_sprite.getPosition().x, m_sprite.getPosition().y});
            break;

        case sonify::Direction::CIRCLE_OUTWARDS:
        case sonify::Direction::CIRCLE_INWARDS:
            break;
    }
}

void
MainWindow::main_loop()
{
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
    m_window.draw(m_cursor_rect);

    m_window.display();
}

void
MainWindow::update() noexcept
{
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
    m_paused = !m_paused;
    if (m_paused)
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
    if (!m_audio_engine || !m_audio_engine->is_playing())
        return;

    const std::size_t sample_idx = m_audio_engine->sample_index();

    switch (m_direction)
    {
        case sonify::Direction::LEFT_TO_RIGHT:
        case sonify::Direction::RIGHT_TO_LEFT:
        {
            const int samples_per_column
                = std::max(1, static_cast<int>(m_sonifier.sample_rate()
                                               * m_sonifier.secs_per_unit()));

            const int w = static_cast<int>(m_tex_size.x);

            const int col = static_cast<int>(
                sample_idx / static_cast<std::size_t>(samples_per_column));

            // Convert column -> pixel x in DISPLAY space (taking sprite scale
            // into account)
            const float scale = m_sprite.getScale().x;

            float x;
            if (m_direction == sonify::Direction::RIGHT_TO_LEFT)
            {
                x = m_sprite.getPosition().x
                    + static_cast<float>(w - col) * scale;
            }
            else if (m_direction == sonify::Direction::LEFT_TO_RIGHT)
                x = m_sprite.getPosition().x + static_cast<float>(col) * scale;

            const float y = m_sprite.getPosition().y;

            m_cursor_rect.setSize(
                {m_cursor_width, static_cast<float>(m_tex_size.y) * scale});
            m_cursor_rect.setPosition({x, y});
        }
        break;

        case sonify::Direction::TOP_TO_BOTTOM:
        case sonify::Direction::BOTTOM_TO_TOP:
        {
            const int samples_per_row
                = std::max(1, static_cast<int>(m_sonifier.sample_rate()
                                               * m_sonifier.secs_per_unit()));

            const int h = static_cast<int>(m_tex_size.y);

            const int row = static_cast<int>(
                sample_idx / static_cast<std::size_t>(samples_per_row));

            // Convert row -> pixel y in DISPLAY space (taking sprite scale
            // into account)
            const float scale = m_sprite.getScale().y;

            float y;
            if (m_direction == sonify::Direction::BOTTOM_TO_TOP)
            {
                y = m_sprite.getPosition().y
                    + static_cast<float>(h - row) * scale;
            }
            else if (m_direction == sonify::Direction::TOP_TO_BOTTOM)
                y = m_sprite.getPosition().y + static_cast<float>(row) * scale;

            const float x = m_sprite.getPosition().x;

            m_cursor_rect.setSize(
                {static_cast<float>(m_tex_size.x) * scale, m_cursor_width});
            m_cursor_rect.setPosition({x, y});
        }
        break;

        case sonify::Direction::CIRCLE_OUTWARDS:
        case sonify::Direction::CIRCLE_INWARDS:
            break;
    }
}
