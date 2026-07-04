# Owned Character Readiness Audit

Date: 2026-07-03

## Scope

This audit covers only the owned character packages:

- `game/chars/A.Ben`
- `game/chars/I.Chie`

Compatibility fixture characters such as KFM and the compatibility roster are still useful for engine verification, but they should not be treated as owned content or player-facing content.

## Current Roster Policy

`game/data/select.def` currently lists only:

- `A.Ben, stages/kfm.def`
- `I.Chie, stages/kfm.def`

`game/data/compatibility_select.def` keeps the non-owned compatibility fixtures separated for runtime and parser testing.

## A.Ben Status

A.Ben is loadable and now has a usable prototype control and action-sprite foundation, but he is not ready for final visual combat testing.

Verified content:

- AIR actions: 54
- Unique AIR sprite references: 77
- Missing AIR sprite references: 0
- SFF sprite records reported by the compatibility audit: 79
- Original walk group: `20,0` through `20,7`
- Derived prototype action groups: `40`, `41`, `42`, `47`, `50`, `52`, `100`, `105`, `200`, `210`, `220`, `230`, `240`, `250`, `1000`, `1010`, `1020`, `1100`, `1110`, `1120`, and `3000`

Important readiness gaps:

- Idle, crouch, guard, get-hit, recover, intro, taunt, and win still reuse placeholder sprite `0,0`.
- Dash, jump, punch, kick, special, and super states now use derived prototype action sprites rather than the old front-facing placeholder.
- Attack states and hitboxes exist, but the action sprites are derived bridge art, not final authored animation.
- The accepted walk cycle remains the strongest authored movement animation.
- SFF contains repeated `0,0` records, which should be cleaned when the character art pipeline is finalized.
- Combat testing should still treat A.Ben as prototype content until his final frame set is expanded and approved.

## I.Chie Status

I.Chie is currently stronger as a shopkeeper/NPC package than as a combat character.

Verified content:

- AIR actions: 55
- Unique AIR sprite references: 2
- Missing AIR sprite references: 0
- SFF sprite records: 20
- Shopkeeper-specific action: `9100`
- Non-placeholder action: `9100`

Important readiness gaps:

- Movement, attacks, specials, super, guard, hit, recover, intro, taunt, and win actions still reuse placeholder sprite `0,0`.
- I.Chie should remain shopkeeper-first until a fighting animation set exists.
- Fight-mode failures or weak visuals from I.Chie should not be treated as engine failures yet.
- SFF contains repeated `0,0` records, which should be cleaned when the character art pipeline is finalized.

## Testing Guidance

Use owned characters for:

- Shop presentation and interaction checks.
- A.Ben walk/run, dash, jump, punch, and kick smoke checks.
- Basic load and state-flow smoke checks.
- Verifying no missing AIR sprite references.

Do not use owned characters yet for:

- Final combat feel.
- Final hit reaction quality.
- Animation timing polish.
- Visual proof of final attack/special/super quality.
- Judging the fight renderer against finished-content expectations.

Use compatibility fixtures for engine behavior until A.Ben and I.Chie have complete authored frame sets.

## Recommended Next Character Work

1. Replace A.Ben's derived prototype action sprites with final authored animation:
   - idle
   - turn
   - crouch start/hold/end
   - jump start/up/forward/back/land
   - guard
   - light/medium/heavy punches and kicks
   - hit high/low/air
   - liedown and recover
   - intro and win
2. Rebuild A.Ben SFF/AIR from source frames with consistent center-bottom feet origins.
3. Keep I.Chie in shopkeeper/NPC use until combat frames exist.
4. Add an owned-character visual readiness verifier that reports placeholder AIR actions without failing normal engine verification.
5. Promote owned characters to full combat regression coverage only after their animation sets are authored.
