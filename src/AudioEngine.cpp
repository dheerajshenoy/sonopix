#include "AudioEngine.hpp"

#include "SonifyEngine.hpp"
#include "logging.hpp"
#include "utils.hpp"

AudioEngine::AudioEngine()
    : m_sound(m_sound_buffer), m_scrub_sound(m_scrub_buffer)
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

// Sets the audio data for the engine. The input is a vector of floats
// representing audio samples.
void
AudioEngine::set_data(std::vector<float> &&audio_data, float sample_rate)
{
    m_sample_rate = sample_rate;
    m_dataf       = std::move(audio_data);
    m_data  = convert_to_int16(m_dataf);

    std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::Mono};

    if (!m_sound_buffer.loadFromSamples(m_data.data(), m_data.size(),
                                        m_channel_count, m_sample_rate,
                                        channelMap))
    {
        LOG("Unable to load samples from audio data", LogLevel::ERROR);
    }
}

bool
AudioEngine::save(const std::string &filename) const noexcept
{
    if (m_sound_buffer.getSampleCount() == 0)
    {
        LOG("save: no audio data to write", LogLevel::ERROR);
        return false;
    }
    if (!m_sound_buffer.saveToFile(filename))
    {
        LOG("save: failed to write " + filename, LogLevel::ERROR);
        return false;
    }
    return true;
}

void
AudioEngine::play_sample_at(std::size_t sample) noexcept
{
    constexpr std::size_t WINDOW = 2048;
    const std::size_t total      = m_data.size();
    if (total == 0)
        return;

    const std::size_t start = std::min(sample, total - 1);
    const std::size_t end   = std::min(start + WINDOW, total);

    std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::Mono};
    if (!m_scrub_buffer.loadFromSamples(m_data.data() + start, end - start,
                                        m_channel_count, m_sample_rate,
                                        channelMap))
        return;

    m_scrub_sound = sf::Sound(m_scrub_buffer);
    m_scrub_sound.play();
}

void
AudioEngine::seek_to_sample(std::size_t sample) noexcept
{
    const float secs = static_cast<float>(sample)
                       / (m_sample_rate * static_cast<float>(m_channel_count));
    m_sound.setPlayingOffset(sf::seconds(secs));
}

// Returns the current sample index based on the playing offset of the sound.
const std::size_t
AudioEngine::sample_index() const noexcept
{
    if (m_sound.getStatus() != sf::Sound::Status::Playing
        && m_sound.getStatus() != sf::Sound::Status::Paused)
        return 0;

    const float s = m_sound.getPlayingOffset().asSeconds();

    return static_cast<std::size_t>(s * m_sample_rate * m_channel_count);
}
