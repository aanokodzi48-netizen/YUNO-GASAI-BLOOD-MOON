# YUNO GASAI: BLOOD MOON

A Mega Drive / Genesis game prototype built with SGDK.

## Current flow

1. The ten-frame opening animation from `res/frame_*.gif`.
2. Yuno title splash.
3. Main menu with START and SETTINGS.
4. START opens a plain white playfield with Yuno.

## Current playable test scene

- Pure white background across the full 320x224 playfield.
- No room image, wall, floor, furniture, clock, time-of-day system or object prompts.
- Yuno can move freely across the screen with the D-pad.
- The configured RUN button increases movement speed.
- START opens the pause menu; B returns to the main menu.

The current small placeholder Yuno sprite is intentionally unchanged. A taller
character sprite and detailed environments will be handled in later stages.

## Animated opening

The ten uploaded GIFs in `res/frame_*.gif` are converted during CI by
`tools/make_intro_frames.py` into 208x144, 4-bit indexed PNG images. All frames
share one 16-color Mega Drive palette. The animation is centered on a black
screen. The next frame is streamed into an inactive VRAM buffer in small VBlank
DMA chunks before the displayed frame is switched. START or A skips it.

## Build and download

GitHub Actions builds SGDK with the Marsdev toolchain. A successful run
publishes the ROM, checksum and MD.emu ZIP as workflow artifacts.

Expected ROM output: `out/rom.bin`.

A successful build still needs an emulator play-test before it can be called tested.
