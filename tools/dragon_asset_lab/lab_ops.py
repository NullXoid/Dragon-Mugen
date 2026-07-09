from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path
from typing import Any

from lab_comfy import submit_comfy_image_to_video, submit_comfy_smoke_video, submit_comfy_workflow
from lab_config import configured_path, load_config, save_config
from lab_paths import (
    ACTIONS,
    OWNED_CHARACTERS,
    VIDEO_SUFFIXES,
    backup_file,
    curated_root,
    load_json,
    parse_selected_frames,
    safe_run_name,
    source_videos_root,
    timestamp,
    write_json_atomic_with_backup,
)
from lab_result import CommandResult, result_error, run_command, shell_text
from lab_workspace import character_summary, run_summary_for_action

PROMOTE_CELL_WIDTH = 512
PROMOTE_CELL_HEIGHT = 672


def require_owned_character(character: str) -> None:
    if character not in OWNED_CHARACTERS:
        raise ValueError(f"{character} is browse-only in Asset Lab; writes are limited to {', '.join(OWNED_CHARACTERS)}.")


def require_action(action: str) -> str:
    normalized = action.strip().lower()
    if normalized not in ACTIONS:
        raise ValueError(f"Unsupported action: {action!r}")
    return normalized


def selected_frames_from_action(root: Path, character: str, action: str) -> list[int]:
    summary = character_summary(root, character)
    actions = summary["curated_manifest"].get("actions", {}) if isinstance(summary["curated_manifest"].get("actions"), dict) else {}
    action_data = actions.get(action, {}) if isinstance(actions.get(action), dict) else {}
    selected = action_data.get("selected_source_frames", [])
    return [int(value) for value in selected] if isinstance(selected, list) else []


def validate_selected_frames(root: Path, character: str, action: str, selected: list[int]) -> dict[str, Any]:
    summary = character_summary(root, character)
    actions = summary["curated_manifest"].get("actions", {}) if isinstance(summary["curated_manifest"].get("actions"), dict) else {}
    action_data = actions.get(action)
    if not isinstance(action_data, dict):
        raise ValueError(f"No curated manifest entry exists for {character} {action}.")
    run = run_summary_for_action(root, character, action, action_data)
    source_count = run.get("source_count")
    if not isinstance(source_count, int) or source_count <= 0:
        raise ValueError(f"Cannot validate {action}: no source frames were found for its run.")
    invalid = [frame for frame in selected if frame < 0 or frame >= source_count]
    if invalid:
        raise ValueError(f"Invalid frame number(s) for {action}; expected 0..{source_count - 1}, got {invalid}.")
    return {"summary": summary, "action_data": action_data, "run": run, "source_count": source_count}


def save_selected_frames(root: Path, character: str, action: str, raw_selected: str) -> CommandResult:
    title = "Save selected_source_frames"
    try:
        require_owned_character(character)
        action = require_action(action)
        selected = parse_selected_frames(raw_selected)
        validation = validate_selected_frames(root, character, action, selected)
        summary = validation["summary"]
        manifest_path = summary["curated_manifest_path"]
        if not manifest_path:
            raise ValueError(f"Curated manifest does not exist for {character}.")
        manifest = load_json(manifest_path)
        actions = manifest.setdefault("actions", {})
        if not isinstance(actions, dict):
            raise ValueError("Curated manifest field 'actions' must be an object.")
        action_data = actions.get(action)
        if not isinstance(action_data, dict):
            raise ValueError(f"Curated manifest action '{action}' must be an object.")
        action_data["selected_source_frames"] = selected
        backup = write_json_atomic_with_backup(manifest_path, manifest)
        backup_text = f"\nBackup: {backup}" if backup else "\nBackup: none; manifest was newly created"
        return CommandResult(
            title=title,
            command=f"write {manifest_path}",
            returncode=0,
            stdout=f"Saved {len(selected)} selected frame(s) for {character} {action}.{backup_text}",
        )
    except (OSError, ValueError) as exc:
        return result_error(title, "manifest save", str(exc))


def promote_selected_frames(root: Path, character: str, action: str, raw_selected: str) -> CommandResult:
    title = "Promote selected frames"
    try:
        require_owned_character(character)
        action = require_action(action)
        selected = parse_selected_frames(raw_selected)
        if not selected:
            selected = selected_frames_from_action(root, character, action)
        if not selected:
            raise ValueError("No selected frames were provided or saved in the curated manifest.")
        validation = validate_selected_frames(root, character, action, selected)
        run = validation["run"]
        run_manifest_path = run.get("run_manifest_path")
        run_dir = run.get("run_dir")
        run_action = run.get("run_action")
        if not run_manifest_path or not run_dir:
            raise ValueError(f"No LTX run manifest found for {character} {action}.")
        if run_action and run_action != action:
            raise ValueError(
                f"Refusing promote for {action}: run manifest action is {run_action!r}. "
                "Save the selection, then promote with an action-specific run."
            )
        curated_manifest_path = validation["summary"]["curated_manifest_path"]
        backups = [path for path in (backup_file(run_manifest_path), backup_file(curated_manifest_path)) if path]
        command = [
            sys.executable,
            "engine/tools/ltx_sprite_pipeline.py",
            "promote",
            "--run-dir",
            str(run_dir),
            "--selected",
            ",".join(str(frame) for frame in selected),
            "--cell-width",
            str(PROMOTE_CELL_WIDTH),
            "--cell-height",
            str(PROMOTE_CELL_HEIGHT),
        ]
        result = run_command(root, title, command, timeout=900)
        backup_lines = "\n".join(f"Backup: {path}" for path in backups)
        if backup_lines:
            result.stdout = (backup_lines + "\n" + result.stdout).strip() + "\n"
        return result
    except (OSError, ValueError) as exc:
        return result_error(title, "ltx_sprite_pipeline.py promote", str(exc))


def build_sprites(root: Path, character: str, mode: str) -> CommandResult:
    title = "Rebuild sprites"
    try:
        require_owned_character(character)
        if character != "A.Ben":
            raise ValueError("No I.Chie SFF builder is available yet; only A.Ben has a safe build tool.")
        frames_root = curated_root(root, character) / "frames"
        if not frames_root.exists():
            raise ValueError(f"Curated frame root is missing: {frames_root}")
        command = [sys.executable, "engine/tools/build_aben_walk_sff.py"]
        if mode == "full":
            command.extend(["--action-source-root", str(frames_root)])
        elif mode == "walk":
            command.append("--skip-actions")
        else:
            raise ValueError(f"Unsupported build mode: {mode!r}")
        return run_command(root, title, command, timeout=900)
    except (OSError, ValueError) as exc:
        return result_error(title, "build_aben_walk_sff.py", str(exc))


def copy_video_to_source_folder(root: Path, character: str, video_path: str, overwrite: bool) -> tuple[Path, str]:
    source = Path(video_path).expanduser()
    if not source.is_absolute():
        source = root / source
    source = source.resolve()
    if not source.exists() or not source.is_file():
        raise FileNotFoundError(f"Source video not found: {source}")
    if source.suffix.lower() not in VIDEO_SUFFIXES:
        raise ValueError(f"Unsupported video suffix {source.suffix!r}; expected one of {sorted(VIDEO_SUFFIXES)}")
    target_dir = source_videos_root(root, character)
    target_dir.mkdir(parents=True, exist_ok=True)
    target = target_dir / source.name
    copied = "already in source_videos"
    if source.resolve() != target.resolve():
        if target.exists() and not overwrite:
            copied = "kept existing source_videos copy"
        else:
            shutil.copy2(source, target)
            copied = f"copied from {source}"
    return target, copied


def register_source_video(root: Path, character: str, action: str, video: Path, original_source: str, notes: str, run_name: str) -> Path | None:
    manifest_path = source_videos_root(root, character) / "manifest.json"
    manifest = load_json(manifest_path)
    manifest.setdefault("character", character)
    manifest.setdefault("purpose", "Source videos used by the Comfy/LTX sprite post-processing pipeline.")
    videos = manifest.setdefault("videos", {})
    if not isinstance(videos, dict):
        raise ValueError("source_videos manifest field 'videos' must be an object.")
    existing = videos.get(action, {}) if isinstance(videos.get(action), dict) else {}
    entry = dict(existing)
    entry["file"] = video.name
    entry["source"] = original_source or video.name
    if run_name:
        entry["run"] = f"../source_art/ltx_runs/{run_name}"
    entry.setdefault("selected_source_frames", [])
    if notes.strip():
        entry["notes"] = notes.strip()
    videos[action] = entry
    return write_json_atomic_with_backup(manifest_path, manifest)


def import_or_prepare_video(root: Path, character: str, action: str, form: dict[str, list[str]]) -> CommandResult:
    title = "Import/prepare source video"
    try:
        require_owned_character(character)
        action = require_action(action)
        video_path = form.get("video_path", [""])[0].strip()
        if not video_path:
            raise ValueError("Video path is required.")
        overwrite = form.get("overwrite_video", [""])[0] == "1"
        prepare = form.get("prepare_video", [""])[0] == "1"
        force = form.get("force_prepare", [""])[0] == "1"
        notes = form.get("video_notes", [""])[0]
        run_name = safe_run_name(form.get("run_name", [""])[0])
        sample_fps = form.get("sample_fps", [""])[0].strip()

        target, copy_note = copy_video_to_source_folder(root, character, video_path, overwrite)
        backup = register_source_video(root, character, action, target, video_path, notes, run_name)
        setup_lines = [
            f"Video: {target}",
            f"Import: {copy_note}",
            f"Manifest backup: {backup}" if backup else "Manifest backup: none; manifest was newly created",
        ]
        if not prepare:
            return CommandResult(title=title, command="source_videos manifest update", returncode=0, stdout="\n".join(setup_lines))

        command = [
            sys.executable,
            "engine/tools/ltx_sprite_pipeline.py",
            "prepare",
            "--character",
            character,
            "--action",
            action,
            "--video",
            str(target),
        ]
        if run_name:
            command.extend(["--run-name", run_name])
        if sample_fps:
            try:
                fps_value = float(sample_fps)
            except ValueError as exc:
                raise ValueError(f"sample_fps must be numeric: {sample_fps!r}") from exc
            if fps_value <= 0:
                raise ValueError("sample_fps must be greater than zero.")
            command.extend(["--sample-fps", f"{fps_value:g}"])
        if force:
            command.append("--force")
        workflow_path = configured_path(root, str(load_config(root).get("workflow_json", "")))
        if workflow_path and workflow_path.exists():
            command.extend(["--workflow", str(workflow_path)])
        elif workflow_path:
            setup_lines.append(f"Workflow skipped because it is missing: {workflow_path}")

        result = run_command(root, title, command, timeout=900)
        result.stdout = "\n".join(setup_lines) + "\n" + result.stdout
        return result
    except (OSError, ValueError) as exc:
        return result_error(title, "ltx_sprite_pipeline.py prepare", str(exc))


def run_proof(root: Path, proof_kind: str) -> CommandResult:
    exe = root / "build" / ("dragon_mugen.exe" if os.name == "nt" else "dragon_mugen")
    if proof_kind == "dev_check_skip_build":
        return run_command(root, "Dev check without build", [sys.executable, "engine/tools/dev_check.py", ".", "--skip-build"], timeout=900)
    if proof_kind == "launch_stub":
        return CommandResult(
            title="Manual game launch",
            command=f"{exe} game",
            returncode=0,
            stdout="Command stub only. Launch the game manually from a terminal for interactive proof.",
        )
    if not exe.exists():
        return result_error("Run proof", str(exe), f"dragon_mugen executable is missing: {exe}")
    if proof_kind == "owned_roster_screens":
        output_dir = root / "artifacts" / "asset_lab" / "owned_roster_proof" / timestamp()
        output_dir.mkdir(parents=True, exist_ok=True)
        env = {"DRAGON_ROSTER_SCREENSHOT_DIR": str(output_dir)}
        result = run_command(root, "Owned roster proof with screenshots", [str(exe), "--verify", "owned-character-readiness"], env=env, timeout=900)
        result.stdout = f"DRAGON_ROSTER_SCREENSHOT_DIR={output_dir}\n" + result.stdout
        return result
    if proof_kind == "roster_screens":
        output_dir = root / "artifacts" / "asset_lab" / "roster_proof" / timestamp()
        output_dir.mkdir(parents=True, exist_ok=True)
        env = {"DRAGON_ROSTER_SCREENSHOT_DIR": str(output_dir)}
        result = run_command(root, "Roster proof with screenshots", [str(exe), "--verify", "roster-compatibility-smoke"], env=env, timeout=900)
        result.stdout = f"DRAGON_ROSTER_SCREENSHOT_DIR={output_dir}\n" + result.stdout
        return result
    if proof_kind == "cpu_baseline":
        return run_command(root, "CPU baseline verifier", [str(exe), "--verify", "cpu-baseline"], timeout=900)
    return result_error("Run proof", proof_kind, f"Unknown proof command: {proof_kind}")


def command_for_build(root: Path, mode: str) -> str:
    frames_root = curated_root(root, "A.Ben") / "frames"
    command = [sys.executable, "engine/tools/build_aben_walk_sff.py"]
    if mode == "full":
        command.extend(["--action-source-root", str(frames_root)])
    elif mode == "walk":
        command.append("--skip-actions")
    return shell_text(command)
