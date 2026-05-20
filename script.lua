local s = sonopix

-- Zigzag (serpentine) — alternates horizontal direction each row
-- traversal_func signature: function(strip_index, total, w, h) -> x, y
local zigzag_traversal_func = function(i, total, w, h)
    local row     = math.floor(i / w)
    local col_raw = i % w
    if row % 2 == 0 then
        return col_raw, row
    else
        return w - 1 - col_raw, row
    end
end

-- Random pixel order (with replacement)
local random_traversal_func = function(i, total, w, h)
    return math.random(0, w - 1), math.random(0, h - 1)
end

local phase = 0.0
local sonify_func = function(ctx)
    local freq = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    phase = phase + 2 * math.pi * freq / ctx.sample_rate
    return ctx.brightness * math.sin(phase)
end

s.opts = {
    show_progress_bar = false,
    direction = "top-to-bottom",
    frequency = {
        min   = 20,
        max   = 20000,
        scale = "log",
    },
    cursor = {
        width = 5,
        color = "#FF5000FF",
    },
    sonify_func     = sonify_func,
    -- traversal_func  = zigzag_traversal_func,
    spu             = 1e-3,
}

local opened = s.open_file("/home/neo/Gits/wallpapers/WALLPAPER-2025042107243114.png")
if opened then
    if s.sonify() then
        s.play()
    end
end
