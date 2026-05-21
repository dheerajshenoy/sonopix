#pragma once

#include <cstdint>
#include <vector>

class Effects
{
public:
    static std::vector<float> Reverb(std::vector<float> &audio_data,
                                     float sample_rate,
                                     float reverb_time = 0.5f);

    // delay
    static std::vector<float> Delay(std::vector<float> &audio_data,
                                    float sample_rate, float delay_time = 0.3f,
                                    float feedback = 0.5f);

    // distortion
    static std::vector<float> Distortion(std::vector<float> &audio_data,
                                         float sample_rate, float gain = 20.0f);
};
