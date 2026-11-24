# YUNO GASAI: BLOOD MOON

A Mega Drive / Genesis action-game prototype built with SGDK.

## First playable prototype

- Title screen and restart flow
- Player movement, running, jumping, and crouching
- A-button punch and C-button kick
- Approaching ninja enemy with health and contact damage
- Three enemy waves, pause, game-over, and victory screens
- Designed for a standard Mega Drive controller and MD.emu

## Controls

| Input | Action |
|---|---|
| D-pad Left / Right | Move |
| D-pad Up | Jump |
| D-pad Down | Crouch |
| A | Punch |
| C | Kick |
| Hold B + Left / Right | Run |
| START | Start / Pause |

## Build and download

The GitHub Actions workflow builds SGDK with the Marsdev toolchain. On a successful run it publishes the ROM, checksum, and MD.emu ZIP as downloadable workflow artifacts.

Expected ROM output: `out/rom.bin`.

## Development status

This is the first mechanics prototype, not the finished art/audio-rich game. Character animation, detailed stages, Russian story scenes, music, and voice reactions remain future development tasks. The ROM must pass the automated build before it can be called compiled; it still needs an emulator play-test before it can be called tested.
