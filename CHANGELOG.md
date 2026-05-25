# CHANGELOG

## 0.1 (Unreleased)

### Features

- Add webp image support
- Show image
- Command line arguments
- Sonification Engine
- Traversals
    - LEFT TO RIGHT
    - RIGHT TO LEFT
    - TOP TO BOTTOM
    - BOTTOM TO TOP
    - CIRCLE OUTWARDS
    - CIRCLE INWARDS
    - ZIGZAG H
    - ZIGZAG V
- Image Resizing
- **`--hot-reload` flag** — re-executes the Lua script automatically whenever the file is saved; uses inotify `IN_CLOSE_WRITE` on a background thread (zero CPU when idle); errors print to stderr without crashing; enable alongside `--script`
- **Waveform strip** — RMS peak waveform drawn just above the oscilloscope; spans the width of the image; built once after sonification completes; background, bar color, height, and visibility all configurable via `sonopix.opts.waveform`
- **Oscilloscope strip** — real-time rolling signal view between the waveform and progress bar; shows a configurable sample window centered on the playback position; updated every frame; configurable via `sonopix.opts.oscilloscope` (`height`, `color`, `window_samples`, `visible`)
- **Image layout** — image is now flush against the top of the window; bottom area reserved for waveform + oscilloscope + progress bar; image scales to fill full window width
- **Loop playback** — `sonopix.opts.loop = true` or press `L` to toggle; delegates to `sf::Sound::setLooping`
- **`sonopix.set_roi(x, y, w, h)` / `sonopix.clear_roi()`** — restrict sonification to a rectangular sub-region of the image; all traversal modes (left-to-right, top-to-bottom, rotate, circle) respect the bounds; values are clamped to the image dimensions; call `clear_roi()` to revert to the full image
- **Proportional UI heights** — `waveform.height`, `oscilloscope.height`, and `progress_bar.height` are now fractions of the window height (e.g. `0.10` = 10%) instead of fixed pixel values; all strips scale correctly on window resize
- **Waveform seek** — clicking or dragging on the waveform strip seeks the audio; drag is clamped to `[0, 1]` so scrubbing past either edge is safe; works in playing and paused states; plays a short (~46 ms) audio preview at each seek position so dragging across the waveform gives live scrubbing feedback
- **`"rotate-cw"` and `"rotate-ccw"` directions** — radar-sweep traversal; a radial line rotates from 12 o'clock (clockwise or counter-clockwise); brightness per strip is the average along that ray; cursor is a clock-hand rectangle pivoted at the image centre
- **Progress bar** — mpv-style 4 px bar pinned to the bottom of the window; dark semi-transparent background track with a bright fill tracking playback position; toggled via `sonopix.opts.show_progress_bar` (default: `true`)
- **Audio export** — `-o / --output FILE` sonifies automatically then saves to WAV or OGG and closes; defaults to `.wav` if no extension given; prints an error and exits if no `--input` was provided

#### Lua scripting

- Add lua scripting support
    - `sonopix` table
    - `sonopix.opts` table
- **`sonopix.save_audio(filepath)`** Lua binding — saves audio from a script; blocks until sonification completes if still running
- **`sonopix.is_paused()` / `sonopix.is_stopped()`** Lua bindings added to match the existing `is_playing()`
- **`sonopix.opts.amplitude`** — master gain applied to the audio buffer after sonification (default: `1.0`, must be `>= 0`); captured at `sonify()` call time
- **`sonopix.opts.traversal_func`** — custom pixel-level traversal; called **once per strip** by C++ with `(strip_index, total, width, height)`; return `(x, y)` for that strip — no table allocation; a `cursor_width × cursor_width` point cursor tracks the current pixel during playback
- **`sonopix.opts.waveform`** sub-table — `visible`, `height`, `color`; supports both inline and table-form assignment
- **`sonopix.opts.oscilloscope`** sub-table — `visible`, `height`, `window_samples`, `color`; supports both inline and table-form assignment
- **`sonopix.opts.progress_bar`** sub-table — `visible`, `height`, `color`; replaces the old boolean-only `show_progress_bar` flag (which is kept for backwards compatibility)
- `sonopix.opts.antialiasing_level` — sets MSAA sample count (0 = off, 2/4/8 typical); applied at window creation so it takes effect without recreating the window
- `sonopix.opts.sonify_func` — custom sonification function; called per sample with a reused `SonifyContext` table (`brightness`, `x`, `y`, `width`, `height`, `fmin`, `fmax`, `scale`, `sample_rate`, `sample_index`, `frame_index`) and must return a float in `[-1, 1]`
- `sonopix.opts.frequency` sub-table with `fmin`, `fmax`, and `scale` fields
    - Supports both inline (`sonopix.opts.frequency.fmin = 200`) and table form (`sonopix.opts.frequency = { fmin = 200, fmax = 8000, scale = "log" }`)
- `sonopix.opts.cursor` sub-table with `width` and `color` fields
    - Supports both inline (`sonopix.opts.cursor.width = 3`) and table form (`sonopix.opts.cursor = { width = 3, color = "#FF0000" }`)
- `sonopix.opts = { ... }` table assignment now works for all opts including nested `cursor` and `frequency`
- **Image rotation** — `sonopix.opts.image_rotation` sets the display angle in degrees (default `0`); image pivots around its center; scale is computed from the axis-aligned bounding box of the rotated image so edges never clip; cursor tracks pixel positions on the rotated image using the sprite transform; rotation is now baked into the sonification pixel buffer so `sonify_func` sees the rotated image
- **`sonopix.opts.sonify_func` is now per-strip** — called once per strip (not per sample); receives `ctx` with `n_samples`; must return a table of `n_samples` floats; 100–1000× fewer Lua/C++ boundary crossings; `strip_t` removed (compute as `(i-1)/ctx.n_samples` if needed)
- **`SonifyContext` exposes full pixel colour** — `r`, `g`, `b` (linear `[0, 1]`) and `h` (hue `[0, 360]`), `s`, `v` (HSV saturation/value `[0, 1]`) are now documented and available in the Lua context alongside `brightness`; enables timbre to be shaped by colour, not just luminance
- **`sonopix.opts.volume`** — master playback volume `[0, 100]` (default `100`); backed by `sf::Sound::setVolume`, takes effect immediately during playback without re-sonifying; distinct from `amplitude` which is baked into the buffer at sonify time
- **`sonopix.opts.fps`** — framerate limit (default `60`); `0` = unlimited; uses `setFramerateLimit` (sleep-based throttling) so it works on Wayland where vsync via `GLX_EXT_swap_control` is unavailable
- **Stereo output** — `sonopix.opts.channel_count = 2` enables stereo; `sonify_func` must return `n_samples * channel_count` interleaved samples (`[L1, R1, L2, R2, ...]`); `ctx.channel_count` is exposed so the function can branch on mono vs. stereo; built-in sine oscillator duplicates across channels; cursor, waveform playhead, and progress bar all account for channel count correctly
- **`sonopix.pixel_brightness(x, y)`** — returns brightness `[0, 1]` of the pixel at `(x, y)` in the loaded image; intended for use in `traversal_func` setup to build data-driven orderings (e.g. brightest-first sort)
- **`sonopix.opts.audio_effects.process_func`** — custom DSP function called once after all built-in effects; receives `(samples, sample_rate)` and returns a modified samples table; runs in the sonify thread
- **`sonopix.opts.audio_effects`** sub-table — post-sonification audio DSP: `gain` (master multiplier), `delay` (`{time, feedback, mix}`), `reverb` (`{room_size, damping, mix}` — 4-comb Schroeder), `distortion` (`{drive, mix}` — tanh soft clip); effects are applied in the sonify thread in distortion → reverb → delay order; `mix = 0` skips each effect; implemented in `src/Effects.cpp`
- **`sonopix.opts.image_effects`** sub-table — real-time GPU image effects via GLSL fragment shader: `grayscale` (boolean), `brightness`, `saturation`, `contrast`, `hue`, `blur` (Gaussian), `sharpen` (Laplacian), `threshold` (luminance cutoff), `invert` (boolean); supports both inline (`sonopix.opts.image_effects.blur = 2`) and table form; gracefully disabled if shaders are unavailable; shader source lives in `src/shaders/image_effects.cpp`

### Bug Fixes

- Fix crash (`std::length_error` in `vector::resize`) when a Lua script calls `open_file` before the window is created — `open_file` now falls back to `m_window_size` instead of `m_window.getSize()` when the window is not yet open
- Fix negative scale in `rescale_recenter_image` when reserved bottom strip height exceeds window height — `available_h` and `scale` are now clamped to `≥ 0`
- Fix `zigzag-h` / `zigzag-v` still listed in `--direction` CLI choices after removal
- Fix keyboard scrubbing (`←` / `→`) using fixed seconds — now seeks by 2 % / 10 % of total duration so the step is appropriate regardless of audio length
- Fix `sonify()` copying audio buffer — now uses `take_audio()` to move instead
- Fix `set_cursor_width` not updating the live cursor shape
- Fix `~` home directory expansion in `load_image` — guarded on `[0] == '~'` and passed `fixed_filename` to `loadFromFile`
- Fix `sonopix.pause()` missing from Lua bindings
- Fix `toggle_pause` using stale `m_paused` flag — now reads `is_playing()` directly from the audio engine
- Fix Lua `sonify_func` errors silently swallowed — failures now print to stderr
- Fix `sonopix.opts` metatable not being attached (global was already popped when opts table was set)
- Fix `sonopix.opts = { ... }` not applying options — `__newindex` was never triggered because `"opts"` existed as a raw key in the `sonopix` table
- Fix setting `direction` via Lua not updating cursor orientation — `MainWindow::m_direction` was not updated, only the sonifier's copy
- Fix `freq_scale` never reaching `SonifyContext` — custom sonify functions always saw `LINEAR` regardless of what was set
- Fix `FreqMap::min` default of `0.0f` causing divide-by-zero in log/exponential scale
- Fix pausing playing from lua not working properly
- Fix circle cursor looking polygonal — point count now scales with circumference (~1 vertex per 1.5 px) instead of the fixed default of 30
- Fix circle cursor ring drifting outward as radius grows — outline is now centred on the ring boundary by subtracting half stroke width from the shape radius
- Fix circle cursor disappearing after window resize when audio is paused or stopped — `m_last_sample_index` now tracks the last valid playback position so `move_cursor` can restore the ring radius after `init_cursor` resets it
