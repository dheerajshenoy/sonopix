#pragma once

#include "SonifyEngine.hpp"

#include <SFML/Graphics.hpp>
#include <cstdint>

struct CursorOpts
{
    float width     = 5.0f;
    sf::Color color = sf::Color(255, 0, 0, 128);
};

struct ProgressBarOpts
{
    float height    = 0.007f; // fraction of window height
    bool visible    = true;
    sf::Color color = sf::Color(255, 255, 255, 200);
};

struct WaveformOpts
{
    float height    = 0.08f; // fraction of window height
    bool visible    = true;
    sf::Color color = sf::Color(255, 255, 255, 200);
};

struct OscilloscopeOpts
{
    float height      = 0.1f; // fraction of window height
    bool visible      = true;
    int window_samples = 4096;
    sf::Color color   = sf::Color(0, 255, 180, 220);
};

struct ImageEffectsOpts
{
    float grayscale  = 0.f;   // 0 = off, 1 = full grayscale
    float brightness = 0.f;   // -1 to 1 additive shift
    float saturation = 1.f;   // 0 = greyscale, 1 = original, >1 = vivid
    float contrast   = 1.f;   // 0 = flat grey, 1 = original, >1 = high contrast
    float hue        = 0.f;   // degrees to shift hue (0–360)
    float blur       = 0.f;   // 0 = off, >0 = Gaussian radius in texels
    float sharpen    = 0.f;   // 0 = off, >0 = Laplacian strength
    float threshold  = -1.f;  // <0 = off, 0–1 = luminance cutoff (applied last)
    bool  invert     = false;
};

struct AudioEffectsOpts
{
    float gain             = 1.0f;  // master gain (stacks with amplitude)
    // delay
    float delay_time       = 0.3f;  // seconds
    float delay_feedback   = 0.5f;  // 0–1
    float delay_mix        = 0.0f;  // 0 = off
    // reverb
    float reverb_room      = 0.5f;  // 0–1
    float reverb_damping   = 0.5f;  // 0–1
    float reverb_mix       = 0.0f;  // 0 = off
    // distortion
    float distortion_drive = 0.5f;  // 0–1
    float distortion_mix   = 0.0f;  // 0 = off
    // custom process func
    bool has_process_func  = false;
};

struct Config
{
    AudioEffectsOpts audio_effects;
    ImageEffectsOpts image_effects;
    CursorOpts cursor;
    ProgressBarOpts progress_bar;
    WaveformOpts waveform;
    OscilloscopeOpts oscilloscope;
    float amplitude             = 1.0f;
    sonify::Direction direction = sonify::Direction::LEFT_TO_RIGHT;
    sf::ContextSettings window;
    float image_rotation = 0.f;
    bool loop    = false;
    bool verbose = false;
};
