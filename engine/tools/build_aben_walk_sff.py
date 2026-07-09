#!/usr/bin/env python3
"""Build A.Ben's SFF and shop walk PNGs from curated source frames."""

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
    CHAR_DIR / "source_art" / "curated_game_sprites" / "frames" / "walk",
)
TARGET_WALK_HEIGHT = 122
CROUCH_TARGET_HEIGHT = 88
IDLE_GROUP = 0
CROUCH_GROUP = 10
WALK_GROUP = 20
DEPTH_TOWARD_GROUP = 24
DEPTH_AWAY_GROUP = 25
ACTION_GROUPS = {
    "dash": 100,
    "jump": 40,
    "jump_forward": 42,
    "jump_back": 43,
    "punch": 200,
    "kick": 230,
}
ACTION_PADDING = {
    "kick": 24,
}
DEFAULT_ACTION_SOURCE_ROOTS = (
    CHAR_DIR / "source_art" / "curated_game_sprites" / "frames",
)
DEFAULT_SHOP_FRAME_DIR = CHAR_DIR / "shop" / "walk"
PORTRAIT_GROUP = 9000
PORTRAIT_BIG_IMAGE = 1
IDLE_FRAME_SEQUENCE = (
    ("punch", 0),
    ("punch", 1),
    ("punch", 9),
    ("punch", 9),
    ("punch", 1),
    ("punch", 0),
)


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
    if not frame_paths:
        frame_paths = sorted(source_dir.glob("walk_*.png"))
    if len(frame_paths) < 8:
        raise ValueError(f"Expected at least 8 aben_walk_f*.png or walk_*.png frames in {source_dir}, found {len(frame_paths)}")
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


def _load_action_frames(source_root: Path) -> tuple[list[Sprite], dict[str, list[Image.Image]]]:
    sprites: list[Sprite] = []
    previews: dict[str, list[Image.Image]] = {}
    for action_name, group in ACTION_GROUPS.items():
        action_dir = source_root / action_name
        frame_paths = sorted(action_dir.glob(f"{action_name}_*.png"))
        if not frame_paths:
            raise ValueError(f"Expected {action_name}_*.png frames in {action_dir}")

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

        pad = ACTION_PADDING.get(action_name, 8)
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

        action_preview: list[Image.Image] = []
        for image_number, image in enumerate(frames):
            cropped = image.crop((crop_left, crop_top, crop_right, crop_bottom))
            resized = cropped.resize((target_width, target_height), Image.Resampling.LANCZOS)
            action_preview.append(resized)
            sprites.append(
                Sprite(
                    group=group,
                    image=image_number,
                    axis_x=origin_x,
                    axis_y=origin_y,
                    data=_rgba_to_indexed_pcx(resized),
                )
            )
        previews[action_name] = action_preview

    return sprites, previews


def _clone_walk_group(walk_sprites: list[Sprite], group: int) -> list[Sprite]:
    return [
        Sprite(
            group=group,
            image=sprite.image,
            axis_x=sprite.axis_x,
            axis_y=sprite.axis_y,
            data=sprite.data,
            linked_index=sprite.linked_index,
            shared_palette=sprite.shared_palette,
        )
        for sprite in walk_sprites
    ]


def _load_crouch_frames(source_root: Path) -> tuple[list[Sprite], list[Image.Image]]:
    crouch_dir = source_root / "crouch"
    frame_paths = sorted(crouch_dir.glob("crouch_*.png"))
    if len(frame_paths) < 3:
        raise ValueError(f"Expected at least 3 crouch_*.png frames in {crouch_dir}, found {len(frame_paths)}")
    frame_paths = frame_paths[:3]

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

    pad = 12
    crop_left = max(0, left - pad)
    crop_top = max(0, top - pad)
    crop_right = min(frames[0].width, right + pad)
    crop_bottom = min(frames[0].height, bottom + pad)

    crop_height = crop_bottom - crop_top
    scale = CROUCH_TARGET_HEIGHT / crop_height
    target_width = max(1, round((crop_right - crop_left) * scale))
    target_height = CROUCH_TARGET_HEIGHT
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
                group=CROUCH_GROUP,
                image=image_number,
                axis_x=origin_x,
                axis_y=origin_y,
                data=_rgba_to_indexed_pcx(resized),
            )
        )

    return sprites, preview_frames


def _load_idle_frames(source_root: Path) -> tuple[list[Sprite], list[Image.Image]]:
    idle_dir = source_root / "idle"
    idle_candidates = sorted(idle_dir.glob("idle_*.png"))
    if idle_candidates:
        if len(idle_candidates) < 6:
            raise ValueError(f"Expected at least 6 idle_*.png frames in {idle_dir}, found {len(idle_candidates)}")
        frame_paths = idle_candidates[:6]
    else:
        frame_paths = []
        for action_name, requested_index in IDLE_FRAME_SEQUENCE:
            action_dir = source_root / action_name
            candidates = sorted(action_dir.glob(f"{action_name}_*.png"))
            if not candidates:
                raise ValueError(f"Expected {action_name}_*.png frames in {action_dir}")
            frame_paths.append(candidates[min(requested_index, len(candidates) - 1)])

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
                group=IDLE_GROUP,
                image=image_number,
                axis_x=origin_x,
                axis_y=origin_y,
                data=_rgba_to_indexed_pcx(resized),
            )
        )

    return sprites, preview_frames


def _cover_resize(image: Image.Image, target_size: tuple[int, int]) -> Image.Image:
    target_width, target_height = target_size
    scale = max(target_width / image.width, target_height / image.height)
    resized = image.resize(
        (max(1, round(image.width * scale)), max(1, round(image.height * scale))),
        Image.Resampling.LANCZOS,
    )
    left = max(0, (resized.width - target_width) // 2)
    top = max(0, (resized.height - target_height) // 2)
    return resized.crop((left, top, left + target_width, top + target_height))


def _load_big_portrait(path: Path, existing_sprites: list[Sprite]) -> tuple[Sprite, Image.Image]:
    if not path.exists():
        raise FileNotFoundError(path)

    existing = next(
        (
            sprite
            for sprite in existing_sprites
            if sprite.group == PORTRAIT_GROUP and sprite.image == PORTRAIT_BIG_IMAGE
        ),
        None,
    )
    target_size = (120, 140)
    axis_x = target_size[0] // 2
    axis_y = target_size[1] - 1
    if existing:
        try:
            with Image.open(io.BytesIO(existing.data)) as old_image:
                target_size = old_image.size
        except Exception:
            target_size = (max(1, existing.axis_x * 2), max(1, existing.axis_y + 1))
        axis_x = existing.axis_x
        axis_y = existing.axis_y

    portrait = _cover_resize(Image.open(path).convert("RGBA"), target_size)
    return (
        Sprite(
            group=PORTRAIT_GROUP,
            image=PORTRAIT_BIG_IMAGE,
            axis_x=axis_x,
            axis_y=axis_y,
            data=_rgba_to_indexed_pcx(portrait),
        ),
        portrait,
    )


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


def _resolve_action_source_root(action_source_root: Path | None) -> Path | None:
    if action_source_root is not None:
        if not action_source_root.exists():
            raise FileNotFoundError(action_source_root)
        return action_source_root
    for candidate in DEFAULT_ACTION_SOURCE_ROOTS:
        if candidate.exists():
            return candidate
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, help="Folder containing walk_*.png or aben_walk_f*.png frames")
    parser.add_argument("--char-dir", type=Path, default=CHAR_DIR, help="A.Ben character directory")
    parser.add_argument("--action-source-root", type=Path, help="Folder containing crouch/dash/jump/punch/kick frame folders")
    parser.add_argument("--skip-actions", action="store_true", help="Only rebuild the walk cycle")
    parser.add_argument("--big-portrait", type=Path, help="PNG used to replace SFF sprite 9000,1")
    parser.add_argument("--portrait-only", action="store_true", help="Replace only SFF sprite 9000,1")
    parser.add_argument("--preview", type=Path, help="Optional output GIF preview path")
    parser.add_argument(
        "--shop-frame-dir",
        type=Path,
        default=DEFAULT_SHOP_FRAME_DIR,
        help="Output directory for shop_player_walk_*.png; defaults to the A.Ben character shop folder",
    )
    args = parser.parse_args()

    if args.portrait_only and not args.big_portrait:
        parser.error("--portrait-only requires --big-portrait")
    char_dir = args.char_dir
    source_dir = None if args.portrait_only else _resolve_source_dir(args.source_dir)
    action_source_root = None if args.skip_actions or args.portrait_only else _resolve_action_source_root(args.action_source_root)
    sff_path = char_dir / "A.Ben.sff"

    header, existing_sprites = _read_sff_v1(sff_path)
    generated_groups = set() if args.portrait_only else {WALK_GROUP, DEPTH_TOWARD_GROUP, DEPTH_AWAY_GROUP}
    if action_source_root:
        generated_groups.add(IDLE_GROUP)
        generated_groups.add(CROUCH_GROUP)
        generated_groups.update(ACTION_GROUPS.values())
    kept_sprites = [
        sprite
        for sprite in existing_sprites
        if sprite.group not in generated_groups
        and not (args.big_portrait and sprite.group == PORTRAIT_GROUP and sprite.image == PORTRAIT_BIG_IMAGE)
    ]
    walk_sprites: list[Sprite] = []
    preview_frames: list[Image.Image] = []
    idle_sprites: list[Sprite] = []
    crouch_sprites: list[Sprite] = []
    action_sprites: list[Sprite] = []
    depth_sprites: list[Sprite] = []
    action_previews: dict[str, list[Image.Image]] = {}
    portrait_sprites: list[Sprite] = []
    if source_dir:
        walk_sprites, preview_frames = _load_walk_frames(source_dir)
        depth_sprites.extend(_clone_walk_group(walk_sprites, DEPTH_TOWARD_GROUP))
        depth_sprites.extend(_clone_walk_group(walk_sprites, DEPTH_AWAY_GROUP))
    if action_source_root:
        idle_sprites, _ = _load_idle_frames(action_source_root)
        crouch_sprites, _ = _load_crouch_frames(action_source_root)
        action_sprites, action_previews = _load_action_frames(action_source_root)
    if args.big_portrait:
        portrait_sprite, portrait_preview = _load_big_portrait(args.big_portrait, existing_sprites)
        portrait_sprites.append(portrait_sprite)
        portrait_preview.save(char_dir / "A.Ben_face_preview.png")

    _write_sff_v1(
        sff_path,
        header,
        kept_sprites + portrait_sprites + idle_sprites + crouch_sprites + walk_sprites + depth_sprites + action_sprites,
    )
    if args.shop_frame_dir and preview_frames:
        shop_frame_dir = args.shop_frame_dir
        if not shop_frame_dir.is_absolute():
            shop_frame_dir = REPO_ROOT / shop_frame_dir
        _write_shop_frames(shop_frame_dir, preview_frames)
    if args.preview and preview_frames:
        args.preview.parent.mkdir(parents=True, exist_ok=True)
        _write_preview(args.preview, preview_frames)

    print(f"Updated {sff_path}")
    if source_dir:
        print(f"Kept {len(kept_sprites)} existing sprites; added {len(walk_sprites)} walk sprites from {source_dir}")
    for sprite in walk_sprites:
        print(
            f"  {sprite.group},{sprite.image}: axis=({sprite.axis_x},{sprite.axis_y}) "
            f"bytes={len(sprite.data)}"
        )
    if action_source_root:
        print(f"Added {len(idle_sprites)} idle sprites from {action_source_root}")
        print(f"Added {len(crouch_sprites)} crouch sprites from {action_source_root}")
        print(f"Added {len(action_sprites)} action sprites from {action_source_root}")
        for action_name, group in ACTION_GROUPS.items():
            count = len(action_previews.get(action_name, []))
            print(f"  {action_name}: group={group} frames={count}")
        print(f"  depth_toward: group={DEPTH_TOWARD_GROUP} frames={len(walk_sprites)} source=walk fallback")
        print(f"  depth_away: group={DEPTH_AWAY_GROUP} frames={len(walk_sprites)} source=walk fallback")
    if portrait_sprites:
        sprite = portrait_sprites[0]
        print(
            f"Replaced big portrait {sprite.group},{sprite.image}: "
            f"axis=({sprite.axis_x},{sprite.axis_y}) bytes={len(sprite.data)}"
        )
    if args.shop_frame_dir and preview_frames:
        print(f"Wrote shop walk PNGs to {shop_frame_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
