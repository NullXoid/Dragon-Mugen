# Fight Correctness

Status: Complete

## Goal

Make classic Training, Single Player, and VS fight outcomes reliable enough to support higher modes. The immediate focus is deterministic KO, double-KO, time-over, round scoring, match result, and pause routing verification before adding story, tournament, or richer AI.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/FEATURE_LEDGER.md`
- `game/data/fight.def`
- `game/data/common1.cns`
- active character DEF/CMD/CNS/AIR/SFF/SND files selected through `game/data/select.def`

## Scope

In scope:

- Classic Single Player and VS round outcomes.
- Round wins, match completion, match-result routing, and result-menu readiness.
- Guard, fall recovery, KO, double-KO, time-over, and pause-action verification.
- Scripted verifiers for deterministic fight-rule behavior.

Out of scope:

- Arena-specific rules beyond preserving existing verifiers.
- Story, tournament, shop, equipment, editor, networking, or new content modes.
- Character-specific engine branches that bypass CMD/CNS/AIR/SFF/SND data.

## Feature Slice

The active feature slice is complete classic fight-rule correctness for Training, Single Player, and VS outcomes at the current compatibility roster level. It includes deterministic outcome/routing verification, guard/fall/KO coverage, generic character-data-driven fixes for confirmed regressions, and matching roadmap, ledger, and regression documentation.

Incremental commits may land inside this slice, but they must keep closing fight correctness as a full capability. Do not replace this slice with isolated micro-verifiers or character-specific patches.

## Ownership

- Fight outcome runtime currently lives in the fight/session runtime path used by `App.cpp`.
- Scripted runtime proof lives in the verification scenarios.
- Preservation requirements live in `docs/FEATURE_LEDGER.md`.
- Repeatable checks live in `docs/REGRESSION_CHECKLIST.md`.

## Implementation Checklist

- [x] Add deterministic classic fight outcome verification for P1 KO, P2 KO, double-KO, and time-over draw.
- [x] Add deterministic round scoring and match-result verification.
- [x] Add pause actions beyond Resume verification for Single Player and VS.
- [x] Add guard, guard spark/sound, knockdown, fall recovery, and KO coverage for KFM, Evil Ken, and Evil Ryu.
- [x] Fix any confirmed fight-rule regressions found by the verifier or live pass.
- [x] Update preservation and regression docs after each meaningful feature-slice milestone.

## Verification

Run:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify classic-fight-outcomes
build\dragon_mugen.exe --verify classic-fight-routing
build\dragon_mugen.exe --verify classic-fight-combat
build\dragon_mugen.exe --verify cpu-baseline
build\dragon_mugen.exe --verify vs-p2-runtime
build\dragon_mugen.exe --verify kfm-guard-recovery
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
git diff --check
python tools\check_file_sizes.py
```

Manual smoke should cover Single Player and VS pause/result actions, including Restart Match, Fighter Select, Stage Select, Mode Select, Rematch, and menu return.

## Done Means

- Classic Single Player and VS have verifier or recorded live coverage for P1 KO, P2 KO, double-KO, time-over draw, round scoring, match completion, rematch, restart, and menu return.
- Guard and fall recovery are verified for the active compatibility roster.
- Existing Training, CPU, VS, and Arena verifier suites stay green.
- Higher-level modes can rely on deterministic classic fight outcomes without duplicating fight-rule logic.
