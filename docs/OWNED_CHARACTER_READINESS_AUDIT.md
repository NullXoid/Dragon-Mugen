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

A.Ben is loadable and has a usable prototype control foundation, but he is not ready for final visual combat testing.

Verified content:

- AIR actions: 54
- Unique AIR sprite references: 9
- Missing AIR sprite references: 0
- SFF sprite records: 27
- Unique SFF group/index pairs used by the current AIR: `0,0` and walk group `20,0` through `20,7`
- Non-placeholder movement actions: `20`, `21`, `100`, `105`

Important readiness gaps:

- Idle, crouch, jump, landing, hit, guard, recover, intro, taunt, win, attacks, specials, and super all still reuse placeholder sprite `0,0`.
- Attack states and hitboxes exist, but their visuals are not authored yet.
- The current walk/run cycle is the only meaningful authored movement animation.
- SFF contains repeated `0,0` records, which should be cleaned when the character art pipeline is finalized.
- Combat testing should use A.Ben only for control and state-flow checks until his frame set is expanded.

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
- A.Ben walk/run movement checks.
- Basic load and state-flow smoke checks.
- Verifying no missing AIR sprite references.

Do not use owned characters yet for:

- Final combat feel.
- Final hit reaction quality.
- Animation timing polish.
- Visual proof of attack/special/super quality.
- Judging the fight renderer against finished-content expectations.

Use compatibility fixtures for engine behavior until A.Ben and I.Chie have complete authored frame sets.

## Recommended Next Character Work

1. Build A.Ben's minimum playable animation set:
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

