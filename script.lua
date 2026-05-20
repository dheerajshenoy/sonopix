local s = sonopix

local opened = s.open_file("/home/neo/Gits/wallpapers/WALLPAPER-2025042107243114.png")
if opened then
    s.opts = {
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
        spu = 1e-3,
    }

    local phase = 0.0
    s.opts.sonify_func = function(ctx)
        local freq = ctx.fmin * (ctx.fmax / ctx.fmin) ^ ctx.brightness
        phase = phase + 2 * math.pi * freq / ctx.sample_rate
        return ctx.brightness * math.sin(phase)
    end

    if s.sonify() then
        s.play()
    end
end
