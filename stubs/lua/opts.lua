---@meta
sonopix = sonopix or {}
sonopix.opts = {}

---@class FrequencyRange
---A frequency range for sonification, defined by a minimum and maximum frequency in Hz
---@field min number The minimum frequency in Hz (e.g., 20)
---@field max number The maximum frequency in Hz (e.g., 20000)

---@class Directions
Directions = {
    "left-to-right",
    "right-to-left",
    "top-to-bottom",
    "bottom-to-top",
    "circle-inward",
    "circle-outward",
}

---@function Sets the direction for sonification in Sonopix
---@overload fun(): Directions Returns the current direction for sonification
---@param method Directions The direction method to use
sonopix.opts.direction = function (method) end

---@function Sets the seconds per unit for sonification in Sonopix
---@overload fun(): number Returns the current seconds per unit
---@param spu number The seconds per unit to use
sonopix.opts.spu = function (spu) end

sonopix.opts.cursor = {}

---@function Sets the color of the cursor in Sonopix
---@overload fun(): string Returns the current color of the cursor
---@param color string The color to set for the cursor (e.g., "#FF0000" for red)
sonopix.opts.cursor.color = function (color) end

---@function Sets the size of the cursor in Sonopix
---@overload fun(): number Returns the current size of the cursor
---@param size number The size to set for the cursor (e.g., 5 for a 5-pixel cursor)
sonopix.opts.cursor.size = function (size) end

---@function Sets the frequency range for sonification in Sonopix
---@overload fun(): FrequencyRange Returns the current frequency range for sonification
---@param range FrequencyRange The frequency range to use for sonification (e.g., { min = 20, max = 20000 })
sonopix.opts.freq = function (range) end

sonopix.opts.
