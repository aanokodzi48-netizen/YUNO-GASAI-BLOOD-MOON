#!/usr/bin/env python3
"""Static regression checks for the Mega Drive menu and phone controls."""
import re
import sys
from pathlib import Path

source = Path(sys.argv[1] if len(sys.argv) > 1 else "src/main.c").read_text(encoding="utf-8")

required = {
    "fixed movement step": "#define PLAYER_STEP 2",
    "phone contacts state": "STATE_PHONE_CONTACTS",
    "phone chat state": "STATE_PHONE_CHAT",
    "six-button X selection": "pressed&BUTTON_X",
    "C returns from chat": "pressed&BUTTON_C",
    "high-priority phone tiles": "TILE_ATTR_FULL(PAL1,TRUE",
    "phone tile allocation guard": "TILE_SPRITE_INDEX",
    "Yuno hidden on phone": "YunoMeta_setVisible(FALSE)",
    "Yuno restored on close": "YunoMeta_setVisible(TRUE)",
}
for label, token in required.items():
    if token not in source:
        raise SystemExit(f"FAIL: missing {label}")

for forbidden in ("RUN_SPEED", "runButton", "КНОПКА БЕГА", "cycleRunButton", "STATE_PAUSE"):
    if forbidden in source:
        raise SystemExit(f"FAIL: obsolete speed/pause option remains: {forbidden}")

if source.count("if(pressed&BUTTON_START)closePhone();") != 2:
    raise SystemExit("FAIL: START must close the phone from both contacts and chat")

if 'if(pressed&BUTTON_X)' not in source or 'else if(pressed&BUTTON_C)' not in source:
    raise SystemExit("FAIL: phone button transitions are incomplete")

if "TITLE IMAGE ERROR" in source or 'VDP_drawText("' in source:
    raise SystemExit("FAIL: English/default-font text remains in the game UI")

match = re.search(r"ruGlyphRows\\[RU_GLYPH_COUNT\\]\\[8\\]=\\{([\\s\\S]*?)\\n\\};", source)
if not match:
    raise SystemExit("FAIL: Russian glyph table not found")
glyphs = re.findall(r"\\{([^{}]+)\\}", match.group(1))
if len(glyphs) != 51:
    raise SystemExit(f"FAIL: expected 51 glyphs, found {len(glyphs)}")
for index, glyph in enumerate(glyphs):
    values = [v.strip() for v in glyph.split(",")]
    if len(values) != 8 or any(not v.isdigit() or not 0 <= int(v) <= 255 for v in values):
        raise SystemExit(f"FAIL: malformed 8x8 glyph at index {index}")

print("OK: fixed movement, phone controls, priority, Yuno pause state and 51 Russian/Latin keypad glyphs")
