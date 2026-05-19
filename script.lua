local s = sonopix

local opened = s.open_file("/home/dheeraj/Downloads/1-bmp-sample-2.bmp")
if opened then
    s.traversal = "left-to-right"
    s.frequency_range = { 20, 20000 }
    s.samples_per_unit = 0.1
    s.duration = 5
    s.amplitude = 0.5

    if s.sonify() then
        s.play()
    end
end
