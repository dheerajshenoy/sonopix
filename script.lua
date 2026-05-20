local s = sonopix


sonopix.opts.sonify_func = function(ctx)
    local freq = ctx.fmin + ctx.brightness * (ctx.fmax - ctx.fmin)
    -- ctx fields: brightness, x, y, width, height, fmin, fmax, scale,
    --             sample_rate, sample_index, frame_index
    return ctx.brightness * math.cos(2 * math.pi * freq * ctx.sample_index / ctx.sample_rate)
end


local opened = s.open_file("/home/neo/Downloads/abstract-whale-png.png")
if opened then
    s.opts = {

        direction = "circle-outwards",
        -- direction = "top-to-bottom",
        frequency = {
            min = 20,
            max = 20000,
            scale = "log",
        },
        cursor = {
            width = 5,
            color = "#FF5000FF",
        },
        spu = 1e-3,
        amplitude = 0.5,
    }
    -- s.opts.direction = "top-to-bottom"

    if s.sonify() then
        s.play()
    end
end
