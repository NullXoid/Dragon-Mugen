#!/usr/bin/env python3
"""Build A.Ben's SFF and shop walk PNGs from the extracted 8-frame cycle."""

from __future__ import annotations

import argparse
import io
import struct
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image


REPO_ROOT = Path(__file__).resolve().parents[2]
CHAR_DIR = REPO_ROOT / "game" / "chars" / "A.Ben"
DEFAULT_SOURCE_DIRS = (
    Path(r"C:\Users\kasom\Desktop\New folder\a-ben\aben_walk_assets\frames_8"),
    Path(r"C:\Users\kasom\projects\aben-walk-animation\aben_walk_assets\frames_8"),
)
TARGET_WALK_HEIGHT = 122
WALK_GROUP = 20


@dataclass
class Sprite:
    group: int
    image: int
    axis_x: int
    axis_y: int
    data: bytes
    linked_index: int = -1
    shared_palette: int = 0


def _read_sff_v1(path: Path) -> tuple[bytearray, list[Sprite]]:
    data = path.read_bytes()
    if data[:12] != b"ElecbyteSpr\0":
        raise ValueError(f"{path} is not an Elecbyte SFF")
    if data[15] >= 2:
        raise ValueError("This builder preserves SFF v1 files only")

    sprite_count = struct.unpack_from("<I", data, 20)[0]
    subfile_offset = struct.unpack_from("<I", data, 24)[0]
    subheader_size = struct.unpack_from("<I", data, 28)[0]
    sprites: list[Sprite] = []

    offset = subfile_offset
    for index in range(sprite_count):
        if offset <= 0 or offset + subheader_size > len(data):
            raise ValueError(f"Invalid SFF subfile offset at sprite {index}: {offset}")

        next_offset, length = struct.unpack_from("<II", data, offset)
        axis_x, axis_y, group, image, linked_index = struct.unpack_from("<hhhhh", data, offset + 8)
        shared_palette = data[offset + 18]
        payload_start = offset + subheader_size
        payload_end = payload_start + length
        if payload_end > len(data):
            raise ValueError(f"Sprite {group},{image} payload extends past EOF")

        sprites.append(
            Sprite(
                group=group,
                image=image,
                axis_x=axis_x,
                axis_y=axis_y,
                data=data[payload_start:payload_end],
                linked_index=linked_index,
                shared_palette=shared_palette,
            )
        )
        if next_offset == 0:
            break
        offset = next_offset

    return bytearray(data[:512]), sprites


def _rgba_to_indexed_pcx(image: Image.Image) -> bytes:
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")

    rgb = Image.new("RGB", rgba.size, (0, 0, 0))
    rgb.paste(rgba.convert("RGB"), mask=alpha)
    quantized = rgb.quantize(colors=255, method=Image.Quantize.MEDIANCUT)

    pixels = bytearray(quantized.tobytes())
    alpha_pixels = alpha.tobytes()
    for index, alpha_value in enumerate(alpha_pixels):
        pixels[index] = 0 if alpha_value < 8 else min(pixels[index] + 1, 255)

    palette = quantized.getpalette()[: 255 * 3]
    palette = [0, 0, 0] + palette
    palette.extend([0] * (768 - len(palette)))

    indexed = Image.frombytes("P", quantized.size, bytes(pixels))
    indexed.putpalette(palette[:768])

    output = io.BytesIO()
    indexed.save(output, format="PCX")
    return output.getvalue()


def _remove_green_fringe(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    arr = np.array(rgba)
    rgb = arr[:, :, :3].astype(np.int16)
    alpha = arr[:, :, 3]
    red = rgb[:, :, 0]
    green = rgb[:, :, 1]
    blue = rgb[:, :, 2]

    chroma_green = (alpha > 0) & (green > red + 18) & (green > blue + 18) & (green > 42)
    strong_green = chroma_green & (red < 90) & (blue < 120)
    soft_fringe = chroma_green & ~strong_green & (red < 140) & (blue < 170)

    alpha[strong_green] = 0
    arr[strong_green, 0:3] = 0
    arr[soft_fringe, 1] = np.maximum(arr[soft_fringe, 0], arr[soft_fringe, 2])
    return Image.fromarray(arr, "RGBA")


def _load_walk_frames(source_dir: Path) -> tuple[list[Sprite], list[Image.Image]]:
    frame_paths = sorted(source_dir.glob("aben_walk_f*.png"))
    if len(frame_paths) < 8:
        raise ValueError(f"Expected at least 8 walk frames in {source_dir}, found {len(frame_paths)}")
    frame_paths = frame_paths[:8]

    frames = [_remove_green_fringe(Image.open(path).convert("RGBA")) for path in frame_paths]
    boxes = []
    for path, image in zip(frame_paths, frames):
        box = image.getchannel("A").getbbox()
        if not box:
            raise ValueError(f"{path} has no visible pixels")
        boxes.append(box)

    left = min(box[0] for box in boxes)
    top = min(box[1] for box in boxes)
    right = max(box[2] for box in boxes)
    bottom = max(box[3] for box in boxes)

    pad = 8
    crop_left = max(0, left - pad)
    crop_top = max(0, top - pad)
    crop_right = min(frames[0].width, right + pad)
    crop_bottom = min(frames[0].height, bottom + pad)

    crop_height = crop_bottom - crop_top
    scale = TARGET_WALK_HEIGHT / crop_height
    target_width = max(1, round((crop_right - crop_left) * scale))
    target_height = TARGET_WALK_HEIGHT
    origin_x = round((((left + right) / 2.0) - crop_left) * scale)
    origin_y = round((bottom - crop_top) * scale)

    sprites: list[Sprite] = []
    preview_frames: list[Image.Image] = []
    for image_number, image in enumerate(frames):
        cropped = image.crop((crop_left, crop_top, crop_right, crop_bottom))
        resized = cropped.resize((target_width, target_height), Image.Resampling.LANCZOS)
        preview_frames.append(resized)
        sprites.append(
            Sprite(
                group=WALK_GROUP,
                image=image_number,
                axis_x=origin_x,
                axis_y=origin_y,
                data=_rgba_to_indexed_pcx(resized),
            )
        )

    return sprites, preview_frames


def _write_sff_v1(path: Path, header: bytearray, sprites: list[Sprite]) -> None:
    if len(header) != 512:
        raise ValueError("SFF v1 header must be 512 bytes")

    groups = len({sprite.group for sprite in sprites})
    struct.pack_into("<I", header, 16, groups)
    struct.pack_into("<I", header, 20, len(sprites))
    struct.pack_into("<I", header, 24, 512)
    struct.pack_into("<I", header, 28, 32)

    chunks = bytearray(header)
    offset = 512
    for index, sprite in enumerate(sprites):
        next_offset = offset + 32 + len(sprite.data)
        if index == len(sprites) - 1:
            next_offset = 0

        subheader = bytearray(32)
        struct.pack_into("<II", subheader, 0, next_offset, len(sprite.data))
        struct.pack_into(
            "<hhhhh",
            subheader,
            8,
            sprite.axis_x,
            sprite.axis_y,
            sprite.group,
            sprite.image,
            sprite.linked_index,
        )
        subheader[18] = sprite.shared_palette & 0xFF

        chunks.extend(subheader)
        chunks.extend(sprite.data)
        offset += 32 + len(sprite.data)

    path.write_bytes(chunks)


def _write_preview(path: Path, frames: list[Image.Image]) -> None:
    scaled = []
    for frame in frames:
        scale = 2
        scaled.append(frame.resize((frame.width * scale, frame.height * scale), Image.Resampling.NEAREST))
    scaled[0].save(
        path,
        save_all=True,
        append_images=scaled[1:],
        duration=96,
        loop=0,
        disposal=2,
    )


def _write_shop_frames(path: Path, frames: list[Image.Image]) -> None:
    path.mkdir(parents=True, exist_ok=True)
    for index, frame in enumerate(frames):
        frame.save(path / f"shop_player_walk_{index}.png")


def _resolve_source_dir(source_dir: Path | None) -> Path:
    if source_dir is not None:
        if not source_dir.exists():
            raise FileNotFoundError(source_dir)
        return source_dir
    for candidate in DEFAULT_SOURCE_DIRS:
        if candidate.exists():
            return candidate
    raise FileNotFoundError("Could not find aben_walk_assets/frames_8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, help="Folder containing aben_walk_f*.png frames")
    parser.add_argument("--char-dir", type=Path, default=CHAR_DIR, help="A.Ben character directory")
    parser.add_argument("--preview", type=Path, help="Optional output GIF preview path")
    parser.add_argument("--shop-frame-dir", type=Path, help="Optional output directory for shop_player_walk_*.png")
    args = parser.parse_args()

    char_dir = args.char_dir
    source_dir = _resolve_source_dir(args.source_dir)
    sff_path = char_dir / "A.Ben.sff"

    header, existing_sprites = _read_sff_v1(sff_path)
    kept_sprites = [sprite for sprite in existing_sprites if sprite.group != WALK_GROUP]
    walk_sprites, preview_frames = _load_walk_frames(source_dir)

    _write_sff_v1(sff_path, header, kept_sprites + walk_sprites)
    if args.shop_frame_dir:
        shop_frame_dir = args.shop_frame_dir
        if not shop_frame_dir.is_absolute():
            shop_frame_dir = REPO_ROOT / shop_frame_dir
        _write_shop_frames(shop_frame_dir, preview_frames)
    if args.preview:
        args.preview.parent.mkdir(parents=True, exist_ok=True)
        _write_preview(args.preview, preview_frames)

    print(f"Updated {sff_path}")
    print(f"Kept {len(kept_sprites)} existing sprites; added {len(walk_sprites)} walk sprites from {source_dir}")
    for sprite in walk_sprites:
        print(
            f"  {sprite.group},{sprite.image}: axis=({sprite.axis_x},{sprite.axis_y}) "
            f"bytes={len(sprite.data)}"
        )
    if args.shop_frame_dir:
        print(f"Wrote shop walk PNGs to {shop_frame_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
