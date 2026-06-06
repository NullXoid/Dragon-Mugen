#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def run_step(label: str, command: list[str], cwd: Path) -> int:
    print(f"\n== {label} ==", flush=True)
    print(" ".join(command), flush=True)
    completed = subprocess.run(command, cwd=cwd)
    return completed.returncode


def exe_path(repo: Path) -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    return repo / "build" / f"dragon_mugen{suffix}"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Dragon MUGEN development checks.")
    parser.add_argument("root", nargs="?", default=Path(__file__).resolve().parents[2], type=Path)
    parser.add_argument("--quick", action="store_true", help="Only run the architecture guard.")
    parser.add_argument("--skip-build", action="store_true", help="Skip CMake configure/build and console loader check.")
    args = parser.parse_args()

    repo = args.root.resolve()
    checks: list[tuple[str, list[str]]] = [
        ("architecture guard", [sys.executable, "engine/tools/guard_architecture.py", "."]),
        ("feature spec check", [sys.executable, "engine/tools/check_feature_specs.py", "."]),
    ]

    if not args.quick:
        checks.extend(
            [
                ("content inspection", [sys.executable, "engine/tools/inspect_mugen_content.py", "game"]),
                ("compatibility audit", [sys.executable, "engine/tools/audit_mugen_compat.py", "game"]),
            ]
        )

    if not args.quick and not args.skip_build:
        checks.extend(
            [
                ("cmake configure", ["cmake", "-S", ".", "-B", "build"]),
                ("build dragon_mugen", ["cmake", "--build", "build", "--target", "dragon_mugen", "--config", "Debug"]),
            ]
        )
        exe = exe_path(repo)
        checks.append(("console roster check", [str(exe), "game", "--console"]))
        checks.extend(
            [
                ("verify kfm-baseline", [str(exe), "--verify", "kfm-baseline"]),
                ("verify evilken-smoke", [str(exe), "--verify", "evilken-smoke"]),
                ("verify kfm-air-state", [str(exe), "--verify", "kfm-air-state"]),
                ("verify cpu-baseline", [str(exe), "--verify", "cpu-baseline"]),
            ]
        )

    for label, command in checks:
        code = run_step(label, command, repo)
        if code != 0:
            print(f"\nFAILED: {label}")
            return code

    print("\nDragon MUGEN development checks: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
