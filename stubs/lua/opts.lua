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

---@class WaveformOpts
---@field visible? boolean Show/hide the waveform strip (default: true)
---@field height? integer Height of the waveform strip in pixels (default: 40)
---@field color? string RMS bar color as "#RRGGBB" or "#RRGGBBAA" (default: "#FFFFFFC8")

---@class OscilloscopeOpts
---@field visible? boolean Show/hide the oscilloscope strip (default: true)
---@field height? integer Height of the oscilloscope strip in pixels (default: 60)
---@field window_samples? integer Number of audio samples shown at once (default: 4096)
---@field color? string Line color as "#RRGGBB" or "#RRGGBBAA" (default: "#00FFB4DC")

---@class ProgressBarOpts
---@field visible? boolean Show/hide the progress bar (default: true)
---@field height? integer Height of the progress bar in pixels (default: 4)
---@field color? string Fill color as "#RRGGBB" or "#RRGGBBAA" (default: "#FFFFFFC8")

---@class ImageEffectsOpts
---@field grayscale?  number Desaturate toward greyscale; 0 = off, 1 = full greyscale (default: 0)
---@field brightness? number Additive brightness shift in [-1, 1] (default: 0)
---@field saturation? number Colour saturation multiplier; 0 = greyscale, 1 = original, >1 = vivid (default: 1)
---@field contrast?   number Contrast multiplier around mid-grey; 0 = flat, 1 = original (default: 1)
---@field hue?        number Hue rotation in degrees [0, 360) (default: 0)
---@field blur?       number Gaussian blur radius in texels; 0 = off (default: 0)
---@field sharpen?    number Laplacian sharpen strength; 0 = off (default: 0)
---@field threshold?  number Luminance cutoff [0, 1]: pixels above → white, below → black; negative = off (default: -1)
---@field invert?     boolean Invert all colours (default: false)

---@class AudioEffectsOpts
---@field gain? number Master gain applied to the audio buffer after sonification (default: 1.0, must be >= 0)
---@field delay? { time: number, feedback: number, mix: number } Simple delay effect with time in seconds, feedback amount [0, 1], and wet/dry mix [0, 1]
---@field reverb? { room_size: number, damping: number, mix: number } Simple reverb effect with room size [0, 1], damping [0, 1], and wet/dry mix [0, 1]
---@field distortion? { drive: number, mix: number } Simple distortion effect with drive amount [0, 1] and wet/dry mix [0, 1]

---@class SonopixOpts
---@field direction? "left-to-right"|"right-to-left"|"top-to-bottom"|"bottom-to-top"|"circle-outwards"|"circle-inwards"|"rotate-cw"|"rotate-ccw" Scan direction
---@field frequency? FrequencyOpts Frequency mapping options
---@field spu? number Seconds per unit (column or row); must be > 0
---@field cursor? SonopixCursorOpts Cursor appearance options
---@field waveform? WaveformOpts Full-audio waveform strip options
---@field oscilloscope? OscilloscopeOpts Live signal oscilloscope strip options
---@field progress_bar? ProgressBarOpts Playback progress bar options
---@field sample_rate? number Sample rate in Hz (must be > 0)
---@field channel_count? integer Number of audio channels (must be > 0)
---@field amplitude? number Master gain applied to the audio buffer after sonification (default: 1.0, must be >= 0)
---@field image_effects? ImageEffectsOpts Real-time image effects applied via GLSL shader
---@field audio_effects? AudioEffectsOpts Real-time audio effects applied in the audio callback (not yet implemented)
---@field loop? boolean Loop playback when the audio reaches the end (default: false)
---@field image_rotation? number Rotation of the displayed image in degrees (default: 0); cursor tracks the rotated image
---@field antialiasing_level? integer MSAA sample count (0 = off, 2/4/8 typical); applied at window creation
---@field window_title? string Window title string
---@field window_size? { width: integer, height: integer } Window dimensions in pixels
---@field traversal_func? fun(strip_index: integer, total: integer, width: integer, height: integer): integer, integer Custom pixel traversal; called once per strip with (strip_index, total, width, height); return (x, y) for that strip
---@field sonify_func? fun(ctx: SonifyContext): number Custom sonification function; receives context per sample and returns a float in [-1, 1]

---@type SonopixOpts
sonopix.opts = {}
