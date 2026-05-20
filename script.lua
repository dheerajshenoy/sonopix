local s = sonopix

local opened = s.open_file("/home/neo/Downloads/abstract-whale-png.png")
if opened then
    s.opts = {
        direction = "top-to-bottom",
        frequency_range = { 20, 2000 },
        cursor = {
            width = 5,
            color = "#FF5000FF",
        },
        spu = 0.005,
        amplitude = 0.5,
    }
    -- s.opts.direction = "top-to-bottom"

    if s.sonify() then
        s.play()
    end
end
