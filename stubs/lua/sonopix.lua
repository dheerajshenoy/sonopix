---@meta
sonopix = sonopix or {}

---Loads an image file and opens it in Sonopix
---@param filepath string Path to the image file
---@return boolean success True if the file was successfully opened
sonopix.open_file = function(filepath) end

---Returns the file path of the currently open image, or nil if no file is open
---@return string|nil
sonopix.file_path = function () end

---Closes the currently open image file
sonopix.close_file = function() end

---Sonifies the currently open image using the current opts
---@return boolean success True if sonification was successful
sonopix.sonify = function() end

---Plays the sonified audio
sonopix.play = function() end

---Pauses playback
sonopix.pause = function() end

---Stops playback and resets position
sonopix.stop = function() end

---Returns true if audio is currently playing
---@return boolean
sonopix.is_playing = function() end

---Returns true if audio is currently paused
---@return boolean
sonopix.is_paused = function() end

---Returns true if audio is currently stopped
---@return boolean
sonopix.is_stopped = function() end

---Returns the current playback sample index
---@return number
sonopix.current_time = function() end

---Returns the audio data as a int16 array
sonopix.audio_data = function () end

---Returns the audio data as a float32 array in the range [-1, 1]
sonopix.audio_dataf = function() end

---Saves the sonified audio to a file. Blocks until sonification is complete if still running.
---Supported formats: WAV, OGG.
---@param filepath string Output file path
---@return boolean success True if the file was written successfully
sonopix.save_audio = function(filepath) end

