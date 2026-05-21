#pragma once

#include "SonifyEngine.hpp"

#include <SFML/Graphics.hpp>

struct CursorOpts
{
    float width     = 5.0f;
    sf::Color color = sf::Color(255, 0, 0, 128);
};

struct ProgressBarOpts
{
    int height      = 4;
    bool visible    = true;
    sf::Color color = sf::Color(255, 255, 255, 200);
};

struct WaveformOpts
{
    int height      = 40;
    bool visible    = true;
    sf::Color color = sf::Color(255, 255, 255, 200);
};

struct OscilloscopeOpts
{
    int height        = 60;
    bool visible      = true;
    int window_samples = 4096; // samples shown at once
    sf::Color color   = sf::Color(0, 255, 180, 220);
};

struct Config
{
    CursorOpts cursor;
    ProgressBarOpts progress_bar;
    WaveformOpts waveform;
    OscilloscopeOpts oscilloscope;
    float amplitude             = 1.0f;
    sonify::Direction direction = sonify::Direction::LEFT_TO_RIGHT;
    sf::ContextSettings window;
    bool loop    = false;
    bool verbose = false;
};
