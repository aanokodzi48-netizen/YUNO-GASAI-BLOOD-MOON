#!/usr/bin/env python3
"""Validate the Mega Drive phone UI background image."""
import os
import struct
import zlib
import binascii
import sys

MD_LEVELS = {0, 36, 73, 109, 146, 182, 219, 255}

def validate(path):
    data = open(path, "rb").read()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("invalid PNG signature")
    pos = 8
    chunks = {}
    idat = bytearray()
    ended = False
    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError("truncated PNG chunk")
        size = struct.unpack(">I", data[pos:pos+4])[0]
        tag = data[pos+4:pos+8]
        end = pos + size + 12
        if end > len(data):
            raise ValueError("PNG chunk exceeds file size")
        payload = data[pos+8:pos+8+size]
        crc = struct.unpack(">I", data[pos+8+size:end])[0]
        if (binascii.crc32(tag+payload) & 0xFFFFFFFF) != crc:
            raise ValueError("PNG CRC mismatch in "+repr(tag))
        if tag == b"IDAT":
            idat.extend(payload)
        else:
            chunks.setdefault(tag, []).append(payload)
        pos = end
        if tag == b"IEND":
            if size != 0:
                raise ValueError("invalid IEND")
            ended = True
            break
    if not ended or pos != len(data):
        raise ValueError("missing IEND or trailing data")

    width, height, depth, color, comp, filt, interlace = struct.unpack(
        ">IIBBBBB", chunks[b"IHDR"][0])
    if (width, height) != (128, 192):
        raise ValueError("phone image must be 128x192")
    if (depth, color, comp, filt, interlace) != (4, 3, 0, 0, 0):
        raise ValueError("expected non-interlaced 4-bit indexed PNG")
    palette = chunks.get(b"PLTE", [b""])[0]
    transparency = chunks.get(b"tRNS", [b""])[0]
    if len(palette) != 48:
        raise ValueError("expected 16-entry RGB palette")
    if transparency != b"\x00":
        raise ValueError("transparent index must be 0")
    if any(v not in MD_LEVELS for v in palette):
        raise ValueError("palette is not quantized to Mega Drive RGB333 levels")

    raw = zlib.decompress(bytes(idat))
    row_bytes = width // 2
    if len(raw) != height * (row_bytes + 1):
        raise ValueError("decompressed pixel data length mismatch")
    used = set()
    for y in range(height):
        offset = y * (row_bytes + 1)
        if raw[offset] != 0:
            raise ValueError("unsupported PNG row filter")
        for b in raw[offset+1:offset+1+row_bytes]:
            used.add(b >> 4)
            used.add(b & 15)
    if 0 in used:
        raise ValueError("index 0 must remain unused/transparent in the phone background")
    if not used.issubset(set(range(1,16))):
        raise ValueError("phone image uses an index outside 1..15")
    opaque = [tuple(palette[i:i+3]) for i in range(3,48,3)]
    if not opaque:
        raise ValueError("empty phone palette")
    print("OK %s: 128x192, 4-bit indexed, %d visible colors, transparent index 0" %
          (path, len(set(opaque))))
    print("PASS: phone UI asset is Mega Drive-ready.")

if __name__ == "__main__":
    validate(sys.argv[1] if len(sys.argv)>1 else "res/phone_ui.png")
