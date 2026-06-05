#!/usr/bin/env python3
"""
Dragon MUGEN file-size guard.

Purpose:
Prevent the codebase from turning back into a monolith.

Usage:
    python tools/check_file_sizes.py
"""

from __future__ import annotations

import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

SMALL_TARGET_LINES = 350
MEDIUM_TARGET_LINES = 750
FAIL_LINES = 1000

SOURCE_EXTENSIONS = {
    ".py", ".js", ".jsx", ".ts", ".tsx",
    ".cs", ".cpp", ".cc", ".cxx", ".c",
    ".h", ".hpp", ".hh",
    ".go", ".rs", ".lua",
    ".gd", ".java", ".kt",
}

IGNORE_DIRS = {
    ".git",
    ".hg",
    ".svn",
    ".venv",
    "venv",
    "env",
    "__pycache__",
    ".pytest_cache",
    ".mypy_cache",
    ".ruff_cache",
    "node_modules",
    "dist",
    "build",
    "out",
    "target",
    "bin",
    "obj",
    "vendor",
    "third_party",
    "assets",
    "cache",
    ".cache",
}

IGNORE_FILE_NAMES = {
    "package-lock.json",
    "yarn.lock",
    "pnpm-lock.yaml",
    "Cargo.lock",
    "poetry.lock",
}

ALLOWLIST_PATH = ROOT / "tools" / "file_size_allowlist.txt"


def load_allowlist() -> set[str]:
    if not ALLOWLIST_PATH.exists():
        return set()

    allowed: set[str] = set()

    for raw_line in ALLOWLIST_PATH.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()

        if not line or line.startswith("#"):
            continue

        path_part = line.split("#", 1)[0].strip()
        if path_part:
            allowed.add(path_part.replace("\\", "/"))

    return allowed


def should_ignore(path: Path) -> bool:
    relative_parts = set(path.relative_to(ROOT).parts)

    if relative_parts & IGNORE_DIRS:
        return True

    if path.name in IGNORE_FILE_NAMES:
        return True

    if path.suffix.lower() not in SOURCE_EXTENSIONS:
        return True

    if ".generated." in path.name or path.name.startswith("generated_"):
        return True

    return False


def count_lines(path: Path) -> int:
    try:
        with path.open("r", encoding="utf-8", errors="ignore") as file:
            return sum(1 for _ in file)
    except OSError:
        return 0


def main() -> int:
    allowlist = load_allowlist()
    records: list[tuple[int, str]] = []

    for dirpath, dirnames, filenames in os.walk(ROOT):
        current = Path(dirpath)

        dirnames[:] = [
            name for name in dirnames
            if name not in IGNORE_DIRS
        ]

        for filename in filenames:
            path = current / filename

            if should_ignore(path):
                continue

            rel = path.relative_to(ROOT).as_posix()
            lines = count_lines(path)
            records.append((lines, rel))

    records.sort(reverse=True)

    medium_warnings = []
    large_warnings = []
    failures = []

    for lines, rel in records:
        if lines > FAIL_LINES and rel not in allowlist:
            failures.append((lines, rel))
        elif lines > MEDIUM_TARGET_LINES:
            large_warnings.append((lines, rel))
        elif lines > SMALL_TARGET_LINES:
            medium_warnings.append((lines, rel))

    print("\nLargest source files:")
    for lines, rel in records[:20]:
        print(f"  {lines:5d}  {rel}")

    if medium_warnings:
        print(f"\nMedium files over {SMALL_TARGET_LINES} lines:")
        for lines, rel in medium_warnings:
            print(f"  {lines:5d}  {rel}")

    if large_warnings:
        print(f"\nLarge files over {MEDIUM_TARGET_LINES} lines:")
        for lines, rel in large_warnings:
            print(f"  {lines:5d}  {rel}")

    if failures:
        print(f"\nFailures: files over {FAIL_LINES} lines:")
        for lines, rel in failures:
            print(f"  {lines:5d}  {rel}")

        print("\nFix by extracting one responsibility into a smaller module.")
        print("Only allowlist generated, vendor, or clearly justified files.")
        return 1

    print("\nFile-size guard passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
