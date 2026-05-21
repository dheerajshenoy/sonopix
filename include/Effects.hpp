#pragma once

#include <vector>

class Effects
{
public:
    // Gain: multiply every sample by `gain`
    static void Gain(std::vector<float> &data, float gain) noexcept;

    // Delay line: delay_time in seconds, feedback in [0,1], mix in [0,1]
    static std::vector<float> Delay(const std::vector<float> &data,
                                    float sample_rate,
                                    float delay_time = 0.3f,
                                    float feedback   = 0.5f,
                                    float mix        = 0.5f);

    // Schroeder reverb: room_size in [0,1], damping in [0,1], mix in [0,1]
    static std::vector<float> Reverb(const std::vector<float> &data,
                                     float sample_rate,
                                     float room_size = 0.5f,
                                     float damping   = 0.5f,
                                     float mix       = 0.3f);

    // Soft-clip distortion: drive in [0,1], mix in [0,1]
    static std::vector<float> Distortion(const std::vector<float> &data,
                                         float drive = 0.5f,
                                         float mix   = 1.0f);
};
