#include "Effects.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

void
Effects::Gain(std::vector<float> &data, float gain) noexcept
{
    for (float &s : data)
        s *= gain;
}

std::vector<float>
Effects::Delay(const std::vector<float> &data, float sample_rate,
               float delay_time, float feedback, float mix)
{
    if (data.empty())
        return {};

    const std::size_t delay_samples
        = std::max(std::size_t{1},
                   static_cast<std::size_t>(delay_time * sample_rate));

    std::vector<float> delay_buf(delay_samples, 0.f);
    std::vector<float> out(data.size());

    for (std::size_t i = 0; i < data.size(); ++i)
    {
        const std::size_t pos = i % delay_samples;
        const float delayed   = delay_buf[pos];
        delay_buf[pos]        = data[i] + feedback * delayed;
        out[i]                = (1.f - mix) * data[i] + mix * delayed;
    }

    return out;
}

// ---------------------------------------------------------------------------
// Schroeder reverb
// Four parallel feedback comb filters → two series allpass filters.
// Delay lengths are prime-ish multiples tuned for natural-sounding diffusion.
// ---------------------------------------------------------------------------

namespace
{

struct CombFilter
{
    std::vector<float> buf;
    std::size_t pos  = 0;
    float feedback   = 0.f;
    float damp       = 0.f;
    float last_out   = 0.f;

    CombFilter(std::size_t delay, float fb, float d)
        : buf(delay, 0.f), feedback(fb), damp(d)
    {}

    float process(float x) noexcept
    {
        const float out = buf[pos];
        // One-pole low-pass damping inside the feedback loop
        last_out     = out * (1.f - damp) + last_out * damp;
        buf[pos]     = x + last_out * feedback;
        pos          = (pos + 1) % buf.size();
        return out;
    }
};

struct AllpassFilter
{
    std::vector<float> buf;
    std::size_t pos = 0;
    float feedback  = 0.5f;

    AllpassFilter(std::size_t delay, float fb)
        : buf(delay, 0.f), feedback(fb)
    {}

    float process(float x) noexcept
    {
        const float bufout = buf[pos];
        buf[pos]           = x + bufout * feedback;
        pos                = (pos + 1) % buf.size();
        return bufout - x;
    }
};

} // namespace

std::vector<float>
Effects::Reverb(const std::vector<float> &data, float sample_rate,
                float room_size, float damping, float mix)
{
    if (data.empty())
        return {};

    // Scale canonical 44100 Hz delay lengths to the actual sample rate
    const float sr_scale = sample_rate / 44100.f;

    // Feedback proportional to room_size; clamped to keep it stable
    const float fb = std::clamp(0.70f + room_size * 0.28f, 0.f, 0.98f);
    const float d  = std::clamp(damping, 0.f, 1.f);

    // Classic Schroeder comb delay lengths (samples at 44100 Hz)
    const std::size_t comb_delays[4] = {1557, 1617, 1491, 1422};
    CombFilter combs[4] = {
        {static_cast<std::size_t>(comb_delays[0] * sr_scale), fb, d},
        {static_cast<std::size_t>(comb_delays[1] * sr_scale), fb, d},
        {static_cast<std::size_t>(comb_delays[2] * sr_scale), fb, d},
        {static_cast<std::size_t>(comb_delays[3] * sr_scale), fb, d},
    };

    const std::size_t ap_delays[2] = {225, 556};
    AllpassFilter allpasses[2] = {
        {static_cast<std::size_t>(ap_delays[0] * sr_scale), 0.5f},
        {static_cast<std::size_t>(ap_delays[1] * sr_scale), 0.5f},
    };

    std::vector<float> out(data.size());

    for (std::size_t i = 0; i < data.size(); ++i)
    {
        float wet = 0.f;
        for (auto &c : combs)
            wet += c.process(data[i]);
        wet *= 0.25f; // average

        wet = allpasses[0].process(wet);
        wet = allpasses[1].process(wet);

        out[i] = (1.f - mix) * data[i] + mix * wet;
    }

    return out;
}

std::vector<float>
Effects::Distortion(const std::vector<float> &data, float drive, float mix)
{
    if (data.empty())
        return {};

    // Map drive [0,1] to a gain factor [1,20]
    const float g    = 1.f + drive * 19.f;
    const float norm = std::tanh(g); // tanh(g) normalises the output to [-1,1]

    std::vector<float> out(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        const float wet = std::tanh(data[i] * g) / norm;
        out[i]          = (1.f - mix) * data[i] + mix * wet;
    }

    return out;
}
