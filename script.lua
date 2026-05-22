local s = sonopix

local random_file_from_dir = function(dir)
    local p = io.popen('find "' .. dir .. '" -maxdepth 1 -type f')
    if not p then return nil end
    local files = {}
    for file in p:lines() do
        files[#files + 1] = file
    end
    p:close()
    if #files == 0 then return nil end
    return files[math.random(1, #files)]
end

-- Zigzag (serpentine): scans left→right on even rows, right→left on odd rows.
-- Each call returns the (x, y) coordinate of the i-th pixel in that order.
-- Anti-diagonal stripes: sweeps top-right → bottom-left diagonals,
-- starting from the top-left corner. Precomputes the order once per image size.
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

-- Additive synthesis: four harmonics with a falling amplitude series
-- (1, 1/2, 1/3, 1/4) — warm, organ-like tone that tracks image brightness.
local phases = { 0.0, 0.0, 0.0, 0.0 }
local function sonify(ctx)
    local f = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    local samples = {}
    for i = 1, ctx.n_samples do
        local sum = 0.0
        for k = 1, #phases do
            phases[k] = phases[k] + 2 * math.pi * f * k / ctx.sample_rate
            sum = sum + math.sin(phases[k]) / k
        end
        samples[i] = ctx.brightness * sum * 0.48  -- ~1/sum(1/k,k=1..4)
    end
    return samples
end

s.opts = {
    -- traversal_func overrides direction; keep direction commented out.
    -- traversal_func = diagonal,
    sonify_func    = sonify,
    -- 5e-6 s/pixel ≈ 1 sample/pixel at 44100 Hz — keeps audio duration sane.
    -- spu       = 5e-6,
    spu=1e-3,
    frequency = { min = 20, max = 20000, scale = "exponential" },
    cursor    = { width = 3, color = "#FF5000FF" },
    waveform        = { height = 0.12, color = "#FFFFFFC8" },
    oscilloscope    = { height = 0.10, window_samples = 2048, color = "#00FFB4DC" },
    progress_bar    = { height = 0.01, color = "#FF8800FF" },
    antialiasing_level = 16,
}

-- local random_file = random_file_from_dir("/home/neo/Gits/wallpapers/")

s.on("sonify_complete", function()
    s.play()
end)

if s.open_file("/home/neo/Downloads/space.webp") then
    s.sonify()
end
