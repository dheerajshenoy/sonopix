#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
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
    float brightness;   // luminance  [0, 1]
    float r, g, b;      // avg channel values [0, 1]
    float h, s, v;      // HSV — h in [0, 360], s/v in [0, 1]

    // Traversal info (so user can do position-dependent effects)
    int x;
    int y;
    int width;
    int height;

    // Strip info (playback order, independent of scan direction)
    int strip_index;
    int strip_count;

    // Timing info for the generated audio
    float t;         // time in seconds since start of audio
    int   n_samples; // frames to generate (samples per channel); return n_samples * channel_count interleaved
    int   channel_count; // 1 = mono, 2 = stereo

    // Frequency mapping parameters
    sonify::FreqScale freq_scale;
    float fmin;
    float fmax;
};

using SonifyFunc = std::function<void(const SonifyContext &, std::vector<float> &)>;

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
    CIRCLE_INWARDS,
    ROTATE_CW,  // radar sweep clockwise from 12 o'clock
    ROTATE_CCW, // radar sweep counter-clockwise from 12 o'clock
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

    return [phase](const SonifyContext &ctx, std::vector<float> &out) mutable
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
        for (int i = 0; i < ctx.n_samples; ++i)
        {
            phase += two_pi * freq / ctx.sample_rate;
            if (phase >= two_pi)
                phase -= two_pi;
            const float s = b * std::sin(phase);
            for (int ch = 0; ch < ctx.channel_count; ++ch)
                out.push_back(s);
        }
    };
}

} // namespace sonify_functions

struct ROI
{
    int x = 0, y = 0, w = 0, h = 0;
    bool active = false;
};

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

    // Custom pixel-order traversal: each pair (x, y) becomes one strip whose
    // brightness is the single pixel at that coordinate.
    void sonify_with_pixels(const std::vector<std::pair<int, int>> &pixels)
    {
        validate();
        const int w     = m_img.width;
        const int h     = m_img.height;
        const int spu   = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        const int total = static_cast<int>(pixels.size());
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(total) * spu * m_channel_count);
        for (int i = 0; i < total; ++i)
        {
            const auto [x, y] = pixels[i];
            const float *px = &m_img.data[y * m_img.stride + x * m_img.channels];
            const float r = px[0];
            const float g = (m_img.channels >= 3) ? px[1] : px[0];
            const float b = (m_img.channels >= 3) ? px[2] : px[0];
            emit_strip(make_strip_data(r, g, b), x, y, w, h, spu, i, total);
        }
    }

    inline void set_sonify_func(const SonifyFunc &func) noexcept { m_sonify_func = func; }
    const SonifyFunc &sonify_func() const noexcept               { return m_sonify_func; }

    inline void set_channel_count(int ch) noexcept { m_channel_count = std::max(1, ch); }
    inline int  channel_count() const noexcept     { return m_channel_count; }

    void set_roi(int x, int y, int w, int h) noexcept
    {
        m_roi = ROI{
            std::clamp(x, 0, m_img.width),
            std::clamp(y, 0, m_img.height),
            std::clamp(w, 1, m_img.width),
            std::clamp(h, 1, m_img.height),
            true
        };
        m_roi.w = std::min(m_roi.w, m_img.width  - m_roi.x);
        m_roi.h = std::min(m_roi.h, m_img.height - m_roi.y);
    }
    void        clear_roi() noexcept          { m_roi.active = false; }
    const ROI  &roi() const noexcept          { return m_roi; }
    bool        has_roi() const noexcept      { return m_roi.active; }

    float pixel_brightness_at(int x, int y) const noexcept
    {
        if (m_img.data.empty()
            || x < 0 || x >= m_img.width
            || y < 0 || y >= m_img.height)
            return 0.0f;
        return pixel_brightness(
            &m_img.data[y * m_img.stride + x * m_img.channels],
            m_img.channels);
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
    }

    void sonify()
    {
        validate();

        switch (m_direction)
        {
            case Direction::LEFT_TO_RIGHT:   sonify_left_to_right(); break;
            case Direction::RIGHT_TO_LEFT:   sonify_right_to_left(); break;
            case Direction::TOP_TO_BOTTOM:   sonify_top_to_bottom(); break;
            case Direction::BOTTOM_TO_TOP:   sonify_bottom_to_top(); break;
            case Direction::CIRCLE_OUTWARDS: sonify_circle(true);    break;
            case Direction::CIRCLE_INWARDS:  sonify_circle(false);   break;
            case Direction::ROTATE_CW:       sonify_rotate(true);    break;
            case Direction::ROTATE_CCW:      sonify_rotate(false);   break;
        }
    }

private:
    float m_sample_rate   = 44100.0f;
    int   m_channel_count = 1;
    RawImage m_img;
    Direction m_direction = Direction::LEFT_TO_RIGHT;
    float m_secs_per_unit = 0.001f;
    FreqMap m_freq_map;
    ROI m_roi;
    std::vector<float> m_audio_data;
    SonifyFunc m_sonify_func = sonify_functions::sine();

    struct Bounds { int x0, y0, x1, y1; };
    Bounds effective_bounds() const noexcept
    {
        if (m_roi.active)
            return {m_roi.x, m_roi.y, m_roi.x + m_roi.w, m_roi.y + m_roi.h};
        return {0, 0, m_img.width, m_img.height};
    }

    struct StripData { float r, g, b, brightness, h, s, v; };

    static void rgb_to_hsv(float r, float g, float b,
                           float &h, float &s, float &v) noexcept
    {
        v = std::max({r, g, b});
        const float mn    = std::min({r, g, b});
        const float delta = v - mn;
        s = (v > 1e-6f) ? delta / v : 0.f;
        if (delta < 1e-6f) { h = 0.f; return; }
        if (v == r)      h = 60.f * std::fmod((g - b) / delta, 6.f);
        else if (v == g) h = 60.f * ((b - r) / delta + 2.f);
        else             h = 60.f * ((r - g) / delta + 4.f);
        if (h < 0.f) h += 360.f;
    }

    static StripData make_strip_data(float r, float g, float b) noexcept
    {
        StripData d;
        d.r = r; d.g = g; d.b = b;
        d.brightness = 0.299f * r + 0.587f * g + 0.114f * b;
        rgb_to_hsv(r, g, b, d.h, d.s, d.v);
        return d;
    }

    static float pixel_brightness(const float *px, int channels)
    {
        if (channels == 1)
            return px[0];
        if (channels >= 3)
            return 0.299f * px[0] + 0.587f * px[1] + 0.114f * px[2];
        throw std::runtime_error("Unsupported channel count");
    }

    StripData column_data(int x, int y0, int y1) const
    {
        float sr = 0.f, sg = 0.f, sb = 0.f;
        int count = 0;
        for (int y = y0; y < y1; ++y)
        {
            const float *px = &m_img.data[y * m_img.stride + x * m_img.channels];
            if (m_img.channels >= 4 && px[3] < 0.5f)
                continue;
            sr += px[0];
            sg += (m_img.channels >= 3) ? px[1] : px[0];
            sb += (m_img.channels >= 3) ? px[2] : px[0];
            ++count;
        }
        if (count == 0)
            return make_strip_data(0.f, 0.f, 0.f);
        const float n = static_cast<float>(count);
        return make_strip_data(sr / n, sg / n, sb / n);
    }

    StripData row_data(int y, int x0, int x1) const
    {
        float sr = 0.f, sg = 0.f, sb = 0.f;
        int count = 0;
        for (int x = x0; x < x1; ++x)
        {
            const float *px = &m_img.data[y * m_img.stride + x * m_img.channels];
            if (m_img.channels >= 4 && px[3] < 0.5f)
                continue;
            sr += px[0];
            sg += (m_img.channels >= 3) ? px[1] : px[0];
            sb += (m_img.channels >= 3) ? px[2] : px[0];
            ++count;
        }
        if (count == 0)
            return make_strip_data(0.f, 0.f, 0.f);
        const float n = static_cast<float>(count);
        return make_strip_data(sr / n, sg / n, sb / n);
    }

    void emit_strip(const StripData &d, int x, int y, int w, int h, int spu,
                    int strip_index, int strip_count)
    {
        SonifyContext ctx{
            .sample_rate   = m_sample_rate,
            .brightness    = d.brightness,
            .r             = d.r,
            .g             = d.g,
            .b             = d.b,
            .h             = d.h,
            .s             = d.s,
            .v             = d.v,
            .x             = x,
            .y             = y,
            .width         = w,
            .height        = h,
            .strip_index   = strip_index,
            .strip_count   = strip_count,
            .t             = static_cast<float>(m_audio_data.size())
                             / (m_sample_rate * m_channel_count),
            .n_samples     = spu,
            .channel_count = m_channel_count,
            .freq_scale    = m_freq_map.scale,
            .fmin          = m_freq_map.min,
            .fmax          = m_freq_map.max,
        };
        m_sonify_func(ctx, m_audio_data);
    }

    void sonify_left_to_right()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        const auto [x0, y0, x1, y1] = effective_bounds();
        const int count = x1 - x0;
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(count) * spu * m_channel_count);
        for (int i = 0; i < count; ++i)
            emit_strip(column_data(x0 + i, y0, y1), x0 + i, y0, w, h, spu, i, count);
    }

    void sonify_right_to_left()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        const auto [x0, y0, x1, y1] = effective_bounds();
        const int count = x1 - x0;
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(count) * spu * m_channel_count);
        for (int i = 0; i < count; ++i)
            emit_strip(column_data(x1 - 1 - i, y0, y1), x1 - 1 - i, y0, w, h, spu, i, count);
    }

    void sonify_top_to_bottom()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        const auto [x0, y0, x1, y1] = effective_bounds();
        const int count = y1 - y0;
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(count) * spu * m_channel_count);
        for (int i = 0; i < count; ++i)
            emit_strip(row_data(y0 + i, x0, x1), x0, y0 + i, w, h, spu, i, count);
    }

    void sonify_bottom_to_top()
    {
        const int w   = m_img.width;
        const int h   = m_img.height;
        const int spu = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        const auto [x0, y0, x1, y1] = effective_bounds();
        const int count = y1 - y0;
        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(count) * spu * m_channel_count);
        for (int i = 0; i < count; ++i)
            emit_strip(row_data(y1 - 1 - i, x0, x1), x0, y1 - 1 - i, w, h, spu, i, count);
    }

    // Shared implementation for ROTATE_CW (clockwise=true) and ROTATE_CCW.
    // Sweeps radial lines from the image centre, one strip per angle step.
    // Brightness of each strip = average brightness along that ray.
    // ctx.x / ctx.y = tip pixel of the ray (last in-bounds sample).
    void sonify_rotate(bool clockwise)
    {
        const int   w    = m_img.width;
        const int   h    = m_img.height;
        const int   spu  = std::max(1, static_cast<int>(m_sample_rate * m_secs_per_unit));
        const float cx   = (w - 1) * 0.5f;
        const float cy   = (h - 1) * 0.5f;
        const int   num_strips = std::max(w, h);
        const int   max_steps  = static_cast<int>(std::sqrt(cx * cx + cy * cy)) + 2;
        constexpr float two_pi = 6.28318530718f;

        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(num_strips) * spu * m_channel_count);

        const auto [bx0, by0, bx1, by1] = effective_bounds();

        for (int i = 0; i < num_strips; ++i)
        {
            const float angle = static_cast<float>(i) * two_pi
                                / static_cast<float>(num_strips);
            // CW from 12 o'clock: dx=sin, dy=-cos
            // CCW from 12 o'clock: dx=-sin, dy=-cos
            const float dx = clockwise ? std::sin(angle) : -std::sin(angle);
            const float dy = -std::cos(angle);

            float sr = 0.f, sg = 0.f, sb = 0.f;
            int   count = 0;
            for (int step = 0; step <= max_steps; ++step)
            {
                const int ix = static_cast<int>(std::round(cx + dx * step));
                const int iy = static_cast<int>(std::round(cy + dy * step));
                if (ix < bx0 || ix >= bx1 || iy < by0 || iy >= by1)
                    break;
                const float *px = &m_img.data[iy * m_img.stride + ix * m_img.channels];
                sr += px[0];
                sg += (m_img.channels >= 3) ? px[1] : px[0];
                sb += (m_img.channels >= 3) ? px[2] : px[0];
                ++count;
            }

            const float n   = count > 0 ? static_cast<float>(count) : 1.f;
            const int   tip = std::max(0, count - 1);
            const int   tx  = static_cast<int>(std::round(cx + dx * tip));
            const int   ty  = static_cast<int>(std::round(cy + dy * tip));
            emit_strip(make_strip_data(sr / n, sg / n, sb / n), tx, ty, w, h, spu, i, num_strips);
        }
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

        std::vector<float> ring_r(max_r + 1, 0.f);
        std::vector<float> ring_g(max_r + 1, 0.f);
        std::vector<float> ring_b(max_r + 1, 0.f);
        std::vector<int>   ring_count(max_r + 1, 0);

        const auto [bx0, by0, bx1, by1] = effective_bounds();
        for (int y = by0; y < by1; ++y)
            for (int x = bx0; x < bx1; ++x)
            {
                const float dx = x - cx;
                const float dy = y - cy;
                const int r = static_cast<int>(std::sqrt(dx * dx + dy * dy));
                const float *px = &m_img.data[y * m_img.stride + x * m_img.channels];
                ring_r[r] += px[0];
                ring_g[r] += (m_img.channels >= 3) ? px[1] : px[0];
                ring_b[r] += (m_img.channels >= 3) ? px[2] : px[0];
                ++ring_count[r];
            }

        m_audio_data.clear();
        m_audio_data.reserve(static_cast<std::size_t>(max_r + 1) * spu * m_channel_count);

        const int strip_count = max_r + 1;
        for (int i = 0; i <= max_r; ++i)
        {
            const int r = outwards ? i : (max_r - i);
            const float n = ring_count[r] > 0 ? static_cast<float>(ring_count[r]) : 1.f;
            emit_strip(make_strip_data(ring_r[r] / n, ring_g[r] / n, ring_b[r] / n),
                       r, 0, w, h, spu, i, strip_count);
        }
    }
};

} // namespace sonify
