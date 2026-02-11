#pragma once

#include <cmath>
#include <functional>
#include <print>
#include <stdexcept>
#include <string>
#include <vector>

namespace sonify
{

enum class FreqScale
{
    LINEAR = 0,
    LOG,
    EXPONENTIAL
};

/* Context passed to the sonify functions */
struct SonifyContext
{
    float sample_rate;
    float brightness;

    // Traversal info (so user can do position-dependent effects)
    int x;
    int y;
    int width;
    int height;

    // Timing info for the generated audio
    std::size_t sample_index;
    std::size_t frame_index;

    // Frequency mapping parameters
    sonify::FreqScale freq_scale;
    float fmin;
    float fmax;
};

using SonifyFunc = std::function<float(const SonifyContext &)>;

/* Helper function to normalize uint8_t data to float within range [0 .. 1] */
inline std::vector<float>
normalize_u8_data(const std::uint8_t *data, std::size_t size)
{
    std::vector<float> norm_data;
    norm_data.reserve(size);

    for (std::size_t i = 0; i < size; i++)
        norm_data.push_back(static_cast<float>(data[i]) / 255.0f);

    return norm_data;
}

enum class LogLevel
{
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR
};

inline void
log(std::string msg, LogLevel level = LogLevel::INFO) noexcept
{
    std::string prefix;
    switch (level)
    {
        case LogLevel::DEBUG:
            prefix = "DEBUG";
            break;

        case LogLevel::INFO:
            prefix = "INFO";
            break;

        case LogLevel::WARNING:
            prefix = "WARNING";
            break;

        case LogLevel::ERROR:
            prefix = "ERROR";
            break;

        default:
            prefix = "UNKNOWN";
            break;
    }
    std::println("{}: {}", prefix, msg);
}

/* Normalized image data representation */
struct RawImage
{
    int width{0};
    int height{0};
    int channels{0};
    int stride{0};
    std::vector<float> data;
};

struct ImageLoader
{
    virtual ~ImageLoader()                      = default;
    virtual RawImage load(const char *filename) = 0;
};

enum class Direction
{
    LEFT_TO_RIGHT = 0,
    RIGHT_TO_LEFT,
    TOP_TO_BOTTOM,
    BOTTOM_TO_TOP,
    CIRCLE_OUTWARDS,
    CIRCLE_INWARDS
};

struct FreqMap
{
    float min{0.0f};
    float max{2500.0f};
    FreqScale scale{FreqScale::LINEAR};
};

namespace sonify_functions
{

inline SonifyFunc
sine_frequency()
{
    float phase = 0.0f;

    return [phase](const SonifyContext &ctx) mutable -> float
    {
        float b = std::clamp(ctx.brightness, 0.0f, 1.0f);

        float freq = ctx.fmin + b * (ctx.fmax - ctx.fmin);

        const float two_pi = 6.28318530718f;
        phase += two_pi * freq / ctx.sample_rate;

        if (phase >= two_pi)
            phase -= two_pi;

        return std::sin(phase);
    };
}

inline SonifyFunc
sine()
{
    float phase = 0.0f;

    return [phase](const SonifyContext &ctx) mutable -> float
    {
        float b = std::clamp(ctx.brightness, 0.0f, 1.0f);

        float freq = ctx.fmin + b * (ctx.fmax - ctx.fmin);

        // apply frequency scaling
        switch (ctx.freq_scale)
        {
            case FreqScale::LINEAR:
                break;

            case FreqScale::LOG:
                freq = ctx.fmin * std::pow(ctx.fmax / ctx.fmin, b);
                break;

            case FreqScale::EXPONENTIAL:
                freq = ctx.fmin * std::exp(b * std::log(ctx.fmax / ctx.fmin));
                break;

            default:
                break;
        }

        const float two_pi = 6.28318530718f;

        phase += two_pi * freq / ctx.sample_rate;

        if (phase >= two_pi)
            phase -= two_pi;

        return b * std::sin(phase);
    };
}

} // namespace sonify_functions

class Sonify
{

public:
    Sonify() = default;

    inline void set_raw_image(int w, int h, int ch, int stride,
                              std::vector<float> &&data) noexcept
    {
        m_img = (RawImage){w, h, ch, stride, std::move(data)};
    }

    const RawImage &raw_image() const noexcept
    {
        return m_img;
    }

    RawImage &raw_image() noexcept
    {
        return m_img;
    }

    inline void set_sample_rate(float sample_rate) noexcept
    {
        m_sample_rate = sample_rate;
    }

    inline float sample_rate() const noexcept
    {
        return m_sample_rate;
    }

    inline void set_secs_per_unit(float spu) noexcept
    {
        m_secs_per_unit = spu;
    }

    inline float secs_per_unit() const noexcept
    {
        return m_secs_per_unit;
    }

    inline void set_direction(Direction dir) noexcept
    {
        m_direction = dir;
    }

    inline Direction direction() const noexcept
    {
        return m_direction;
    }

    inline void set_freq_map(FreqMap f) noexcept
    {
        m_freq_map = f;
    }

    inline void set_freq_range(float fmin, float fmax) noexcept
    {
        m_freq_map.min = fmin;
        m_freq_map.max = fmax;
    }

    inline void set_freq_scale(FreqScale scale) noexcept
    {
        m_freq_map.scale = scale;
    }

    inline FreqMap freq_map() const noexcept
    {
        return m_freq_map;
    }

    const std::vector<float> &const_audio() const noexcept
    {
        return m_audio_data;
    }

    std::vector<float> &audio() noexcept
    {
        return m_audio_data;
    }

    inline std::vector<float> take_audio() noexcept
    {
        return std::move(m_audio_data);
    }

    inline void set_sonify_func(const SonifyFunc &func) noexcept
    {
        m_sonify_func = func;
    }

    const SonifyFunc &sonify_func() const noexcept
    {
        return m_sonify_func;
    }

    void validate() const
    {
        if (!m_sonify_func)
            throw std::runtime_error("sonify: `sonify_func' not set");

        if (m_img.data.empty())
            throw std::runtime_error("sonify: raw_image data is empty");

        if (m_sample_rate <= 0.0f)
            throw std::runtime_error("sonify: invalid `sample_rate'");

        if (m_secs_per_unit <= 0.0f)
            throw std::runtime_error("sonify: invalid `seconds_per_unit'");

        // if (m_freq_map.max >= 0.49f * m_sample_rate)
        //     throw std::runtime_error(
        //         "sonify: freq_map.max must be < `sample_rate/2'");
    }

    void sonify()
    {
        validate();

        switch (m_direction)
        {

            case Direction::LEFT_TO_RIGHT:
                sonify_left_to_right();
                break;

            case Direction::RIGHT_TO_LEFT:
                sonify_right_to_left();
                break;

            case Direction::TOP_TO_BOTTOM:
                sonify_top_to_bottom();
                break;

            case Direction::BOTTOM_TO_TOP:
                sonify_bottom_to_top();
                break;

            case Direction::CIRCLE_OUTWARDS:
            case Direction::CIRCLE_INWARDS:
                throw std::runtime_error("Not yet implemented!");

            default:
                break;
        }
    }

    void sonify_left_to_right()
    {
        m_audio_data.clear();

        const int w      = m_img.width;
        const int h      = m_img.height;
        const int stride = m_img.stride;
        const int ch     = m_img.channels;

        const int samples_per_column
            = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));

        m_audio_data.reserve(static_cast<std::size_t>(w)
                             * static_cast<std::size_t>(samples_per_column));

        for (int x = 0; x < w; ++x)
        {
            float column_sum = 0.0f;

            for (int y = 0; y < h; ++y)
            {

                const int idx = y * stride + x * ch;

                float v = 0.0f;
                if (ch == 1)
                {
                    v = m_img.data[idx];
                }
                else if (ch == 3 || ch == 4)
                {
                    // assume RGB(A): luminance, ignore alpha
                    const float r = m_img.data[idx + 0];
                    const float g = m_img.data[idx + 1];
                    const float b = m_img.data[idx + 2];
                    v             = 0.299f * r + 0.587f * g + 0.114f * b;
                }
                else
                {
                    throw std::runtime_error(
                        "Number of channels is not supported");
                }

                column_sum += v;
            }
            const float avg = column_sum / static_cast<float>(h);

            // 2) generate audio for this column
            for (int s = 0; s < samples_per_column; ++s)
            {
                SonifyContext ctx{.sample_rate  = m_sample_rate,
                                  .brightness   = avg,
                                  .x            = x,
                                  .width        = w,
                                  .height       = h,
                                  .sample_index = m_audio_data.size(),
                                  .frame_index  = static_cast<std::size_t>(s),
                                  .fmin         = m_freq_map.min,
                                  .fmax         = m_freq_map.max};

                float val = m_sonify_func(ctx);
                m_audio_data.push_back(val);
            }
        }
    }

    void sonify_right_to_left()
    {
        m_audio_data.clear();

        const int w      = m_img.width;
        const int h      = m_img.height;
        const int stride = m_img.stride;
        const int ch     = m_img.channels;

        const int samples_per_column
            = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));

        m_audio_data.reserve(static_cast<std::size_t>(w)
                             * static_cast<std::size_t>(samples_per_column));

        for (int x = w - 1; x >= 0; --x)
        {
            float column_sum = 0.0f;

            for (int y = 0; y < h; ++y)
            {

                const int idx = y * stride + x * ch;

                float v = 0.0f;
                if (ch == 1)
                {
                    v = m_img.data[idx];
                }
                else if (ch == 3 || ch == 4)
                {
                    // assume RGB(A): luminance, ignore alpha
                    const float r = m_img.data[idx + 0];
                    const float g = m_img.data[idx + 1];
                    const float b = m_img.data[idx + 2];
                    v             = 0.299f * r + 0.587f * g + 0.114f * b;
                }
                else
                {
                    throw std::runtime_error(
                        "Number of channels is not supported");
                }

                column_sum += v;
            }
            const float avg = column_sum / static_cast<float>(h);

            // 2) generate audio for this column
            for (int s = 0; s < samples_per_column; ++s)
            {
                SonifyContext ctx{.sample_rate  = m_sample_rate,
                                  .brightness   = avg,
                                  .x            = x,
                                  .width        = w,
                                  .height       = h,
                                  .sample_index = m_audio_data.size(),
                                  .frame_index  = static_cast<std::size_t>(s),
                                  .fmin         = m_freq_map.min,
                                  .fmax         = m_freq_map.max};

                float val = m_sonify_func(ctx);
                m_audio_data.push_back(val);
            }
        }
    }

    void sonify_top_to_bottom()
    {
        m_audio_data.clear();

        const int w      = m_img.width;
        const int h      = m_img.height;
        const int stride = m_img.stride;
        const int ch     = m_img.channels;

        const int samples_per_column
            = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));

        m_audio_data.reserve(static_cast<std::size_t>(h)
                             * static_cast<std::size_t>(samples_per_column));

        for (int y = 0; y < h; ++y)
        {
            float row_sum = 0.0f;

            for (int x = 0; x < w; ++x)
            {

                const int idx = y * stride + x * ch;

                float v = 0.0f;
                if (ch == 1)
                {
                    v = m_img.data[idx];
                }
                else if (ch == 3 || ch == 4)
                {
                    // assume RGB(A): luminance, ignore alpha
                    const float r = m_img.data[idx + 0];
                    const float g = m_img.data[idx + 1];
                    const float b = m_img.data[idx + 2];
                    v             = 0.299f * r + 0.587f * g + 0.114f * b;
                }
                else
                {
                    throw std::runtime_error(
                        "Number of channels is not supported");
                }

                row_sum += v;
            }

            const float avg = row_sum / static_cast<float>(w);

            // 2) generate audio for this row
            for (int s = 0; s < samples_per_column; ++s)
            {
                SonifyContext ctx{.sample_rate  = m_sample_rate,
                                  .brightness   = avg,
                                  .y            = y,
                                  .width        = w,
                                  .height       = h,
                                  .sample_index = m_audio_data.size(),
                                  .frame_index  = static_cast<std::size_t>(s),
                                  .fmin         = m_freq_map.min,
                                  .fmax         = m_freq_map.max};

                float val = m_sonify_func(ctx);
                m_audio_data.push_back(val);
            }
        }
    }

    void sonify_bottom_to_top()
    {

        m_audio_data.clear();

        const int w      = m_img.width;
        const int h      = m_img.height;
        const int stride = m_img.stride;
        const int ch     = m_img.channels;

        const int samples_per_column
            = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));

        m_audio_data.reserve(static_cast<std::size_t>(h)
                             * static_cast<std::size_t>(samples_per_column));

        for (int y = h - 1; y >= 0; --y)
        {
            float row_sum = 0.0f;

            for (int x = 0; x < w; ++x)
            {

                const int idx = y * stride + x * ch;

                float v = 0.0f;
                if (ch == 1)
                {
                    v = m_img.data[idx];
                }
                else if (ch == 3 || ch == 4)
                {
                    // assume RGB(A): luminance, ignore alpha
                    const float r = m_img.data[idx + 0];
                    const float g = m_img.data[idx + 1];
                    const float b = m_img.data[idx + 2];
                    v             = 0.299f * r + 0.587f * g + 0.114f * b;
                }
                else
                {
                    throw std::runtime_error(
                        "Number of channels is not supported");
                }

                row_sum += v;
            }

            const float avg = row_sum / static_cast<float>(w);

            // 2) generate audio for this row
            for (int s = 0; s < samples_per_column; ++s)
            {
                SonifyContext ctx{.sample_rate  = m_sample_rate,
                                  .brightness   = avg,
                                  .y            = y,
                                  .width        = w,
                                  .height       = h,
                                  .sample_index = m_audio_data.size(),
                                  .frame_index  = static_cast<std::size_t>(s),
                                  .fmin         = m_freq_map.min,
                                  .fmax         = m_freq_map.max};

                float val = m_sonify_func(ctx);
                m_audio_data.push_back(val);
            }
        }
    }

private:
    float m_sample_rate{44100.0f};
    RawImage m_img;
    Direction m_direction{Direction::LEFT_TO_RIGHT};
    float m_secs_per_unit{0.001f};
    FreqMap m_freq_map;
    std::vector<float> m_audio_data{};
    SonifyFunc m_sonify_func{sonify_functions::sine()};
};
}; // namespace sonify
