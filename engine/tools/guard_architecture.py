#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


APP_CPP_LINE_BUDGET = 17000

APP_LAYER_FILES = [
    Path("engine/src/App.cpp"),
    Path("engine/src/main.cpp"),
]

REQUIRED_GAME_DIRS = [
    Path("game/chars"),
    Path("game/data"),
    Path("game/font"),
    Path("game/plugins"),
    Path("game/save"),
    Path("game/sound"),
    Path("game/stages"),
]

REQUIRED_GAME_FILES = [
    Path("game/data/select.def"),
    Path("game/data/fight.def"),
]

REQUIRED_MODULE_FILES = [
    Path("engine/include/dragon/MugenData.h"),
    Path("engine/src/MugenData.cpp"),
    Path("engine/include/dragon/FightData.h"),
    Path("engine/src/FightData.cpp"),
]

RESERVED_BENCHMARK_CHARACTERS = {
    "DragonBench": [
        Path("game/chars/DragonBench/DragonBench.def"),
        Path("game/chars/DragonBench/DragonBench.cmd"),
        Path("game/chars/DragonBench/DragonBench.cns"),
        Path("game/chars/DragonBench/DragonBench.air"),
        Path("game/chars/DragonBench/DragonBench.sff"),
    ],
    "A.Ben": [
        Path("game/chars/A.Ben/A.Ben.def"),
        Path("game/chars/A.Ben/A.Ben.cmd"),
        Path("game/chars/A.Ben/A.Ben.cns"),
        Path("game/chars/A.Ben/A.Ben.air"),
        Path("game/chars/A.Ben/A.Ben.sff"),
    ],
    "I.Chie": [
        Path("game/chars/I.Chie/I.Chie.def"),
        Path("game/chars/I.Chie/I.Chie.cmd"),
        Path("game/chars/I.Chie/I.Chie.cns"),
        Path("game/chars/I.Chie/I.Chie.air"),
        Path("game/chars/I.Chie/I.Chie.sff"),
    ],
}


@dataclass
class Violation:
    path: Path
    line: int
    message: str

    def format(self) -> str:
        if self.line:
            return f"{self.path}:{self.line}: {self.message}"
        return f"{self.path}: {self.message}"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def line_number(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def add_regex_violations(
    violations: list[Violation],
    repo: Path,
    relative: Path,
    pattern: str,
    message: str,
    flags: int = 0,
) -> None:
    path = repo / relative
    text = read_text(path)
    for match in re.finditer(pattern, text, flags):
        violations.append(Violation(relative, line_number(text, match.start()), message))


def guard_required_layout(repo: Path, violations: list[Violation]) -> None:
    for relative in REQUIRED_GAME_DIRS:
        if not (repo / relative).is_dir():
            violations.append(Violation(relative, 0, "required M.U.G.E.N runtime folder is missing"))

    for relative in REQUIRED_GAME_FILES:
        if not (repo / relative).is_file():
            violations.append(Violation(relative, 0, "required M.U.G.E.N data file is missing"))

    for relative in REQUIRED_MODULE_FILES:
        if not (repo / relative).is_file():
            violations.append(Violation(relative, 0, "required engine data module file is missing"))


def guard_cmake_membership(repo: Path, violations: list[Violation]) -> None:
    cmake = Path("CMakeLists.txt")
    text = read_text(repo / cmake)
    for source in ("engine/src/MugenData.cpp", "engine/src/FightData.cpp"):
        if source not in text:
            violations.append(Violation(cmake, 0, f"{source} must stay compiled into dragon_core"))


def guard_app_size(repo: Path, violations: list[Violation]) -> None:
    app = Path("engine/src/App.cpp")
    line_count = len(read_text(repo / app).splitlines())
    if line_count > APP_CPP_LINE_BUDGET:
        violations.append(
            Violation(
                app,
                0,
                f"App.cpp has {line_count} lines; budget is {APP_CPP_LINE_BUDGET}. Split a real subsystem before adding more app code.",
            )
        )


def guard_app_layer_ownership(repo: Path, violations: list[Violation]) -> None:
    protected_structs = (
        "CharacterSlot",
        "CharacterFiles",
        "CharacterConstants",
        "StageSlot",
        "FightComboSettings",
        "FightPowerbarSettings",
        "FightRoundSettings",
    )
    protected_functions = (
        "loadCharacters",
        "loadStages",
        "resolveCharacterFiles",
        "loadCharacterConstants",
        "loadFightRoundSettings",
        "loadFightComboSettings",
        "loadFightPowerbarSettings",
    )

    struct_pattern = re.compile(r"\bstruct\s+(" + "|".join(protected_structs) + r")\b")
    function_definition_pattern = re.compile(
        r"^[ \t]*(?:[A-Za-z_][\w:<>,*&]*[ \t]+)+(" + "|".join(protected_functions) + r")\s*\("
    )
    protected_file_tokens = (
        ('"select.def"', "select.def roster parsing must flow through MugenData"),
        ('"fight.def"', "fight.def parsing must flow through FightData"),
        ('"chars/"', "character folder path rules must flow through MugenData"),
        ('"stages/"', "stage folder path rules must flow through MugenData"),
    )
    protected_path_patterns = (
        (re.compile(r'/\s*"chars"\b'), "character folder path rules must flow through MugenData"),
        (re.compile(r'/\s*"stages"\b'), "stage folder path rules must flow through MugenData"),
        (re.compile(r'/\s*"data"\s*/\s*"select\.def"'), "select.def roster parsing must flow through MugenData"),
        (re.compile(r'/\s*"data"\s*/\s*"fight\.def"'), "fight.def parsing must flow through FightData"),
    )

    for relative in APP_LAYER_FILES:
        text = read_text(repo / relative)
        for line_no, line in enumerate(text.splitlines(), 1):
            if struct_pattern.search(line):
                violations.append(
                    Violation(relative, line_no, "data ownership struct belongs in MugenData/FightData, not the app layer")
                )
            if function_definition_pattern.match(line):
                violations.append(
                    Violation(relative, line_no, "data loading function belongs in MugenData/FightData, not the app layer")
                )
            for token, message in protected_file_tokens:
                if token in line:
                    violations.append(Violation(relative, line_no, message))
            for pattern, message in protected_path_patterns:
                if pattern.search(line):
                    violations.append(Violation(relative, line_no, message))


def guard_module_ownership(repo: Path, violations: list[Violation]) -> None:
    mugen_data = read_text(repo / "engine/src/MugenData.cpp")
    fight_data = read_text(repo / "engine/src/FightData.cpp")

    if '"select.def"' not in mugen_data:
        violations.append(Violation(Path("engine/src/MugenData.cpp"), 0, "MugenData must own select.def parsing"))
    if '/ "chars"' not in mugen_data or '/ "stages"' not in mugen_data:
        violations.append(
            Violation(Path("engine/src/MugenData.cpp"), 0, "MugenData must own chars/stages path resolution")
        )
    if '"fight.def"' not in fight_data:
        violations.append(Violation(Path("engine/src/FightData.cpp"), 0, "FightData must own fight.def parsing"))


def guard_reserved_benchmark_characters(repo: Path, violations: list[Violation]) -> None:
    select_def = Path("game/data/select.def")
    text = read_text(repo / select_def)
    section = ""
    for line_no, raw in enumerate(text.splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip().lower()
            continue
        if section != "characters":
            continue

        character_entry = line.split(",", 1)[0].strip()
        folder = character_entry.split("/", 1)[0].strip()
        required_files = RESERVED_BENCHMARK_CHARACTERS.get(folder)
        if not required_files:
            continue

        missing = [str(path) for path in required_files if not (repo / path).is_file()]
        if missing:
            violations.append(
                Violation(
                    select_def,
                    line_no,
                    f"{folder} is a reserved benchmark character; do not list it until real M.U.G.E.N files exist: "
                    + ", ".join(missing),
                )
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", nargs="?", default=Path(__file__).resolve().parents[2], type=Path)
    args = parser.parse_args()

    repo = args.root.resolve()
    violations: list[Violation] = []

    guard_required_layout(repo, violations)
    guard_cmake_membership(repo, violations)
    guard_app_size(repo, violations)
    guard_app_layer_ownership(repo, violations)
    guard_module_ownership(repo, violations)
    guard_reserved_benchmark_characters(repo, violations)

    if violations:
        print("Dragon architecture guard failed:")
        for violation in violations:
            print(f"  - {violation.format()}")
        return 1

    print("Dragon architecture guard: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
