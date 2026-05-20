# CHANGELOG

## 0.1 (Unreleased)

### Features

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
- Image Resizing

#### Window

- `sonopix.opts.antialiasing_level` — sets MSAA sample count (0 = off, 2/4/8 typical); applied at window creation so it takes effect without recreating the window

#### Lua scripting

- `sonopix.opts.sonify_func` — custom sonification function; called per sample with a reused `SonifyContext` table (`brightness`, `x`, `y`, `width`, `height`, `fmin`, `fmax`, `scale`, `sample_rate`, `sample_index`, `frame_index`) and must return a float in `[-1, 1]`

- Add lua scripting support
    - `sonopix` table
    - `sonopix.opts` table
- `sonopix.opts.frequency` sub-table with `fmin`, `fmax`, and `scale` fields
    - Supports both inline (`sonopix.opts.frequency.fmin = 200`) and table form (`sonopix.opts.frequency = { fmin = 200, fmax = 8000, scale = "log" }`)
- `sonopix.opts.cursor` sub-table with `width` and `color` fields
    - Supports both inline (`sonopix.opts.cursor.width = 3`) and table form (`sonopix.opts.cursor = { width = 3, color = "#FF0000" }`)
- `sonopix.opts = { ... }` table assignment now works for all opts including nested `cursor` and `frequency`

### Bug Fixes (post-0.1)

- Fix `sonify()` copying audio buffer — now uses `take_audio()` to move instead
- Fix `set_cursor_width` not updating the live cursor shape
- Fix `~` home directory expansion in `load_image` — guarded on `[0] == '~'` and passed `fixed_filename` to `loadFromFile`
- Fix `sonopix.pause()` missing from Lua bindings
- Fix `toggle_pause` using stale `m_paused` flag — now reads `is_playing()` directly from the audio engine
- Fix Lua `sonify_func` errors silently swallowed — failures now print to stderr

### Refactor (post-0.1)

- `m_sonifier` and `m_audio_engine` converted from raw pointers to `std::unique_ptr`; destructor is `= default`
- Removed redundant `m_paused` member — audio engine state is the single source of truth
- `sonify()` now runs on a background thread via `std::async`; window title shows `[sonifying...]` while in progress

### Refactor

- `SonifyEngine`: extracted `pixel_brightness`, `column_brightness`, `row_brightness`, and `emit_strip` helpers — eliminates the four near-identical traversal functions
- `SonifyEngine`: traversal methods (`sonify_left_to_right`, etc.) moved to `private`
- `SonifyEngine`: removed redundant `sine_frequency()` sonify function
- `AudioEngine`: removed `sample_rate` constructor parameter and `set_sample_rate()` — rate is now supplied once via `set_data()`, eliminating the duplicated field between the two engines
- Cursor replaced with a `std::unique_ptr<sf::Shape>` — allocates `sf::RectangleShape` for linear directions and `sf::CircleShape` for circular directions

### Bug Fixes

- Fix `sonopix.opts` metatable not being attached (global was already popped when opts table was set)
- Fix `sonopix.opts = { ... }` not applying options — `__newindex` was never triggered because `"opts"` existed as a raw key in the `sonopix` table
- Fix setting `direction` via Lua not updating cursor orientation — `MainWindow::m_direction` was not updated, only the sonifier's copy
- Fix `freq_scale` never reaching `SonifyContext` — custom sonify functions always saw `LINEAR` regardless of what was set
- Fix `FreqMap::min` default of `0.0f` causing divide-by-zero in log/exponential scale
- Fix pausing playing from lua not working properly
- Fix circle cursor looking polygonal — point count now scales with circumference (~1 vertex per 1.5 px) instead of the fixed default of 30
- Fix circle cursor ring drifting outward as radius grows — outline is now centred on the ring boundary by subtracting half stroke width from the shape radius
- Fix circle cursor disappearing after window resize when audio is paused or stopped — `m_last_sample_index` now tracks the last valid playback position so `move_cursor` can restore the ring radius after `init_cursor` resets it
