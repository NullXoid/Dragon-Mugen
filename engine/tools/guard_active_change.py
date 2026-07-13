#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ENGINE_CHANGE_PREFIXES = (
    "engine/src/",
    "engine/include/",
)

ENGINE_CHANGE_FILES = {
    "CMakeLists.txt",
}

PRESERVATION_DOCS = {
    "docs/FEATURE_LEDGER.md",
    "docs/REGRESSION_CHECKLIST.md",
}

FEATURE_SPEC_PREFIX = "docs/FEATURE_SPECS/"
CHANGELOG_PATH = "CHANGELOG.md"


def staged_paths(repo: Path) -> set[str]:
    completed = subprocess.run(
        ["git", "diff", "--cached", "--name-only"],
        cwd=repo,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        print(completed.stderr.strip())
        raise SystemExit(completed.returncode)
    return {line.strip().replace("\\", "/") for line in completed.stdout.splitlines() if line.strip()}


def is_engine_change(path: str) -> bool:
    return path in ENGINE_CHANGE_FILES or any(path.startswith(prefix) for prefix in ENGINE_CHANGE_PREFIXES)


def is_preservation_update(path: str) -> bool:
    return path in PRESERVATION_DOCS or path.startswith(FEATURE_SPEC_PREFIX)


def documentation_failures(paths: set[str]) -> tuple[list[str], list[str]]:
    engine_changes = sorted(path for path in paths if is_engine_change(path))
    if not engine_changes:
        return engine_changes, []

    failures: list[str] = []
    if not any(is_preservation_update(path) for path in paths):
        failures.append("preservation")
    if CHANGELOG_PATH not in paths:
        failures.append("changelog")
    return engine_changes, failures


def main() -> int:
    parser = argparse.ArgumentParser(description="Guard active Dragon MUGEN changes against undocumented regressions.")
    parser.add_argument("root", nargs="?", default=Path(__file__).resolve().parents[2], type=Path)
    args = parser.parse_args()

    repo = args.root.resolve()
    paths = staged_paths(repo)
    engine_changes, failures = documentation_failures(paths)
    if not failures:
        print("Dragon active-change guard: PASS")
        return 0

    print("Dragon active-change guard failed:")
    if "preservation" in failures:
        print("  - Engine/app code is staged without a preservation documentation update.")
        print("  - Update docs/FEATURE_LEDGER.md, docs/REGRESSION_CHECKLIST.md, or the active feature spec.")
    if "changelog" in failures:
        print("  - Engine/app code is staged without an Unreleased notification in CHANGELOG.md.")
    print("  - Staged engine files:")
    for path in engine_changes:
        print(f"    - {path}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
