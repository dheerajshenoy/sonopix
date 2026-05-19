---@meta
sonopix = sonopix or {}

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
