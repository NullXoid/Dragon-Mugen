#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


VALID_STATUSES = {"planned", "in progress", "blocked", "complete"}

REQUIRED_HEADINGS = [
    "## Goal",
    "## Source References",
    "## Scope",
    "## Ownership",
    "## Implementation Checklist",
    "## Verification",
    "## Done Means",
]

EXCLUDED_FILES = {"README.md", "TEMPLATE.md"}


@dataclass
class Violation:
    path: Path
    message: str


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def feature_specs(repo: Path) -> list[Path]:
    specs_dir = repo / "docs" / "FEATURE_SPECS"
    if not specs_dir.is_dir():
        return []
    return sorted(path for path in specs_dir.glob("*.md") if path.name not in EXCLUDED_FILES)


def status_for(text: str) -> str | None:
    match = re.search(r"^Status:\s*(.+?)\s*$", text, flags=re.IGNORECASE | re.MULTILINE)
    if not match:
        return None
    return match.group(1).strip()


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Dragon MUGEN feature-completion specs.")
    parser.add_argument("root", nargs="?", default=Path(__file__).resolve().parents[2], type=Path)
    args = parser.parse_args()

    repo = args.root.resolve()
    specs = feature_specs(repo)
    violations: list[Violation] = []

    if not specs:
        violations.append(Violation(Path("docs/FEATURE_SPECS"), "at least one feature spec is required"))

    in_progress: list[Path] = []
    for spec in specs:
        relative = spec.relative_to(repo)
        text = read_text(spec)
        status = status_for(text)
        if status is None:
            violations.append(Violation(relative, "missing top-level 'Status: ...' line"))
        elif status.lower() not in VALID_STATUSES:
            violations.append(
                Violation(
                    relative,
                    f"invalid status '{status}'. Use one of: Planned, In Progress, Blocked, Complete",
                )
            )
        elif status.lower() == "in progress":
            in_progress.append(relative)

        for heading in REQUIRED_HEADINGS:
            if heading not in text:
                violations.append(Violation(relative, f"missing required heading '{heading}'"))

        if re.search(r"\b(TBD|TODO)\b", text, flags=re.IGNORECASE):
            violations.append(
                Violation(relative, "feature specs must not contain TODO/TBD placeholders; use explicit checklist items")
            )

    if len(in_progress) > 1:
        names = ", ".join(str(path) for path in in_progress)
        violations.append(Violation(Path("docs/FEATURE_SPECS"), f"only one feature may be In Progress: {names}"))

    if violations:
        print("Dragon feature spec check failed:")
        for violation in violations:
            print(f"  - {violation.path}: {violation.message}")
        return 1

    print("Dragon feature spec check: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
