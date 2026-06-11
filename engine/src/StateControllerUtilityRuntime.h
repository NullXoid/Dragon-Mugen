#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local controller/runtime types and helpers.
// Include only from App.cpp after runtime expression helpers and fighter index
// helpers are defined. Do not include from other translation units.

int stateControllerFireKey(int controllerId, const FighterState& fighter, int animElem) {
    return controllerId * 100000 + std::max(0, fighter.stateTime) * 100 + std::max(0, animElem);
}

bool stateRuntimeControllerAlreadyFired(const FighterState& fighter, int controllerId);
void markStateRuntimeControllerFired(FighterState& fighter, int controllerId);
bool shouldRunStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    const FighterState* opponent,
    const StageSlot* stage);
bool shouldRunSimpleStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    int triggerTime,
    int triggerAnimElem);

int stateControllerDomainKey(int domain, int controllerId) {
    return -(domain * 100000 + std::max(1, controllerId));
}

StateControllerCooldown* findStateRuntimeControllerCooldown(FighterState& fighter, int controllerId) {
    auto it = std::find_if(
        fighter.stateRuntimeControllerCooldowns.begin(),
        fighter.stateRuntimeControllerCooldowns.end(),
        [controllerId](const StateControllerCooldown& cooldown) {
            return cooldown.controllerId == controllerId;
        });
    return it == fighter.stateRuntimeControllerCooldowns.end() ? nullptr : &*it;
}

int stateRuntimeControllerCooldownTicks(FighterState& fighter, int controllerId) {
    if (const auto* cooldown = findStateRuntimeControllerCooldown(fighter, controllerId)) {
        return cooldown->ticks;
    }
    return 0;
}

void setStateRuntimeControllerCooldown(FighterState& fighter, int controllerId, int ticks) {
    if (ticks <= 0) {
        return;
    }
    if (auto* cooldown = findStateRuntimeControllerCooldown(fighter, controllerId)) {
        cooldown->ticks = std::max(cooldown->ticks, ticks);
        return;
    }
    fighter.stateRuntimeControllerCooldowns.push_back(StateControllerCooldown{ controllerId, ticks });
}

bool stateRuntimeControllerBlockedByHitPause(
    const FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger) {
    if (fighter.hitPauseTicks <= 0) {
        return false;
    }
    if (fighter.hitPauseChangeStateControllerId == controllerId) {
        return true;
    }
    return !trigger.ignoreHitPause;
}

bool stateRuntimeControllerPersistenceAllowsRun(
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger) {
    if (trigger.persistent == 0) {
        if (stateRuntimeControllerAlreadyFired(fighter, controllerId)) {
            return false;
        }
        markStateRuntimeControllerFired(fighter, controllerId);
        return true;
    }
    if (trigger.persistent > 1) {
        if (stateRuntimeControllerCooldownTicks(fighter, controllerId) > 0) {
            return false;
        }
        setStateRuntimeControllerCooldown(fighter, controllerId, trigger.persistent);
    }
    return true;
}

bool stateSoundFireKeyAlreadyFired(const FighterState& fighter, int fireKey) {
    return std::find(
        fighter.firedStateSoundControllerIds.begin(),
        fighter.firedStateSoundControllerIds.end(),
        fireKey) != fighter.firedStateSoundControllerIds.end();
}

bool stateSoundControllerIdAlreadyFired(const FighterState& fighter, int soundControllerId) {
    return std::find(
        fighter.firedStateSoundControllerIds.begin(),
        fighter.firedStateSoundControllerIds.end(),
        soundControllerId) != fighter.firedStateSoundControllerIds.end();
}

void markStateSoundFireKeyFired(FighterState& fighter, int fireKey) {
    if (!stateSoundFireKeyAlreadyFired(fighter, fireKey)) {
        fighter.firedStateSoundControllerIds.push_back(fireKey);
    }
}

void markStateSoundControllerIdFired(FighterState& fighter, int soundControllerId) {
    if (!stateSoundControllerIdAlreadyFired(fighter, soundControllerId)) {
        fighter.firedStateSoundControllerIds.push_back(soundControllerId);
    }
}

bool stateSoundTriggerActive(AppState& state, FighterState& fighter, const StateSoundController& sound) {
    return sound.trigger.hasTrigger
        ? stateControllerTriggerActive(state, fighter, sound.trigger, nullptr, nullptr)
        : simpleControllerTriggerActive(state, fighter, sound.triggerTime, sound.triggerAnimElem);
}

void executePlaySoundController(AppState& state, FighterState& fighter, const StateSoundController& sound) {
    const int animElem = currentAnimElemForFighter(state, fighter);
    const int controllerKey = stateControllerDomainKey(1, sound.id);
    if (sound.trigger.hasTrigger) {
        if (!shouldRunStateRuntimeController(state, fighter, controllerKey, sound.trigger, nullptr, nullptr)) {
            return;
        }
    } else if (!shouldRunSimpleStateRuntimeController(
        state,
        fighter,
        controllerKey,
        sound.trigger,
        sound.triggerTime,
        sound.triggerAnimElem)) {
        return;
    }

    const int fireKey = stateControllerFireKey(controllerKey, fighter, animElem);
    if (stateSoundFireKeyAlreadyFired(fighter, fireKey)) {
        return;
    }
    markStateSoundFireKeyFired(fighter, fireKey);
    if (shouldPlayFightSounds(state)) {
        const int ownerIndex = fighter.helper ? fighter.ownerIndex : fighterIndexInState(state, fighter);
        playSound(
            state,
            sound.group,
            sound.index,
            sound.forceCommon,
            sound.channel,
            sound.lowPriority,
            sound.gain,
            sound.loop,
            ownerIndex);
    }
}

bool stateRuntimeControllerAlreadyFired(const FighterState& fighter, int controllerId) {
    return std::find(
        fighter.firedStateRuntimeControllerIds.begin(),
        fighter.firedStateRuntimeControllerIds.end(),
        controllerId) != fighter.firedStateRuntimeControllerIds.end();
}

void markStateRuntimeControllerFired(FighterState& fighter, int controllerId) {
    if (!stateRuntimeControllerAlreadyFired(fighter, controllerId)) {
        fighter.firedStateRuntimeControllerIds.push_back(controllerId);
    }
}

bool stateRuntimeControllerFrameKeyAlreadyFired(const FighterState& fighter, int fireKey) {
    return std::find(
        fighter.firedStateRuntimeControllerFrameKeys.begin(),
        fighter.firedStateRuntimeControllerFrameKeys.end(),
        fireKey) != fighter.firedStateRuntimeControllerFrameKeys.end();
}

void markStateRuntimeControllerFrameKeyFired(FighterState& fighter, int fireKey) {
    if (!stateRuntimeControllerFrameKeyAlreadyFired(fighter, fireKey)) {
        fighter.firedStateRuntimeControllerFrameKeys.push_back(fireKey);
    }
}

bool shouldRunStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (stateRuntimeControllerBlockedByHitPause(fighter, controllerId, trigger)) {
        return false;
    }
    if (!stateControllerTriggerActive(state, fighter, trigger, opponent, stage)) {
        return false;
    }
    return stateRuntimeControllerPersistenceAllowsRun(fighter, controllerId, trigger);
}

bool shouldRunSimpleStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    int triggerTime,
    int triggerAnimElem) {
    if (stateRuntimeControllerBlockedByHitPause(fighter, controllerId, trigger)) {
        return false;
    }
    if (!simpleControllerTriggerActive(state, fighter, triggerTime, triggerAnimElem)) {
        return false;
    }
    return stateRuntimeControllerPersistenceAllowsRun(fighter, controllerId, trigger);
}

bool shouldRunStateOneShotController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (!shouldRunStateRuntimeController(state, fighter, controllerId, trigger, opponent, stage)) {
        return false;
    }

    const int fireKey = stateControllerFireKey(controllerId, fighter, currentAnimElemForFighter(state, fighter));
    if (stateRuntimeControllerFrameKeyAlreadyFired(fighter, fireKey)) {
        return false;
    }
    markStateRuntimeControllerFrameKeyFired(fighter, fireKey);
    return true;
}

bool shouldRunStatePauseController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    return shouldRunStateOneShotController(state, fighter, controllerId, trigger, opponent, stage);
}

void executeStopSoundController(
    AppState& state,
    FighterState& fighter,
    const StateStopSoundController& stopSound,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    bool triggerActive = false;
    if (stopSound.trigger.hasTrigger) {
        if (!shouldRunStateRuntimeController(state, fighter, stopSound.id, stopSound.trigger, opponent, stage)) {
            return;
        }
        triggerActive = true;
    } else {
        if (!shouldRunSimpleStateRuntimeController(
            state,
            fighter,
            stopSound.id,
            stopSound.trigger,
            stopSound.triggerTime,
            stopSound.triggerAnimElem)) {
            return;
        }
        triggerActive = true;
    }

    if (triggerActive) {
        stopSoundChannel(state.audio, stopSound.channel);
    }
}

void executeStateAudioControllers(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    for (const auto& ref : stateDef.audioControllers) {
        if (ref.kind == StateAudioControllerKind::PlaySnd) {
            if (ref.index < stateDef.sounds.size()) {
                executePlaySoundController(state, fighter, stateDef.sounds[ref.index]);
            }
        } else if (ref.index < stateDef.stopSounds.size()) {
            executeStopSoundController(state, fighter, stateDef.stopSounds[ref.index], opponent, stage);
        }
    }
}

void updateStateAudioControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    const auto stateDefs = runtimeControllerStateDefinitions(state, fighter);
    for (auto stateDefIt = stateDefs.begin(); stateDefIt != stateDefs.end(); ++stateDefIt) {
        const StateDefinition* stateDef = *stateDefIt;
        if (!stateDef || std::find(stateDefs.begin(), stateDefIt, stateDef) != stateDefIt) {
            continue;
        }
        executeStateAudioControllers(state, fighter, *stateDef, opponent, stage);
    }
}

bool fighterHasAssertSpecialFlag(const FighterState& fighter, std::string_view flag) {
    const std::string normalized = normalizeAssertSpecialFlag(flag);
    return std::find(
        fighter.assertSpecialFlags.begin(),
        fighter.assertSpecialFlags.end(),
        normalized) != fighter.assertSpecialFlags.end();
}

bool anyFighterHasAssertSpecialFlag(const AppState& state, std::string_view flag) {
    return std::any_of(
        state.fighters.begin(),
        state.fighters.end(),
        [flag](const FighterState& fighter) {
            return fighterHasAssertSpecialFlag(fighter, flag);
        });
}

void addFighterAssertSpecialFlag(FighterState& fighter, const std::string& flag) {
    if (flag.empty()
        || std::find(fighter.assertSpecialFlags.begin(), fighter.assertSpecialFlags.end(), flag) != fighter.assertSpecialFlags.end()) {
        return;
    }
    fighter.assertSpecialFlags.push_back(flag);
}

void clearFightAssertSpecialFlags(AppState& state) {
    for (auto& fighter : state.fighters) {
        fighter.assertSpecialFlags.clear();
    }
}

void updateStateAssertSpecialControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& assertSpecial : stateDef.assertSpecials) {
            if (!shouldRunStateRuntimeController(state, fighter, assertSpecial.id, assertSpecial.trigger, opponent, stage)) {
                continue;
            }
            for (const auto& flag : assertSpecial.flags) {
                addFighterAssertSpecialFlag(fighter, flag);
            }
        }
        return true;
    });
}

void updateFightAssertSpecialControllers(AppState& state, const StageSlot& stage) {
    updateStateAssertSpecialControllers(state, state.fighters[0], &state.fighters[1], &stage);
    updateStateAssertSpecialControllers(state, state.fighters[1], &state.fighters[0], &stage);
}

bool changeAnimTriggerActive(const AppState& state, const FighterState& fighter, const StateChangeAnimController& controller) {
    if (controller.triggerTime >= 0 || controller.triggerAnimElem > 0) {
        if (!simpleControllerTriggerActive(state, fighter, controller.triggerTime, controller.triggerAnimElem)) {
            return false;
        }
    }
    if (controller.requiresMoveContact && !fighter.moveContact) {
        return false;
    }
    for (const auto& condition : controller.animElemTimeConditions) {
        if (!compareIntValue(animElemTimeForFighter(state, fighter, condition.elem), condition.op, condition.value)) {
            return false;
        }
    }
    return controller.requiresMoveContact
        || !controller.animElemTimeConditions.empty()
        || controller.triggerTime >= 0
        || controller.triggerAnimElem > 0;
}

ActorBlendMode transBlendMode(std::string_view trans) {
    const std::string value = lowercaseCopy(trim(trans));
    if (value.find("addalpha") != std::string::npos) {
        return ActorBlendMode::AddAlpha;
    }
    if (value.find("add1") != std::string::npos) {
        return ActorBlendMode::Add1;
    }
    if (value.find("add") != std::string::npos) {
        return ActorBlendMode::Add;
    }
    return ActorBlendMode::Normal;
}

std::string decodeClipboardEscapes(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            const char next = text[++i];
            if (next == 'n') {
                out.push_back('\n');
            } else if (next == 't') {
                out.push_back('\t');
            } else {
                out.push_back(next);
            }
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

std::string formatClipboardText(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot* stage,
    const StateClipboardController& clipboard) {
    std::array<float, 5> values{};
    const size_t count = std::min<size_t>(values.size(), clipboard.params.size());
    for (size_t i = 0; i < count; ++i) {
        if (const auto value = evalMugenExpression(state, fighter, clipboard.params[i], opponent, stage)) {
            values[i] = *value;
        }
    }

    const std::string text = decodeClipboardEscapes(clipboard.text);
    std::ostringstream out;
    size_t paramIndex = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '%' || i + 1 >= text.size()) {
            out << text[i];
            continue;
        }
        const char spec = text[++i];
        if (spec == '%') {
            out << '%';
            continue;
        }
        const float value = paramIndex < values.size() ? values[paramIndex++] : 0.0f;
        if (spec == 'd' || spec == 'i') {
            out << static_cast<int>(std::lround(value));
        } else if (spec == 'f') {
            out << std::fixed << std::setprecision(3) << value;
        } else {
            out << '%' << spec;
        }
    }
    return out.str();
}

std::string selectedVictoryQuoteText(const AppState& state, const FighterState& fighter) {
    const auto& quotes = victoryQuotesForActor(state, fighter);
    if (fighter.victoryQuote < 0 || fighter.victoryQuote >= static_cast<int>(quotes.size())) {
        return {};
    }
    return quotes[static_cast<size_t>(fighter.victoryQuote)];
}

std::optional<PaletteRemap> evaluatePaletteRemap(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot* stage,
    const PaletteRemap& remap) {
    auto evalInt = [&](const std::string& expression) -> std::optional<int> {
        const auto value = evalMugenExpression(state, fighter, expression, opponent, stage);
        return value ? std::optional<int>{ static_cast<int>(std::lround(*value)) } : std::nullopt;
    };
    const auto sourceGroup = evalInt(remap.sourceGroupExpression);
    const auto sourceItem = evalInt(remap.sourceItemExpression);
    const auto destGroup = evalInt(remap.destGroupExpression);
    const auto destItem = evalInt(remap.destItemExpression);
    if (!sourceGroup || !sourceItem || !destGroup || !destItem) {
        return std::nullopt;
    }
    PaletteRemap evaluated;
    evaluated.sourceGroupExpression = std::to_string(*sourceGroup);
    evaluated.sourceItemExpression = std::to_string(*sourceItem);
    evaluated.destGroupExpression = std::to_string(*destGroup);
    evaluated.destItemExpression = std::to_string(*destItem);
    return evaluated;
}

void updateStateVisualControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& angle : stateDef->angleControllers) {
        if (!shouldRunStateRuntimeController(state, fighter, angle.id, angle.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, angle.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        switch (angle.operation) {
        case StateAngleOperation::Set:
            fighter.drawAngle = *value;
            break;
        case StateAngleOperation::Add:
            fighter.drawAngle += *value;
            break;
        case StateAngleOperation::Mul:
            fighter.drawAngle *= *value;
            break;
        }
    }

    for (const auto& angleDraw : stateDef->angleDraws) {
        if (!shouldRunStateRuntimeController(state, fighter, angleDraw.id, angleDraw.trigger, opponent, stage)) {
            continue;
        }
        fighter.angleDrawActive = true;
    }

    for (const auto& offset : stateDef->offsets) {
        if (!shouldRunStateRuntimeController(state, fighter, offset.id, offset.trigger, opponent, stage)) {
            continue;
        }
        if (offset.hasX) {
            if (const auto value = evalMugenExpression(state, fighter, offset.xExpression, opponent, stage)) {
                fighter.displayOffsetX = *value;
            }
        }
        if (offset.hasY) {
            if (const auto value = evalMugenExpression(state, fighter, offset.yExpression, opponent, stage)) {
                fighter.displayOffsetY = *value;
            }
        }
    }

    for (const auto& clipboard : stateDef->clipboards) {
        if (!shouldRunStateRuntimeController(state, fighter, clipboard.id, clipboard.trigger, opponent, stage)) {
            continue;
        }
        const std::string text = formatClipboardText(state, fighter, opponent, stage, clipboard);
        if (clipboard.append) {
            fighter.debugClipboard += text;
        } else {
            fighter.debugClipboard = text;
        }
    }

    for (const auto& victoryQuote : stateDef->victoryQuotes) {
        if (fighter.helper || !shouldRunStateRuntimeController(state, fighter, victoryQuote.id, victoryQuote.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, victoryQuote.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        const int index = static_cast<int>(std::lround(*value));
        fighter.victoryQuote = (index >= 0 && index < 100) ? index : -1;
    }

    for (const auto& remapPal : stateDef->remapPals) {
        if (!shouldRunStateRuntimeController(state, fighter, remapPal.id, remapPal.trigger, opponent, stage)) {
            continue;
        }
        const auto evaluated = evaluatePaletteRemap(state, fighter, opponent, stage, remapPal.remap);
        if (!evaluated) {
            continue;
        }
        const bool removeMapping = evaluated->destGroupExpression == "-1"
            || (evaluated->sourceGroupExpression == evaluated->destGroupExpression
                && evaluated->sourceItemExpression == evaluated->destItemExpression);
        const auto sameSource = [&](const PaletteRemap& active) {
            return active.sourceGroupExpression == evaluated->sourceGroupExpression
                && active.sourceItemExpression == evaluated->sourceItemExpression;
        };
        fighter.paletteRemaps.erase(
            std::remove_if(fighter.paletteRemaps.begin(), fighter.paletteRemaps.end(), sameSource),
            fighter.paletteRemaps.end());
        if (!removeMapping) {
            fighter.paletteRemaps.push_back(*evaluated);
        }
    }

    for (const auto& trans : stateDef->transControllers) {
        if (!shouldRunStateRuntimeController(state, fighter, trans.id, trans.trigger, opponent, stage)) {
            continue;
        }
        const auto source = evalMugenExpression(state, fighter, trans.alphaSourceExpression, opponent, stage);
        const auto dest = evalMugenExpression(state, fighter, trans.alphaDestExpression, opponent, stage);
        fighter.transEffect.active = true;
        fighter.transEffect.mode = transBlendMode(trans.trans);
        fighter.transEffect.alphaSource = std::clamp(static_cast<int>(std::lround(source.value_or(256.0f))), 0, 256);
        fighter.transEffect.alphaDest = std::clamp(static_cast<int>(std::lround(dest.value_or(0.0f))), 0, 256);
    }

    for (const auto& afterImage : stateDef->afterImages) {
        if (!shouldRunStateRuntimeController(state, fighter, afterImage.id, afterImage.trigger, opponent, stage)) {
            continue;
        }
        fighter.afterImage.configured = true;
        fighter.afterImage.active = true;
        fighter.afterImage.ticksLeft = afterImage.time < 0 ? 9999 : std::max(0, afterImage.time);
        fighter.afterImage.length = std::max(1, afterImage.length);
        fighter.afterImage.timeGap = std::max(1, afterImage.timeGap);
        fighter.afterImage.frameGap = std::max(1, afterImage.frameGap);
        fighter.afterImage.captureCountdown = 0;
        fighter.afterImage.blendMode = transBlendMode(afterImage.trans);
        fighter.afterImage.palBright = afterImage.palBright;
        fighter.afterImage.palContrast = afterImage.palContrast;
        fighter.afterImage.palPostBright = afterImage.palPostBright;
        fighter.afterImage.palAdd = afterImage.palAdd;
        fighter.afterImage.palMul = afterImage.palMul;
        fighter.afterImage.trail.clear();
    }

    for (const auto& afterImageTime : stateDef->afterImageTimes) {
        if (!shouldRunStateRuntimeController(state, fighter, afterImageTime.id, afterImageTime.trigger, opponent, stage)) {
            continue;
        }
        if (!fighter.afterImage.configured || fighter.afterImage.timeGap <= 0) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, afterImageTime.timeExpression, opponent, stage);
        if (!value) {
            continue;
        }
        int ticks = static_cast<int>(std::lround(*value));
        if (ticks == 1) {
            ticks = 0;
        }
        fighter.afterImage.ticksLeft = ticks < 0 ? 9999 : std::max(0, ticks);
        fighter.afterImage.active = fighter.afterImage.ticksLeft > 0;
        if (!fighter.afterImage.active) {
            fighter.afterImage.trail.clear();
        }
    }
        return true;
    });
}

void runForceFeedbackController(AppState& state, const FighterState& fighter, const StateForceFeedbackController& forceFeedback) {
    const int fighterIndex = fighter.helper && fighter.ownerIndex >= 0
        ? fighter.ownerIndex
        : fighterIndexInState(state, fighter);
    const int targetPlayer = forceFeedback.self ? fighterIndex : (fighterIndex == 0 ? 1 : 0);
    const GamepadDevice* device = assignedGamepad(state, targetPlayer);
    if (!device || !device->handle) {
        return;
    }

    const std::string waveform = lowercaseCopy(forceFeedback.waveform);
    const Uint16 amplitude = static_cast<Uint16>(std::clamp(forceFeedback.amplitude, 0, 255) * 257);
    Uint16 lowFrequency = 0;
    Uint16 highFrequency = 0;
    if (waveform == "off") {
        SDL_RumbleGamepad(device->handle, 0, 0, 0);
        return;
    }
    if (waveform == "square") {
        highFrequency = amplitude;
    } else if (waveform == "sinesquare") {
        lowFrequency = amplitude;
        highFrequency = amplitude;
    } else {
        lowFrequency = amplitude;
    }

    const Uint32 durationMs = static_cast<Uint32>(std::max(0, forceFeedback.time) * 1000 / 60);
    SDL_RumbleGamepad(device->handle, lowFrequency, highFrequency, durationMs);
}

void updateAfterImageEffect(FighterState& fighter) {
    auto& afterImage = fighter.afterImage;
    if (!afterImage.active) {
        afterImage.trail.clear();
        return;
    }
    if (afterImage.ticksLeft <= 0) {
        afterImage.active = false;
        afterImage.trail.clear();
        return;
    }

    if (afterImage.captureCountdown <= 0) {
        afterImage.trail.push_back(AfterImageSnapshot{
            fighter.action,
            fighter.animTick,
            fighter.x,
            fighter.y,
            fighter.depthZ,
            fighter.facing,
            0,
        });
        afterImage.captureCountdown = std::max(0, afterImage.timeGap - 1);
        while (static_cast<int>(afterImage.trail.size()) > afterImage.length) {
            afterImage.trail.erase(afterImage.trail.begin());
        }
    } else {
        --afterImage.captureCountdown;
    }

    for (auto& snapshot : afterImage.trail) {
        ++snapshot.ageTicks;
    }
    --afterImage.ticksLeft;
    if (afterImage.ticksLeft <= 0) {
        afterImage.active = false;
    }
}
