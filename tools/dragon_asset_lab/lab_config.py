from __future__ import annotations

from pathlib import Path

from lab_paths import config_path, load_json, write_json_atomic_with_backup
from lab_result import CommandResult


def default_config() -> dict[str, object]:
    return {
        "comfy_server_url": "",
        "workflow_json": "",
        "image_to_image_workflow_json": "",
        "video_output_dir": "",
        "enable_direct_submit": False,
    }


def load_config(root: Path) -> dict[str, object]:
    config = default_config()
    saved = load_json(config_path(root))
    for key in config:
        if key in saved:
            config[key] = saved[key]
    config["enable_direct_submit"] = bool(config.get("enable_direct_submit"))
    return config


def save_config(root: Path, form: dict[str, list[str]]) -> CommandResult:
    title = "Save Comfy/LTX config"
    config = default_config()
    config["comfy_server_url"] = form.get("comfy_server_url", [""])[0].strip()
    config["workflow_json"] = form.get("workflow_json", [""])[0].strip()
    config["image_to_image_workflow_json"] = form.get("image_to_image_workflow_json", [""])[0].strip()
    config["video_output_dir"] = form.get("video_output_dir", [""])[0].strip()
    config["enable_direct_submit"] = form.get("enable_direct_submit", [""])[0] == "1"
    path = config_path(root)
    backup = write_json_atomic_with_backup(path, config)
    backup_text = f"\nBackup: {backup}" if backup else ""
    return CommandResult(title=title, command=f"write {path}", returncode=0, stdout=f"Saved config: {path}{backup_text}")


def configured_path(root: Path, raw_value: str) -> Path | None:
    if not raw_value.strip():
        return None
    path = Path(raw_value).expanduser()
    if not path.is_absolute():
        path = root / path
    return path.resolve()
