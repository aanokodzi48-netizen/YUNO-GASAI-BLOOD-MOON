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

    # Detailed side-view bed: carved headboard, layered mattress and quilt.
    d.ellipse((202, 164, 317, 181), fill=C[8])
    d.rectangle((293, 101, 317, 173), fill=C[8])
    d.rectangle((297, 105, 313, 168), fill=C[10])
    d.rectangle((300, 108, 310, 165), fill=C[12])
    d.line((299, 111, 311, 111), fill=C[14], width=2)
    d.line((299, 163, 311, 163), fill=C[9], width=2)
    d.rectangle((302, 118, 305, 151), fill=C[9])
    d.rectangle((207, 120, 302, 171), fill=C[8])
    d.rectangle((210, 123, 298, 164), fill=C[9])
    d.rectangle((213, 125, 296, 160), fill=C[14])
    d.line((215, 126, 294, 126), fill=C[15], width=2)
    d.line((214, 159, 296, 159), fill=C[12], width=2)
    # Pillow with piping and soft highlight.
    d.rectangle((220, 128, 251, 143), fill=C[12])
    d.rectangle((222, 129, 249, 140), fill=C[15])
    d.line((224, 131, 247, 131), fill=C[14], width=2)
    d.line((224, 139, 247, 139), fill=C[13])
    d.line((251, 131, 251, 139), fill=C[12])
    # Folded mauve blanket, stitched edge and small quilt diamonds.
    d.polygon([(252, 132), (294, 132), (296, 157), (286, 162),
               (253, 158)], fill=C[5])
    d.polygon([(255, 135), (291, 135), (292, 153), (285, 157),
               (256, 154)], fill=C[6])
    d.line((256, 137, 289, 137), fill=C[7], width=2)
    d.line((257, 154, 285, 157), fill=C[4])
    for x, y in ((263, 143), (275, 143), (285, 145)):
        d.polygon([(x, y-3), (x+3, y), (x, y+3), (x-3, y)], fill=C[7])
        d.point((x, y), fill=C[14])
    d.line((254, 157, 291, 160), fill=C[4], width=2)
    # Polished frame rail and sturdy feet.
    d.rectangle((207, 160, 300, 169), fill=C[8])
    d.line((211, 162, 296, 162), fill=C[11])
    d.rectangle((213, 168, 220, 177), fill=C[8])
    d.rectangle((291, 168, 299, 177), fill=C[8])
    d.line((215, 170, 215, 175), fill=C[11])
    d.line((294, 170, 294, 175), fill=C[11])
    d.rectangle((210, 177, 302, 180), fill=C[9])

    # Bedside cabinet with a crafted ceramic lamp (OFF state).
    d.rectangle((169, 130, 212, 165), fill=C[8])
    d.rectangle((173, 134, 208, 160), fill=C[10])
    d.line((174, 135, 207, 135), fill=C[11], width=2)
    d.rectangle((176, 139, 205, 144), fill=C[9])
    d.rectangle((177, 148, 204, 157), fill=C[9])
    d.rectangle((185, 150, 196, 154), fill=C[8])
    d.rectangle((171, 161, 210, 167), fill=C[8])
    d.line((174, 162, 207, 162), fill=C[11])
    # Brass foot, narrow stem and flared fabric shade.
    d.rectangle((183, 126, 199, 131), fill=C[8])
    d.rectangle((188, 116, 194, 128), fill=C[9])
    d.line((189, 117, 189, 126), fill=C[11])
    d.polygon([(179, 116), (183, 101), (199, 101), (204, 116),
               (199, 121), (184, 121)], fill=C[8])
    d.polygon([(182, 114), (185, 104), (197, 104), (201, 114),
               (197, 118), (186, 118)], fill=C[9])
    d.line((185, 105, 197, 105), fill=C[12], width=2)
    d.line((184, 116, 199, 116), fill=C[10], width=2)
    d.rectangle((188, 121, 195, 125), fill=C[11])

    # Large woven plum rug with a double border, tassels and floral medallion.
    d.rectangle((67, 174, 305, 214), fill=C[8])
    d.rectangle((70, 176, 302, 211), fill=C[5])
    d.rectangle((74, 179, 298, 208), fill=C[6])
    d.rectangle((78, 182, 294, 205), fill=C[4])
    d.rectangle((82, 185, 290, 202), fill=C[5])
    d.rectangle((86, 188, 286, 199), fill=C[6])
    d.line((74, 180, 298, 180), fill=C[7], width=2)
    d.line((74, 207, 298, 207), fill=C[2], width=2)
    d.line((77, 183, 295, 183), fill=C[7])
    d.line((77, 204, 295, 204), fill=C[7])
    # Woven side motifs and corner ornaments.
    for x in range(88, 288, 16):
        d.polygon([(x, 184), (x+3, 187), (x, 190), (x-3, 187)], fill=C[11])
        d.polygon([(x, 197), (x+3, 200), (x, 203), (x-3, 200)], fill=C[11])
    for x in (80, 284):
        for y in (185, 199):
            d.rectangle((x, y, x+4, y+3), fill=C[7])
            d.point((x+2, y+1), fill=C[14])
    # Central ornamental medallion.
    d.ellipse((158, 185, 214, 201), fill=C[4])
    d.ellipse((163, 187, 209, 199), fill=C[5])
    d.polygon([(186, 186), (197, 193), (186, 200), (175, 193)], fill=C[11])
    d.polygon([(186, 189), (192, 193), (186, 197), (180, 193)], fill=C[7])
    d.rectangle((184, 192, 188, 194), fill=C[14])
    # Short fringe along both long edges.
    for x in range(76, 300, 8):
        d.line((x, 211, x+2, 214), fill=C[12])
        d.line((x, 174, x+2, 176), fill=C[12])
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
    # Warm stepped pools of light around the shade, not a pink screen tint.
    d.polygon([(177, 101), (181, 94), (201, 94), (207, 101),
               (211, 117), (205, 130), (177, 130), (171, 117)],
              fill=C[11])
    d.polygon([(180, 103), (185, 98), (198, 98), (203, 104),
               (206, 116), (200, 125), (181, 125), (176, 116)],
              fill=C[12])
    d.polygon([(182, 114), (185, 103), (197, 103), (201, 114),
               (197, 119), (186, 119)], fill=C[14])
    d.line((185, 104, 197, 104), fill=C[15], width=2)
    d.rectangle((188, 120, 194, 128), fill=C[15])
    d.rectangle((184, 128, 198, 132), fill=C[11])
    # A subtle warm reflection on the cabinet top and floor.
    d.line((174, 133, 206, 133), fill=C[13], width=2)
    d.line((180, 168, 201, 168), fill=C[11])
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
