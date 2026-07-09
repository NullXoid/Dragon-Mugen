from __future__ import annotations

import os
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass
class CommandResult:
    title: str
    command: str
    returncode: int
    stdout: str = ""
    stderr: str = ""

    @property
    def ok(self) -> bool:
        return self.returncode == 0


def shell_text(command: list[str]) -> str:
    return subprocess.list2cmdline(command)


def result_error(title: str, command: str, message: str) -> CommandResult:
    return CommandResult(title=title, command=command, returncode=1, stderr=message)


def run_command(root: Path, title: str, command: list[str], env: dict[str, str] | None = None, timeout: int = 600) -> CommandResult:
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)
    try:
        completed = subprocess.run(command, cwd=root, env=merged_env, capture_output=True, text=True, timeout=timeout)
    except FileNotFoundError as exc:
        return result_error(title, shell_text(command), f"Required executable not found: {exc.filename}")
    except subprocess.TimeoutExpired as exc:
        return CommandResult(
            title=title,
            command=shell_text(command),
            returncode=1,
            stdout=exc.stdout or "",
            stderr=(exc.stderr or "") + f"\nTimed out after {timeout} seconds.",
        )
    return CommandResult(
        title=title,
        command=shell_text(command),
        returncode=completed.returncode,
        stdout=completed.stdout,
        stderr=completed.stderr,
    )
