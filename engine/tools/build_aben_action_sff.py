#!/usr/bin/env python3
"""Derive A.Ben prototype action sprites from the accepted walk frame source.

The output is intentionally a prototype bridge: it replaces the old front-facing
placeholder during movement and strikes while keeping every generated frame in a
replaceable sprite group.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from PIL import Image, ImageDraw

from build_aben_walk_sff import (
    CHAR_DIR,
    Sprite,
    _load_walk_frames,
    _read_sff_v1,
    _resolve_source_dir,
    _rgba_to_indexed_pcx,
    _write_sff_v1,
)


ACTION_GROUPS = {
    40,
    41,
    42,
    47,
    50,
    52,
    100,
    105,
    200,
    210,
    220,
    230,
    240,
    250,
    1000,
    1010,
    1020,
    1100,
    1110,
    1120,
    3000,
}


@dataclass(frozen=True)
class GeneratedFrame:
    group: int
    image: int
    png: Image.Image
    axis_x: int
    axis_y: int


OverlayFn = Callable[[ImageDraw.ImageDraw, int, int], None]


def _visible_bbox(image: Image.Image) -> tuple[int, int, int, int]:
    box = image.getchannel("A").getbbox()
    if not box:
        raise ValueError("Generated action frame has no visible pixels")
    return box


def _transform_body(frame: Image.Image, scale_x: float = 1.0, scale_y: float = 1.0, rotate: float = 0.0) -> Image.Image:
    rgba = frame.convert("RGBA")
    width = max(1, round(rgba.width * scale_x))
    height = max(1, round(rgba.height * scale_y))
    transformed = rgba.resize((width, height), Image.Resampling.LANCZOS)
    if rotate:
        transformed = transformed.rotate(rotate, resample=Image.Resampling.BICUBIC, expand=True)
    return transformed


def _compose(
    group: int,
    image_no: int,
    frame: Image.Image,
    *,
    scale_x: float = 1.0,
    scale_y: float = 1.0,
    rotate: float = 0.0,
    overlay: OverlayFn | None = None,
    pad: int = 6,
) -> GeneratedFrame:
    origin_x = 72
    origin_y = 132
    canvas = Image.new("RGBA", (194, 152), (0, 0, 0, 0))
    body = _transform_body(frame, scale_x, scale_y, rotate)
    body_box = _visible_bbox(body)
    body_center = (body_box[0] + body_box[2]) * 0.5
    body_bottom = body_box[3]
    paste_x = round(origin_x - body_center)
    paste_y = round(origin_y - body_bottom)
    canvas.alpha_composite(body, (paste_x, paste_y))

    if overlay:
        draw = ImageDraw.Draw(canvas, "RGBA")
        overlay(draw, origin_x, origin_y)

    box = _visible_bbox(canvas)
    crop = (
        max(0, box[0] - pad),
        max(0, box[1] - pad),
        min(canvas.width, box[2] + pad),
        min(canvas.height, box[3] + pad),
    )
    cropped = canvas.crop(crop)
    return GeneratedFrame(
        group=group,
        image=image_no,
        png=cropped,
        axis_x=origin_x - crop[0],
        axis_y=origin_y - crop[1],
    )


def _speed_lines(draw: ImageDraw.ImageDraw, origin_x: int, origin_y: int) -> None:
    for offset, alpha in ((-56, 90), (-44, 70), (-32, 55)):
        y = origin_y - 66 + (offset // 8)
        draw.line((origin_x + offset, y, origin_x - 10, y - 5), fill=(35, 175, 255, alpha), width=2)
        draw.line((origin_x + offset - 4, y + 22, origin_x - 12, y + 17), fill=(35, 175, 255, alpha), width=2)


def _energy_lines(draw: ImageDraw.ImageDraw, origin_x: int, origin_y: int) -> None:
    for yoff, alpha in ((-84, 150), (-68, 120), (-50, 95)):
        draw.line((origin_x + 18, origin_y + yoff, origin_x + 76, origin_y + yoff - 4), fill=(63, 219, 255, alpha), width=2)


def _draw_punch(draw: ImageDraw.ImageDraw, origin_x: int, origin_y: int, reach: int, high: int, energy: bool = False) -> None:
    shoulder = (origin_x + 8, origin_y - high)
    elbow = (origin_x + reach // 2, origin_y - high - 3)
    fist = (origin_x + reach, origin_y - high - 5)
    draw.line((shoulder, elbow, fist), fill=(7, 10, 15, 255), width=9, joint="curve")
    draw.line((shoulder[0] + 1, shoulder[1] - 2, fist[0] - 4, fist[1] - 2), fill=(0, 117, 214, 230), width=2)
    draw.ellipse((fist[0] - 4, fist[1] - 5, fist[0] + 7, fist[1] + 5), fill=(237, 219, 171, 255))
    if energy:
        _energy_lines(draw, origin_x, origin_y)


def _draw_kick(draw: ImageDraw.ImageDraw, origin_x: int, origin_y: int, reach: int, height: int, energy: bool = False) -> None:
    hip = (origin_x + 2, origin_y - 54)
    knee = (origin_x + reach // 2, origin_y - height + 7)
    foot = (origin_x + reach, origin_y - height)
    draw.line((hip, knee, foot), fill=(6, 8, 12, 255), width=11, joint="curve")
    draw.line((hip[0] + 2, hip[1] - 1, foot[0] - 8, foot[1] - 1), fill=(0, 128, 222, 235), width=2)
    draw.ellipse((foot[0] - 3, foot[1] - 4, foot[0] + 14, foot[1] + 5), fill=(242, 248, 255, 255))
    draw.line((foot[0] + 3, foot[1] + 2, foot[0] + 14, foot[1] + 2), fill=(0, 94, 210, 255), width=2)
    if energy:
        _energy_lines(draw, origin_x, origin_y)


def _draw_dash_strike(draw: ImageDraw.ImageDraw, origin_x: int, origin_y: int) -> None:
    _speed_lines(draw, origin_x, origin_y)
    _draw_punch(draw, origin_x, origin_y, 74, 70, energy=True)


def _generate_frames(walk_frames: list[Image.Image]) -> list[GeneratedFrame]:
    base = walk_frames
    frames: list[GeneratedFrame] = []

    # Jump start, air, fall, and land frames.
    frames.append(_compose(40, 0, base[1], scale_x=1.07, scale_y=0.90))
    frames.append(_compose(41, 0, base[5], scale_x=0.96, scale_y=1.05, rotate=-4))
    frames.append(_compose(41, 1, base[6], scale_x=0.96, scale_y=1.05, rotate=4))
    frames.append(_compose(42, 0, base[0], scale_x=1.02, scale_y=1.03, rotate=-8))
    frames.append(_compose(42, 1, base[4], scale_x=1.02, scale_y=1.03, rotate=-5))
    frames.append(_compose(47, 0, base[2], rotate=4))
    frames.append(_compose(50, 0, base[3], rotate=6))
    frames.append(_compose(52, 0, base[7], scale_x=1.08, scale_y=0.92))

    # Dash/run forward and back-hop frames.
    for image_no, source_index in enumerate((4, 5, 6, 7, 0, 1)):
        frames.append(_compose(100, image_no, base[source_index], scale_x=1.05, rotate=-3, overlay=_speed_lines))
    for image_no, source_index in enumerate((7, 6, 5, 4, 3, 2)):
        frames.append(_compose(105, image_no, base[source_index], scale_x=1.02, rotate=3))

    # Normal punches.
    for group, reach, high, count in ((200, 46, 70, 3), (210, 56, 72, 4), (220, 66, 74, 5)):
        frames.append(_compose(group, 0, base[7], scale_x=0.98))
        frames.append(_compose(group, 1, base[4], scale_x=1.02, rotate=-2, overlay=lambda d, x, y, r=reach, h=high: _draw_punch(d, x, y, r, h)))
        frames.append(_compose(group, 2, base[5], scale_x=1.00, rotate=2))
        for extra in range(3, count):
            frames.append(_compose(group, extra, base[6], scale_x=0.98))

    # Normal kicks.
    for group, reach, height, count in ((230, 42, 34, 3), (240, 56, 48, 4), (250, 66, 64, 5)):
        frames.append(_compose(group, 0, base[2], scale_x=0.98))
        frames.append(_compose(group, 1, base[3], scale_x=1.02, rotate=-2, overlay=lambda d, x, y, r=reach, h=height: _draw_kick(d, x, y, r, h)))
        frames.append(_compose(group, 2, base[4], scale_x=1.00))
        for extra in range(3, count):
            frames.append(_compose(group, extra, base[5], scale_x=0.98))

    # Prototype specials reuse stronger punch/kick silhouettes with light energy.
    for group, reach, high in ((1000, 58, 70), (1010, 68, 74), (1020, 78, 76)):
        frames.append(_compose(group, 0, base[7], scale_x=0.98))
        frames.append(_compose(group, 1, base[4], scale_x=1.04, rotate=-4, overlay=lambda d, x, y, r=reach, h=high: _draw_punch(d, x, y, r, h, energy=True)))
        frames.append(_compose(group, 2, base[5], scale_x=1.00))
    for group, reach, height in ((1100, 58, 56), (1110, 68, 68), (1120, 78, 78)):
        frames.append(_compose(group, 0, base[2], scale_x=0.98))
        frames.append(_compose(group, 1, base[3], scale_x=1.04, rotate=-4, overlay=lambda d, x, y, r=reach, h=height: _draw_kick(d, x, y, r, h, energy=True)))
        frames.append(_compose(group, 2, base[4], scale_x=1.00))

    for image_no, source_index in enumerate((4, 5, 6, 7, 0, 1)):
        frames.append(_compose(3000, image_no, base[source_index], scale_x=1.06, rotate=-5, overlay=_draw_dash_strike))

    return frames


def _write_png_sources(output_dir: Path, frames: list[GeneratedFrame]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for stale in output_dir.glob("aben_action_*.png"):
        stale.unlink()
    for frame in frames:
        frame.png.save(output_dir / f"aben_action_{frame.group}_{frame.image:02}.png")


def _write_contact_sheet(path: Path, frames: list[GeneratedFrame]) -> None:
    groups: list[int] = []
    for frame in frames:
        if frame.group not in groups:
            groups.append(frame.group)

    cell_w = 110
    cell_h = 146
    label_h = 16
    cols = 6
    rows = sum((len([f for f in frames if f.group == group]) + cols - 1) // cols for group in groups)
    sheet = Image.new("RGBA", (cols * cell_w, rows * (cell_h + label_h)), (18, 22, 28, 255))
    draw = ImageDraw.Draw(sheet)
    row = 0
    for group in groups:
        group_frames = [f for f in frames if f.group == group]
        for index, frame in enumerate(group_frames):
            col = index % cols
            if index and col == 0:
                row += 1
            x = col * cell_w
            y = row * (cell_h + label_h)
            draw.text((x + 4, y + 2), f"{group},{frame.image}", fill=(233, 237, 243, 255))
            origin_x = x + cell_w // 2
            origin_y = y + label_h + 126
            paste_x = origin_x - frame.axis_x
            paste_y = origin_y - frame.axis_y
            sheet.alpha_composite(frame.png, (paste_x, paste_y))
            draw.line((x, origin_y, x + cell_w, origin_y), fill=(81, 210, 198, 80))
        row += 1
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


def _to_sprite(frame: GeneratedFrame) -> Sprite:
    return Sprite(
        group=frame.group,
        image=frame.image,
        axis_x=frame.axis_x,
        axis_y=frame.axis_y,
        data=_rgba_to_indexed_pcx(frame.png),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, help="Folder containing accepted aben_walk_f*.png frames")
    parser.add_argument("--char-dir", type=Path, default=CHAR_DIR, help="A.Ben character directory")
    parser.add_argument(
        "--source-output-dir",
        type=Path,
        default=Path("game/chars/A.Ben/generated_actions"),
        help="Project-relative folder for generated action PNG sources",
    )
    parser.add_argument("--contact-sheet", type=Path, help="Optional contact sheet output path")
    args = parser.parse_args()

    char_dir = args.char_dir
    sff_path = char_dir / "A.Ben.sff"
    source_dir = _resolve_source_dir(args.source_dir)
    _, walk_frames = _load_walk_frames(source_dir)
    generated = _generate_frames(walk_frames)

    source_output_dir = args.source_output_dir
    if not source_output_dir.is_absolute():
        source_output_dir = Path(__file__).resolve().parents[2] / source_output_dir
    _write_png_sources(source_output_dir, generated)

    header, existing_sprites = _read_sff_v1(sff_path)
    kept = [sprite for sprite in existing_sprites if sprite.group not in ACTION_GROUPS]
    _write_sff_v1(sff_path, header, kept + [_to_sprite(frame) for frame in generated])

    if args.contact_sheet:
        contact_sheet = args.contact_sheet
        if not contact_sheet.is_absolute():
            contact_sheet = Path(__file__).resolve().parents[2] / contact_sheet
        _write_contact_sheet(contact_sheet, generated)

    print(f"Updated {sff_path}")
    print(f"Kept {len(kept)} existing sprites; added {len(generated)} action sprites from {source_dir}")
    print(f"Wrote action PNG sources to {source_output_dir}")
    if args.contact_sheet:
        print(f"Wrote contact sheet to {contact_sheet}")
    for group in sorted(ACTION_GROUPS):
        count = len([frame for frame in generated if frame.group == group])
        print(f"  group {group}: {count} sprites")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

