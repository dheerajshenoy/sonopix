#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
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

/* Normalized image data representation */
struct RawImage
{
    int width    = 0;
    int height   = 0;
    int channels = 0;
    int stride   = 0;
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
    float min       = 20.0f;
    float max       = 2500.0f;
    FreqScale scale = FreqScale::LINEAR;
};

namespace sonify_functions
{

inline SonifyFunc
sine()
{
    float phase = 0.0f;

    return [phase](const SonifyContext &ctx) mutable -> float
    {
        const float b = std::clamp(ctx.brightness, 0.0f, 1.0f);

        float freq;
        switch (ctx.freq_scale)
        {
            case FreqScale::LOG:
                freq = ctx.fmin * std::pow(ctx.fmax / ctx.fmin, b);
                break;
            case FreqScale::EXPONENTIAL:
                freq = ctx.fmin * std::exp(b * std::log(ctx.fmax / ctx.fmin));
                break;
            default:
                freq = ctx.fmin + b * (ctx.fmax - ctx.fmin);
                break;
        }

        constexpr float two_pi = 6.28318530718f;
        phase += two_pi * freq / ctx.sample_rate;
        if (phase >= two_pi)
            phase -= two_pi;

        return b * std::sin(phase);
    };
}

} // namespace sonify_functions

class SonifyEngine
{

public:
    SonifyEngine() = default;

    inline void set_raw_image(int w, int h, int ch, int stride,
                              std::vector<float> &&data) noexcept
    {
        m_img = RawImage{w, h, ch, stride, std::move(data)};
    }

    const RawImage &raw_image() const noexcept { return m_img; }
    RawImage       &raw_image() noexcept       { return m_img; }

    inline void  set_sample_rate(float sr) noexcept { m_sample_rate = sr; }
    inline float sample_rate() const noexcept       { return m_sample_rate; }

    inline void  set_secs_per_unit(float spu) noexcept { m_secs_per_unit = spu; }
    inline float secs_per_unit() const noexcept        { return m_secs_per_unit; }

    inline void      set_direction(Direction dir) noexcept { m_direction = dir; }
    inline Direction direction() const noexcept            { return m_direction; }

    inline void set_freq_map(FreqMap f) noexcept              { m_freq_map = f; }
    inline FreqMap freq_map() const noexcept                  { return m_freq_map; }
    inline void set_freq_range(float fmin, float fmax) noexcept
    {
        m_freq_map.min = fmin;
        m_freq_map.max = fmax;
    }
    inline void set_freq_scale(FreqScale scale) noexcept { m_freq_map.scale = scale; }

    const std::vector<float> &const_audio() const noexcept { return m_audio_data; }
    std::vector<float>       &audio() noexcept             { return m_audio_data; }
    inline std::vector<float> take_audio() noexcept        { return std::move(m_audio_data); }

    inline void set_sonify_func(const SonifyFunc &func) noexcept { m_sonify_func = func; }
    const SonifyFunc &sonify_func() const noexcept               { return m_sonify_func; }

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
    }

    void sonify()
    {
        validate();

        switch (m_direction)
        {
            case Direction::LEFT_TO_RIGHT: sonify_left_to_right(); break;
            case Direction::RIGHT_TO_LEFT: sonify_right_to_left(); break;
            case Direction::TOP_TO_BOTTOM: sonify_top_to_bottom(); break;
            case Direction::BOTTOM_TO_TOP: sonify_bottom_to_top(); break;
            case Direction::CIRCLE_OUTWARDS: sonify_circle(true);  break;
            case Direction::CIRCLE_INWARDS:  sonify_circle(false); break;
            default: break;
        }
    }

private:
    float m_sample_rate   = 44100.0f;
    RawImage m_img;
    Direction m_direction = Direction::LEFT_TO_RIGHT;
    float m_secs_per_unit = 0.001f;
    FreqMap m_freq_map;
    std::vector<float> m_audio_data;
    SonifyFunc m_sonify_func = sonify_functions::sine();

    static float pixel_brightness(const float *px, int channels)
    {
        if (channels == 1)
            return px[0];
        if (channels >= 3)
            return 0.299f * px[0] + 0.587f * px[1] + 0.114f * px[2];
        throw std::runtime_error("Unsupported channel count");
    }

    float column_brightness(int x) const
    {
        float sum = 0.0f;
        for (int y = 0; y < m_img.height; ++y)
            sum += pixel_brightness(
                &m_img.data[y * m_img.stride + x * m_img.channels],
                m_img.channels);
        return sum / static_cast<float>(m_img.height);
    }

    float row_brightness(int y) const
    {
        float sum = 0.0f;
        for (int x = 0; x < m_img.width; ++x)
            sum += pixel_brightness(
                &m_img.data[y * m_img.stride + x * m_img.channels],
                m_img.channels);
        return sum / static_cast<float>(m_img.width);
    }

    void emit_strip(float brightness, int x, int y, int w, int h, int spu)
    {
        for (int s = 0; s < spu; ++s)
        {
            SonifyContext ctx{
                .sample_rate  = m_sample_rate,
                .brightness   = brightness,
                .x            = x,
                .y            = y,
                .width        = w,
                .height       = h,
                .sample_index = m_audio_data.size(),
                .frame_index  = static_cast<std::size_t>(s),
                .freq_scale   = m_freq_map.scale,
                .fmin         = m_freq_map.min,
                .fmax         = m_freq_map.max,
            };
            m_audio_data.push_back(m_sonify_func(ctx));
        }
    }

    void sonify_left_to_right()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(w) * spu);
        for (int x = 0; x < w; ++x)
            emit_strip(column_brightness(x), x, 0, w, h, spu);
    }

    void sonify_right_to_left()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(w) * spu);
        for (int x = w - 1; x >= 0; --x)
            emit_strip(column_brightness(x), x, 0, w, h, spu);
    }

    void sonify_top_to_bottom()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(h) * spu);
        for (int y = 0; y < h; ++y)
            emit_strip(row_brightness(y), 0, y, w, h, spu);
    }

    void sonify_bottom_to_top()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(h) * spu);
        for (int y = h - 1; y >= 0; --y)
            emit_strip(row_brightness(y), 0, y, w, h, spu);
    }

    // Shared implementation for CIRCLE_OUTWARDS (outwards=true) and
    // CIRCLE_INWARDS (outwards=false). Pixels are bucketed by their integer
    // distance from the image centre; each bucket becomes one audio strip.
    // ctx.x carries the ring radius so custom sonify functions can use it.
    void sonify_circle(bool outwards)
    {
        const int w = m_img.width;
        const int h = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));

        const float cx = (w - 1) * 0.5f;
        const float cy = (h - 1) * 0.5f;
        const int max_r = static_cast<int>(std::sqrt(cx * cx + cy * cy)) + 1;

        std::vector<float> ring_sum(max_r + 1, 0.0f);
        std::vector<int>   ring_count(max_r + 1, 0);

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float dx = x - cx;
                const float dy = y - cy;
                const int r = static_cast<int>(std::sqrt(dx * dx + dy * dy));
                ring_sum[r] += pixel_brightness(
                    &m_img.data[y * m_img.stride + x * m_img.channels],
                    m_img.channels);
                ++ring_count[r];
            }

        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(max_r + 1) * spu);

        for (int i = 0; i <= max_r; ++i)
        {
            const int r = outwards ? i : (max_r - i);
            const float brightness = ring_count[r] > 0
                ? ring_sum[r] / static_cast<float>(ring_count[r])
                : 0.0f;
            emit_strip(brightness, r, 0, w, h, spu);
        }
    }
};

} // namespace sonify
