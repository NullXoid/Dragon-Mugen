#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import struct
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Section:
    name: str
    line: int
    properties: dict[str, str] = field(default_factory=dict)
    body: list[str] = field(default_factory=list)


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].strip()


def parse_mugen_text(path: Path) -> list[Section]:
    sections: list[Section] = []
    current: Section | None = None
    for line_no, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        line = strip_comment(raw)
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            current = Section(line[1:-1].strip(), line_no)
            sections.append(current)
            continue
        if current is None:
            continue
        current.body.append(line)
        if "=" in line:
            key, value = line.split("=", 1)
            current.properties[key.strip().lower()] = value.strip()
    return sections


def section(sections: list[Section], name: str) -> Section | None:
    target = name.lower()
    return next((s for s in sections if s.name.lower() == target), None)


def parse_sff_v1(path: Path) -> dict[str, int | str]:
    data = path.read_bytes()
    if len(data) < 32:
        raise ValueError(f"{path} is too small to be an SFF")
    signature = data[:12]
    if signature != b"ElecbyteSpr\x00":
        raise ValueError(f"{path} has invalid SFF signature {signature!r}")
    ver3, ver2, ver1, ver0 = data[12:16]
    group_count, image_count, first_subfile_offset = struct.unpack_from("<III", data, 16)
    subheader_size = struct.unpack_from("<I", data, 28)[0]
    return {
        "version": f"{ver3}.{ver2}.{ver1}.{ver0}",
        "groups": group_count,
        "images": image_count,
        "first_subfile_offset": first_subfile_offset,
        "subheader_size": subheader_size,
        "bytes": len(data),
    }


def count_air_actions(path: Path) -> int:
    text = path.read_text(errors="replace")
    return len(re.findall(r"^\s*\[Begin Action\s+[-\d]+\]", text, flags=re.IGNORECASE | re.MULTILINE))


def count_cns_states(path: Path) -> tuple[int, int]:
    text = path.read_text(errors="replace")
    statedefs = len(re.findall(r"^\s*\[Statedef\s+[-\d]+\]", text, flags=re.IGNORECASE | re.MULTILINE))
    controllers = len(re.findall(r"^\s*\[State\s+[^]]+\]", text, flags=re.IGNORECASE | re.MULTILINE))
    return statedefs, controllers


def count_cmd_commands(path: Path) -> int:
    sections = parse_mugen_text(path)
    return sum(1 for s in sections if s.name.lower() == "command")


def first_active_select_entry(root: Path) -> tuple[str, str | None] | None:
    select_path = root / "data" / "select.def"
    if not select_path.exists():
        return None

    section_name = ""
    for raw in select_path.read_text(errors="replace").splitlines():
        line = strip_comment(raw)
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            section_name = line[1:-1].strip().lower()
            continue
        if section_name != "characters":
            continue

        parts = [part.strip() for part in line.split(",")]
        character = parts[0] if parts else ""
        if not character or character.lower() == "randomselect":
            continue
        stage = parts[1] if len(parts) > 1 and parts[1].strip().lower() != "random" else None
        return character, stage

    return None


def resolve_character_def(root: Path, entry: str) -> Path:
    normalized = entry.replace("\\", "/").strip()
    if normalized.lower().endswith(".def") or "/" in normalized:
        return root / "chars" / normalized
    return root / "chars" / normalized / f"{normalized}.def"


def resolve_relative(base: Path, value: str | None) -> Path | None:
    if not value:
        return None
    normalized = value.strip().strip('"').replace("\\", "/")
    if not normalized:
        return None
    path = Path(normalized)
    if path.is_absolute():
        return path
    return base / path


def resolve_stage_def(root: Path, stage_entry: str | None) -> Path | None:
    if not stage_entry:
        fallback = root / "stages" / "kfm.def"
        return fallback if fallback.exists() else None
    normalized = stage_entry.replace("\\", "/").strip().strip('"')
    path = Path(normalized)
    if path.is_absolute():
        return path
    return root / normalized


def character_files_from_def(char_def_path: Path, sections: list[Section]) -> dict[str, list[Path]]:
    files = section(sections, "Files")
    if not files:
        return {}

    def_dir = char_def_path.parent
    resolved: dict[str, list[Path]] = {}

    for key in ("cmd", "anim", "air", "sprite", "sound"):
        path = resolve_relative(def_dir, files.properties.get(key))
        if path:
            resolved[key] = [path]

    state_keys = [key for key in files.properties if key == "cns" or key == "st" or re.fullmatch(r"st\d+", key)]
    state_paths: list[Path] = []
    for key in sorted(state_keys):
        path = resolve_relative(def_dir, files.properties.get(key))
        if path and path not in state_paths:
            state_paths.append(path)
    if state_paths:
        resolved["states"] = state_paths

    return resolved


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", nargs="?", default="game")
    args = parser.parse_args()

    root = Path(args.root)
    select_entry = first_active_select_entry(root)
    if select_entry:
        character_entry, stage_entry = select_entry
    else:
        character_entry, stage_entry = "A.Ben", "stages/kfm.def"

    char_def_path = resolve_character_def(root, character_entry)
    stage_def_path = resolve_stage_def(root, stage_entry)

    required = [char_def_path]
    if stage_def_path:
        required.append(stage_def_path)
    missing = [p for p in required if not p.exists()]
    if missing:
        for path in missing:
            print(f"missing: {path}")
        return 1

    char_def = parse_mugen_text(char_def_path)
    info = section(char_def, "Info")
    files = section(char_def, "Files")
    resolved_files = character_files_from_def(char_def_path, char_def)

    file_requirements: list[Path] = []
    for key in ("cmd", "anim", "air", "sprite", "states"):
        file_requirements.extend(resolved_files.get(key, []))
    missing = [p for p in file_requirements if not p.exists()]
    if missing:
        for path in missing:
            print(f"missing: {path}")
        return 1

    optional_missing = [p for p in resolved_files.get("sound", []) if not p.exists()]

    stage_def = parse_mugen_text(stage_def_path) if stage_def_path else []

    print("Dragon MUGEN content inspection")
    print(f"root: {root}")
    print(f"select character: {character_entry}")
    print(f"character def: {char_def_path}")
    if info:
        print(f"character name: {info.properties.get('name', '(unknown)')}")
        print(f"character author: {info.properties.get('author', '(unknown)')}")
    if files:
        print("character files:")
        for key, value in sorted(files.properties.items()):
            print(f"  {key}: {value}")
    for path in optional_missing:
        print(f"optional missing: {path}")

    air_path = (resolved_files.get("anim") or resolved_files.get("air") or [None])[0]
    if air_path:
        print(f"air actions: {count_air_actions(air_path)}")

    statedefs = 0
    controllers = 0
    for state_path in resolved_files.get("states", []):
        state_count, controller_count = count_cns_states(state_path)
        statedefs += state_count
        controllers += controller_count
    print(f"cns statedefs: {statedefs}")
    print(f"cns controllers: {controllers}")
    cmd_path = (resolved_files.get("cmd") or [None])[0]
    if cmd_path:
        print(f"cmd commands: {count_cmd_commands(cmd_path)}")

    sprite_path = (resolved_files.get("sprite") or [None])[0]
    if sprite_path:
        print("character sff:")
        for key, value in parse_sff_v1(sprite_path).items():
            print(f"  {key}: {value}")

    print("stage:")
    stage_info = section(stage_def, "Info")
    if stage_info:
        for key in ("name", "displayname", "author"):
            if key in stage_info.properties:
                print(f"  {key}: {stage_info.properties[key]}")
    bg_count = sum(1 for s in stage_def if s.name.lower() == "bg" or s.name.lower().startswith("bg "))
    print(f"  bg elements: {bg_count}")
    bg_def = section(stage_def, "BGdef")
    stage_sff = resolve_relative(root, bg_def.properties.get("spr")) if bg_def else None
    if stage_sff and stage_sff.exists():
        print("stage sff:")
        for key, value in parse_sff_v1(stage_sff).items():
            print(f"  {key}: {value}")
    elif stage_sff:
        print(f"optional missing: {stage_sff}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
