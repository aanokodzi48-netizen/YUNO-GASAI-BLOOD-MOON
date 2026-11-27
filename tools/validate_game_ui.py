#!/usr/bin/env python3
"""Regression checks for the Mega Drive controls and image-backed phone UI."""
import re
import sys
from pathlib import Path

source = Path(sys.argv[1] if len(sys.argv) > 1 else "src/main.c").read_text(encoding="utf-8")
resources = Path(sys.argv[2] if len(sys.argv) > 2 else "res/resources.res").read_text(encoding="utf-8")

required = {
    "fixed movement step": "#define PLAYER_STEP 2",
    "phone contacts state": "STATE_PHONE_CONTACTS",
    "phone chat state": "STATE_PHONE_CHAT",
    "six-button X selection": "pressed&BUTTON_X",
    "C returns from chat": "pressed&BUTTON_C",
    "image-backed phone renderer": "VDP_drawImageEx(BG_A,&phone_ui",
    "high-priority phone image": "TILE_ATTR_FULL(PAL1,TRUE",
    "phone VRAM budget guard": "phone_ui.tileset->numTile",
    "phone user VRAM base": "TILE_USER_INDEX",
    "sprite VRAM boundary": "TILE_SPRITE_INDEX",
    "Yuno hidden on phone": "YunoMeta_setVisible(FALSE)",
    "Yuno restored on close": "YunoMeta_setVisible(TRUE)",
}
for label, token in required.items():
    if token not in source:
        raise SystemExit(f"FAIL: missing {label}")

for forbidden in (
    "RUN_SPEED", "runButton", "КНОПКА БЕГА", "cycleRunButton", "STATE_PAUSE",
    "phoneFillRect", "phonePatternTile", "phonePatternMap", "static void phoneTile",
):
    if forbidden in source:
        raise SystemExit(f"FAIL: obsolete speed selector or primitive phone code remains: {forbidden}")

if source.count("if(pressed&BUTTON_START)closePhone();") != 2:
    raise SystemExit("FAIL: START must close the phone from contacts and chat")
if "else if(pressed&BUTTON_X)" not in source:
    raise SystemExit("FAIL: X must open the selected contact")
if "else if(pressed&BUTTON_C)" not in source:
    raise SystemExit("FAIL: C must return from chat to contacts")
if 'IMAGE phone_ui "phone_ui.png" BEST' not in resources:
    raise SystemExit("FAIL: phone image missing from resources.res")
if "VDP_drawText(" in source:
    raise SystemExit("FAIL: default-font/English screen text remains")

glyph_marker = "static const u8 ruGlyphRows[RU_GLYPH_COUNT][8]={"
if glyph_marker not in source:
    raise SystemExit("FAIL: Russian glyph table not found")
glyph_block = source.split(glyph_marker, 1)[1].split("};", 1)[0]
glyphs = re.findall(r"\{([0-9,]+)\}", glyph_block)
if len(glyphs) != 51:
    raise SystemExit(f"FAIL: expected 51 glyphs, found {len(glyphs)}")
for index, glyph in enumerate(glyphs):
    values = [v.strip() for v in glyph.split(",")]
    if len(values) != 8 or any(not v.isdigit() or not 0 <= int(v) <= 255 for v in values):
        raise SystemExit(f"FAIL: malformed 8x8 glyph at index {index}")

print("OK: fixed movement, image-backed phone, Russian font, X/C/START transitions")
