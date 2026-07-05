from __future__ import annotations

import os
import re
from pathlib import Path
from typing import Any

from lab_paths import (
    ACTIONS,
    character_dir,
    character_kind,
    curated_root,
    first_existing,
    is_relative_to,
    load_json,
    localize_manifest_path,
    relpath,
    source_art_root,
    source_videos_root,
)


MUGEN_FILE_PRIORITY = (
    "cmd",
    "cns",
    "st",
    "stcommon",
    "sprite",
    "anim",
    "sound",
    "movelist",
    "intro.storyboard",
    "ending.storyboard",
)
AIR_ACTION_RE = re.compile(r"^\s*\[\s*Begin\s+Action\s+(-?\d+)\s*\]", re.IGNORECASE)


def strip_mugen_comment(line: str) -> str:
    return line.split(";", 1)[0].strip()


def clean_mugen_value(value: str) -> str:
    return strip_mugen_comment(value).strip().strip('"')


def find_character_def(char_dir: Path, character: str) -> Path | None:
    candidates = [
        char_dir / f"{character}.def",
        char_dir / f"{character.lower()}.def",
        char_dir / f"{character.replace('.', '')}.def",
        char_dir / f"{character.replace('.', '').lower()}.def",
    ]
    candidates.extend(sorted(char_dir.glob("*.def")))
    seen: set[Path] = set()
    for candidate in candidates:
        normalized = candidate.resolve()
        if normalized in seen:
            continue
        seen.add(normalized)
        if candidate.exists():
            return candidate
    return None


def parse_mugen_def(def_path: Path | None) -> dict[str, dict[str, str]]:
    if not def_path or not def_path.exists():
        return {"info": {}, "files": {}}
    section = ""
    info: dict[str, str] = {}
    files: dict[str, str] = {}
    try:
        lines = def_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except OSError:
        return {"info": {}, "files": {}}
    for raw_line in lines:
        line = strip_mugen_comment(raw_line)
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line.strip("[]").strip().lower()
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip().lower()
        value = clean_mugen_value(value)
        if section == "info":
            info[key] = value
        elif section == "files":
            files[key] = value
    return {"info": info, "files": files}


def resolve_mugen_file(root: Path, char_dir: Path, raw_path: str) -> Path | None:
    value = clean_mugen_value(raw_path)
    if not value:
        return None
    normalized = value.replace("/", os.sep).replace("\\", os.sep)
    char_candidate = (char_dir / normalized).resolve()
    if char_candidate.exists():
        return char_candidate
    data_candidate = (root / "game" / "data" / normalized).resolve()
    if data_candidate.exists():
        return data_candidate
    return char_candidate


def count_air_actions(path: Path | None) -> int | None:
    if not path or not path.exists():
        return None
    try:
        return sum(1 for line in path.read_text(encoding="utf-8", errors="ignore").splitlines() if AIR_ACTION_RE.match(line))
    except OSError:
        return None


def format_file_size(path: Path | None) -> str:
    if not path or not path.exists() or not path.is_file():
        return ""
    size = path.stat().st_size
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.1f} MB"
    if size >= 1024:
        return f"{size / 1024:.1f} KB"
    return f"{size} B"


def mugen_file_summary(root: Path, char_dir: Path, files: dict[str, str]) -> list[dict[str, Any]]:
    ordered_keys = [key for key in MUGEN_FILE_PRIORITY if key in files]
    ordered_keys.extend(sorted(key for key in files if key not in ordered_keys and (key.startswith("pal") or key.endswith("storyboard"))))

    rows = []
    for key in ordered_keys:
        raw_value = files.get(key, "")
        resolved = resolve_mugen_file(root, char_dir, raw_value)
        present = bool(resolved and resolved.exists())
        detail = format_file_size(resolved)
        if key == "anim":
            action_count = count_air_actions(resolved)
            if action_count is not None:
                detail = f"{action_count} AIR actions"
        rows.append(
            {
                "key": key,
                "value": raw_value,
                "path": resolved,
                "present": present,
                "detail": detail,
            }
        )
    return rows


def frame_paths(folder: Path | None) -> list[Path]:
    if not folder or not folder.exists():
        return []
    return sorted(folder.glob("*.png"))


def run_summary_for_action(root: Path, character: str, action: str, manifest_action: dict[str, Any]) -> dict[str, Any]:
    source_art = source_art_root(root, character)
    default_manifest_path = source_art / "ltx_runs" / action / "manifest.json"
    run_manifest_path = localize_manifest_path(manifest_action.get("run_manifest"), source_art, root) or (
        default_manifest_path if default_manifest_path.exists() else None
    )
    run_manifest = load_json(run_manifest_path) if run_manifest_path else {}
    run_dir = run_manifest_path.parent if run_manifest_path else None

    source_dir = localize_manifest_path(manifest_action.get("source_frame_dir"), source_art, root)
    paths = run_manifest.get("paths", {}) if isinstance(run_manifest.get("paths"), dict) else {}
    if not source_dir and run_dir:
        source_dir = localize_manifest_path(paths.get("frames_clean"), run_dir, root) or localize_manifest_path(paths.get("frames_raw"), run_dir, root)

    source_frames = frame_paths(source_dir)
    declared_frame_count = run_manifest.get("frame_count")
    source_count = len(source_frames) if source_frames else (int(declared_frame_count) if isinstance(declared_frame_count, int) else None)

    source_contact = localize_manifest_path(paths.get("contact"), run_dir or source_art, root)
    if not source_contact and run_dir:
        source_contact = first_existing([run_dir / "sheets" / "contact_all.png"])
    source_preview = localize_manifest_path(paths.get("preview"), run_dir or source_art, root)
    if not source_preview and run_dir:
        source_preview = first_existing([run_dir / "previews" / "preview.gif"])

    return {
        "run_manifest_path": run_manifest_path,
        "run_manifest": run_manifest,
        "run_dir": run_dir,
        "run_action": str(run_manifest.get("action", "")).lower() if run_manifest else "",
        "source_dir": source_dir,
        "source_count": source_count,
        "source_contact": source_contact,
        "source_preview": source_preview,
    }


def source_video_for_action(root: Path, character: str, action: str, video_actions: dict[str, Any]) -> tuple[Path | None, dict[str, Any]]:
    source_videos = source_videos_root(root, character)
    video_info = video_actions.get(action, {}) if isinstance(video_actions.get(action), dict) else {}
    video_file = source_videos / str(video_info.get("file", ""))
    source_video = video_file if video_file.exists() else first_existing(sorted(source_videos.glob(f"{action}*.mp4")))
    return source_video, video_info


def character_summary(root: Path, character: str) -> dict[str, Any]:
    char_dir = character_dir(root, character)
    source_art = source_art_root(root, character)
    source_videos = source_videos_root(root, character)
    curated = curated_root(root, character)
    contacts = curated / "contacts"
    previews = curated / "previews"
    frames = curated / "frames"
    shop = char_dir / "shop"

    curated_manifest_path = curated / "manifest.json"
    video_manifest_path = source_videos / "manifest.json"
    curated_manifest = load_json(curated_manifest_path)
    video_manifest = load_json(video_manifest_path)
    curated_actions = curated_manifest.get("actions", {}) if isinstance(curated_manifest.get("actions"), dict) else {}
    video_actions = video_manifest.get("videos", {}) if isinstance(video_manifest.get("videos"), dict) else {}
    def_path = find_character_def(char_dir, character)
    def_data = parse_mugen_def(def_path)
    mugen_files = mugen_file_summary(root, char_dir, def_data["files"])

    action_rows = []
    for action in ACTIONS:
        manifest_action = curated_actions.get(action, {}) if isinstance(curated_actions.get(action), dict) else {}
        frame_dir = frames / action
        frame_files = frame_paths(frame_dir)
        selected = manifest_action.get("selected_source_frames")
        promoted = manifest_action.get("promoted_frames")
        frame_count = len(frame_files)
        if frame_count == 0 and isinstance(promoted, list):
            frame_count = len(promoted)
        if frame_count == 0 and isinstance(selected, list):
            frame_count = len(selected)

        contact = localize_manifest_path(manifest_action.get("contact"), source_art, root, contacts)
        if not contact:
            contact = first_existing([contacts / f"{action}_selected_contact.png"])
        preview = localize_manifest_path(manifest_action.get("gif_preview") or manifest_action.get("preview"), source_art, root, previews)
        if not preview:
            preview = first_existing([previews / f"{action}_selected_preview.gif"])

        source_video, video_info = source_video_for_action(root, character, action, video_actions)
        run = run_summary_for_action(root, character, action, manifest_action)

        action_rows.append(
            {
                "name": action,
                "frame_count": frame_count,
                "contact": contact,
                "preview": preview,
                "source_video": source_video,
                "video_info": video_info,
                "manifest": manifest_action,
                "run": run,
            }
        )

    workspace_dirs = [
        ("source_art", source_art),
        ("source_videos", source_videos),
        ("shop", shop),
        ("curated_game_sprites", curated),
    ]
    return {
        "character": character,
        "kind": character_kind(character),
        "char_dir": char_dir,
        "exists": char_dir.exists(),
        "def_path": def_path,
        "def_info": def_data["info"],
        "def_files": def_data["files"],
        "mugen_files": mugen_files,
        "workspace_dirs": workspace_dirs,
        "curated_manifest_path": curated_manifest_path if curated_manifest_path.exists() else None,
        "video_manifest_path": video_manifest_path if video_manifest_path.exists() else None,
        "curated_manifest": curated_manifest,
        "video_manifest": video_manifest,
        "action_rows": action_rows,
    }


def display_path(path: Path | None, root: Path, missing: str = "not found") -> str:
    if not path:
        return missing
    return relpath(path, root) if is_relative_to(path, root) else str(path)


def configured_build_modes(root: Path, character: str) -> list[dict[str, str]]:
    if character != "A.Ben":
        return []
    frames_root = curated_root(root, character) / "frames"
    if not frames_root.exists():
        return []
    return [
        {
            "mode": "full",
            "label": "Rebuild A.Ben SFF with curated actions",
            "description": "Uses build_aben_walk_sff.py with --action-source-root for idle, crouch, dash, jump, punch, kick, and walk.",
        },
        {
            "mode": "walk",
            "label": "Rebuild A.Ben walk only",
            "description": "Uses build_aben_walk_sff.py --skip-actions for the walk cycle and shop walk PNGs.",
        },
    ]


def proof_command_catalog(root: Path) -> list[dict[str, str]]:
    exe = root / "build" / ("dragon_mugen.exe" if os.name == "nt" else "dragon_mugen")
    return [
        {
            "kind": "owned_roster_screens",
            "label": "Owned roster proof with screenshots",
            "command": f"{exe} --verify owned-character-readiness",
            "description": "Checks only A.Ben and I.Chie readiness and writes proof screenshots under artifacts/asset_lab.",
        },
        {
            "kind": "roster_screens",
            "label": "Roster proof with screenshots",
            "command": f"{exe} --verify roster-compatibility-smoke",
            "description": "Sets DRAGON_ROSTER_SCREENSHOT_DIR under artifacts/asset_lab before running the verifier.",
        },
        {
            "kind": "cpu_baseline",
            "label": "CPU baseline verifier",
            "command": f"{exe} --verify cpu-baseline",
            "description": "Runs the existing scripted gameplay baseline.",
        },
        {
            "kind": "dev_check_skip_build",
            "label": "Dev check without build",
            "command": "python engine/tools/dev_check.py . --skip-build",
            "description": "Runs architecture, feature spec, content, and compatibility checks without CMake.",
        },
        {
            "kind": "launch_stub",
            "label": "Manual game launch",
            "command": f"{exe} game",
            "description": "Displayed as a command stub; browser automation is intentionally not used for live play proof.",
        },
    ]


def path_status(path_value: str, root: Path) -> str:
    if not path_value:
        return "not configured"
    path = Path(path_value).expanduser()
    if not path.is_absolute():
        path = root / path
    return "present" if path.exists() else "missing"
