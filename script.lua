local s = sonopix

local diag_w, diag_h = 0, 0
local diag_x, diag_y = {}, {}

local function diagonal(i, _, w, h)
    if w ~= diag_w or h ~= diag_h then
        diag_w, diag_h = w, h
        diag_x, diag_y = {}, {}
        local idx = 1
        for d = 0, w + h - 2 do
            for x = math.max(0, d - (h - 1)), math.min(d, w - 1) do
                diag_x[idx] = x
                diag_y[idx] = d - x
                idx = idx + 1
            end
        end
    end
    return diag_x[i + 1], diag_y[i + 1]
end

-- Chromatic stellar sonar: each star fires a decaying FM ping whose timbre
-- is shaped by the star's color.
--
--   Frequency  — brightness-mapped (dim star = low pitch, bright = high)
--   FM ratio   — hue-driven: red(0°)→1.5 (warm/harmonic),
--                            blue(240°)→~2.1 (metallic/inharmonic)
--                smooth sinusoidal interpolation, so the orange-red and
--                blue stars sound distinctly different.
--   FM index   — saturation-driven: white/grey stars (s≈0) → near-pure sine;
--                vivid colored stars → rich sidebands.
--   Decay      — ~120 ms; FM index shrinks with envelope for natural zing→tail.
--   Drone      — whisper-quiet bass sine tracking the rolling average, giving
--                the void of space a faint atmosphere.
local drone_phase    = 0.0
local ping_phase     = 0.0
local ping_mod_phase = 0.0
local ping_env       = 0.0
local ping_freq      = 440.0
local ping_ratio     = 2.1
local ping_idx_max   = 0.0
local avg_bright     = 0.05

---@param ctx SonifyContext
local function sonify_chromatic(ctx)
    avg_bright = avg_bright * 0.97 + ctx.brightness * 0.03

    local excess = ctx.brightness - avg_bright * 1.5
    if excess > 0.02 then
        ping_env     = math.min(1.0, ping_env + excess * 4.0)
        ping_freq    = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
        -- sin(h * 0.5 deg) rises from 0 at red(0°) to peak near green(180°)
        -- and lands at ~0.87 at blue(240°) → ratio sweeps 1.5 → 2.2
        ping_ratio   = 1.5 + 0.7 * math.sin(math.rad(ctx.h * 0.5))
        -- white stars (s≈0) stay near pure sine; colorful stars get rich FM
        ping_idx_max = ctx.s * 7.0
    end

    local drone_freq = ctx.fmin * (ctx.fmax / ctx.fmin) ^ (avg_bright * 0.3)
    local decay      = math.exp(-1.0 / (ctx.sample_rate * 0.12))
    -- x position → stereo pan: left edge = hard left, right edge = hard right
    local pan_r      = ctx.x / ctx.width
    local pan_l      = 1.0 - pan_r

    local samples = {}
    for i = 1, ctx.n_samples do
        drone_phase = drone_phase + 2 * math.pi * drone_freq / ctx.sample_rate
        local drone = 0.06 * math.sin(drone_phase)

        ping_mod_phase = ping_mod_phase + 2 * math.pi * ping_freq * ping_ratio / ctx.sample_rate
        ping_phase     = ping_phase     + 2 * math.pi * ping_freq              / ctx.sample_rate
        local index = ping_env * ping_idx_max
        local ping  = ping_env * math.sin(ping_phase + index * math.sin(ping_mod_phase))
        ping_env = ping_env * decay

        local s = drone + ping
        if ctx.channel_count == 2 then
            samples[i * 2 - 1] = pan_l * s   -- left
            samples[i * 2]     = pan_r * s   -- right
        else
            samples[i] = s
        end
    end
    return samples
end

s.opts = {
    -- traversal_func = diagonal,
    -- sonify_func    = sonify_chromatic,
    amplitude = 0.5,
    fps = 144,
    loop = true,
    direction = "left-to-right",
    channel_count  = 2,
    spu            = 1e-3,
    image_effects = {
        invert = true,
    },
    frequency      = { min = 99, max = 100, scale = "linear" },
    cursor         = { width = 3, color = "#FF5000FF" },
    waveform       = { height = 0.12, color = "#FFFFFFC8" },
    oscilloscope   = { height = 0.10, window_samples = 2048, color = "#00FFB4DC" },
    progress_bar   = { height = 0.01, color = "#FF8800FF" },
    antialiasing_level = 12,
}

s.on("sonify_complete", function()
    s.play()
end)

if s.open_file("/home/neo/Downloads/vbar.png") then
    s.sonify()
end
