---@meta
sonopix = sonopix or {}
sonopix.opts = {}

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
