---@meta
sonopix = sonopix or {}

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
