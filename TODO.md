# Sonopix TODO

## Audio

- [x] **Export to file** — save sonified audio as WAV/OGG via CLI `-o` flag or `sonopix.save_audio(path)` from Lua
- [ ] **Stereo output** — map x position to left/right pan, or sonify left and right channels independently
- [ ] **Per-channel sonification** — treat R, G, B channels as separate brightness signals; mix or layer them
- [ ] **Amplitude control** — expose `sonopix.opts.amplitude` (master gain, default 1.0)
- [ ] **Effects** — optional reverb/delay as post-processing on the audio buffer before playback

## Traversal

- [ ] **Diagonal traversal** — scan along diagonals (top-left → bottom-right, etc.)
- [ ] **Zigzag / serpentine** — like left-to-right but alternates direction each row (boustrophedon)
- [ ] **Hilbert curve** — space-filling curve traversal for better spatial locality
- [ ] **Custom traversal via Lua** — let `sonopix.opts.traversal_func` yield `(x, y)` pairs, replacing built-in scan order

## Image

- [ ] **Region of interest** — allow selecting a rectangle to sonify instead of the whole image
- [ ] **Pre-processing filters** — blur, sharpen, edge-detect, threshold before sonification
- [ ] **Grayscale mode options** — choose between luminance, average, max channel, or a single channel (R/G/B)

## UI

- [ ] **File picker** — open image without restarting via a native dialog (keyboard shortcut `O`)
- [ ] **Drag and drop** — drop an image onto the window to open it
- [ ] **Playback progress bar** — thin bar at the bottom of the window showing current position
- [ ] **Spectrum/waveform overlay** — show a live FFT or waveform of the generated audio
- [ ] **Zoom and pan** — allow zooming into the image and panning while it plays
- [ ] **Keybinding to re-open last file** — useful when iterating on a Lua script

## Lua

- [ ] **Hot-reload** — watch the script file and re-execute on change without restarting
- [ ] **Event hooks** — `sonopix.on_play`, `sonopix.on_stop`, `sonopix.on_strip` callbacks
- [ ] **Lua REPL** — in-app console to run Lua commands live while audio plays
- [ ] **`sonopix.opts.traversal_func`** — custom traversal (see Traversal above)

## MIDI / OSC

- [ ] **MIDI output** — emit MIDI note-on/off events instead of or alongside audio
- [ ] **OSC output** — send brightness and position data as OSC messages for use in external synths

## Misc

- [ ] **Session save/load** — persist current opts and script path to a JSON/TOML file
- [ ] **Multiple images** — load a sequence and sonify them in order (slideshow mode)
- [ ] **`--output` implementation** — finish the CLI flag that writes audio to disk
