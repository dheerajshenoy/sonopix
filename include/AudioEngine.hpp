#pragma once

#include <SFML/Audio.hpp>
#include <algorithm>
#include <vector>

class AudioEngine
{
public:
    AudioEngine();

    inline const sf::Sound &sound() const noexcept
    {
        return m_sound;
    }

    inline bool is_playing() const noexcept
    {
        return m_sound.getStatus() == sf::Sound::Status::Playing;
    }

    inline bool is_paused() const noexcept
    {
        return m_sound.getStatus() == sf::Sound::Status::Paused;
    }

    inline bool is_stopped() const noexcept
    {
        return m_sound.getStatus() == sf::Sound::Status::Stopped;
    }

    inline float sample_rate() const noexcept
    {
        return m_sample_rate;
    }

    inline int channel_count() const noexcept
    {
        return m_channel_count;
    }

    inline void set_channel_count(int count) noexcept
    {
        m_channel_count = count;
    }

    inline const std::vector<float> &dataf() const noexcept
    {
        return m_dataf;
    }

    inline const std::vector<std::int16_t> &data() const noexcept
    {
        return m_data;
    }

    inline const sf::SoundBuffer &sound_buffer() const noexcept
    {
        return m_sound_buffer;
    }

    void play() noexcept;
    void stop() noexcept;
    void pause() noexcept;
    void seek_to_sample(std::size_t sample) noexcept;
    void play_sample_at(std::size_t sample) noexcept;

    inline void set_looping(bool loop) noexcept { m_sound.setLooping(loop); }
    inline bool is_looping() const noexcept     { return m_sound.isLooping(); }

    inline void  set_volume(float v) noexcept { m_sound.setVolume(std::clamp(v, 0.f, 100.f)); }
    inline float volume() const noexcept      { return m_sound.getVolume(); }
    const std::size_t sample_index() const noexcept;
    void set_data(std::vector<float> &&audio_data, float sample_rate);
    bool save(const std::string &filename) const noexcept;

private:
    std::vector<float> m_dataf;
    std::vector<std::int16_t> m_data;
    sf::SoundBuffer m_sound_buffer;
    sf::Sound m_sound;
    sf::SoundBuffer m_scrub_buffer;
    sf::Sound m_scrub_sound;
    float m_sample_rate = 44100.0f;
    int m_channel_count = 1;
};
