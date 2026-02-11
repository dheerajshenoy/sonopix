#pragma once

#include <SFML/Audio.hpp>
#include <vector>

class AudioEngine
{
public:
    AudioEngine(float sample_rate);
    void play() noexcept;
    void stop() noexcept;
    void pause() noexcept;
    const sf::Sound &sound() const noexcept
    {
        return m_sound;
    }

    const std::size_t sample_index() const noexcept;
    void set_data(std::vector<float> &&audio_data);

    bool is_playing() const noexcept
    {
        return m_sound.getStatus() == sf::Sound::Status::Playing;
    }

private:
    void convert_to_int16();
    std::vector<float> m_dataf;
    std::vector<std::int16_t> m_data;
    sf::SoundBuffer m_sound_buffer;
    sf::Sound m_sound;
    float m_sample_rate{44100.0f};
    int m_channel_count{1};
};
