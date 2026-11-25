#!/usr/bin/env python3
"""Validate SGDK indexed PNG assets before invoking ResComp."""
import struct, sys, zlib
from pathlib import Path

SIG = b"\x89PNG\r\n\x1a\n"

def validate(path):
    data = Path(path).read_bytes()
    if not data.startswith(SIG):
        raise ValueError(f"{path}: invalid PNG signature")
    pos, seen_ihdr, seen_iend, idat = 8, False, False, bytearray()
    width = height = depth = color = None
    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError(f"{path}: truncated chunk header")
        n = struct.unpack(">I", data[pos:pos+4])[0]
        kind = data[pos+4:pos+8]
        end = pos + 12 + n
        if end > len(data):
            raise ValueError(f"{path}: {kind.decode('ascii', 'replace')} chunk exceeds file")
        payload = data[pos+8:pos+8+n]
        expected = struct.unpack(">I", data[pos+8+n:end])[0]
        actual = zlib.crc32(kind + payload) & 0xffffffff
        if actual != expected:
            raise ValueError(f"{path}: bad CRC in {kind.decode('ascii', 'replace')} chunk")
        if kind == b"IHDR":
            if seen_ihdr or n != 13:
                raise ValueError(f"{path}: invalid IHDR")
            width, height, depth, color = struct.unpack(">IIBB", payload[:10])
            seen_ihdr = True
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            if n != 0:
                raise ValueError(f"{path}: invalid IEND")
            seen_iend = True
            pos = end
            break
        pos = end
    if not seen_ihdr or not seen_iend or pos != len(data):
        raise ValueError(f"{path}: missing chunk or trailing/truncated bytes")
    if color != 3 or depth not in (4, 8):
        raise ValueError(f"{path}: expected indexed PNG (color type 3, 4/8-bit), got type={color}, depth={depth}")
    zlib.decompress(bytes(idat))
    print(f"OK {path}: {width}x{height}, indexed {depth}-bit, {len(data)} bytes")

if __name__ == "__main__":
    for item in sys.argv[1:]:
        validate(item)
