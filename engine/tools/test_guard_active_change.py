#!/usr/bin/env python3
from __future__ import annotations

import unittest

import guard_active_change


class ActiveChangeGuardTests(unittest.TestCase):
    def test_no_engine_change_passes(self) -> None:
        engine_changes, failures = guard_active_change.documentation_failures({"docs/README.md"})
        self.assertEqual(engine_changes, [])
        self.assertEqual(failures, [])

    def test_missing_preservation_fails(self) -> None:
        _, failures = guard_active_change.documentation_failures(
            {"engine/src/App.cpp", "CHANGELOG.md"}
        )
        self.assertEqual(failures, ["preservation"])

    def test_missing_changelog_fails(self) -> None:
        _, failures = guard_active_change.documentation_failures(
            {"engine/src/App.cpp", "docs/FEATURE_LEDGER.md"}
        )
        self.assertEqual(failures, ["changelog"])

    def test_complete_documentation_passes(self) -> None:
        _, failures = guard_active_change.documentation_failures(
            {"engine/src/App.cpp", "docs/FEATURE_SPECS/0001_architecture_recovery.md", "CHANGELOG.md"}
        )
        self.assertEqual(failures, [])


if __name__ == "__main__":
    unittest.main()
