# YUNO GASAI: BLOOD MOON

A Mega Drive / Genesis game prototype built with SGDK.

## Current flow

1. Yuno title splash.
2. Main menu with START and SETTINGS.
3. Settings let the player assign the RUN action to A, B, or C.
4. START loads Yuno's bedroom.

## Bedroom prototype

- 320x224 4-bit indexed Mega Drive room backgrounds generated from tools/generate_room_assets.py; lamp-on/off variants share the same 16-color palette.
- D-pad movement with a compact foot-level collision box.
- Solid collision footprints for the entrance door, wardrobe, bedside cabinet, bed, and bookcase.
- Context interaction button is A by default. If A is assigned to RUN, C becomes the interaction button.
- Lamp can be toggled on/off without reloading the room background.
- Bed starts a short rest sequence, turns the lamp off, and transitions the window view to morning.
- Wardrobe is present and solid; outfit selection is planned for a later update.

The current small placeholder Yuno sprite is intentionally unchanged. A taller character sprite and clothing sets will be a separate later milestone.

## Build and download

GitHub Actions builds SGDK with the Marsdev toolchain. A successful run publishes the ROM, checksum, and MD.emu ZIP as workflow artifacts.

Expected ROM output: `out/rom.bin`.

A successful build still needs an emulator play-test before it can be called tested.
