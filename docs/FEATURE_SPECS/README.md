# Feature Specs

Feature specs are the active work contract for Dragon MUGEN.

Every implementation feature gets one Markdown file in this folder. The file must have a top-level `Status:` line and the required headings enforced by `engine/tools/check_feature_specs.py`.

Allowed statuses:

- `Planned`
- `In Progress`
- `Blocked`
- `Complete`

Only one feature may be `In Progress` at a time.

Required headings:

- `## Goal`
- `## Source References`
- `## Scope`
- `## Ownership`
- `## Implementation Checklist`
- `## Verification`
- `## Done Means`
