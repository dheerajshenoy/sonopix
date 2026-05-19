---@meta
sonopix = sonopix or {}
sonopix.opts = {}

---@class TraversalMethod
TraversalMethod = {
    "left-to-right",
    "right-to-left",
    "top-to-bottom",
    "bottom-to-top",
    "circle-inward",
    "circle-outward",
}

---@function Sets the traversal method for sonification in Sonopix
---@overload fun(): TraversalMethod Returns the current traversal method
---@param method TraversalMethod The traversal method to use
sonopix.opts.traversal = function (method) end
