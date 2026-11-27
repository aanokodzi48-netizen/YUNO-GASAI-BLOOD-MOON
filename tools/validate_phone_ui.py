#!/usr/bin/env python3
"""Validate the Mega Drive phone UI background image, including PNG filters."""
import binascii
import struct
import sys
import zlib

MD_LEVELS = {0, 36, 73, 109, 146, 182, 219, 255}


def paeth_predictor(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def decode_png_rows(raw, width, height, bit_depth):
    """Undo PNG filters 0..4 and return unfiltered packed scanlines."""
    row_bytes = (width * bit_depth + 7) // 8
    bpp = max(1, (bit_depth + 7) // 8)
    expected = height * (row_bytes + 1)
    if len(raw) != expected:
        raise ValueError(
            f"decompressed pixel data length mismatch: {len(raw)} != {expected}"
        )

    rows = []
    previous = bytearray(row_bytes)
    pos = 0
    filters_seen = set()

    for _ in range(height):
        filter_type = raw[pos]
        pos += 1
        scan = raw[pos:pos + row_bytes]
        pos += row_bytes
        filters_seen.add(filter_type)
        recon = bytearray(row_bytes)

        if filter_type > 4:
            raise ValueError(f"invalid PNG row filter {filter_type}")

        for i, value in enumerate(scan):
            left = recon[i - bpp] if i >= bpp else 0
            above = previous[i]
            upper_left = previous[i - bpp] if i >= bpp else 0

            if filter_type == 0:
                predictor = 0
            elif filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            else:
                predictor = paeth_predictor(left, above, upper_left)

            recon[i] = (value + predictor) & 0xFF

        rows.append(recon)
        previous = recon

    return rows, filters_seen


def validate(path):
    with open(path, "rb") as fh:
        data = fh.read()

    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("invalid PNG signature")

    pos = 8
    chunks = {}
    idat = bytearray()
    ended = False
    seen_ihdr = False

    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError("truncated PNG chunk")

        size = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        end = pos + size + 12
        if end > len(data):
            raise ValueError("PNG chunk exceeds file size")

        payload = data[pos + 8:pos + 8 + size]
        stored_crc = struct.unpack(">I", data[pos + 8 + size:end])[0]
        actual_crc = binascii.crc32(tag + payload) & 0xFFFFFFFF
        if actual_crc != stored_crc:
            raise ValueError(f"PNG CRC mismatch in {tag!r}")

        if tag == b"IHDR":
            if seen_ihdr or pos != 8 or size != 13:
                raise ValueError("invalid/duplicate IHDR")
            seen_ihdr = True
            chunks.setdefault(tag, []).append(payload)
        elif tag == b"IDAT":
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
    if not seen_ihdr:
        raise ValueError("missing IHDR")

    width, height, depth, color, comp, filt, interlace = struct.unpack(
        ">IIBBBBB", chunks[b"IHDR"][0]
    )
    if (width, height) != (128, 192):
        raise ValueError(f"phone image must be 128x192, got {width}x{height}")
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

    try:
        raw = zlib.decompress(bytes(idat))
    except zlib.error as exc:
        raise ValueError(f"invalid PNG image data: {exc}") from exc

    rows, filters_seen = decode_png_rows(raw, width, height, depth)
    used = set()
    for row in rows:
        for byte in row:
            used.add(byte >> 4)
            used.add(byte & 0x0F)

    # Index 0 is intentionally transparent around the phone silhouette, so it
    # is valid and expected in the packed pixels. All pixels must stay in 0..15.
    if not used.issubset(set(range(16))):
        raise ValueError("phone image uses an index outside 0..15")

    opaque = [tuple(palette[i:i + 3]) for i in range(3, 48, 3)]
    unique_opaque = len(set(opaque))
    if unique_opaque != 15:
        raise ValueError(f"expected 15 opaque palette colors, got {unique_opaque}")

    print(
        f"OK {path}: 128x192, 4-bit indexed, {len(used)} used indices, "
        f"{unique_opaque} visible colors, transparent index 0"
    )
    print("PNG row filters accepted:", ", ".join(map(str, sorted(filters_seen))))
    print("PASS: phone UI asset is Mega Drive-ready.")


if __name__ == "__main__":
    validate(sys.argv[1] if len(sys.argv) > 1 else "res/phone_ui.png")
