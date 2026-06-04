#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


APP_CPP = Path("engine/src/App.cpp")
MINIMUM_APP_CPP_EXTRACTION_TARGET = 15500


def staged_paths(repo: Path) -> set[Path]:
    completed = subprocess.run(
        ["git", "diff", "--cached", "--name-only", "--", str(APP_CPP).replace("\\", "/")],
        cwd=repo,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        print(completed.stderr.strip())
        raise SystemExit(completed.returncode)
    return {Path(line.strip()) for line in completed.stdout.splitlines() if line.strip()}


def line_count(path: Path) -> int:
    return len(path.read_text(encoding="utf-8", errors="replace").splitlines())


def main() -> int:
    parser = argparse.ArgumentParser(description="Guard active Dragon MUGEN recovery commits.")
    parser.add_argument("root", nargs="?", default=Path(__file__).resolve().parents[2], type=Path)
    args = parser.parse_args()

    repo = args.root.resolve()
    if APP_CPP not in staged_paths(repo):
        print("Dragon active-change guard: PASS")
        return 0

    current_lines = line_count(repo / APP_CPP)
    if current_lines > MINIMUM_APP_CPP_EXTRACTION_TARGET:
        print("Dragon active-change guard failed:")
        print(
            f"  - {APP_CPP} is staged but still has {current_lines} lines. "
            f"Architecture recovery requires a complete extraction batch at or below "
            f"{MINIMUM_APP_CPP_EXTRACTION_TARGET} lines before App.cpp changes may be committed."
        )
        print("  - Do not commit symbolic App.cpp reductions. Complete a real module extraction first.")
        return 1

    print("Dragon active-change guard: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
