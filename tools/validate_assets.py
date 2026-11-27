#!/usr/bin/env python3
"""Validate generated Mega Drive PNG assets and existing character sprites."""
import struct
import sys
import zlib
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MD_LEVELS = {0, 36, 73, 109, 146, 182, 219, 255}
STRICT_ASSETS = {
    "title.png", "room_bg.png", "window_morning.png",
    "lamp_glow.png", "sky_fx.png", "ui_tiles.png"
}
EXPECTED_SIZE = {
    "title.png": (320, 224),
    "room_bg.png": (320, 224),
    "window_morning.png": (112, 64),
    "lamp_glow.png": (32, 32),
    "sky_fx.png": (32, 16),
    "ui_tiles.png": (128, 32),
}
FX_ASSETS = {"window_morning.png", "lamp_glow.png", "sky_fx.png"}
INTRO_ASSETS = {f"intro_{index:02d}.png" for index in range(10)}
STRICT_ASSETS.update(INTRO_ASSETS)
for intro_name in INTRO_ASSETS:
    EXPECTED_SIZE[intro_name] = (208, 144)

def validate(path):
    path = Path(path)
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"{path}: invalid PNG signature")

    pos = 8
    seen_ihdr = False
    seen_iend = False
    width = height = depth = color = None
    palette = None
    idat = bytearray()

    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError(f"{path}: truncated chunk header")

        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        end = pos + 12 + length
        if end > len(data):
            raise ValueError(f"{path}: {kind.decode('ascii', 'replace')} chunk exceeds file")

        payload = data[pos + 8:pos + 8 + length]
        expected_crc = struct.unpack(">I", data[pos + 8 + length:end])[0]
        actual_crc = zlib.crc32(kind + payload) & 0xFFFFFFFF
        if actual_crc != expected_crc:
            raise ValueError(f"{path}: bad CRC in {kind.decode('ascii', 'replace')} chunk")

        if kind == b"IHDR":
            if seen_ihdr or length != 13:
                raise ValueError(f"{path}: invalid IHDR")
            width, height, depth, color = struct.unpack(">IIBB", payload[:10])
            seen_ihdr = True
        elif kind == b"PLTE":
            if length == 0 or length % 3:
                raise ValueError(f"{path}: invalid PLTE chunk")
            palette = payload
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            if length != 0:
                raise ValueError(f"{path}: invalid IEND")
            seen_iend = True
            pos = end
            break

        pos = end

    if not seen_ihdr or not seen_iend or pos != len(data):
        raise ValueError(f"{path}: missing chunk or trailing/truncated bytes")
    if color != 3 or depth != 4:
        raise ValueError(
            f"{path}: expected 4-bit indexed PNG (color type 3), "
            f"got type={color}, depth={depth}"
        )
    if palette is None or len(palette) % 3 or not (1 <= len(palette) // 3 <= 16):
        raise ValueError(f"{path}: indexed palette must contain 1..16 entries")

    expected = EXPECTED_SIZE.get(path.name)
    if expected and (width, height) != expected:
        raise ValueError(f"{path}: expected {expected[0]}x{expected[1]}, got {width}x{height}")

    if path.name in STRICT_ASSETS:
        if len(palette) != 16 * 3:
            raise ValueError(f"{path}: generated room/UI art must have exactly 16 palette entries")
        invalid = [value for value in palette if value not in MD_LEVELS]
        if invalid:
            raise ValueError(
                f"{path}: palette contains colors not aligned to Mega Drive 3-bit RGB levels"
            )

    decoded = zlib.decompress(bytes(idat))
    expected_row_bytes = (width + 1) // 2
    expected_raw_size = height * (expected_row_bytes + 1)
    if len(decoded) != expected_raw_size:
        raise ValueError(
            f"{path}: decompressed image data length mismatch "
            f"({len(decoded)} != {expected_raw_size})"
        )

    print(
        f"OK {path}: {width}x{height}, indexed 4-bit, "
        f"{len(palette) // 3} colors, {len(data)} bytes"
    )
    return palette

if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("usage: validate_assets.py PNG [PNG ...]")

    palettes = {Path(item).name: validate(item) for item in sys.argv[1:]}
    if not FX_ASSETS.issubset(palettes):
        raise SystemExit("ERROR: missing one or more room FX assets")

    fx = palettes["window_morning.png"]
    for name in ("lamp_glow.png", "sky_fx.png"):
        if palettes[name] != fx:
            raise SystemExit(
                f"ERROR: {name} must use the exact same palette as window_morning.png"
            )
    missing_intro = INTRO_ASSETS.difference(palettes)
    if missing_intro:
        raise SystemExit("ERROR: missing intro frames: " + ", ".join(sorted(missing_intro)))

    shared_intro_palette = palettes["intro_00.png"]
    for name in sorted(INTRO_ASSETS):
        if palettes[name] != shared_intro_palette:
            raise SystemExit(
                f"ERROR: {name} does not use the same palette as intro_00.png"
            )
    print("OK all ten intro PNGs share the same 16-color Mega Drive palette.")
