#!/usr/bin/env python3
"""Validate indexed PNG assets before passing them to SGDK ResComp."""
import struct
import sys
import zlib
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MD_LEVELS = {0, 36, 73, 109, 146, 182, 219, 255}
TITLE_ASSETS = {"title.png", "room_on.png", "room_off.png"}

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
    if color != 3 or depth not in (4, 8):
        raise ValueError(
            f"{path}: expected indexed PNG (color type 3, 4/8-bit), "
            f"got type={color}, depth={depth}"
        )
    if palette is None or len(palette) // 3 > (1 << depth):
        raise ValueError(f"{path}: missing or oversized indexed palette")

    zlib.decompress(bytes(idat))

    if path.name in TITLE_ASSETS:
        if (width, height) != (320, 224):
            raise ValueError(f"{path}: expected exactly 320x224, got {width}x{height}")
        if depth != 4 or len(palette) != 16 * 3:
            raise ValueError(f"{path}: expected 4-bit indexed PNG with exactly 16 palette entries")
        invalid = [value for value in palette if value not in MD_LEVELS]
        if invalid:
            raise ValueError(
                f"{path}: palette contains colors not aligned to Mega Drive 3-bit RGB levels"
            )

    print(
        f"OK {path}: {width}x{height}, indexed {depth}-bit, "
        f"{len(palette) // 3} palette entries, {len(data)} bytes"
    )
    return palette

if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("usage: validate_assets.py PNG [PNG ...]")

    palettes = {}
    for item in sys.argv[1:]:
        palettes[Path(item).name] = validate(item)

    if "room_on.png" in palettes and "room_off.png" in palettes:
        if palettes["room_on.png"] != palettes["room_off.png"]:
            raise SystemExit("ERROR: room_on.png and room_off.png must use the exact same palette")
