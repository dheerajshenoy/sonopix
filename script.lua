local s = sonopix

-- Zigzag (serpentine) — alternates horizontal direction each row
-- traversal_func signature: function(strip_index, total, w, h) -> x, y
-- local zigzag_traversal_func = function(i, total, w, h)
--     local row     = math.floor(i / w)
--     local col_raw = i % w
--     if row % 2 == 0 then
--         return col_raw, row
--     else
--         return w - 1 - col_raw, row
--     end
-- end
--
-- -- Random pixel order (with replacement)
-- local random_traversal_func = function(i, total, w, h)
--     return math.random(0, w - 1), math.random(0, h - 1)
-- end

local phase = 0.0
local sonify_func = function(ctx)
    local freq = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
    phase = phase + 2 * math.pi * freq / ctx.sample_rate
    return ctx.brightness * math.sin(phase)
end

s.opts = {
    image_rotation = 45,
    show_progress_bar = true,
    direction = "circle-outwards",
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

local random_file_from_dir = function(dir)
    local p = io.popen('find "' .. dir .. '" -type f')
    if not p then return nil end

    local files = {}
    for file in p:lines() do
        table.insert(files, file)
    end
    if #files == 0 then
        return nil
    end
    return files[math.random(1, #files)]
end

local random_file = random_file_from_dir("/home/dheeraj/Gits/wallpapers/")

if random_file then
    local opened = s.open_file(random_file)
    if opened then
        if s.sonify() then
            s.play()
        end
    end
end
