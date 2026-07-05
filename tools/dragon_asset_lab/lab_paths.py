from __future__ import annotations

import json
import os
import re
import shutil
from datetime import datetime
from pathlib import Path
from typing import Any


DEFAULT_REPO_ROOT = Path(r"C:\Users\kasom\projects\dragon-mugen-arena")
OWNED_CHARACTERS = ("A.Ben", "I.Chie")
LOCAL_TEST_CHARACTERS = ("EvilRyu", "EvilKen")
ACTIONS = ("idle", "crouch", "walk", "dash", "jump", "jump_forward", "jump_back", "punch", "kick")
MEDIA_SUFFIXES = {".png", ".gif", ".mp4", ".jpg", ".jpeg", ".webp"}
VIDEO_SUFFIXES = {".mp4", ".mov", ".mkv", ".webm"}
CONFIG_REL_PATH = Path("artifacts") / "asset_lab" / "dragon_asset_lab_config.json"


def find_repo_root() -> Path:
    local_root = Path(__file__).resolve().parents[2]
    if (local_root / "game" / "chars").exists():
        return local_root
    return DEFAULT_REPO_ROOT


def load_json(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    return data if isinstance(data, dict) else {}


def timestamp() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S_%f")


def backup_file(path: Path) -> Path | None:
    if not path.exists():
        return None
    backup = path.with_name(f"{path.name}.{timestamp()}.bak")
    shutil.copy2(path, backup)
    return backup


def write_json_atomic_with_backup(path: Path, data: dict[str, Any]) -> Path | None:
    path.parent.mkdir(parents=True, exist_ok=True)
    backup = backup_file(path)
    tmp_path = path.with_name(f"{path.name}.{os.getpid()}.tmp")
    tmp_path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    os.replace(tmp_path, path)
    return backup


def is_relative_to(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def relpath(path: Path, root: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def resolve_repo_path(raw_path: str | Path, root: Path, base_dir: Path | None = None, require_exists: bool = True) -> Path | None:
    if raw_path is None or str(raw_path).strip() == "":
        return None
    candidate = Path(str(raw_path))
    attempts: list[Path] = []
    if candidate.is_absolute():
        attempts.append(candidate)
    else:
        if base_dir is not None:
            attempts.append(base_dir / candidate)
        attempts.append(root / candidate)

    for attempt in attempts:
        resolved = attempt.resolve()
        if not is_relative_to(resolved, root):
            continue
        if not require_exists or resolved.exists():
            return resolved
    return None


def localize_manifest_path(raw_path: object, base_dir: Path, root: Path, fallback_dir: Path | None = None) -> Path | None:
    resolved = resolve_repo_path(str(raw_path), root, base_dir) if raw_path else None
    if resolved:
        return resolved
    if raw_path and fallback_dir is not None:
        fallback = (fallback_dir / Path(str(raw_path)).name).resolve()
        if fallback.exists() and is_relative_to(fallback, root):
            return fallback
    return None


def first_existing(paths: list[Path]) -> Path | None:
    for path in paths:
        if path.exists():
            return path
    return None


def character_names(root: Path) -> tuple[str, ...]:
    names = []
    chars_root = root / "game" / "chars"
    for name in (*OWNED_CHARACTERS, *LOCAL_TEST_CHARACTERS):
        if (chars_root / name).is_dir() and name not in names:
            names.append(name)
    return tuple(names) if names else OWNED_CHARACTERS


def character_kind(character: str) -> str:
    return "owned" if character in OWNED_CHARACTERS else "local test"


def character_dir(root: Path, character: str) -> Path:
    return root / "game" / "chars" / character


def source_art_root(root: Path, character: str) -> Path:
    return character_dir(root, character) / "source_art"


def curated_root(root: Path, character: str) -> Path:
    return source_art_root(root, character) / "curated_game_sprites"


def source_videos_root(root: Path, character: str) -> Path:
    return character_dir(root, character) / "source_videos"


def config_path(root: Path) -> Path:
    return root / CONFIG_REL_PATH


def media_url(path: Path | None, root: Path) -> str:
    if not path:
        return ""
    from urllib.parse import quote

    return "/asset/" + quote(relpath(path, root))


def parse_selected_frames(raw_value: str | None) -> list[int]:
    if raw_value is None or not raw_value.strip():
        return []
    frames: list[int] = []
    for piece in re.split(r"[\s,]+", raw_value.strip()):
        if not piece:
            continue
        try:
            frame = int(piece)
        except ValueError as exc:
            raise ValueError(f"Invalid frame number: {piece!r}") from exc
        if frame < 0:
            raise ValueError(f"Frame numbers must be zero or greater: {frame}")
        frames.append(frame)
    return frames


def format_selected_frames(frames: object) -> str:
    if not isinstance(frames, list):
        return ""
    return ", ".join(str(value) for value in frames if isinstance(value, int))


def safe_run_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "_", value.strip())
    return cleaned.strip("._")
