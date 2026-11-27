#!/usr/bin/env python3
"""Convert ten uploaded GIF frames to a shared 16-color Mega Drive PNG set."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
RES = ROOT / "res"
WIDTH, HEIGHT = 320, 224
FRAME_COUNT = 10
MD_LEVELS = (0, 36, 73, 109, 146, 182, 219, 255)


def snap(channel):
    return min(MD_LEVELS, key=lambda value: abs(value - channel))


def cover_crop(source):
    """Scale with nearest-neighbour and crop to the 320x224 console screen."""
    source = source.convert("RGB")
    src_w, src_h = source.size
    if not src_w or not src_h:
        raise ValueError("GIF frame has an empty image")
    scale = max(WIDTH / src_w, HEIGHT / src_h)
    dst_w = max(WIDTH, int(round(src_w * scale)))
    dst_h = max(HEIGHT, int(round(src_h * scale)))
    resampling = getattr(Image, "Resampling", Image).NEAREST
    source = source.resize((dst_w, dst_h), resampling)
    left = (dst_w - WIDTH) // 2
    top = (dst_h - HEIGHT) // 2
    return source.crop((left, top, left + WIDTH, top + HEIGHT))


def load_sources():
    paths = sorted(RES.glob("frame_*.gif"))
    if len(paths) != FRAME_COUNT:
        raise SystemExit(
            f"ERROR: expected exactly {FRAME_COUNT} GIFs named frame_*.gif "
            f"in res/, found {len(paths)}"
        )

    expected_prefixes = [f"frame_{index:02d}_" for index in range(FRAME_COUNT)]
    for index, path in enumerate(paths):
        if not path.name.startswith(expected_prefixes[index]):
            raise SystemExit(
                f"ERROR: frame order/name mismatch at position {index}: {path.name}"
            )

    frames = []
    for path in paths:
        with Image.open(path) as gif:
            gif.seek(0)
            frame = cover_crop(gif.copy())
        frames.append(frame)
        print(f"Loaded {path.name}: {frame.width}x{frame.height}")
    return frames


def main():
    RES.mkdir(parents=True, exist_ok=True)
    frames = load_sources()

    # Build one shared palette from all frames together, so no palette flicker
    # occurs when the game swaps the background image.
    sheet = Image.new("RGB", (WIDTH, HEIGHT * FRAME_COUNT))
    for index, frame in enumerate(frames):
        sheet.paste(frame, (0, HEIGHT * index))

    quantized_sheet = sheet.quantize(colors=16, dither=Image.Dither.NONE)
    raw_palette = quantized_sheet.getpalette()
    if raw_palette is None or len(raw_palette) < 48:
        raise SystemExit("ERROR: Pillow did not produce a 16-entry palette")

    snapped = [snap(value) for value in raw_palette[:48]]
    palette_image = Image.new("P", (16, 1))
    palette_image.putpalette(snapped + [0] * (768 - len(snapped)))

    reference_palette = None
    for index, frame in enumerate(frames):
        indexed = frame.quantize(palette=palette_image, dither=Image.Dither.NONE)
        indexed.info.clear()
        output = RES / f"intro_{index:02d}.png"
        indexed.save(output, format="PNG", bits=4, optimize=False, compress_level=9)

        # Re-open the file to check the encoder did preserve indexed 4-bit data.
        with Image.open(output) as check:
            if check.mode != "P" or check.size != (WIDTH, HEIGHT):
                raise SystemExit(f"ERROR: invalid output image {output.name}")
            current_palette = check.getpalette()[:48]
            if reference_palette is None:
                reference_palette = current_palette
            elif current_palette != reference_palette:
                raise SystemExit(f"ERROR: palette mismatch in {output.name}")

        print(f"OK {output.name}: 320x224, indexed 4-bit, shared palette")

    print("Converted 10 intro frames with one common Mega Drive palette.")


if __name__ == "__main__":
    main()
