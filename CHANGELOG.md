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

- Add lua scripting support
    - `sonopix` table
    - `sonopix.opts` table
- `sonopix.opts.frequency` sub-table with `fmin`, `fmax`, and `scale` fields
    - Supports both inline (`sonopix.opts.frequency.fmin = 200`) and table form (`sonopix.opts.frequency = { fmin = 200, fmax = 8000, scale = "log" }`)
- `sonopix.opts.cursor` sub-table with `width` and `color` fields
    - Supports both inline (`sonopix.opts.cursor.width = 3`) and table form (`sonopix.opts.cursor = { width = 3, color = "#FF0000" }`)
- `sonopix.opts = { ... }` table assignment now works for all opts including nested `cursor` and `frequency`

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
