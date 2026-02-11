#include "AudioEngine.hpp"

#include "sonify.hpp"

AudioEngine::AudioEngine(float sample_rate)
    : m_sample_rate(sample_rate), m_sound(m_sound_buffer)
{
}

void
AudioEngine::play() noexcept
{
    m_sound.play();
}

void
AudioEngine::pause() noexcept
{
    m_sound.pause();
}

void
AudioEngine::stop() noexcept
{
    m_sound.stop();
}

void
AudioEngine::convert_to_int16()
{
    m_data = std::vector<std::int16_t>(m_dataf.size());

    for (int i = 0; i < m_dataf.size(); i++)
    {
        float clamped = std::clamp(m_dataf[i], -1.0f, 1.0f);
        m_data[i]     = static_cast<std::int16_t>(clamped * 32767);
    }
}

void
AudioEngine::set_data(std::vector<float> &&audio_data)
{
    m_dataf = std::move(audio_data);
    convert_to_int16();

    std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::Mono};

    if (!m_sound_buffer.loadFromSamples(m_data.data(), m_data.size(), m_channel_count,
                                        m_sample_rate, channelMap))
    {
        sonify::log("Unable to load samples from audio data",
                    sonify::LogLevel::ERROR);
    }
}

const std::size_t
AudioEngine::sample_index() const noexcept
{
    if (m_sound.getStatus() != sf::Sound::Status::Playing
        && m_sound.getStatus() != sf::Sound::Status::Paused)
        return 0;

    const float s = m_sound.getPlayingOffset().asSeconds();

    return static_cast<std::size_t>(s * m_sample_rate * m_channel_count);
}
