# YUNO GASAI: BLOOD MOON

A Mega Drive / Genesis game prototype built with SGDK.

## Current flow

1. Static title splash with PRESS START.
2. Main menu with START and SETTINGS.
3. START opens the plain white playfield with the six-part Yuno idle metasprite.

## Current playable test scene

- Solid white background across the playfield.
- No room image, wall, floor, furniture, clock, time-of-day system, GIF or intro animation.
- Yuno is rendered as six separate hardware sprites assembled as one 40x120-pixel metasprite.
- The six sprite parts use one shared 4-bit palette with transparent index 0 on PAL3.
- Yuno moves freely in four directions with the D-pad; the RUN button increases movement speed.
- START opens the pause menu; B returns to the main menu.

The original small `yuno` resource remains in `res/resources.res` for compatibility, but the playable scene uses `yuno_meta_*`. The new module owns and releases only its six sprite handles. It does not initialize, end, reset, or globally clear the shared Sprite Engine.

## Build and download

GitHub Actions builds SGDK with the Marsdev toolchain. A successful run publishes the ROM, checksum and MD.emu ZIP as workflow artifacts.

Expected ROM output: `out/rom.bin`.

A successful build still needs an emulator play-test before it can be called tested.
