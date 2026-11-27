#!/usr/bin/env python3
"""Validate the six indexed PNG parts used by the Yuno SGDK metasprite."""
import os
import struct
import sys
import zlib
import binascii

EXPECTED = {
    "yuno_idle_r1_c1.png": (32, 32),
    "yuno_idle_r2_c1.png": (32, 32),
    "yuno_idle_r2_c2.png": (8, 32),
    "yuno_idle_r3_c1.png": (32, 32),
    "yuno_idle_r4_c1.png": (32, 24),
    "yuno_idle_r4_c2.png": (8, 24),
}
MD_LEVELS = {0, 36, 73, 109, 146, 182, 219, 255}


def check_png(path):
    data = open(path, "rb").read()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError(f"{path}: invalid PNG signature")

    pos = 8
    chunks = {}
    idat = bytearray()
    ended = False

    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError(f"{path}: truncated PNG chunk")

        size = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        end = pos + size + 12
        if end > len(data):
            raise ValueError(f"{path}: chunk exceeds file length")

        payload = data[pos + 8:pos + 8 + size]
        crc = struct.unpack(">I", data[pos + 8 + size:end])[0]
        if (binascii.crc32(tag + payload) & 0xFFFFFFFF) != crc:
            raise ValueError(f"{path}: CRC mismatch in {tag!r}")

        if tag == b"IDAT":
            idat.extend(payload)
        else:
            chunks.setdefault(tag, []).append(payload)

        pos = end
        if tag == b"IEND":
            if size != 0:
                raise ValueError(f"{path}: invalid IEND")
            ended = True
            break

    if not ended or pos != len(data):
        raise ValueError(f"{path}: missing IEND or trailing data")

    width, height, depth, color, comp, filt, interlace = struct.unpack(
        ">IIBBBBB", chunks[b"IHDR"][0]
    )
    if (depth, color, comp, filt, interlace) != (4, 3, 0, 0, 0):
        raise ValueError(f"{path}: expected non-interlaced 4-bit indexed PNG")

    palette = chunks.get(b"PLTE", [b""])[0]
    transparency = chunks.get(b"tRNS", [b""])[0]

    if len(palette) != 48:
        raise ValueError(f"{path}: expected a 16-entry RGB palette")
    if transparency != b"\x00":
        raise ValueError(f"{path}: only palette index 0 must be transparent")
    if any(value not in MD_LEVELS for value in palette):
        raise ValueError(f"{path}: palette RGB levels are not Mega Drive aligned")

    opaque_colors = [tuple(palette[i:i + 3]) for i in range(3, 48, 3)]
    if len(set(opaque_colors)) != 15:
        raise ValueError(f"{path}: expected 15 unique opaque colors")

    raw = zlib.decompress(bytes(idat))
    row_bytes = (width + 1) // 2
    if len(raw) != height * (row_bytes + 1):
        raise ValueError(f"{path}: decompressed pixel data length mismatch")

    for row in range(height):
        if raw[row * (row_bytes + 1)] != 0:
            raise ValueError(f"{path}: unsupported PNG row filter")

    return width, height, palette


def main(folder):
    shared_palette = None

    for filename, expected_size in EXPECTED.items():
        path = os.path.join(folder, filename)
        width, height, palette = check_png(path)

        if (width, height) != expected_size:
            raise ValueError(
                f"{filename}: expected {expected_size}, got {(width, height)}"
            )
        if width > 32 or height > 32 or width % 8 or height % 8:
            raise ValueError(f"{filename}: invalid hardware sprite dimensions")

        if shared_palette is None:
            shared_palette = palette
        elif palette != shared_palette:
            raise ValueError(f"{filename}: shared palette does not match")

        print(
            f"OK {filename}: {width}x{height}, 4-bit indexed, "
            "15 visible colors + transparent index 0"
        )

    print("PASS: all six metasprite PNG assets validated.")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "res")
