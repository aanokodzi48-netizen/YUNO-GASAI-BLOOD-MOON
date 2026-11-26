#!/usr/bin/env python3
"""Generate the bedroom art as native-resolution 4-bit Mega Drive PNGs.

This source generator is committed instead of opaque binary assets so each
CI build recreates the exact same 320x224, 16-entry indexed images.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageEnhance

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "res"
W, H = 320, 224

# Every RGB component is one of the Mega Drive's 3-bit channel levels.
PALETTE = [
    (0, 0, 36),       # 0 deep night
    (36, 36, 73),     # 1 blue night
    (73, 36, 109),    # 2 violet shadow
    (73, 73, 109),    # 3 slate violet
    (109, 73, 109),   # 4 muted violet
    (146, 73, 146),   # 5 violet accent
    (182, 73, 146),   # 6 restrained pink
    (219, 109, 182),  # 7 soft pink
    (73, 36, 36),     # 8 dark wood
    (109, 73, 36),    # 9 wood shadow
    (146, 109, 73),   # 10 wood
    (182, 146, 73),   # 11 warm gold
    (182, 146, 109),  # 12 beige shadow
    (219, 182, 146),  # 13 beige
    (219, 219, 182),  # 14 cream
    (255, 255, 219),  # 15 warm light
]
C = PALETTE

def build_room():
    im = Image.new("RGB", (W, H), C[13])
    d = ImageDraw.Draw(im)

    # Warm plaster walls and dark timber framing.
    d.rectangle((0, 0, 319, 154), fill=C[13])
    d.rectangle((0, 0, 319, 11), fill=C[8])
    d.rectangle((0, 11, 319, 15), fill=C[10])
    d.rectangle((0, 146, 319, 154), fill=C[8])
    d.rectangle((0, 151, 319, 154), fill=C[11])
    for x in (4, 156, 316):
        d.rectangle((x, 17, x + 3, 145), fill=C[12])
        d.line((x + 1, 18, x + 1, 143), fill=C[14])

    # Wooden floor and staggered plank seams.
    d.rectangle((0, 155, 319, 223), fill=C[10])
    for y in (156, 173, 190, 207, 222):
        d.line((0, y, 319, y), fill=C[8], width=2)
    for row, y in enumerate((158, 175, 192, 209)):
        offset = 0 if row % 2 == 0 else 28
        for x in range(offset, 320, 56):
            d.line((x, y, x, y + 14), fill=C[9])
            d.line((x + 2, y + 2, x + 2, y + 12), fill=C[11])

    # Wide window: quiet evening street, small moon, trees and lanterns.
    d.rectangle((91, 22, 225, 108), fill=C[8])
    d.rectangle((96, 27, 220, 103), fill=C[10])
    d.rectangle((101, 32, 215, 98), fill=C[1])
    d.rectangle((102, 53, 214, 70), fill=C[2])
    d.rectangle((102, 71, 214, 97), fill=C[3])
    d.line((102, 51, 214, 51), fill=C[4])
    d.line((102, 69, 214, 69), fill=C[4])
    d.ellipse((177, 37, 196, 56), fill=C[14])
    d.ellipse((184, 32, 201, 52), fill=C[1])
    for x, y in ((117, 43), (146, 38), (206, 61), (132, 78)):
        d.rectangle((x, y, x + 2, y + 2), fill=C[14])

    # Low Japanese house roofs and glowing windows beyond the glass.
    d.polygon([(101, 82), (119, 70), (137, 82), (137, 97), (101, 97)], fill=C[0])
    d.polygon([(145, 84), (165, 70), (184, 84), (184, 97), (145, 97)], fill=C[0])
    d.polygon([(187, 83), (202, 73), (215, 83), (215, 97), (187, 97)], fill=C[0])
    for x, y in ((111, 86), (155, 86), (194, 86)):
        d.rectangle((x, y, x + 5, y + 7), fill=C[11])
        d.rectangle((x + 2, y, x + 3, y + 7), fill=C[13])

    # Tree silhouettes and warm street lanterns.
    d.rectangle((105, 74, 109, 97), fill=C[0])
    d.rectangle((207, 76, 211, 97), fill=C[0])
    for x, y in ((102, 68), (108, 65), (114, 70), (199, 67), (207, 64), (214, 70)):
        d.rectangle((x, y, x + 7, y + 5), fill=C[0])
    for x in (126, 202):
        d.rectangle((x, 72, x + 2, 91), fill=C[8])
        d.rectangle((x - 3, 73, x + 5, 80), fill=C[11])
        d.rectangle((x - 2, 74, x + 4, 78), fill=C[15])

    # Window muntins, sill and calm curtains.
    d.rectangle((150, 32, 154, 98), fill=C[8])
    d.rectangle((101, 64, 215, 67), fill=C[8])
    d.rectangle((88, 104, 228, 111), fill=C[8])
    d.rectangle((91, 103, 225, 105), fill=C[14])
    d.rectangle((83, 24, 94, 115), fill=C[4])
    d.rectangle((84, 26, 88, 111), fill=C[5])
    for y in range(36, 110, 16):
        d.line((90, y, 94, y + 7), fill=C[12])
    d.rectangle((222, 24, 234, 115), fill=C[4])
    d.rectangle((228, 26, 231, 110), fill=C[5])
    for y in range(36, 110, 16):
        d.line((222, y + 7, 226, y), fill=C[12])

    # Entry door on the left.
    d.rectangle((8, 39, 49, 153), fill=C[8])
    d.rectangle((12, 43, 45, 151), fill=C[9])
    d.rectangle((16, 48, 41, 98), fill=C[10])
    d.rectangle((16, 104, 41, 144), fill=C[10])
    d.rectangle((19, 51, 38, 95), fill=C[9])
    d.rectangle((19, 108, 38, 141), fill=C[9])
    d.line((16, 100, 41, 100), fill=C[8], width=2)
    d.rectangle((38, 96, 42, 101), fill=C[11])
    d.rectangle((40, 97, 42, 99), fill=C[15])
    d.rectangle((7, 151, 51, 156), fill=C[8])

    # Full-height wooden wardrobe beside the door.
    d.rectangle((54, 64, 105, 157), fill=C[8])
    d.rectangle((58, 68, 101, 153), fill=C[10])
    d.rectangle((61, 71, 79, 148), fill=C[12])
    d.rectangle((81, 71, 98, 148), fill=C[12])
    d.rectangle((64, 75, 76, 144), fill=C[13])
    d.rectangle((84, 75, 95, 144), fill=C[13])
    d.line((80, 70, 80, 150), fill=C[8], width=2)
    d.rectangle((76, 104, 78, 111), fill=C[11])
    d.rectangle((82, 104, 84, 111), fill=C[11])
    d.rectangle((56, 154, 103, 160), fill=C[8])

    # Small framed wall picture.
    d.rectangle((55, 25, 77, 52), fill=C[8])
    d.rectangle((58, 28, 74, 49), fill=C[1])
    d.rectangle((61, 35, 70, 43), fill=C[6])
    d.rectangle((64, 31, 68, 39), fill=C[14])

    # Bookcase on the far right.
    d.rectangle((280, 62, 316, 139), fill=C[8])
    d.rectangle((284, 66, 312, 135), fill=C[10])
    d.rectangle((284, 89, 312, 92), fill=C[8])
    d.rectangle((284, 112, 312, 115), fill=C[8])
    books = (
        (287,72,6),(292,70,14),(298,73,11),(304,69,5),
        (287,96,12),(293,95,5),(300,97,14),(306,94,6),
        (287,119,5),(294,117,11),(301,119,6),(307,117,14)
    )
    for x, y, col in books:
        d.rectangle((x, y, x + 3, y + 15), fill=C[col])
    d.rectangle((278, 136, 318, 142), fill=C[8])

    # Bed moved to the right: normal furniture proportions, cream linens.
    d.rectangle((207, 113, 315, 171), fill=C[8])
    d.rectangle((212, 118, 310, 165), fill=C[9])
    d.rectangle((218, 124, 301, 160), fill=C[13])
    d.rectangle((219, 123, 302, 130), fill=C[14])
    d.rectangle((220, 134, 301, 159), fill=C[12])
    d.rectangle((220, 143, 301, 158), fill=C[6])
    d.rectangle((222, 145, 298, 156), fill=C[7])
    d.rectangle((225, 126, 250, 138), fill=C[14])
    d.rectangle((227, 128, 248, 136), fill=C[15])
    d.line((252, 144, 252, 156), fill=C[13], width=2)
    d.rectangle((207, 162, 315, 171), fill=C[8])
    d.rectangle((212, 168, 218, 175), fill=C[9])
    d.rectangle((302, 168, 309, 175), fill=C[9])
    d.rectangle((294, 102, 315, 171), fill=C[8])
    d.rectangle((298, 107, 311, 160), fill=C[10])
    d.line((298, 112, 311, 112), fill=C[12], width=2)

    # Bedside cabinet and lamp.
    d.rectangle((169, 130, 212, 165), fill=C[8])
    d.rectangle((173, 134, 208, 160), fill=C[10])
    d.rectangle((176, 138, 205, 143), fill=C[9])
    d.rectangle((177, 147, 204, 157), fill=C[9])
    d.rectangle((185, 149, 196, 154), fill=C[8])
    d.rectangle((171, 161, 210, 167), fill=C[8])
    d.rectangle((187, 114, 194, 132), fill=C[8])
    d.rectangle((184, 128, 198, 132), fill=C[11])
    d.polygon([(180, 119), (184, 102), (198, 102), (202, 119)], fill=C[11])
    d.rectangle((184, 104, 198, 107), fill=C[15])
    d.rectangle((187, 119, 195, 126), fill=C[14])

    # Front rug and small plant near the window.
    d.rectangle((70, 174, 302, 211), fill=C[8])
    d.rectangle((74, 178, 298, 207), fill=C[12])
    d.rectangle((79, 182, 293, 203), fill=C[13])
    d.rectangle((84, 186, 288, 199), fill=C[12])
    d.line((84, 187, 287, 187), fill=C[14])
    d.line((84, 198, 287, 198), fill=C[14])
    for x in range(90, 286, 24):
        d.rectangle((x, 191, x + 7, 194), fill=C[7])
    d.rectangle((118, 126, 137, 141), fill=C[8])
    d.rectangle((121, 129, 134, 138), fill=C[4])
    d.rectangle((127, 111, 131, 129), fill=C[15])
    d.rectangle((115, 115, 128, 121), fill=C[15])
    d.rectangle((130, 112, 142, 118), fill=C[15])
    d.rectangle((120, 108, 130, 114), fill=C[15])
    d.rectangle((134, 105, 141, 112), fill=C[15])

    return im

def make_on_variant(base):
    on = base.copy()
    d = ImageDraw.Draw(on)
    d.rectangle((168, 114, 211, 135), fill=C[12])
    d.rectangle((173, 110, 207, 132), fill=C[13])
    d.rectangle((179, 107, 201, 126), fill=C[14])
    d.polygon([(179, 111), (184, 101), (198, 101), (203, 111)], fill=C[11])
    d.rectangle((184, 103, 198, 107), fill=C[15])
    d.rectangle((188, 117, 194, 128), fill=C[15])
    return on

def indexed_png(image, path, palette_image):
    indexed = image.quantize(palette=palette_image, dither=Image.Dither.NONE)
    indexed.save(path, format="PNG", bits=4, optimize=False, compress_level=9)

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    palette_bytes = []
    for rgb in PALETTE:
        palette_bytes.extend(rgb)
    palette_bytes.extend([0] * (768 - len(palette_bytes)))
    palette_image = Image.new("P", (16, 1))
    palette_image.putpalette(palette_bytes)

    base = build_room()
    on = make_on_variant(base)
    off = ImageEnhance.Brightness(base).enhance(0.72)

    indexed_png(on, OUT / "room_on.png", palette_image)
    indexed_png(off, OUT / "room_off.png", palette_image)
    print("Generated room_on.png and room_off.png at 320x224, 4-bit indexed, 16-entry palette.")

if __name__ == "__main__":
    main()
