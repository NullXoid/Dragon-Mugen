#!/usr/bin/env python3
"""Post-process Comfy/LTX videos into curated sprite-frame folders.

ComfyUI remains an external generator. This tool owns the deterministic steps
after a video exists: frame rip, review sheets, selected-frame promotion, and
manifest updates for the game-side SFF builder.
"""

from __future__ import annotations

import argparse
import json
import math
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageOps


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CHARACTER_ROOT = REPO_ROOT / "game" / "chars"
SUPPORTED_ACTIONS = {"idle", "walk", "dash", "jump", "punch", "kick"}
DEFAULT_CELL_WIDTH = 384
DEFAULT_CELL_HEIGHT = 672


def _abs(path: Path | None) -> str | None:
    return str(path.resolve()) if path is not None else None


def _character_folder_name(character: str) -> str:
    return character.strip().replace("/", "_").replace("\\", "_")


def _character_source_art_root(character: str) -> Path:
    return DEFAULT_CHARACTER_ROOT / _character_folder_name(character) / "source_art"


def _run(command: list[str]) -> None:
    try:
        subprocess.run(command, check=True)
    except FileNotFoundError as exc:
        raise RuntimeError(f"Required executable not found: {command[0]}") from exc
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"Command failed with exit {exc.returncode}: {' '.join(command)}") from exc


def _require_file(path: Path, label: str) -> Path:
    if not path.exists() or not path.is_file():
        raise FileNotFoundError(f"{label} not found: {path}")
    return path


def _load_json(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def _parse_rate(rate: str | None) -> float | None:
    if not rate or rate == "0/0":
        return None
    if "/" in rate:
        numerator, denominator = rate.split("/", 1)
        denominator_value = float(denominator)
        if denominator_value == 0:
            return None
        return float(numerator) / denominator_value
    return float(rate)


def _video_info(video: Path) -> dict[str, Any]:
    command = [
        "ffprobe",
        "-v",
        "error",
        "-select_streams",
        "v:0",
        "-show_entries",
        "stream=width,height,r_frame_rate,avg_frame_rate,nb_frames,duration",
        "-of",
        "json",
        str(video),
    ]
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
    except FileNotFoundError as exc:
        raise RuntimeError("Required executable not found: ffprobe") from exc
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"ffprobe failed for {video}: {exc.stderr}") from exc

    data = json.loads(result.stdout)
    streams = data.get("streams") or []
    if not streams:
        raise RuntimeError(f"No video stream found in {video}")
    stream = streams[0]
    fps = _parse_rate(stream.get("avg_frame_rate")) or _parse_rate(stream.get("r_frame_rate"))
    return {
        "width": int(stream.get("width") or 0),
        "height": int(stream.get("height") or 0),
        "fps": fps,
        "duration": float(stream.get("duration") or 0.0),
        "nb_frames": int(stream["nb_frames"]) if str(stream.get("nb_frames", "")).isdigit() else None,
    }


def _safe_reset_dir(path: Path, expected_parent: Path) -> None:
    resolved = path.resolve()
    parent = expected_parent.resolve()
    try:
        inside_parent = resolved.is_relative_to(parent)
    except AttributeError:
        inside_parent = str(resolved).lower().startswith(str(parent).lower() + "\\")
    if not inside_parent:
        raise RuntimeError(f"Refusing to remove directory outside {parent}: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)


def _checkerboard(size: tuple[int, int], cell: int = 16) -> Image.Image:
    width, height = size
    image = Image.new("RGBA", size, (230, 230, 230, 255))
    draw = ImageDraw.Draw(image)
    for y in range(0, height, cell):
        for x in range(0, width, cell):
            if ((x // cell) + (y // cell)) % 2:
                draw.rectangle((x, y, x + cell - 1, y + cell - 1), fill=(200, 200, 200, 255))
    return image


def _flatten_for_preview(image: Image.Image, background: tuple[int, int, int] = (34, 38, 44)) -> Image.Image:
    rgba = image.convert("RGBA")
    if rgba.getchannel("A").getextrema()[0] == 255:
        return rgba.convert("RGB")
    base = _checkerboard(rgba.size)
    base.alpha_composite(rgba)
    return base.convert("RGB")


def _fit_frame(image: Image.Image, cell_width: int, cell_height: int) -> Image.Image:
    rgba = image.convert("RGBA")
    fitted = ImageOps.contain(rgba, (cell_width, cell_height), method=Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (cell_width, cell_height), (0, 0, 0, 0))
    x = (cell_width - fitted.width) // 2
    y = (cell_height - fitted.height) // 2
    canvas.alpha_composite(fitted, (x, y))
    return canvas


def _frame_paths(folder: Path) -> list[Path]:
    return sorted(folder.glob("*.png"))


def _write_preview_gif(path: Path, frames: list[Image.Image], fps: float | None, max_height: int = 240) -> None:
    if not frames:
        raise ValueError("Cannot create preview GIF without frames")
    duration = max(20, round(1000.0 / (fps or 12.0)))
    preview_frames: list[Image.Image] = []
    for frame in frames:
        flattened = _flatten_for_preview(frame)
        if flattened.height > max_height:
            scale = max_height / flattened.height
            flattened = flattened.resize(
                (max(1, round(flattened.width * scale)), max_height),
                Image.Resampling.LANCZOS,
            )
        preview_frames.append(flattened)
    path.parent.mkdir(parents=True, exist_ok=True)
    preview_frames[0].save(
        path,
        save_all=True,
        append_images=preview_frames[1:],
        duration=duration,
        loop=0,
        disposal=2,
    )


def _write_sheet(path: Path, frames: list[Image.Image]) -> None:
    if not frames:
        raise ValueError("Cannot create sheet without frames")
    width = sum(frame.width for frame in frames)
    height = max(frame.height for frame in frames)
    sheet = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    x = 0
    for frame in frames:
        sheet.alpha_composite(frame.convert("RGBA"), (x, 0))
        x += frame.width
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


def _write_cropped_sheet(path: Path, frames: list[Image.Image]) -> tuple[int, int]:
    if not frames:
        raise ValueError("Cannot create cropped sheet without frames")
    boxes = [frame.convert("RGBA").getchannel("A").getbbox() for frame in frames]
    visible_boxes = [box for box in boxes if box]
    if not visible_boxes:
        _write_sheet(path, frames)
        return frames[0].width, frames[0].height
    left = min(box[0] for box in visible_boxes)
    top = min(box[1] for box in visible_boxes)
    right = max(box[2] for box in visible_boxes)
    bottom = max(box[3] for box in visible_boxes)
    cropped = [frame.crop((left, top, right, bottom)) for frame in frames]
    _write_sheet(path, cropped)
    return right - left, bottom - top


def _write_contact_sheet(path: Path, frame_paths: list[Path], labels: list[str], columns: int = 8) -> None:
    if not frame_paths:
        raise ValueError("Cannot create contact sheet without frames")
    thumbs: list[Image.Image] = []
    label_height = 18
    thumb_height = 160
    for frame_path, label in zip(frame_paths, labels):
        frame = Image.open(frame_path).convert("RGBA")
        thumb = ImageOps.contain(frame, (220, thumb_height), method=Image.Resampling.LANCZOS)
        tile = _checkerboard((220, thumb_height + label_height), cell=12)
        x = (220 - thumb.width) // 2
        tile.alpha_composite(thumb, (x, label_height))
        draw = ImageDraw.Draw(tile)
        draw.rectangle((0, 0, 219, label_height - 1), fill=(10, 14, 20, 230))
        draw.text((4, 2), label, fill=(233, 237, 243, 255))
        thumbs.append(tile)

    rows = math.ceil(len(thumbs) / columns)
    sheet = Image.new("RGBA", (columns * 220, rows * (thumb_height + label_height)), (0, 0, 0, 0))
    for index, thumb in enumerate(thumbs):
        x = (index % columns) * 220
        y = (index // columns) * (thumb_height + label_height)
        sheet.alpha_composite(thumb, (x, y))
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


def _parse_selected(value: str | None) -> list[int]:
    if value is None or not value.strip():
        return []
    selected: list[int] = []
    for piece in value.split(","):
        piece = piece.strip()
        if not piece:
            continue
        try:
            selected.append(int(piece))
        except ValueError as exc:
            raise ValueError(f"Invalid frame index in --selected: {piece!r}") from exc
    return selected


def _read_prompt(prompt: str | None, prompt_file: Path | None) -> str:
    parts: list[str] = []
    if prompt:
        parts.append(prompt)
    if prompt_file:
        _require_file(prompt_file, "prompt file")
        parts.append(prompt_file.read_text(encoding="utf-8").strip())
    return "\n\n".join(part for part in parts if part)


def prepare(args: argparse.Namespace) -> int:
    action = args.action.lower()
    if action not in SUPPORTED_ACTIONS:
        raise ValueError(f"Unsupported action {args.action!r}; expected one of {sorted(SUPPORTED_ACTIONS)}")

    source_video = _require_file(args.video, "source video").resolve()
    reference = _require_file(args.reference, "reference image").resolve() if args.reference else None
    workflow = _require_file(args.workflow, "workflow JSON").resolve() if args.workflow else None
    runs_root = (args.runs_root or (_character_source_art_root(args.character) / "ltx_runs")).resolve()
    run_name = args.run_name or f"{datetime.now():%Y%m%d_%H%M%S}_{args.character}_{action}".replace(".", "")
    run_dir = runs_root / run_name
    if run_dir.exists() and not args.force:
        raise FileExistsError(f"Run directory already exists: {run_dir} (use --force to replace it)")
    if args.force:
        _safe_reset_dir(run_dir, runs_root)

    input_dir = run_dir / "input"
    video_dir = run_dir / "video"
    raw_dir = run_dir / "frames_raw"
    clean_dir = run_dir / "frames_clean"
    sheet_dir = run_dir / "sheets"
    preview_dir = run_dir / "previews"
    for folder in (input_dir, video_dir, raw_dir, clean_dir, sheet_dir, preview_dir):
        folder.mkdir(parents=True, exist_ok=True)

    copied_video = video_dir / source_video.name
    shutil.copy2(source_video, copied_video)
    copied_reference = input_dir / reference.name if reference else None
    if reference:
        shutil.copy2(reference, copied_reference)
    copied_workflow = input_dir / workflow.name if workflow else None
    if workflow:
        shutil.copy2(workflow, copied_workflow)

    prompt_text = _read_prompt(args.prompt, args.prompt_file)
    prompt_path = input_dir / "prompt.txt"
    prompt_path.write_text(prompt_text, encoding="utf-8")

    info = _video_info(copied_video)
    command = ["ffmpeg", "-loglevel", "error", "-y", "-i", str(copied_video)]
    if args.sample_fps is not None:
        command += ["-vf", f"fps={args.sample_fps:g}"]
    command += [str(raw_dir / "frame_%04d.png")]
    _run(command)

    frames = _frame_paths(raw_dir)
    if not frames:
        raise RuntimeError(f"ffmpeg produced no PNG frames in {raw_dir}")

    first = Image.open(frames[0])
    frame_width, frame_height = first.size
    contact = sheet_dir / "contact_all.png"
    preview = preview_dir / "preview.gif"
    labels = [f"{index:03d}" for index in range(len(frames))]
    _write_contact_sheet(contact, frames, labels)
    preview_images = [Image.open(path).convert("RGBA") for path in frames]
    _write_preview_gif(preview, preview_images, args.sample_fps or info.get("fps"))

    manifest = {
        "character": args.character,
        "action": action,
        "run_name": run_name,
        "source_video": str(source_video),
        "copied_video": str(copied_video.resolve()),
        "reference_image": _abs(reference),
        "copied_reference_image": _abs(copied_reference),
        "prompt": prompt_text,
        "prompt_file": str(prompt_path.resolve()),
        "workflow_json": _abs(workflow),
        "copied_workflow_json": _abs(copied_workflow),
        "frame_width": frame_width,
        "frame_height": frame_height,
        "frame_count": len(frames),
        "source_fps": info.get("fps"),
        "sample_fps": args.sample_fps or info.get("fps"),
        "selected_source_frames": [],
        "paths": {
            "frames_raw": str(raw_dir.resolve()),
            "frames_clean": str(clean_dir.resolve()),
            "contact": str(contact.resolve()),
            "preview": str(preview.resolve()),
            "full_sheet": None,
            "cropped_sheet": None,
            "promoted_frames": [],
        },
    }
    _write_json(run_dir / "manifest.json", manifest)

    print(f"Prepared {args.character} {action} run: {run_dir}")
    print(f"Frames: {len(frames)} ({frame_width}x{frame_height})")
    print(f"Contact: {contact}")
    print(f"Preview: {preview}")
    return 0


def _source_frames_for_promote(run_dir: Path) -> tuple[Path, list[Path]]:
    clean_dir = run_dir / "frames_clean"
    raw_dir = run_dir / "frames_raw"
    clean_frames = _frame_paths(clean_dir)
    if clean_frames:
        return clean_dir, clean_frames
    raw_frames = _frame_paths(raw_dir)
    if raw_frames:
        return raw_dir, raw_frames
    raise FileNotFoundError(f"No PNG frames found in {clean_dir} or {raw_dir}")


def _clear_action_outputs(curated_root: Path, action: str) -> None:
    targets = [
        curated_root / "frames" / action,
        curated_root / "sheets",
        curated_root / "contacts",
        curated_root / "previews",
    ]
    for target in targets:
        target.mkdir(parents=True, exist_ok=True)
    for path in (curated_root / "frames" / action).glob(f"{action}_*.png"):
        path.unlink()
    for folder, pattern in (
        (curated_root / "sheets", f"{action}_selected*.png"),
        (curated_root / "contacts", f"{action}_selected_contact.png"),
        (curated_root / "previews", f"{action}_selected_preview.gif"),
    ):
        for path in folder.glob(pattern):
            path.unlink()


def promote(args: argparse.Namespace) -> int:
    run_dir = args.run_dir.resolve()
    manifest_path = run_dir / "manifest.json"
    manifest = _load_json(manifest_path)
    if not manifest:
        raise FileNotFoundError(f"Run manifest not found: {manifest_path}")

    action = str(manifest.get("action", "")).lower()
    if action not in SUPPORTED_ACTIONS:
        raise ValueError(f"Unsupported action in manifest: {action!r}")

    selected = _parse_selected(args.selected)
    if not selected:
        selected = [int(value) for value in manifest.get("selected_source_frames", [])]
    if not selected:
        raise ValueError("No selected frames provided. Use --selected or set selected_source_frames in the manifest.")

    source_dir, source_frames = _source_frames_for_promote(run_dir)
    invalid = [index for index in selected if index < 0 or index >= len(source_frames)]
    if invalid:
        raise ValueError(f"Selected frame indices out of range 0..{len(source_frames) - 1}: {invalid}")

    curated_character = str(manifest.get("character") or "A.Ben")
    curated_root = (args.curated_root or (_character_source_art_root(curated_character) / "curated_game_sprites")).resolve()
    cell_width = int(args.cell_width or manifest.get("cell_width") or DEFAULT_CELL_WIDTH)
    cell_height = int(args.cell_height or manifest.get("cell_height") or DEFAULT_CELL_HEIGHT)
    _clear_action_outputs(curated_root, action)

    action_dir = curated_root / "frames" / action
    selected_paths: list[Path] = []
    normalized_frames: list[Image.Image] = []
    for output_index, source_index in enumerate(selected):
        source_path = source_frames[source_index]
        normalized = _fit_frame(Image.open(source_path), cell_width, cell_height)
        output_path = action_dir / f"{action}_{output_index:02d}_src{source_index:03d}_{cell_width}x{cell_height}.png"
        normalized.save(output_path)
        selected_paths.append(output_path)
        normalized_frames.append(normalized)

    sheets_dir = curated_root / "sheets"
    contacts_dir = curated_root / "contacts"
    previews_dir = curated_root / "previews"
    full_sheet = sheets_dir / f"{action}_selected_{cell_width}x{cell_height}.png"
    cropped_sheet = sheets_dir / f"{action}_selected_cropped_{cell_width}x{cell_height}.png"
    contact = contacts_dir / f"{action}_selected_contact.png"
    preview = previews_dir / f"{action}_selected_preview.gif"

    _write_sheet(full_sheet, normalized_frames)
    cropped_width, cropped_height = _write_cropped_sheet(cropped_sheet, normalized_frames)
    _write_contact_sheet(contact, selected_paths, [f"{i:02d}/{src:03d}" for i, src in enumerate(selected)])
    _write_preview_gif(preview, normalized_frames, manifest.get("sample_fps") or 12.0)

    curated_manifest_path = curated_root / "manifest.json"
    curated_manifest = _load_json(curated_manifest_path)
    curated_manifest.setdefault("description", "Curated selected-frame sheets from Comfy/LTX action revisions.")
    curated_manifest["source"] = str(source_dir.resolve())
    curated_manifest["cell_full"] = {"width": cell_width, "height": cell_height}
    actions = curated_manifest.setdefault("actions", {})
    existing_notes = actions.get(action, {}).get("notes", "")
    actions[action] = {
        "selected_source_frames": selected,
        "preview_fps": manifest.get("sample_fps") or 12.0,
        "notes": existing_notes,
        "run_manifest": str(manifest_path.resolve()),
        "source_frame_dir": str(source_dir.resolve()),
        "full_sheet": str(full_sheet.resolve()),
        "cropped_sheet": str(cropped_sheet.resolve()),
        "contact": str(contact.resolve()),
        "gif_preview": str(preview.resolve()),
        "promoted_frames": [str(path.resolve()) for path in selected_paths],
        "cropped_cell": {"width": cropped_width, "height": cropped_height},
    }
    _write_json(curated_manifest_path, curated_manifest)

    manifest["selected_source_frames"] = selected
    manifest["cell_width"] = cell_width
    manifest["cell_height"] = cell_height
    manifest.setdefault("paths", {})
    manifest["paths"].update(
        {
            "full_sheet": str(full_sheet.resolve()),
            "cropped_sheet": str(cropped_sheet.resolve()),
            "contact": str(contact.resolve()),
            "preview": str(preview.resolve()),
            "promoted_frames": [str(path.resolve()) for path in selected_paths],
        }
    )
    _write_json(manifest_path, manifest)

    print(f"Promoted {len(selected_paths)} {action} frames to {action_dir}")
    print(f"Contact: {contact}")
    print(f"Preview: {preview}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    prepare_parser = subparsers.add_parser("prepare", help="Rip a Comfy/LTX video into a reviewable run folder")
    prepare_parser.add_argument("--character", required=True, help="Character label, e.g. A.Ben")
    prepare_parser.add_argument("--action", required=True, help=f"Action name: {', '.join(sorted(SUPPORTED_ACTIONS))}")
    prepare_parser.add_argument("--video", type=Path, required=True, help="Completed Comfy/LTX video path")
    prepare_parser.add_argument("--reference", type=Path, help="Optional reference image copied into the run")
    prepare_parser.add_argument("--prompt", help="Prompt text copied into the run manifest")
    prepare_parser.add_argument("--prompt-file", type=Path, help="Optional prompt text file")
    prepare_parser.add_argument("--workflow", type=Path, help="Optional Comfy workflow JSON copied into the run")
    prepare_parser.add_argument("--run-name", help="Output run folder name")
    prepare_parser.add_argument(
        "--runs-root",
        type=Path,
        help="Root folder for ltx_runs; defaults to game/chars/<character>/source_art/ltx_runs",
    )
    prepare_parser.add_argument("--sample-fps", type=float, help="Optional ffmpeg fps filter value")
    prepare_parser.add_argument("--force", action="store_true", help="Replace an existing run folder under --runs-root")
    prepare_parser.set_defaults(func=prepare)

    promote_parser = subparsers.add_parser("promote", help="Promote selected run frames into curated sprite assets")
    promote_parser.add_argument("--run-dir", type=Path, required=True, help="Run directory created by prepare")
    promote_parser.add_argument("--selected", help="Comma-separated zero-based source frame indices")
    promote_parser.add_argument(
        "--curated-root",
        type=Path,
        help="Curated sprite root; defaults to game/chars/<character>/source_art/curated_game_sprites",
    )
    promote_parser.add_argument("--cell-width", type=int, default=DEFAULT_CELL_WIDTH, help="Promoted cell width")
    promote_parser.add_argument("--cell-height", type=int, default=DEFAULT_CELL_HEIGHT, help="Promoted cell height")
    promote_parser.set_defaults(func=promote)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        return args.func(args)
    except (FileExistsError, FileNotFoundError, RuntimeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
