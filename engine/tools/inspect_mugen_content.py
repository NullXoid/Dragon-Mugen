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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", nargs="?", default="content/reference_mugen_2001")
    args = parser.parse_args()

    root = Path(args.root)
    kfm = root / "chars" / "kfm"
    stages = root / "stages"

    required = [
        kfm / "kfm.def",
        kfm / "kfm.air",
        kfm / "kfm.cmd",
        kfm / "kfm.cns",
        kfm / "kfm.sff",
        kfm / "kfm.snd",
        stages / "kfm.def",
        stages / "kfm.sff",
    ]
    missing = [p for p in required if not p.exists()]
    if missing:
        for path in missing:
            print(f"missing: {path}")
        return 1

    char_def = parse_mugen_text(kfm / "kfm.def")
    info = section(char_def, "Info")
    files = section(char_def, "Files")
    stage_def = parse_mugen_text(stages / "kfm.def")

    print("Dragon MUGEN content inspection")
    print(f"root: {root}")
    if info:
        print(f"character name: {info.properties.get('name', '(unknown)')}")
        print(f"character author: {info.properties.get('author', '(unknown)')}")
    if files:
        print("character files:")
        for key, value in sorted(files.properties.items()):
            print(f"  {key}: {value}")

    print(f"air actions: {count_air_actions(kfm / 'kfm.air')}")
    statedefs, controllers = count_cns_states(kfm / "kfm.cns")
    print(f"cns statedefs: {statedefs}")
    print(f"cns controllers: {controllers}")
    print(f"cmd commands: {count_cmd_commands(kfm / 'kfm.cmd')}")

    print("character sff:")
    for key, value in parse_sff_v1(kfm / "kfm.sff").items():
        print(f"  {key}: {value}")

    print("stage:")
    stage_info = section(stage_def, "Info")
    if stage_info:
        for key in ("name", "displayname", "author"):
            if key in stage_info.properties:
                print(f"  {key}: {stage_info.properties[key]}")
    bg_count = sum(1 for s in stage_def if s.name.lower() == "bg" or s.name.lower().startswith("bg "))
    print(f"  bg elements: {bg_count}")
    print("stage sff:")
    for key, value in parse_sff_v1(stages / "kfm.sff").items():
        print(f"  {key}: {value}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
