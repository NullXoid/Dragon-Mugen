# Benchmark Characters

This file tracks original Dragon MUGEN benchmark characters. These are project-owned test fighters for engine behavior and AI work. They must use normal M.U.G.E.N character structure so the runtime stays compatible with creator content.

## Activation Rule

Reserved folders may exist under `game/chars/` without appearing on the character select screen. A benchmark character becomes selectable only when it has a real M.U.G.E.N-compatible fileset and an explicit entry in `game/data/select.def`.

Do not list these characters in `select.def` while they are README-only placeholders. The architecture guard enforces this.

Minimum selectable files:

- `<character>.def`
- `<character>.cmd`
- `<character>.cns`
- `<character>.air`
- `<character>.sff`

`.snd`, `.act`, and `<character>.dragon.def` are allowed later, but the Dragon sidecar must not replace M.U.G.E.N combat files.

## Planned Characters

| Folder | Purpose | First Complete Version |
| --- | --- | --- |
| `DragonBench` | Deterministic regression fighter for engine audits. | Idle, walk, crouch, jump, six buttons, one guardable hit, one fall hit, one projectile, one helper, one super, and one throw/custom-state path. |
| `A.Ben` | Basic AI benchmark opponent. | Uses simple readable AI gates, forward/back dash, normals, one special, guard decisions, and round-safe behavior. |
| `I.Chie` | Advanced AI and compatibility stress character. | Exercises variables, helpers, projectiles, target controllers, superpause, explods, palette effects, and richer AI decision logic. |

## Work Order

1. Build `DragonBench` first, because it should expose engine regressions without complex AI.
2. Build `A.Ben` after the baseline character is playable, using conservative AI that is easy to debug.
3. Build `I.Chie` after helper/projectile/target semantics are mature enough to stress them intentionally.

## Content Rules

- Keep each character in `game/chars/<folder>/`.
- Keep combat behavior in M.U.G.E.N files: DEF, CMD, CNS/ST, AIR, SFF, SND, ACT.
- Use `<character>.dragon.def` only for Dragon-specific presentation metadata such as command-training labels, editor hints, AI benchmark tags, or future progression defaults.
- Add the character to `game/data/select.def` only after it can load and fight without placeholder runtime behavior.
- Update `docs/COMPATIBILITY_AUDIT.md` when a benchmark character becomes selectable.

## Exit Criteria

A benchmark character is considered real project content when:

- It appears in `game/data/select.def`.
- `python engine/tools/dev_check.py . --skip-build` passes.
- `python engine/tools/audit_mugen_compat.py game` reports it in the active roster.
- It can enter Training, VS, and Single Player without relying on hardcoded character behavior.
