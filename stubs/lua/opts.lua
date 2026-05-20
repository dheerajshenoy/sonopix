---@meta
sonopix = sonopix or {}

---@class SonifyContext
---@field sample_rate number Sample rate in Hz
---@field brightness number Pixel brightness in [0, 1]
---@field x integer Current column (or ring radius for circle modes)
---@field y integer Current row
---@field width integer Image width in pixels
---@field height integer Image height in pixels
---@field strip_index integer Playback-order index of this strip (0 = first strip played, direction-independent)
---@field strip_count integer Total number of strips
---@field t number Time in seconds since the start of audio
---@field strip_t number Normalized position within the current strip [0, 1)
---@field fmin number Minimum frequency in Hz
---@field fmax number Maximum frequency in Hz
---@field scale "linear"|"log"|"exponential" Frequency mapping scale

---@class SonopixCursorOpts
---@field width? number Width of the playback cursor in pixels (must be > 0)
---@field color? string Cursor color as "#RRGGBB" or "#RRGGBBAA" (default: "#FF000080")

---@class FrequencyOpts
---@field min? number Minimum frequency in Hz (must be > 0 and < max)
---@field max? number Maximum frequency in Hz (must be > min)
---@field scale? "linear"|"log"|"exponential" Frequency mapping scale (default: "linear")

---@class SonopixOpts
---@field direction? "left-to-right"|"right-to-left"|"top-to-bottom"|"bottom-to-top"|"circle-outwards"|"circle-inwards"|"zigzag-h"|"zigzag-v" Scan direction
---@field frequency? FrequencyOpts Frequency mapping options
---@field spu? number Seconds per unit (column or row); must be > 0
---@field cursor? SonopixCursorOpts Cursor appearance options
---@field sample_rate? number Sample rate in Hz (must be > 0)
---@field channel_count? integer Number of audio channels (must be > 0)
---@field amplitude? number Master gain applied to the audio buffer after sonification (default: 1.0, must be >= 0)
---@field show_progress_bar? boolean Show/hide the mpv-style progress bar overlaid at the bottom of the image (default: true)
---@field antialiasing_level? integer MSAA sample count (0 = off, 2/4/8 typical); applied at window creation
---@field traversal_func? fun(strip_index: integer, total: integer, width: integer, height: integer): integer, integer Custom pixel traversal; called once per strip with (strip_index, total, width, height); return (x, y) for that strip
---@field sonify_func? fun(ctx: SonifyContext): number Custom sonification function; receives context per sample and returns a float in [-1, 1]

---@type SonopixOpts
sonopix.opts = {}
