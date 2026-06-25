#!/usr/bin/env python3
"""Prepare local shop concept PNGs into runtime-ready alpha assets.

The concept export currently uses a visible checkerboard background instead of
real alpha. This tool crops the requested source art, removes that checkerboard,
defringes bright edge pixels, and writes deterministic PNG assets for the shop
runtime.
"""

from __future__ import annotations

import argparse
from collections import deque
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageFilter


@dataclass(frozen=True)
class AssetJob:
    source_name: str
    output: str
    crop: tuple[int, int, int, int] | None = None
    remove_checker: bool = True
    max_size: tuple[int, int] | None = None


DEFAULT_SOURCE_DIR = Path.home() / "Desktop" / "New folder"


ASSETS: tuple[AssetJob, ...] = (
    AssetJob(
        "1b6aeb1e-fade-466b-88d4-18596525a75f.png",
        "game/data/shop/i_chie_shop_backdrop.png",
        remove_checker=False,
    ),
    AssetJob(
        "6fec4058-15c3-44c0-9e46-43b59927c54a.png",
        "game/data/shop/i_chie_shop_counter_front.png",
        crop=(20, 225, 1652, 760),
    ),
    AssetJob(
        "6fec4058-15c3-44c0-9e46-43b59927c54a.png",
        "game/data/shop/i_chie_shop_counter_back.png",
        crop=(20, 225, 1652, 760),
    ),
    AssetJob(
        "610567c4-ff93-4029-8c61-09106d85b783.png",
        "game/data/shop/shop_player_back_pose.png",
        crop=(45, 42, 365, 1048),
    ),
    AssetJob(
        "36c2ffb4-9c69-4841-92a7-7cceba871fbd.png",
        "game/chars/I.Chie/I.Chie_shopkeeper_pose.png",
        crop=(1018, 42, 1438, 1042),
    ),
    AssetJob(
        "020935bb-462c-49c2-9c73-8470ab3317be.png",
        "game/data/shop/items/training_weight.png",
        crop=(40, 36, 420, 442),
        max_size=(96, 96),
    ),
    AssetJob(
        "020935bb-462c-49c2-9c73-8470ab3317be.png",
        "game/data/shop/items/guard_charm.png",
        crop=(455, 34, 770, 438),
        max_size=(96, 96),
    ),
    AssetJob(
        "020935bb-462c-49c2-9c73-8470ab3317be.png",
        "game/data/shop/items/dragon_sash.png",
        crop=(790, 54, 1235, 430),
        max_size=(112, 96),
    ),
)


def repo_root_from_script() -> Path:
    return Path(__file__).resolve().parents[2]


def crop_to_content(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    bbox = alpha.getbbox()
    return rgba.crop(bbox) if bbox else rgba


def remove_checkerboard(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    arr = np.array(rgba)
    rgb = arr[:, :, :3].astype(np.int16)
    r = rgb[:, :, 0]
    g = rgb[:, :, 1]
    b = rgb[:, :, 2]
    high = rgb.max(axis=2)
    low = rgb.min(axis=2)

    # The generated sources have pale grey/white checker pixels baked in. A
    # border flood keeps internal neutral clothing highlights while removing the
    # connected fake-transparent canvas and its anti-aliased fringe.
    pale_neutral = (high >= 170) & ((high - low) <= 58)
    very_pale = (r >= 226) & (g >= 226) & (b >= 226)
    candidate = pale_neutral | very_pale
    height, width = candidate.shape
    background = np.zeros_like(candidate, dtype=bool)
    queue: deque[tuple[int, int]] = deque()

    def add_seed(x: int, y: int) -> None:
        if candidate[y, x] and not background[y, x]:
            background[y, x] = True
            queue.append((x, y))

    for x in range(width):
        add_seed(x, 0)
        add_seed(x, height - 1)
    for y in range(height):
        add_seed(0, y)
        add_seed(width - 1, y)

    while queue:
        x, y = queue.popleft()
        for nx in (x - 1, x, x + 1):
            for ny in (y - 1, y, y + 1):
                if nx == x and ny == y:
                    continue
                if 0 <= nx < width and 0 <= ny < height and candidate[ny, nx] and not background[ny, nx]:
                    background[ny, nx] = True
                    queue.append((nx, ny))

    edge_neutral = (high >= 142) & ((high - low) <= 76)
    background_img = Image.fromarray(background.astype(np.uint8) * 255, "L")
    expanded_background = np.array(background_img.filter(ImageFilter.MaxFilter(5))) > 0
    final_background = background | (expanded_background & edge_neutral)

    alpha = (~final_background).astype(np.uint8) * 255
    arr[:, :, 3] = alpha
    arr[final_background, 0:3] = 0

    return crop_to_content(Image.fromarray(arr, "RGBA"))


def resize_to_max(image: Image.Image, max_size: tuple[int, int] | None) -> Image.Image:
    if not max_size:
        return image
    resized = image.copy()
    resized.thumbnail(max_size, Image.Resampling.LANCZOS)
    return resized


def prepare_asset(source_root: Path, repo_root: Path, job: AssetJob) -> Path:
    src = source_root / job.source_name
    if not src.exists():
        raise FileNotFoundError(f"missing source asset: {src}")

    image = Image.open(src).convert("RGBA")
    if job.crop:
        image = image.crop(job.crop)
    if job.remove_checker:
        image = remove_checkerboard(image)
    else:
        image = crop_to_content(image)
    image = resize_to_max(image, job.max_size)

    out = repo_root / job.output
    out.parent.mkdir(parents=True, exist_ok=True)
    image.save(out)
    return out


def build_preview(repo_root: Path, output: Path) -> None:
    preview = Image.new("RGBA", (960, 540), (9, 12, 18, 255))
    backdrop = Image.open(repo_root / "game/data/shop/i_chie_shop_backdrop.png").convert("RGBA")
    backdrop.thumbnail((960, 540), Image.Resampling.LANCZOS)
    preview.alpha_composite(backdrop, ((960 - backdrop.width) // 2, (540 - backdrop.height) // 2))

    for rel, pos, max_size in (
        ("game/data/shop/shop_player_back_pose.png", (210, 182), (110, 190)),
        ("game/chars/I.Chie/I.Chie_shopkeeper_pose.png", (600, 154), (130, 210)),
        ("game/data/shop/i_chie_shop_counter_front.png", (160, 312), (680, 120)),
    ):
        sprite = Image.open(repo_root / rel).convert("RGBA")
        sprite.thumbnail(max_size, Image.Resampling.LANCZOS)
        preview.alpha_composite(sprite, pos)

    output.parent.mkdir(parents=True, exist_ok=True)
    preview.save(output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE_DIR)
    parser.add_argument("--repo-root", type=Path, default=repo_root_from_script())
    parser.add_argument("--preview", type=Path, default=None)
    args = parser.parse_args()

    source_root = args.source.expanduser().resolve()
    repo_root = args.repo_root.expanduser().resolve()
    for job in ASSETS:
        out = prepare_asset(source_root, repo_root, job)
        with Image.open(out) as image:
            print(f"wrote {out} {image.width}x{image.height}")

    preview = args.preview or (repo_root / "artifacts/shop_generated_asset_preview.png")
    build_preview(repo_root, preview)
    print(f"wrote {preview}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
