---@meta
sonopix = sonopix or {}

---@class SonopixCursorOpts
---@field width number Width of the playback cursor in pixels (must be > 0)
---@field color string Cursor color as "#RRGGBB" or "#RRGGBBAA" (default: "#FF000080")
sonopix.opts.cursor = {}

---@class SonopixOpts
---@field direction "left-to-right"|"right-to-left"|"top-to-bottom"|"bottom-to-top"|"circle-outwards"|"circle-inwards" Scan direction
---@field frequency_range number[] Two-element table {fmin, fmax} in Hz (must satisfy 0 < fmin < fmax)
---@field spu number Seconds per unit (column or row); must be > 0
---@field cursor SonopixCursorOpts Cursor appearance options
-- Options can be set individually or as a complete table:
--   sonopix.opts.direction = "top-to-bottom"
--   sonopix.opts = { direction = "top-to-bottom", spu = 0.01, frequency_range = { 200, 8000 }, cursor = { width = 3 } }
sonopix.opts = {}

---@function Loads an image file and opens it in Sonopix
---@param filepath string The path to the image file
---@return boolean Returns true if the file was successfully opened, false otherwise
sonopix.open_file = function (filepath) end

---@function Closes the currently open image file in Sonopix
sonopix.close_file = function () end

---@function Sonify the currently open image in Sonopix using the set options
---@returns boolean Returns true if the sonification was successful, false otherwise
sonopix.sonify = function () end

---@function Plays the sonified audio in Sonopix
sonopix.play = function () end

---@function Stops the currently playing audio in Sonopix
sonopix.stop = function () end

---@function Checks if Sonopix is currently playing audio
---@returns boolean Returns true if audio is currently playing, false otherwise
sonopix.is_playing = function () end

---@function Gets the current time position of the audio in Sonopix
---@returns number Returns the current time position in seconds
sonopix.current_time = function () end
