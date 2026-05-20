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
- Image Resizing

#### Lua scripting

- Add lua scripting support
    - `sonopix` table
    - `sonopix.opts` table
- `sonopix.opts.frequency_range` — set min/max frequency in Hz as `{ fmin, fmax }`
- `sonopix.opts.cursor` sub-table with `width` and `color` fields
    - Supports both inline (`sonopix.opts.cursor.width = 3`) and table form (`sonopix.opts.cursor = { width = 3, color = "#FF0000" }`)
- `sonopix.opts = { ... }` table assignment now works for all opts including nested `cursor`

### Bug Fixes

- Fix `sonopix.opts` metatable not being attached (global was already popped when opts table was set)
- Fix `sonopix.opts = { ... }` not applying options — `__newindex` was never triggered because `"opts"` existed as a raw key in the `sonopix` table
- Fix setting `direction` via Lua not updating cursor orientation — `MainWindow::m_direction` was not updated, only the sonifier's copy
- Fix pausing playing from lua not working properly
