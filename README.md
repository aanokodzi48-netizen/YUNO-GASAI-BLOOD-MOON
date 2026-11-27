# YUNO GASAI: BLOOD MOON

A Mega Drive / Genesis game prototype built with SGDK.

## Current flow

1. Static title splash with PRESS START.
2. Main menu with START and SETTINGS.
3. START opens a plain white playfield with Yuno.

## Current playable test scene

- Solid white background across the playfield.
- No room image, wall, floor, furniture, clock, time-of-day system, GIF or intro animation.
- Yuno can move freely in four directions with the D-pad.
- The configured RUN button increases movement speed.
- START opens the pause menu; B returns to the main menu.

The current small placeholder Yuno sprite is intentionally unchanged. A larger
character sprite, room art, time system, interactions, and audio are not part
of this test build yet.

## Build and download

GitHub Actions builds SGDK with the Marsdev toolchain. A successful run
publishes the ROM, checksum and MD.emu ZIP as workflow artifacts.

Expected ROM output: `out/rom.bin`.

A successful build still needs an emulator play-test before it can be called tested.
