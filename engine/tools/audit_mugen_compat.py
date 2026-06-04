#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import struct
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Property:
    key: str
    value: str
    line: int


@dataclass
class Section:
    name: str
    line: int
    properties: list[Property] = field(default_factory=list)
    body: list[str] = field(default_factory=list)


@dataclass
class CharacterFiles:
    root: Path
    definition: Path
    cmd: Path | None
    states: list[Path]
    sprite: Path | None
    anim: Path | None
    sound: Path | None
    palettes: list[Path]


ENGINE_CONTROLLER_SUBSET = {
    "assertspecial",
    "afterimage",
    "afterimagetime",
    "angleadd",
    "angledraw",
    "anglemul",
    "angleset",
    "attackdist",
    "attackmulset",
    "bindtoroot",
    "changestate",
    "changeanim",
    "changeanim2",
    "playsnd",
    "ctrlset",
    "explod",
    "envshake",
    "envcolor",
    "fallenvshake",
    "hitfalldamage",
    "hitfallset",
    "hitfallvel",
    "modifyexplod",
    "removeexplod",
    "remappal",
    "hitvelset",
    "posadd",
    "posfreeze",
    "posset",
    "playerpush",
    "screenbound",
    "selfstate",
    "sprpriority",
    "statetypeset",
    "stopsnd",
    "varadd",
    "varrandom",
    "varrangeset",
    "varset",
    "veladd",
    "velmul",
    "velset",
    "width",
    "victoryquote",
    "hitdef",
    "hitby",
    "hitoverride",
    "hitadd",
    "helper",
    "lifeadd",
    "makedust",
    "null",
    "nothitby",
    "offset",
    "parentvaradd",
    "pause",
    "palfx",
    "poweradd",
    "projectile",
    "bgpalfx",
    "targetbind",
    "targetdrop",
    "targetfacing",
    "targetlifeadd",
    "targetpoweradd",
    "targetstate",
    "targetveladd",
    "targetvelset",
    "turn",
    "superpause",
    "trans",
    "destroyself",
    "defencemulset",
    "displaytoclipboard",
    "appendtoclipboard",
    "forcefeedback",
    "gamemakeanim",
    "bindtoparent",
}

IGNORED_TRIGGER_WORDS = {
    "and",
    "or",
    "not",
    "ifelse",
    "abs",
    "ceil",
    "floor",
    "exp",
    "ln",
    "log",
    "sin",
    "cos",
    "tan",
    "asin",
    "acos",
    "atan",
    "name",
}

INVALID_CONTROLLER_METADATA = {"a", "c", "s", "l", "i", "h", "n"}


def strip_comment(line: str) -> str:
    in_quote = False
    escaped = False
    out: list[str] = []
    for char in line:
        if char == '"' and not escaped:
            in_quote = not in_quote
        if char == ";" and not in_quote:
            break
        out.append(char)
        escaped = char == "\\" and not escaped
        if char != "\\":
            escaped = False
    return "".join(out).strip()


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
            current.properties.append(Property(key.strip(), value.strip(), line_no))
    return sections


def section(sections: list[Section], name: str) -> Section | None:
    target = name.lower()
    return next((s for s in sections if s.name.lower() == target), None)


def properties(section_: Section | None, key: str) -> list[Property]:
    if section_ is None:
        return []
    target = key.lower()
    return [prop for prop in section_.properties if prop.key.lower() == target]


def property_value(section_: Section | None, key: str, default: str = "") -> str:
    values = properties(section_, key)
    return values[-1].value if values else default


def unquote(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        return value[1:-1]
    return value


def split_csv(line: str) -> list[str]:
    parts: list[str] = []
    current: list[str] = []
    depth = 0
    in_quote = False
    for char in line:
        if char == '"':
            in_quote = not in_quote
        elif not in_quote and char == "(":
            depth += 1
        elif not in_quote and char == ")" and depth > 0:
            depth -= 1
        if char == "," and not in_quote and depth == 0:
            parts.append("".join(current).strip())
            current.clear()
        else:
            current.append(char)
    parts.append("".join(current).strip())
    return parts


def parse_int(value: str, default: int | None = None) -> int | None:
    try:
        return int(float(value.strip()))
    except ValueError:
        return default


def resolve_content_path(base: Path, value: str) -> Path | None:
    value = unquote(value)
    if not value:
        return None
    path = Path(value)
    if path.is_absolute():
        return path
    return base / path


def resolve_character_def(root: Path, value: str) -> Path | None:
    value = unquote(value)
    if not value or value.lower() == "randomselect":
        return None
    path = Path(value)
    if path.is_absolute():
        return path
    generic = path.as_posix()
    if generic.lower().startswith("chars/"):
        return root / path
    if path.suffix.lower() == ".def" or "/" in generic or "\\" in value:
        return root / "chars" / path
    return root / "chars" / path / f"{path.name}.def"


def resolve_stage_def(root: Path, value: str) -> Path | None:
    value = unquote(value)
    if not value or value.lower() == "random":
        return None
    path = Path(value)
    if path.is_absolute():
        return path
    if path.as_posix().lower().startswith("stages/"):
        return root / path
    root_relative = root / path
    if root_relative.exists():
        return root_relative
    return root / "stages" / path


def unique_paths(paths: list[Path | None]) -> list[Path]:
    out: list[Path] = []
    seen: set[str] = set()
    for path in paths:
        if path is None:
            continue
        key = str(path)
        if key not in seen:
            out.append(path)
            seen.add(key)
    return out


def resolve_character_files(root: Path, char_def: Path) -> tuple[list[Section], CharacterFiles]:
    doc = parse_mugen_text(char_def)
    files = section(doc, "Files")
    char_root = char_def.parent
    char_id = char_root.name

    def file_prop(key: str, fallback: str = "") -> Path | None:
        value = property_value(files, key, fallback)
        return resolve_content_path(char_root, value)

    state_paths: list[Path | None] = [file_prop("cns", f"{char_id}.cns")]
    if files:
        for prop in files.properties:
            key = prop.key.lower()
            if key.startswith("st") and key != "stcommon":
                state_paths.append(resolve_content_path(char_root, prop.value))
        common = property_value(files, "stcommon")
        if common:
            state_paths.append(resolve_content_path(root / "data", common))

    palettes: list[Path | None] = []
    if files:
        for prop in files.properties:
            if prop.key.lower().startswith("pal"):
                palettes.append(resolve_content_path(char_root, prop.value))

    return doc, CharacterFiles(
        root=char_root,
        definition=char_def,
        cmd=file_prop("cmd", f"{char_id}.cmd"),
        states=unique_paths(state_paths),
        sprite=file_prop("sprite", f"{char_id}.sff"),
        anim=file_prop("anim", f"{char_id}.air"),
        sound=file_prop("sound", f"{char_id}.snd"),
        palettes=unique_paths(palettes),
    )


def load_select_characters(root: Path) -> list[tuple[str, Path, Path | None]]:
    select_path = root / "data" / "select.def"
    doc = parse_mugen_text(select_path)
    chars = section(doc, "Characters")
    if chars is None:
        return []
    out: list[tuple[str, Path, Path | None]] = []
    for line in chars.body:
        parts = split_csv(line)
        if not parts or parts[0].lower() == "randomselect":
            continue
        char_def = resolve_character_def(root, parts[0])
        if char_def is None:
            continue
        stage_def = resolve_stage_def(root, parts[1]) if len(parts) > 1 else None
        out.append((parts[0], char_def, stage_def))
    return out


def parse_sff_v1(path: Path | None) -> tuple[dict[str, int | str], set[tuple[int, int]]]:
    if path is None or not path.exists():
        return {}, set()
    data = path.read_bytes()
    if len(data) < 32 or data[:12] != b"ElecbyteSpr\x00":
        raise ValueError(f"{path} is not an SFF v1 archive")
    ver3, ver2, ver1, ver0 = data[12:16]
    groups, images, first_subfile_offset = struct.unpack_from("<III", data, 16)
    subheader_size = struct.unpack_from("<I", data, 28)[0]
    pairs: set[tuple[int, int]] = set()
    offset = first_subfile_offset
    for _ in range(images):
        if offset == 0 or offset + 32 > len(data):
            break
        group = struct.unpack_from("<h", data, offset + 12)[0]
        image = struct.unpack_from("<h", data, offset + 14)[0]
        pairs.add((group, image))
        offset = struct.unpack_from("<I", data, offset)[0]
    return {
        "version": f"{ver3}.{ver2}.{ver1}.{ver0}",
        "groups": groups,
        "images": images,
        "subheader_size": subheader_size,
        "bytes": len(data),
    }, pairs


def parse_snd(path: Path | None) -> set[tuple[int, int]]:
    if path is None or not path.exists():
        return set()
    data = path.read_bytes()
    if len(data) < 512 or data[:12] != b"ElecbyteSnd\x00":
        raise ValueError(f"{path} is not an SND archive")
    samples: set[tuple[int, int]] = set()
    sample_count = struct.unpack_from("<I", data, 16)[0]
    offset = struct.unpack_from("<I", data, 20)[0]
    for _ in range(sample_count):
        if offset == 0 or offset + 16 > len(data):
            break
        length = struct.unpack_from("<I", data, offset + 4)[0]
        group = struct.unpack_from("<I", data, offset + 8)[0]
        index = struct.unpack_from("<I", data, offset + 12)[0]
        if offset + 16 + length > len(data):
            break
        samples.add((group, index))
        offset = struct.unpack_from("<I", data, offset)[0]
    return samples


def air_sprite_refs(path: Path | None) -> tuple[int, set[tuple[int, int]]]:
    if path is None or not path.exists():
        return 0, set()
    actions = 0
    refs: set[tuple[int, int]] = set()
    for section_ in parse_mugen_text(path):
        if not section_.name.lower().startswith("begin action "):
            continue
        actions += 1
        for line in section_.body:
            lower = line.lower()
            if lower.startswith("clsn") or lower.startswith("loopstart") or "=" in line:
                continue
            parts = split_csv(line)
            if len(parts) < 2:
                continue
            group = parse_int(parts[0])
            image = parse_int(parts[1])
            if group is not None and image is not None and group >= 0 and image >= 0:
                refs.add((group, image))
    return actions, refs


def controller_type(section_: Section) -> str | None:
    value = property_value(section_, "type")
    if not value:
        return None
    match = re.match(r"[A-Za-z][A-Za-z0-9_]*", value.strip())
    return match.group(0).lower() if match else value.strip().lower()


def remove_quoted_strings(expr: str) -> str:
    out: list[str] = []
    in_quote = False
    escaped = False
    for char in expr:
        if char == '"' and not escaped:
            in_quote = not in_quote
            out.append(" ")
        elif in_quote:
            out.append(" ")
        else:
            out.append(char)
        escaped = char == "\\" and not escaped
        if char != "\\":
            escaped = False
    return "".join(out)


def trigger_identifiers(expr: str) -> list[str]:
    names = re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", remove_quoted_strings(expr))
    out: list[str] = []
    for name in names:
        lowered = name.lower()
        if lowered in IGNORED_TRIGGER_WORDS:
            continue
        out.append(lowered)
    return out


def parse_sound_ref(value: str) -> tuple[str, int, int] | None:
    match = re.search(r"\b([sSfF])?\s*(-?\d+)\s*,\s*(-?\d+)", value)
    if not match:
        return None
    prefix = match.group(1).upper() if match.group(1) else ""
    return prefix, int(match.group(2)), int(match.group(3))


def analyze_states(paths: list[Path]) -> dict[str, Counter | int | set[tuple[str, int, int]]]:
    controllers: Counter[str] = Counter()
    triggers: Counter[str] = Counter()
    sound_refs: set[tuple[str, int, int]] = set()
    statedefs = 0
    state_controllers = 0
    hitdefs = 0
    for path in paths:
        if not path.exists():
            continue
        for section_ in parse_mugen_text(path):
            name = section_.name.lower()
            if name.startswith("statedef "):
                statedefs += 1
                continue
            if not name.startswith("state "):
                continue
            kind = controller_type(section_)
            if kind is None:
                continue
            if kind in INVALID_CONTROLLER_METADATA:
                continue
            controllers[kind] += 1
            state_controllers += 1
            if kind == "hitdef":
                hitdefs += 1
                for key in ("hitsound", "guardsound"):
                    ref = parse_sound_ref(property_value(section_, key))
                    if ref:
                        sound_refs.add(ref)
            if kind == "playsnd":
                ref = parse_sound_ref(property_value(section_, "value"))
                if ref:
                    sound_refs.add(ref)
            for prop in section_.properties:
                if prop.key.lower().startswith("trigger"):
                    triggers.update(trigger_identifiers(prop.value))
    return {
        "statedefs": statedefs,
        "controllers_total": state_controllers,
        "controllers": controllers,
        "triggers": triggers,
        "hitdefs": hitdefs,
        "sound_refs": sound_refs,
    }


def analyze_cmd(path: Path | None) -> dict[str, Counter | int]:
    if path is None or not path.exists():
        return {
            "commands": 0,
            "state_minus_one_changes": 0,
            "engine_candidates": 0,
            "extra_triggered_candidates": 0,
            "triggers": Counter(),
        }
    sections = parse_mugen_text(path)
    command_count = sum(1 for item in sections if item.name.lower() == "command")
    in_state_minus_one = False
    changes = 0
    engine_candidates = 0
    extra_triggered_candidates = 0
    triggers: Counter[str] = Counter()
    for section_ in sections:
        lower = section_.name.lower()
        if lower.startswith("statedef "):
            in_state_minus_one = lower == "statedef -1"
            continue
        if not in_state_minus_one or not lower.startswith("state "):
            continue
        if controller_type(section_) != "changestate":
            continue
        changes += 1
        has_command_trigger = False
        extra_identifiers: set[str] = set()
        for prop in section_.properties:
            if not prop.key.lower().startswith("trigger"):
                continue
            ids = trigger_identifiers(prop.value)
            triggers.update(ids)
            if "command" in ids and prop.key.lower() in {"triggerall", "trigger1"}:
                has_command_trigger = True
            extra_identifiers.update(
                i
                for i in ids
                if i not in {"command", "ctrl", "statetype", "time", "stateno", "power", "roundstate", "ailevel", "movecontact"}
            )
        if has_command_trigger and property_value(section_, "value").strip():
            engine_candidates += 1
            if extra_identifiers:
                extra_triggered_candidates += 1
    return {
        "commands": command_count,
        "state_minus_one_changes": changes,
        "engine_candidates": engine_candidates,
        "extra_triggered_candidates": extra_triggered_candidates,
        "triggers": triggers,
    }


def missing_sound_refs(
    refs: set[tuple[str, int, int]],
    char_samples: set[tuple[int, int]],
    common_samples: set[tuple[int, int]],
    fight_samples: set[tuple[int, int]],
) -> list[tuple[str, int, int]]:
    missing: list[tuple[str, int, int]] = []
    shared = common_samples | fight_samples
    all_samples = char_samples | shared
    for prefix, group, index in refs:
        pair = (group, index)
        if prefix == "S":
            exists = pair in char_samples
        elif prefix == "F":
            exists = pair in shared
        else:
            exists = pair in all_samples
        if not exists:
            missing.append((prefix, group, index))
    return sorted(missing)


def top(counter: Counter[str], count: int = 12) -> str:
    return ", ".join(f"{name}:{value}" for name, value in counter.most_common(count)) or "none"


def audit_character(root: Path, entry: str, char_def: Path, stage_def: Path | None) -> int:
    print(f"\n== {entry} ==")
    print(f"def: {char_def}")
    if stage_def:
        print(f"stage: {stage_def}")
    if not char_def.exists():
        print("missing character DEF")
        return 1

    char_doc, files = resolve_character_files(root, char_def)
    info = section(char_doc, "Info")
    print(f"name: {unquote(property_value(info, 'displayname', property_value(info, 'name', entry)))}")
    print(f"mugenversion: {property_value(info, 'mugenversion', '(missing)')}")
    print(f"localcoord: {property_value(info, 'localcoord', '(missing)')}")

    required_paths = [
        ("cmd", files.cmd),
        ("anim", files.anim),
        ("sprite", files.sprite),
        ("sound", files.sound),
    ]
    missing = [label for label, path in required_paths if path is None or not path.exists()]
    missing.extend(f"state:{path.name}" for path in files.states if not path.exists())
    if missing:
        print(f"missing files: {', '.join(missing)}")

    actions, air_refs = air_sprite_refs(files.anim)
    sff_info, sff_pairs = parse_sff_v1(files.sprite)
    missing_air_refs = sorted(air_refs - sff_pairs)
    print(f"air actions: {actions}; air sprite refs: {len(air_refs)}; missing refs in SFF: {len(missing_air_refs)}")
    if missing_air_refs:
        preview = ", ".join(f"{group},{image}" for group, image in missing_air_refs[:10])
        print(f"missing AIR sprite preview: {preview}")
    if sff_info:
        print(
            "sff: "
            f"v{sff_info['version']} images={sff_info['images']} "
            f"subheader={sff_info['subheader_size']} bytes={sff_info['bytes']}"
        )
    print(f"palettes declared: {len(files.palettes)}; existing: {sum(1 for path in files.palettes if path.exists())}")

    state = analyze_states(files.states)
    controllers: Counter[str] = state["controllers"]  # type: ignore[assignment]
    unsupported = Counter({
        name: count for name, count in controllers.items() if name not in ENGINE_CONTROLLER_SUBSET
    })
    print(
        "cns: "
        f"statedefs={state['statedefs']} controllers={state['controllers_total']} "
        f"hitdefs={state['hitdefs']}"
    )
    print(f"engine controller subset present: {top(Counter({k: controllers[k] for k in ENGINE_CONTROLLER_SUBSET if controllers[k]}), 8)}")
    print(f"top unsupported controllers: {top(unsupported)}")
    print(f"top state triggers: {top(state['triggers'])}")  # type: ignore[arg-type]

    cmd = analyze_cmd(files.cmd)
    print(
        "cmd: "
        f"commands={cmd['commands']} state-1-changestates={cmd['state_minus_one_changes']} "
        f"engine-candidates={cmd['engine_candidates']} "
        f"candidates-with-extra-triggers={cmd['extra_triggered_candidates']}"
    )
    print(f"top cmd triggers: {top(cmd['triggers'])}")  # type: ignore[arg-type]

    char_samples = parse_snd(files.sound)
    common_samples = parse_snd(root / "data" / "common.snd")
    fight_samples = parse_snd(root / "data" / "fight.snd")
    refs = state["sound_refs"]  # type: ignore[assignment]
    missing_refs = missing_sound_refs(refs, char_samples, common_samples, fight_samples)
    print(
        "snd: "
        f"character-samples={len(char_samples)} refs={len(refs)} missing-resolved-refs={len(missing_refs)}"
    )
    if missing_refs:
        preview = ", ".join(f"{prefix or '-'}{group},{index}" for prefix, group, index in missing_refs[:10])
        print(f"missing sound preview: {preview}")

    failure_count = 0
    if missing:
        failure_count += 1
    return failure_count


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit selected M.U.G.E.N character compatibility against the current Dragon runtime subset.")
    parser.add_argument("root", nargs="?", default="game", help="Runtime root containing data/select.def")
    parser.add_argument("--character", help="Only audit roster entries whose select.def id/display path contains this text")
    args = parser.parse_args()

    root = Path(args.root)
    failures = 0
    entries = load_select_characters(root)
    if args.character:
        needle = args.character.lower()
        entries = [entry for entry in entries if needle in entry[0].lower() or needle in str(entry[1]).lower()]
    if not entries:
        print("No matching select.def character entries found.")
        return 1

    print("Dragon MUGEN compatibility audit")
    print(f"root: {root}")
    print(
        "Current engine subset: DEF/[Files], SFF v1 PCX, ACT, AIR frames/Clsn, SND WAV, basic Statedef, "
        "ChangeState/SelfState, ChangeAnim/ChangeAnim2, Helper/DestroySelf/BindToParent/BindToRoot, Trans, AfterImage/AfterImageTime, PlaySnd, StopSnd, CtrlSet, PosAdd, PosSet, PosFreeze, ScreenBound, "
        "StateTypeSet, Width, PlayerPush, SprPriority, AngleSet/AngleAdd/AngleMul/AngleDraw, Offset, AttackDist, AttackMulSet, DefenceMulSet, Explod/ModifyExplod/RemoveExplod, MakeDust, EnvShake/FallEnvShake, PalFX/BGPalFX/EnvColor, AssertSpecial, Pause/SuperPause, VarSet, VarAdd, VarRandom, VarRangeSet, "
        "HitVelSet, HitFallDamage/HitFallVel/HitFallSet, HitBy/NotHitBy, HitOverride, HitAdd, LifeAdd, PowerAdd, ForceFeedback, GameMakeAnim, DisplayToClipboard/AppendToClipboard, VictoryQuote, RemapPal state tracking, Projectile lifecycle/priority/bounds/movement/scale/shadow basics, ParentVarAdd, TargetBind/TargetDrop/TargetFacing/TargetLifeAdd/TargetPowerAdd/TargetState/TargetVelAdd/TargetVelSet, "
        "Turn, Null, VelSet, VelAdd, VelMul, HitDef with expression-backed damage/pause/spark/sound/envshake/palfx/velocity/fall runtime resolution."
    )
    print(
        "Current command subset: buffered [Command] names for buttons, directions, simple motions, plus hold and broad direction tokens; "
        "State -1 ChangeState triggerall/trigger1 command =/!=, command OR groups, statetype =/!=, ctrl, movecontact, "
        "movehit, moveguarded, literal and inclusive range stateno/time/power/roundstate/AILevel gates, "
        "p2dist/p2bodydist/edge distance expressions, powermax/hitcount/numtarget/numhelper/numproj gates, "
        "GetHitVar(...) basics, target/enemynear/helper redirection basics, and the current variable/random expression subset."
    )
    for entry, char_def, stage_def in entries:
        failures += audit_character(root, entry, char_def, stage_def)

    print(f"\nAudit status: {'PASS for load-level checks' if failures == 0 else 'CHECK FAILURES'}")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
