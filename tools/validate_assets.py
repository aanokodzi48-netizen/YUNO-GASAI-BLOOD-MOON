#!/usr/bin/env python3
"""Validate indexed PNG assets used by the Mega Drive build."""
import struct
import sys
import zlib
from pathlib import Path

PNG_SIGNATURE = bytes([137, 80, 78, 71, 13, 10, 26, 10])
MD_LEVELS = {0, 36, 73, 109, 146, 182, 219, 255}
STRICT_ASSETS = {"title.png", "phone_ui.png"}
EXPECTED_SIZE = {
    "title.png": (320, 224),
    "phone_ui.png": (128, 192),
}

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
    transparency = None
    idat = bytearray()
    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError(f"{path}: truncated chunk header")
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        end = pos + 12 + length
        if end > len(data):
            raise ValueError(f"{path}: chunk exceeds file size")
        payload = data[pos + 8:pos + 8 + length]
        stored_crc = struct.unpack(">I", data[pos + 8 + length:end])[0]
        actual_crc = zlib.crc32(kind + payload) & 0xFFFFFFFF
        if actual_crc != stored_crc:
            raise ValueError(f"{path}: CRC mismatch in {kind!r}")
        if kind == b"IHDR":
            if seen_ihdr or length != 13:
                raise ValueError(f"{path}: invalid IHDR")
            width, height, depth, color = struct.unpack(">IIBB", payload[:10])
            seen_ihdr = True
        elif kind == b"PLTE":
            if length == 0 or length % 3:
                raise ValueError(f"{path}: invalid PLTE")
            palette = payload
        elif kind == b"tRNS":
            transparency = payload
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
        raise ValueError(f"{path}: incomplete PNG")
    if color != 3 or depth != 4:
        raise ValueError(f"{path}: expected indexed 4-bit PNG, got type={color}, depth={depth}")
    if palette is None or len(palette) % 3 or not (1 <= len(palette) // 3 <= 16):
        raise ValueError(f"{path}: expected 1..16 palette entries")
    if path.name == "phone_ui.png" and transparency != bytes([0]):
        raise ValueError(f"{path}: transparent palette index must be 0")
    expected = EXPECTED_SIZE.get(path.name)
    if expected and (width, height) != expected:
        raise ValueError(f"{path}: expected {expected[0]}x{expected[1]}, got {width}x{height}")
    if path.name in STRICT_ASSETS:
        if len(palette) != 48:
            raise ValueError(f"{path}: strict game art must contain 16 palette entries")
        if any(value not in MD_LEVELS for value in palette):
            raise ValueError(f"{path}: palette is not aligned to Mega Drive RGB333 levels")
    decoded = zlib.decompress(bytes(idat))
    row_bytes = (width + 1) // 2
    if len(decoded) != height * (row_bytes + 1):
        raise ValueError(f"{path}: decompressed pixel data length mismatch")
    if any(decoded[row * (row_bytes + 1)] > 4 for row in range(height)):
        raise ValueError(f"{path}: invalid PNG row filter")
    print(f"OK {path}: {width}x{height}, 4-bit indexed, {len(palette)//3} palette entries")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("usage: validate_assets.py PNG [PNG ...]")
    for item in sys.argv[1:]:
        validate(item)
    print("OK all supplied game assets validated.")
