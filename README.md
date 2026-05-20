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
| `-u, --secs-per-unit SPU` | Seconds of audio per column/row/ring |
| `-r, --sample-rate RATE` | Audio sample rate (default: `44100`) |
| `--cursor-width WIDTH` | Cursor width in pixels |
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

### Keybindings

| Key | Action |
|---|---|
| `Space` | Play / pause |
| `S` | Re-sonify with current settings |

## Lua scripting

Pass a script with `--script file.lua`. The script runs before the main loop, so options set here apply before the first sonification.

```lua
sonopix.open_file("/path/to/image.png")

sonopix.opts = {
    direction = "circle-outwards",
    spu       = 0.001,
    frequency = { min = 20, max = 20000, scale = "exponential" },
    cursor    = { width = 3, color = "#FF5000FF" },
    antialiasing_level = 8,
}

if sonopix.sonify() then
    sonopix.play()
end
```

### `sonopix.opts` fields

| Field | Type | Description |
|---|---|---|
| `direction` | string | Scan direction |
| `spu` | number | Seconds per column/row/ring |
| `sample_rate` | number | Audio sample rate in Hz |
| `frequency.min` | number | Minimum frequency in Hz |
| `frequency.max` | number | Maximum frequency in Hz |
| `frequency.scale` | string | `"linear"`, `"log"`, or `"exponential"` |
| `cursor.width` | number | Cursor width in pixels |
| `cursor.color` | string | Cursor color as `"#RRGGBB"` or `"#RRGGBBAA"` |
| `antialiasing_level` | integer | MSAA sample count; applied at window creation |
| `sonify_func` | function | Custom sonification function (see below) |

### Custom sonification function

Set `sonopix.opts.sonify_func` to replace the built-in sine oscillator. The function is called once per audio sample and must return a float in `[-1, 1]`.

```lua
local phase = 0.0

sonopix.opts.sonify_func = function(ctx)
    local freq = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    phase = phase + 2 * math.pi * freq / ctx.sample_rate
    return ctx.brightness * math.sin(phase)
end
```

Use upvalues (locals captured by the closure) for persistent state like oscillator phase — they survive across strips and samples.

#### Context fields

| Field | Type | Description |
|---|---|---|
| `brightness` | number | Pixel brightness `[0, 1]` |
| `x` | integer | Column index (or ring radius for circle modes) |
| `y` | integer | Row index |
| `width` | integer | Image width in pixels |
| `height` | integer | Image height in pixels |
| `strip_index` | integer | Playback-order index of the current strip (0 = first) |
| `strip_count` | integer | Total number of strips |
| `t` | number | Time in seconds since the start of audio |
| `strip_t` | number | Normalized position within the current strip `[0, 1)` |
| `fmin` | number | Minimum frequency in Hz |
| `fmax` | number | Maximum frequency in Hz |
| `scale` | string | Frequency scale |
| `sample_rate` | number | Sample rate in Hz |

#### Example: FM synthesis

```lua
local phase, mod_phase = 0.0, 0.0

sonopix.opts.sonify_func = function(ctx)
    local carrier   = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    local mod_ratio = 1 + (ctx.strip_index / ctx.strip_count) * 3
    local mod_depth = 200 * ctx.brightness

    mod_phase = mod_phase + 2 * math.pi * (carrier * mod_ratio) / ctx.sample_rate
    phase     = phase     + 2 * math.pi * (carrier + mod_depth * math.sin(mod_phase)) / ctx.sample_rate

    return ctx.brightness * math.sin(phase)
end
```

#### Example: strip envelope (fade in/out per column)

```lua
local phase = 0.0

sonopix.opts.sonify_func = function(ctx)
    local freq  = ctx.fmin + ctx.brightness * (ctx.fmax - ctx.fmin)
    local env   = math.sin(math.pi * ctx.strip_t)  -- half-sine per strip
    phase = phase + 2 * math.pi * freq / ctx.sample_rate
    return env * ctx.brightness * math.sin(phase)
end
```

#### Example: additive harmonics

```lua
local phases = {0, 0, 0, 0}

sonopix.opts.sonify_func = function(ctx)
    local f   = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    local out = 0.0
    for i = 1, #phases do
        phases[i] = phases[i] + 2 * math.pi * f * i / ctx.sample_rate
        out = out + (1 / i) * math.sin(phases[i])
    end
    return ctx.brightness * out * 0.5
end
```
