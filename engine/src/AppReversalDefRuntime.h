#pragma once

// Internal App.cpp hit-routing shard for minimal MUGEN ReversalDef compatibility.

const StateReversalDefController* activeReversalDefForDefender(
    AppState& state,
    FighterState& defender,
    const FighterState& attacker,
    const HitDefinition& hitDef,
    const StageSlot* stage) {
    const auto stateDefs = runtimeControllerStateDefinitions(state, defender);
    for (const StateDefinition* stateDef : stateDefs) {
        if (!stateDef) {
            continue;
        }
        for (const auto& reversal : stateDef->reversalDefs) {
            if (!hitProtectionMatches(reversal.attr, hitDef)
                || !shouldRunStateRuntimeController(state, defender, reversal.id, reversal.trigger, &attacker, stage)) {
                continue;
            }
            return &reversal;
        }
    }
    return nullptr;
}

bool applyReversalDef(
    AppState& state,
    FighterState& attacker,
    FighterState& defender,
    const HitDefinition& hitDef,
    const StateReversalDefController& reversal,
    size_t defenderIndex,
    size_t comboAttackerIndex) {
    markHitDefApplied(attacker, hitDef.id, currentAnimElemForFighter(state, attacker), defenderIndex);
    attacker.moveContact = true;
    defender.moveContact = true;
    defender.moveHit = true;
    attacker.hitPauseTicks = fightHitPauseTicks(state, reversal.pauseTimeP2, 0);
    defender.hitPauseTicks = fightHitPauseTicks(state, reversal.pauseTimeP1, 1);
    defender.targetIndex = fighterIndexInState(state, attacker);
    defender.targetHitId = hitDef.targetId;
    defender.targetTicks = std::max(defender.targetTicks, kStoredTargetLinkTicks);
    if (reversal.p2StateNo >= 0) {
        enterState(state, attacker, reversal.p2StateNo);
        attacker.customHitState = true;
        attacker.customStateOwnerIndex = static_cast<int>(defenderIndex);
        attacker.moveType = 'H';
        attacker.ctrl = false;
    }
    if (reversal.p1StateNo >= 0 && enterState(state, defender, reversal.p1StateNo)) {
        defender.ctrl = false;
    }
    if (shouldPlayFightSounds(state)) {
        playSound(state, reversal.hitSoundGroup, reversal.hitSoundIndex, reversal.hitSoundForceCommon, -1, false, 1.0f, false, static_cast<int>(defenderIndex));
    }
    state.messages.lastHitText = std::string(fighterLabel(defenderIndex))
        + " reversal "
        + std::to_string(hitDef.stateNo)
        + "#"
        + std::to_string(hitDef.id)
        + " attr "
        + hitDef.attr
        + " vs "
        + std::string(fighterLabel(comboAttackerIndex));
    state.messages.lastHitTextTicks = 150;
    logFightHitEvent(state.messages.lastHitText);
    return true;
}
