# Feature Specs

Feature specs are the active work contract for Dragon MUGEN.

Every implementation feature gets one Markdown file in this folder. The file must have a top-level `Status:` line and the required headings enforced by `engine/tools/check_feature_specs.py`.

Allowed statuses:

- `Planned`
- `In Progress`
- `Blocked`
- `Complete`

Only one feature may be `In Progress` at a time.

Current specs:

- `0001_architecture_recovery.md` - Planned architecture recovery and module ownership guardrails.
- `0002_fight_correctness.md` - Complete classic fight outcome/routing/combat correctness slice.
- `0003_roster_compatibility_readiness.md` - Complete selectable-roster compatibility smoke slice.
- `0004_openbor_stage_compatibility_v2.md` - Planned Arena OpenBOR stage compatibility, load progress, and 4-fighter performance slice.
- `0005_dragon_progression_leveling_items.md` - Complete Dragon progression, leveling, and item foundation.
- `0006_dragon_profile_progression_display.md` - Complete user-profile progression ownership and LV/XP display slice.
- `0007_local_player_profiles_vs_progression.md` - Complete local P1/P2 profile slots, Guest rules, and VS progression awards.
- `0008_controls_submenu_unified_options.md` - Complete category Options and per-profile action-based Controls submenu.
- `0009_story_mode_openbor_side_scroller.md` - Complete Story Mode foundation with OpenBOR-style scrolling enemy waves.
- `0010_scott_pilgrim_stage_compatibility.md` - Complete first external Scott stage compatibility proof with SFF v2 PNG/palette sprites, animated stage BGs, and MP3/OGG/WAV stage music.

Required headings:

- `## Goal`
- `## Source References`
- `## Scope`
- `## Feature Slice`
- `## Ownership`
- `## Implementation Checklist`
- `## Verification`
- `## Done Means`
