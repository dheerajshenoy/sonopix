# Sonopix

Convert images to audio by scanning them in various directions and mapping pixel brightness to sound. Supports Lua scripting for full control over traversal options, frequency mapping, and custom sonification functions.

## Dependencies

- [SFML](https://www.sfml-dev.org/) >= 3.0
- [Lua](https://www.lua.org/) > 5.4
- CMake >= 3.16
- C++23 compiler

## Building

```sh
cmake -B build
cmake --build build
```

## Usage

```
sonopix [options]
```

| Flag | Description |
|---|---|
| `-i, --input FILE` | Image to open |
| `-d, --direction DIR` | Scan direction (see below) |
| `-f, --frequency MIN:MAX` | Frequency range in Hz (default: `20:2500`) |
| `-s, --freq-scale SCALE` | `linear`, `log`, or `exponential` |
| `-u, --secs-per-unit SPU` | Seconds of audio per column/row/ring/pixel |
| `-r, --sample-rate RATE` | Audio sample rate (default: `44100`) |
| `--cursor-width WIDTH` | Cursor width in pixels |
| `-o, --output FILE` | Sonify and save to WAV/OGG, then exit; `.wav` appended if no extension given |
| `--script FILE` | Lua script to run before the main loop |
| `-v, --version` | Print version |

### Directions

| Value | Description |
|---|---|
| `left-to-right` | Scans columns left → right (default) |
| `right-to-left` | Scans columns right → left |
| `top-to-bottom` | Scans rows top → bottom |
| `bottom-to-top` | Scans rows bottom → top |
| `circle-outwards` | Scans rings from center outward |
| `circle-inwards` | Scans rings from edge inward |
| `rotate-cw` | Radar sweep clockwise from 12 o'clock; one strip per radial line |
| `rotate-ccw` | Radar sweep counter-clockwise from 12 o'clock |

### Keybindings

| Key | Action |
|---|---|
| `Space` | Play / pause |
| `S` | Re-sonify with current settings |
| `L` | Toggle loop |
| `←` / `→` | Seek ±2 % |
| `Shift+←` / `Shift+→` | Seek ±10 % |

### Batch export

Combine `--input` and `--output` to sonify headlessly and save without interaction:

```sh
sonopix -i image.png -o out.wav
sonopix -i image.png -o out.ogg -d zigzag-h -u 0.0005 -f 20:8000 -s log
```

The window opens, sonifies in the background (title shows `[sonifying...]`), saves the file, then closes automatically.

## Lua scripting

Pass a script with `--script file.lua`. The script runs before the main loop, so options set here apply before the first sonification.

**Playback:** `play()`, `pause()`, `stop()`

**Status:** `is_playing()`, `is_paused()`, `is_stopped()`, `current_time()`

**Image / audio:** `open_file(path)`, `sonify()`, `save_audio(path)`

```lua
sonopix.open_file("/path/to/image.png")

sonopix.opts = {
    direction = "circle-outwards",
    spu       = 0.001,
    frequency = { min = 20, max = 20000, scale = "exponential" },
    cursor    = { width = 3, color = "#FF5000FF" },
    waveform  = { height = 50, color = "#FFFFFFC8" },
    oscilloscope = { height = 60, window_samples = 2048, color = "#00FFB4DC" },
    progress_bar = { height = 6, color = "#FF8800FF" },
    antialiasing_level = 8,
}

if sonopix.sonify() then
    sonopix.play()
end
```

### `sonopix.opts` fields

| Field | Type | Description |
|---|---|---|
| `direction` | string | Scan direction (see table above) |
| `spu` | number | Seconds of audio per unit (column/row/ring, or pixel for zigzag/custom) |
| `sample_rate` | number | Audio sample rate in Hz |
| `frequency.min` | number | Minimum frequency in Hz |
| `frequency.max` | number | Maximum frequency in Hz |
| `frequency.scale` | string | `"linear"`, `"log"`, or `"exponential"` |
| `cursor.width` | number | Cursor width in pixels |
| `cursor.color` | string | Cursor color as `"#RRGGBB"` or `"#RRGGBBAA"` |
| `volume` | number | Master playback volume `[0, 100]` (default: `100`); takes effect immediately without re-sonifying |
| `amplitude` | number | Master gain baked into the audio buffer at sonify time (default: `1.0`) |
| `loop` | boolean | Loop playback when audio ends (default: `false`); also toggled with `L` |
| `image_rotation` | number | Rotation of the displayed image in degrees (default: `0`); cursor tracks the rotated image |
| `audio_effects.gain` | number | Master gain multiplier applied after sonification (default: `1.0`) |
| `audio_effects.delay` | table | `{ time, feedback, mix }` — delay line; `mix = 0` disables |
| `audio_effects.reverb` | table | `{ room_size, damping, mix }` — Schroeder reverb; `mix = 0` disables |
| `audio_effects.distortion` | table | `{ drive, mix }` — soft-clip tanh distortion; `mix = 0` disables |
| `image_effects.grayscale` | boolean | Convert to greyscale (default: `false`) |
| `image_effects.brightness` | number | Additive brightness shift in `[-1, 1]` (default: `0`) |
| `image_effects.saturation` | number | Saturation multiplier; `0` = greyscale, `1` = original (default: `1`) |
| `image_effects.contrast` | number | Contrast multiplier around mid-grey; `1` = original (default: `1`) |
| `image_effects.hue` | number | Hue rotation in degrees `[0, 360)` (default: `0`) |
| `image_effects.blur` | number | Gaussian blur kernel radius in texels; `0` = off (default: `0`) |
| `image_effects.sharpen` | number | Laplacian sharpen strength; `0` = off (default: `0`) |
| `image_effects.threshold` | number | Luminance cutoff `[0, 1]`; pixels above → white, below → black; negative = off (default: `-1`) |
| `image_effects.invert` | boolean | Invert all colours (default: `false`) |
| `waveform.visible` | boolean | Show/hide the RMS waveform strip (default: `true`) |
| `waveform.height` | integer | Height of the waveform strip in pixels (default: `40`) |
| `waveform.color` | string | Waveform bar color as `"#RRGGBB"` or `"#RRGGBBAA"` |
| `oscilloscope.visible` | boolean | Show/hide the live oscilloscope strip (default: `true`) |
| `oscilloscope.height` | integer | Height of the oscilloscope strip in pixels (default: `60`) |
| `oscilloscope.window_samples` | integer | Number of samples shown at once (default: `4096`) |
| `oscilloscope.color` | string | Oscilloscope line color as `"#RRGGBB"` or `"#RRGGBBAA"` |
| `progress_bar.visible` | boolean | Show/hide the progress bar (default: `true`) |
| `progress_bar.height` | integer | Height of the progress bar in pixels (default: `4`) |
| `progress_bar.color` | string | Progress bar fill color as `"#RRGGBB"` or `"#RRGGBBAA"` |
| `fps` | integer | Framerate limit; `0` = unlimited (default: `60`) |
| `antialiasing_level` | integer | MSAA sample count; applied at window creation |
| `window_title` | string | Window title string |
| `window_size` | table | `{ width = W, height = H }` window dimensions |
| `traversal_func` | function | Custom pixel order: `(strip_index, total, w, h) → x, y` (see below) |
| `sonify_func` | function | Custom sonification function: `(ctx) → number[]` (see below) |
| `audio_effects.process_func` | function | Post-sonification DSP: `(samples, sample_rate) → number[]` (see below) |

### Custom traversal order

For arbitrary orderings set `sonopix.opts.traversal_func`. The function is called **once per strip** by C++ with `(strip_index, total, width, height)` and must return `(x, y)` for that strip. Each pixel becomes one audio strip whose brightness is that single pixel's brightness. A point cursor tracks the moving pixel during playback.

Use `sonopix.pixel_brightness(x, y)` to read pixel brightness `[0, 1]` at any coordinate — useful for building data-driven traversal orders before sonification starts.

```lua
-- Horizontal zigzag (equivalent to built-in "zigzag-h")
sonopix.opts.traversal_func = function(i, total, w, h)
    local row     = math.floor(i / w)
    local col_raw = i % w
    if row % 2 == 0 then
        return col_raw, row
    else
        return w - 1 - col_raw, row
    end
end

-- Diagonal stripes (top-left to bottom-right)
sonopix.opts.traversal_func = function(i, total, w, h)
    local diag = i % (w + h - 1)
    local x    = math.min(diag, w - 1)
    local y    = math.max(0, diag - (w - 1))
    return x, y
end

-- Random scatter (with replacement; seed math.randomseed() for reproducibility)
sonopix.opts.traversal_func = function(i, total, w, h)
    return math.random(0, w - 1), math.random(0, h - 1)
end
```

### Custom sonification function

Set `sonopix.opts.sonify_func` to replace the built-in sine oscillator. The function is called **once per strip** and must return a table of `ctx.n_samples` floats in `[-1, 1]`. Use upvalues for state (oscillator phase etc.) that must persist across strips.

```lua
local phase = 0.0

sonopix.opts.sonify_func = function(ctx)
    local freq    = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    local samples = {}
    for i = 1, ctx.n_samples do
        phase      = phase + 2 * math.pi * freq / ctx.sample_rate
        samples[i] = ctx.brightness * math.sin(phase)
    end
    return samples
end
```

Use upvalues (locals captured by the closure) for persistent state like oscillator phase — they survive across strips.

#### Context fields

| Field | Type | Description |
|---|---|---|
| `brightness` | number | Pixel luminance `[0, 1]` |
| `r` | number | Red channel `[0, 1]` |
| `g` | number | Green channel `[0, 1]` |
| `b` | number | Blue channel `[0, 1]` |
| `h` | number | Hue `[0, 360]` |
| `s` | number | HSV saturation `[0, 1]` |
| `v` | number | HSV value `[0, 1]` |
| `x` | integer | Column index (or ring radius for circle modes) |
| `y` | integer | Row index |
| `width` | integer | Image width in pixels |
| `height` | integer | Image height in pixels |
| `strip_index` | integer | Playback-order index of the current strip (0 = first) |
| `strip_count` | integer | Total number of strips |
| `n_samples` | integer | Frames to generate for this strip (samples per channel) |
| `channel_count` | integer | Number of audio channels (`1` = mono, `2` = stereo) |
| `t` | number | Time in seconds since the start of audio (at strip start) |
| `fmin` | number | Minimum frequency in Hz |
| `fmax` | number | Maximum frequency in Hz |
| `scale` | string | Frequency scale |
| `sample_rate` | number | Sample rate in Hz |

#### Stereo output

Set `sonopix.opts.channel_count = 2` to enable stereo. The function must then return `n_samples * 2` interleaved samples in `[L1, R1, L2, R2, ...]` order. Use `ctx.channel_count` to branch so the same function works in both mono and stereo:

```lua
sonopix.opts.channel_count = 2

local phase = 0.0

sonopix.opts.sonify_func = function(ctx)
    local freq    = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    local pan_r   = ctx.x / ctx.width     -- 0 (left edge) → 1 (right edge)
    local pan_l   = 1.0 - pan_r
    local samples = {}
    for i = 1, ctx.n_samples do
        phase = phase + 2 * math.pi * freq / ctx.sample_rate
        local s = ctx.brightness * math.sin(phase)
        if ctx.channel_count == 2 then
            samples[i * 2 - 1] = pan_l * s   -- left
            samples[i * 2]     = pan_r * s   -- right
        else
            samples[i] = s
        end
    end
    return samples
end
```

The cursor, waveform playhead, and progress bar all account for channel count automatically.

#### Example: chromatic FM synthesis

Uses hue and saturation to shape timbre — warm/red pixels get a near-harmonic FM ratio, cool/blue pixels get a metallic inharmonic one; saturation drives the modulation depth so grey pixels stay pure-sine.

```lua
local drone_phase    = 0.0
local ping_phase     = 0.0
local ping_mod_phase = 0.0
local ping_env       = 0.0
local ping_freq      = 440.0
local ping_ratio     = 2.1
local ping_idx_max   = 0.0
local avg_bright     = 0.05

sonopix.opts.sonify_func = function(ctx)
    avg_bright = avg_bright * 0.97 + ctx.brightness * 0.03

    local excess = ctx.brightness - avg_bright * 1.5
    if excess > 0.02 then
        ping_env     = math.min(1.0, ping_env + excess * 4.0)
        ping_freq    = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
        -- hue drives ratio: red(0°)→1.5 (harmonic), blue(240°)→~2.2 (metallic)
        ping_ratio   = 1.5 + 0.7 * math.sin(math.rad(ctx.h * 0.5))
        -- saturation drives FM depth: grey→pure sine, vivid colour→rich sidebands
        ping_idx_max = ctx.s * 7.0
    end

    local drone_freq = ctx.fmin * (ctx.fmax / ctx.fmin) ^ (avg_bright * 0.3)
    local decay      = math.exp(-1.0 / (ctx.sample_rate * 0.12))
    local samples    = {}
    for i = 1, ctx.n_samples do
        drone_phase = drone_phase + 2 * math.pi * drone_freq / ctx.sample_rate
        local drone = 0.06 * math.sin(drone_phase)

        ping_mod_phase = ping_mod_phase + 2 * math.pi * ping_freq * ping_ratio / ctx.sample_rate
        ping_phase     = ping_phase     + 2 * math.pi * ping_freq              / ctx.sample_rate
        local index = ping_env * ping_idx_max
        local ping  = ping_env * math.sin(ping_phase + index * math.sin(ping_mod_phase))
        ping_env = ping_env * decay

        samples[i] = drone + ping
    end
    return samples
end
```

#### Example: strip envelope (fade in/out per column)

```lua
local phase = 0.0

sonopix.opts.sonify_func = function(ctx)
    local freq    = ctx.fmin + ctx.brightness * (ctx.fmax - ctx.fmin)
    local samples = {}
    for i = 1, ctx.n_samples do
        local env  = math.sin(math.pi * (i - 1) / ctx.n_samples)  -- half-sine envelope
        phase      = phase + 2 * math.pi * freq / ctx.sample_rate
        samples[i] = env * ctx.brightness * math.sin(phase)
    end
    return samples
end
```

#### Example: additive harmonics

```lua
local phases = {0, 0, 0, 0}

sonopix.opts.sonify_func = function(ctx)
    local f       = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    local samples = {}
    for i = 1, ctx.n_samples do
        local s = 0.0
        for h = 1, #phases do
            phases[h] = phases[h] + 2 * math.pi * f * h / ctx.sample_rate
            s = s + (1 / h) * math.sin(phases[h])
        end
        samples[i] = ctx.brightness * s * 0.5
    end
    return samples
end
```

### Custom audio post-processing

Set `sonopix.opts.audio_effects.process_func` to apply arbitrary DSP to the final buffer after sonification and all built-in effects. Called once with the full samples table and the sample rate; return a (possibly modified) samples table.

```lua
-- Normalise to peak, then apply a simple DC-block
sonopix.opts.audio_effects.process_func = function(samples, sr)
    local peak = 0.0
    for i = 1, #samples do
        if math.abs(samples[i]) > peak then peak = math.abs(samples[i]) end
    end
    if peak > 0 then
        for i = 1, #samples do samples[i] = samples[i] / peak end
    end
    return samples
end
```

### Data-driven traversal with `sonopix.pixel_brightness`

Use `sonopix.pixel_brightness(x, y)` to query pixel brightness `[0, 1]` when building a custom traversal order. The call is cheap (direct C++ lookup); sort or filter however you like before sonification starts.

```lua
-- Brightest-first traversal
sonopix.open_file("/path/to/image.png")

local order = {}
for y = 0, h - 1 do
    for x = 0, w - 1 do
        order[#order + 1] = { x, y, sonopix.pixel_brightness(x, y) }
    end
end
table.sort(order, function(a, b) return a[3] > b[3] end)

sonopix.opts.traversal_func = function(i, total, w, h)
    local p = order[i + 1]
    return p[1], p[2]
end
```
