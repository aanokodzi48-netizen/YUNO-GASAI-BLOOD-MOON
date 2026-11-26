# YUNO GASAI: BLOOD MOON

A Mega Drive / Genesis game prototype built with SGDK.

## Current flow

1. Yuno title splash.
2. Main menu with START and SETTINGS.
3. Settings currently allow changing the RUN button between A, B and C, or restoring the default.
4. START loads the bedroom scene and the player.

The title artwork is stored as a 320x224, 4-bit indexed PNG with 16 Mega Drive-compatible palette colors.

## Current controls

| Input | Action |
|---|---|
| D-pad | Move around the room |
| Configurable A/B/C button | Run |
| START | Confirm on splash/menu; pause in game |
| A | Confirm menu/settings selection |
| B | Back from settings or pause |

## Build and download

GitHub Actions builds SGDK with the Marsdev toolchain. A successful run publishes the ROM, checksum, and MD.emu ZIP as workflow artifacts.

Expected ROM output: `out/rom.bin`.

## Development status

This is an evolving prototype. The home scene, character art, object interactions, and persistent control settings will be developed in later steps. A successful build still needs an emulator play-test before it can be called tested.
