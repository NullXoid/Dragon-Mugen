#!/usr/bin/env python3
"""Crop approved Shop Hub alpha sheets into named V2 runtime assets."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


PROP_CROPS = {
    "shelf_cabinet.png": (48, 52, 496, 736),
    "ichie_banner.png": (552, 60, 740, 684),
    "dragon_emblem.png": (752, 120, 1032, 492),
    "hologram_terminal.png": (1052, 68, 1380, 380),
    "neon_wall_panel.png": (788, 512, 1408, 656),
    "large_crate.png": (52, 832, 344, 1024),
    "medium_crate.png": (372, 852, 556, 1020),
    "small_crate.png": (592, 852, 792, 1024),
    "crystal_pedestal.png": (1168, 668, 1392, 932),
    "sword_stand.png": (820, 868, 1316, 1044),
}

ICHIE_CROPS = {
    "ichie_front.png": (80, 80, 336, 980),
    "ichie_three_quarter.png": (416, 84, 700, 984),
    "ichie_lean.png": (760, 84, 1024, 988),
    "ichie_welcome.png": (1004, 84, 1440, 988),
}


def crop_sheet(sheet_path: Path, crops: dict[str, tuple[int, int, int, int]], out_dir: Path) -> None:
    image = Image.open(sheet_path).convert("RGBA")
    alpha = image.getchannel("A")
    if alpha.getextrema()[0] != 0:
        raise ValueError(f"{sheet_path} has no transparent pixels")
    for filename, bounds in crops.items():
        crop = image.crop(bounds)
        if crop.getchannel("A").getbbox() is None:
            raise ValueError(f"{filename} crop is empty")
        crop.save(out_dir / filename, optimize=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--props-sheet", type=Path, required=True)
    parser.add_argument("--ichie-sheet", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    crop_sheet(args.props_sheet, PROP_CROPS, args.out_dir)
    crop_sheet(args.ichie_sheet, ICHIE_CROPS, args.out_dir)
    print(f"Prepared {len(PROP_CROPS) + len(ICHIE_CROPS)} Shop V2 assets in {args.out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
