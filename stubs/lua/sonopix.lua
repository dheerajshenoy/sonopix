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

---@function Checks if Sonopix is currently playing audio
---@returns boolean Returns true if audio is currently playing, false otherwise
sonopix.is_playing = function () end

---@function Checks if Sonopix is currently paused
---@returns boolean Returns true if audio is currently paused, false otherwise
sonopix.is_paused = function () end

----@function Checks if Sonopix is currently stopped
---@returns boolean Returns true if audio is currently stopped, false otherwise
sonopix.is_stopped = function () end

---@function Gets the current time position of the audio in Sonopix
---@returns number Returns the current time position in seconds
sonopix.current_time = function () end

