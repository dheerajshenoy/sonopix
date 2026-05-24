# Sonopix TODO

## Audio

- [x] **Export to file** — save sonified audio as WAV/OGG via CLI `-o` flag or `sonopix.save_audio(path)` from Lua
- [ ] **Stereo output** — map x position to left/right pan, or sonify left and right channels independently
- [ ] **Per-channel sonification** — treat R, G, B channels as separate brightness signals; mix or layer them
- [x] **Amplitude control** — expose `sonopix.opts.amplitude` (master gain, default 1.0)
- [x] **Effects** — optional reverb/delay as post-processing on the audio buffer before playback

## Traversal

- [x] **Diagonal traversal** — scan along diagonals (top-left → bottom-right, etc.)
- [x] **Zigzag / serpentine** — like left-to-right but alternates direction each row (boustrophedon)
- [x] **Hilbert curve** — space-filling curve traversal for better spatial locality
- [x] **Custom traversal via Lua** — let `sonopix.opts.traversal_func` yield `(x, y)` pairs, replacing built-in scan order

## Image

- [x] **Region of interest** — allow selecting a rectangle to sonify instead of the whole image
- [x] **Pre-processing filters** — blur, sharpen, edge-detect, threshold before sonification
- [x] **Grayscale mode options** — choose between luminance, average, max channel, or a single channel (R/G/B)

## UI

- [x] **Playback progress bar** — thin bar at the bottom of the window showing current position
- [x] **Spectrum/waveform overlay** — show a live FFT or waveform of the generated audio
- [ ] **Zoom and pan** — allow zooming into the image and panning while it plays

## Lua

- [x] **Hot-reload** — watch the script file and re-execute on change without restarting
- [x] **Event hooks** — `sonopix.on_play`, `sonopix.on_stop`, `sonopix.on_strip` callbacks
- [ ] **Lua REPL** — in-app console to run Lua commands live while audio plays
- [x] **`sonopix.opts.traversal_func`** — custom traversal (see Traversal above)

## MIDI / OSC

- [ ] **MIDI output** — emit MIDI note-on/off events instead of or alongside audio
- [ ] **OSC output** — send brightness and position data as OSC messages for use in external synths

## Misc

- [ ] **Multiple images** — load a sequence and sonify them in order (slideshow mode)
- [ ] **`--output` implementation** — finish the CLI flag that writes audio to disk
