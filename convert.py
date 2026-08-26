#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image


DEFAULT_WIDTH = 640
DEFAULT_HEIGHT = 480
DEFAULT_OUTPUT = "vga565.raw"


def parse_rgb(value):
    value = value.strip().lstrip("#")
    if len(value) != 6:
        raise argparse.ArgumentTypeError("background color must be RRGGBB")
    try:
        return tuple(int(value[i : i + 2], 16) for i in (0, 2, 4))
    except ValueError as exc:
        raise argparse.ArgumentTypeError("background color must be hex") from exc


def resample_filter():
    if hasattr(Image, "Resampling"):
        return Image.Resampling.LANCZOS
    return Image.LANCZOS


def prepare_image(src, width, height, fit, background):
    img = Image.open(src).convert("RGBA")
    canvas = Image.new("RGBA", (width, height), (*background, 255))

    if fit == "stretch":
        resized = img.resize((width, height), resample_filter())
        canvas.alpha_composite(resized, (0, 0))
        return canvas.convert("RGB")

    scale_func = min if fit == "contain" else max
    scale = scale_func(width / img.width, height / img.height)
    new_size = (
        max(1, round(img.width * scale)),
        max(1, round(img.height * scale)),
    )
    resized = img.resize(new_size, resample_filter())

    if fit == "cover":
        left = max(0, (new_size[0] - width) // 2)
        top = max(0, (new_size[1] - height) // 2)
        resized = resized.crop((left, top, left + width, top + height))
        canvas.alpha_composite(resized, (0, 0))
    else:
        pos = ((width - new_size[0]) // 2, (height - new_size[1]) // 2)
        canvas.alpha_composite(resized, pos)

    return canvas.convert("RGB")


def write_rgb565_le(img, dst):
    data = bytearray()
    for r, g, b in img.getdata():
        pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        data.extend(pixel.to_bytes(2, "little"))
    Path(dst).write_bytes(data)
    return len(data)


def main():
    parser = argparse.ArgumentParser(
        description="Convert an image to 640x480 little-endian RGB565 raw framebuffer data."
    )
    parser.add_argument("input", nargs="?", default="logo.png")
    parser.add_argument("-o", "--output", default=DEFAULT_OUTPUT)
    parser.add_argument("--width", type=int, default=DEFAULT_WIDTH)
    parser.add_argument("--height", type=int, default=DEFAULT_HEIGHT)
    parser.add_argument(
        "--fit",
        choices=("contain", "cover", "stretch"),
        default="contain",
        help="contain keeps the logo aspect ratio with borders",
    )
    parser.add_argument("--background", type=parse_rgb, default=parse_rgb("000000"))
    args = parser.parse_args()

    img = prepare_image(args.input, args.width, args.height, args.fit, args.background)
    size = write_rgb565_le(img, args.output)
    expected = args.width * args.height * 2
    print(f"wrote {args.output}: {size} bytes")
    print(f"expected framebuffer bytes: {expected}")


if __name__ == "__main__":
    main()
