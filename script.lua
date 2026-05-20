local s = sonopix

local opened = s.open_file("/home/neo/Downloads/abstract-whale-png.png")
if opened then
    s.opts = {
        direction = "top-to-bottom",
        frequency_range = { 20, 20000 },
        spu = 0.1,
        amplitude = 0.5,
    }

    if s.sonify() then
        s.play()
    end
end
