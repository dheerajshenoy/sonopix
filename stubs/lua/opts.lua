---@meta
sonopix = sonopix or {}

---@class SonopixCursorOpts
---@field width? number Width of the playback cursor in pixels (must be > 0)
---@field color? string Cursor color as "#RRGGBB" or "#RRGGBBAA" (default: "#FF000080")
sonopix.opts.cursor = {}

---@class FrequencyOpts
---@field min? number Minimum frequency in Hz (must be > 0 and < fmax)
---@field max? number Maximum frequency in Hz (must be > fmin)
---@field scale? "linear"|"log"|"exponential" Frequency mapping scale (default: "linear")
sonopix.opts.frequency = {}

---@class SonopixOpts
---@field direction? "left-to-right"|"right-to-left"|"top-to-bottom"|"bottom-to-top"|"circle-outwards"|"circle-inwards" Scan direction
---@field frequency? FrequencyOpts Frequency mapping options
---@field spu? number Seconds per unit (column or row); must be > 0
---@field cursor? SonopixCursorOpts Cursor appearance options
---@field sample_rate? number Sample rate in Hz (must be > 0)
---@field channel_count? integer Number of audio channels (must be > 0)

sonopix.opts = {}
