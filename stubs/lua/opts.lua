---@meta
sonopix = sonopix or {}

---@class SonopixOpts
---@field direction? "left-to-right"|"right-to-left"|"top-to-bottom"|"bottom-to-top"|"circle-outwards"|"circle-inwards" Scan direction
---@field frequency_range? number[] Two-element table {fmin, fmax} in Hz (must satisfy 0 < fmin < fmax)
---@field spu? number Seconds per unit (column or row); must be > 0
---@field cursor? SonopixCursorOpts Cursor appearance options
---@field sample_rate? number Sample rate for sonification in Hz (must be > 0)
---@field channel_count? integer Number of audio channels (must be a positive integer)
-- Options can be set individually or as a complete table:
--   sonopix.opts.direction = "top-to-bottom"
--   sonopix.opts = { direction = "top-to-bottom", spu = 0.01, frequency_range = { 200, 8000 }, cursor = { width = 3 } }
sonopix.opts = {}

---@class SonopixCursorOpts
---@field width? number Width of the playback cursor in pixels (must be > 0)
---@field color? string Cursor color as "#RRGGBB" or "#RRGGBBAA" (default: "#FF000080")
sonopix.opts.cursor = {}
