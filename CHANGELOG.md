# CHANGELOG

## 0.1

### Features

- Show image
- Command line arguments
- Sonification Engine
- Traversals
    - LEFT TO RIGHT
    - RIGHT TO LEFT
- Image Resizing
- Add lua scripting support
    - `sonopix` table
    - `sonopix.opts` table

### Bug Fixes

- Fix `sonopix.opts` metatable not being attached (global was already popped when opts table was set)
