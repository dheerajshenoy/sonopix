#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

template <typename T>
static inline std::vector<std::int16_t>
convert_to_int16(const std::vector<T> &in)
{
    // m_data = std::vector<std::int16_t>(m_dataf.size());
    //
    // for (int i = 0; i < m_dataf.size(); i++)
    // {
    //     float clamped = std::clamp(m_dataf[i], -1.0f, 1.0f);
    //     m_data[i]     = static_cast<std::int16_t>(clamped * 32767);
    // }

    std::vector<std::int16_t> out(in.size());

    std::transform(in.begin(), in.end(), out.begin(), [](T sample)
    {
        float clamped = std::clamp(sample, -1.0f, 1.0f);
        return static_cast<std::int16_t>(clamped * 32767);
    });

    return out;
}
