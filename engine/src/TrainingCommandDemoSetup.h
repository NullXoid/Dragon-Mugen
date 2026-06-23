#pragma once

// Internal App.cpp implementation header.
// Prepares fighter state and scene prerequisites for command demos.
// Include only through TrainingCommandPracticeAssembly.h.
bool trainingCommandEntryNeedsThrowRangeSetup(const CommandStateEntry& entry) {
    const std::string loweredName = lowercaseCopy(moveListEntryName(entry));
    return loweredName.find("throw") != std::string::npos
        || loweredName.find("grab") != std::string::npos
        || loweredName.find("nage") != std::string::npos;
}

std::optional<float> trainingCommandEntryPreferredP2BodyDistX(const CommandStateEntry& entry) {
    float minDistance = 0.0f;
    float maxDistance = 100000.0f;
    bool hasCondition = false;

    for (const auto& condition : entry.expressionConditions) {
        if (!equalsNoCase(trim(condition.lhs), "p2bodydist x")) {
            continue;
        }
        const auto rhs = parsePlainFloatValue(condition.rhs);
        if (!rhs) {
            continue;
        }
        hasCondition = true;
        switch (condition.op) {
        case CompareOp::Equal:
            minDistance = std::max(minDistance, *rhs);
            maxDistance = std::min(maxDistance, *rhs);
            break;
        case CompareOp::Greater:
            minDistance = std::max(minDistance, *rhs + 1.0f);
            break;
        case CompareOp::GreaterEqual:
            minDistance = std::max(minDistance, *rhs);
            break;
        case CompareOp::Less:
            maxDistance = std::min(maxDistance, *rhs - 1.0f);
            break;
        case CompareOp::LessEqual:
            maxDistance = std::min(maxDistance, *rhs);
            break;
        case CompareOp::NotEqual:
            break;
        }
    }

    if (!hasCondition) {
        return std::nullopt;
    }
    minDistance = std::max(0.0f, minDistance);
    maxDistance = std::max(0.0f, maxDistance);
    if (minDistance > maxDistance) {
        return minDistance;
    }
    if (maxDistance < 100000.0f) {
        return (minDistance + maxDistance) * 0.5f;
    }
    return minDistance + 12.0f;
}

float trainingDemoDistanceFromP2BodyDist(AppState& state, float bodyDistance) {
    if (state.fighters.size() < 2) {
        return std::max(34.0f, bodyDistance);
    }

    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    p1.facing = 1;
    p2.facing = -1;
    const float pushWidth =
        fighterPlayerWidthToward(state, p1, 1.0f)
        + fighterPlayerWidthToward(state, p2, -1.0f);
    return std::max(4.0f, pushWidth + bodyDistance);
}

float trainingDemoFighterDistance(AppState& state, const CommandStateEntry& entry) {
    if (state.fighters.size() < 2) {
        return 72.0f;
    }

    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    p1.facing = 1;
    p2.facing = -1;

    const float pushWidth =
        fighterPlayerWidthToward(state, p1, 1.0f)
        + fighterPlayerWidthToward(state, p2, -1.0f);
    if (trainingCommandEntryNeedsThrowRangeSetup(entry)) {
        return 8.0f;
    }
    if (const auto preferredBodyDist = trainingCommandEntryPreferredP2BodyDistX(entry)) {
        return trainingDemoDistanceFromP2BodyDist(state, *preferredBodyDist);
    }
    const float extraRange = commandEntryCategory(entry) == TrainingMoveCategory::Normals ? 6.0f : 36.0f;
    return std::max(34.0f, pushWidth + extraRange);
}

bool trainingCommandEntryUsesCommand(const CommandStateEntry& entry, std::string_view command) {
    if (commandListContains(entry.requiredCommands, command)) {
        return true;
    }
    for (const auto& group : entry.commandOptionGroups) {
        if (commandListContains(group, command)) {
            return true;
        }
    }
    return false;
}

bool textContainsIntLiteral(std::string_view text, int value) {
    const std::string needle = std::to_string(value);
    size_t pos = text.find(needle);
    while (pos != std::string_view::npos) {
        const bool leftOk = pos == 0 || !std::isdigit(static_cast<unsigned char>(text[pos - 1]));
        const size_t right = pos + needle.size();
        const bool rightOk = right >= text.size() || !std::isdigit(static_cast<unsigned char>(text[right]));
        if (leftOk && rightOk) {
            return true;
        }
        pos = text.find(needle, pos + 1);
    }
    return false;
}

bool commandEntryTargetsState(const CommandStateEntry& entry, int stateNo) {
    return entry.targetState == stateNo || textContainsIntLiteral(entry.targetStateExpression, stateNo);
}

bool commandEntryTextContains(const CommandStateEntry& entry, std::string_view needle) {
    const std::string loweredNeedle = lowercaseCopy(needle);
    const auto containsNeedle = [&loweredNeedle](std::string_view text) {
        return lowercaseCopy(text).find(loweredNeedle) != std::string::npos;
    };
    for (const auto& expression : entry.booleanExpressions) {
        if (containsNeedle(expression)) {
            return true;
        }
    }
    for (const auto& condition : entry.expressionConditions) {
        if (containsNeedle(condition.lhs) || containsNeedle(condition.rhs)) {
            return true;
        }
    }
    return containsNeedle(entry.targetStateExpression);
}

bool trainingCommandEntryNeedsFallRecoverySetup(const CommandStateEntry& entry) {
    const bool usesRecoveryRollCommand =
        trainingCommandEntryUsesCommand(entry, "BQCD_x")
        || trainingCommandEntryUsesCommand(entry, "BQCD_y")
        || trainingCommandEntryUsesCommand(entry, "BQCD_z");
    const bool targetsRecoveryRollState =
        commandEntryTargetsState(entry, 2004)
        || commandEntryTargetsState(entry, 2005)
        || commandEntryTargetsState(entry, 2006);
    const bool requiresFallState =
        commandEntryTextContains(entry, "5050")
        && commandEntryTextContains(entry, "5071");
    const bool requiresFallRecovery = commandEntryTextContains(entry, "gethitvar(fall.recover)");
    return usesRecoveryRollCommand && targetsRecoveryRollState && requiresFallState && requiresFallRecovery;
}

bool trainingCommandEntryNeedsAirSetup(const CommandStateEntry& entry) {
    return entry.requiredStateType == 'A';
}

bool trainingCommandEntryNeedsHorizontalAirVelocity(const CommandStateEntry& entry) {
    for (const auto& condition : entry.expressionConditions) {
        if (equalsNoCase(trim(condition.lhs), "vel x")
            && condition.op == CompareOp::NotEqual
            && trim(condition.rhs) == "0") {
            return true;
        }
    }
    for (const auto& expression : entry.booleanExpressions) {
        const std::string lowered = lowercaseCopy(expression);
        if (lowered.find("vel x") != std::string::npos && lowered.find("!= 0") != std::string::npos) {
            return true;
        }
    }
    return false;
}

int trainingCommandEntryGuardCancelState(const CommandStateEntry& entry) {
    if (trainingCommandEntryUsesCommand(entry, "GC") && commandEntryRequiredPower(entry) >= 500) {
        return 151;
    }

    int minState = -1000000;
    int maxState = 1000000;
    bool hasStateNoGate = false;
    for (const auto& condition : entry.intConditions) {
        if (condition.subject != CommandConditionSubject::StateNo) {
            continue;
        }
        if (condition.op == CompareOp::Equal) {
            hasStateNoGate = true;
            minState = std::max(minState, condition.value);
            maxState = std::min(maxState, condition.value);
        } else if (condition.op == CompareOp::GreaterEqual) {
            hasStateNoGate = true;
            minState = std::max(minState, condition.value);
        } else if (condition.op == CompareOp::Greater) {
            hasStateNoGate = true;
            minState = std::max(minState, condition.value + 1);
        } else if (condition.op == CompareOp::LessEqual) {
            hasStateNoGate = true;
            maxState = std::min(maxState, condition.value);
        } else if (condition.op == CompareOp::Less) {
            hasStateNoGate = true;
            maxState = std::min(maxState, condition.value - 1);
        }
    }
    if (!hasStateNoGate || minState > maxState) {
        return -1;
    }
    if (minState <= 151 && maxState >= 151) {
        return 151;
    }
    if (minState <= 155 && maxState >= 155) {
        return 155;
    }
    const auto expressionMentions = [](std::string_view expression, std::string_view needle) {
        return lowercaseCopy(expression).find(needle) != std::string::npos;
    };
    const auto expressionMentionsGuardState = [&](std::string_view expression, std::string_view stateText) {
        return expressionMentions(expression, "stateno") && expressionMentions(expression, stateText);
    };
    for (const auto& expression : entry.booleanExpressions) {
        if (expressionMentionsGuardState(expression, "155")) {
            return 155;
        }
        if (expressionMentionsGuardState(expression, "150") || expressionMentionsGuardState(expression, "151")) {
            return 151;
        }
    }
    return -1;
}

void prepareTrainingCommandDemoGuardCancelSetup(AppState& state, FighterState& fighter, int stateNo) {
    clearFighterHitRuntime(fighter);
    if (!enterState(state, fighter, stateNo)) {
        fighter.prevStateNo = fighter.stateNo;
        fighter.stateNo = stateNo;
        fighter.stateTime = 0;
    }
    fighter.moveType = 'H';
    fighter.ctrl = false;
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 90;
    fighter.getHitHitTime = 90;
    fighter.getHitCtrlTime = 90;
    fighter.getHitSlideTime = 20;
    if (stateNo == 155) {
        fighter.stateType = 'A';
        fighter.physics = 'N';
        fighter.onGround = false;
        fighter.y = -64.0f;
        fighter.vy = 0.0f;
        fighter.vx = 0.0f;
    } else {
        fighter.stateType = 'S';
        fighter.physics = 'S';
        fighter.onGround = true;
        fighter.y = 0.0f;
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
    }
}

std::optional<float> trainingCommandVariableConditionValue(
    const FighterState& fighter,
    const MugenVariableRef& ref,
    CompareOp op,
    float rhs) {
    const float current = fighterVariableValue(fighter, ref);
    if (op == CompareOp::Equal) {
        return rhs;
    }
    if (op == CompareOp::GreaterEqual) {
        return std::max(current, rhs);
    }
    if (op == CompareOp::Greater) {
        return std::max(current, rhs + 1.0f);
    }
    if (op == CompareOp::LessEqual) {
        return std::min(current, rhs);
    }
    if (op == CompareOp::Less) {
        return std::min(current, rhs - 1.0f);
    }
    if (op == CompareOp::NotEqual && current == rhs) {
        return rhs + 1.0f;
    }
    return std::nullopt;
}

void applyTrainingCommandDemoVariablePrereqs(FighterState& fighter, const CommandStateEntry& entry) {
    for (const auto& condition : entry.expressionConditions) {
        const auto ref = parseMugenVariableRef(condition.lhs);
        const auto rhs = parsePlainFloatValue(condition.rhs);
        if (!ref || !rhs) {
            continue;
        }
        if (const auto value = trainingCommandVariableConditionValue(fighter, *ref, condition.op, *rhs)) {
            setFighterVariableValue(fighter, *ref, *value);
        }
    }
}

void prepareTrainingCommandDemoAirSetup(const AppState& state, const CommandStateEntry& entry, FighterState& fighter) {
    clearFighterHitRuntime(fighter);
    if (!enterState(state, fighter, 50)) {
        fighter.prevStateNo = fighter.stateNo;
        fighter.stateNo = 50;
        fighter.stateTime = 0;
    }
    fighter.ctrl = true;
    fighter.stateType = 'A';
    fighter.moveType = 'I';
    fighter.physics = 'A';
    fighter.onGround = false;
    fighter.y = -120.0f;
    fighter.vy = -2.5f;
    fighter.vx = trainingCommandEntryNeedsHorizontalAirVelocity(entry) ? 2.0f * static_cast<float>(fighter.facing) : 0.0f;
    fighter.jumpBaseAction = fighter.vx == 0.0f ? 41 : (fighter.vx * static_cast<float>(fighter.facing) > 0.0f ? 42 : 43);
    fighter.jumpPeakActionApplied = true;
    setFighterAction(fighter, firstExistingActionForActor(state, fighter, { fighter.jumpBaseAction + 3, fighter.jumpBaseAction, 44, 41, 50, 0 }));
}

void prepareTrainingCommandDemoCrouchSetup(const AppState& state, FighterState& fighter) {
    clearFighterHitRuntime(fighter);
    enterCrouchState(state, fighter, 11);
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
}

bool commandEntryAllowsDemoStateType(const CommandStateEntry& entry, char stateType) {
    if (entry.requiredStateType != 0 && stateType != entry.requiredStateType) {
        return false;
    }
    return std::find(entry.forbiddenStateTypes.begin(), entry.forbiddenStateTypes.end(), stateType)
        == entry.forbiddenStateTypes.end();
}

std::optional<int> commandEntryDemoStateNoPrereq(const AppState& state, const FighterState& fighter, const CommandStateEntry& entry) {
    int minState = -1000000;
    int maxState = 1000000;
    bool hasStateGate = false;
    for (const auto& condition : entry.intConditions) {
        if (condition.subject != CommandConditionSubject::StateNo) {
            continue;
        }
        if (condition.op == CompareOp::Equal) {
            hasStateGate = true;
            minState = std::max(minState, condition.value);
            maxState = std::min(maxState, condition.value);
        } else if (condition.op == CompareOp::GreaterEqual) {
            hasStateGate = true;
            minState = std::max(minState, condition.value);
        } else if (condition.op == CompareOp::Greater) {
            hasStateGate = true;
            minState = std::max(minState, condition.value + 1);
        } else if (condition.op == CompareOp::LessEqual) {
            hasStateGate = true;
            maxState = std::min(maxState, condition.value);
        } else if (condition.op == CompareOp::Less) {
            hasStateGate = true;
            maxState = std::min(maxState, condition.value - 1);
        }
    }
    for (const auto& condition : entry.intRangeConditions) {
        if (condition.subject != CommandConditionSubject::StateNo || condition.op != CompareOp::Equal) {
            continue;
        }
        hasStateGate = true;
        minState = std::max(minState, condition.minValue);
        maxState = std::min(maxState, condition.maxValue);
    }
    if (!hasStateGate || minState > maxState) {
        return std::nullopt;
    }

    const auto& states = stateDefinitionsForActor(state, fighter);
    const auto stateFits = [&entry, minState, maxState](const StateDefinition& stateDef) {
        return stateDef.stateNo >= minState
            && stateDef.stateNo <= maxState
            && commandEntryAllowsDemoStateType(entry, stateDef.stateType);
    };
    for (const auto& stateDef : states) {
        if (stateFits(stateDef)) {
            return stateDef.stateNo;
        }
    }
    for (const auto& stateDef : states) {
        if (stateDef.stateNo >= minState && stateDef.stateNo <= maxState) {
            return stateDef.stateNo;
        }
    }
    return std::nullopt;
}

void prepareTrainingCommandDemoStateNoSetup(AppState& state, FighterState& fighter, int stateNo) {
    clearFighterHitRuntime(fighter);
    if (!enterState(state, fighter, stateNo)) {
        return;
    }
    fighter.ctrl = true;
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 0;
    fighter.hitSlideTicks = 0;
    fighter.moveContact = true;
    fighter.moveHit = true;
    fighter.moveGuarded = false;
    if (fighter.stateType == 'A') {
        fighter.onGround = false;
        fighter.y = -72.0f;
        fighter.vy = 0.0f;
    } else {
        fighter.onGround = true;
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
    }
}

void prepareTrainingCommandDemoFallRecovery(AppState& state, FighterState& fighter) {
    clearFighterHitRuntime(fighter);
    const int fallState = canEnterStateForActor(state, fighter, 5050)
        ? 5050
        : (canEnterStateForActor(state, fighter, 5071) ? 5071 : 5050);
    if (!enterState(state, fighter, fallState)) {
        fighter.prevStateNo = fighter.stateNo;
        fighter.stateNo = fallState;
        fighter.stateTime = 0;
        fighter.stateType = 'A';
        fighter.moveType = 'H';
        fighter.physics = 'N';
        fighter.ctrl = false;
        fighter.onGround = false;
        setFighterAction(fighter, firstExistingActionForActor(state, fighter, { 5050, 5035, 0 }));
    }

    fighter.ctrl = false;
    fighter.stateType = 'A';
    fighter.moveType = 'H';
    fighter.physics = 'N';
    fighter.onGround = false;
    fighter.hitFall = true;
    fighter.hitFallRecover = true;
    fighter.hitFallRecoverTime = 60;
    fighter.hitFallYAccel = std::clamp(characterConstantsForActor(state, fighter).movementYAccel, 0.08f, 0.18f);
    const int fallAction = firstExistingActionForActor(state, fighter, { 5050, 5035, 0 });
    fighter.hitFallAirAction = fallAction;
    setFighterAction(fighter, fallAction);
    fighter.y = -46.0f;
    fighter.vy = 0.05f;
    fighter.vx = 0.0f;
}

void resetTrainingDemoFighter(AppState& state, FighterState& fighter) {
    fighter.inputHistory.clear();
    clearFighterHitRuntime(fighter);
    clearFighterVariables(fighter);
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.y = 0.0f;
    fighter.onGround = true;
    fighter.life = characterMaxLifeForActor(state, fighter);
    fighter.hitCount = 0;
    fighter.defenceMultiplier = 1.0f;
    fighter.attackMultiplier = 1.0f;
    fighter.attackDistanceOverride = -1;
    fighter.drawAngle = 0.0f;
    fighter.angleDrawActive = false;
    fighter.displayOffsetX = 0.0f;
    fighter.displayOffsetY = 0.0f;
    fighter.paletteEffect = {};
    fighter.transEffect = {};
    fighter.afterImage = {};
    fighter.paletteRemaps.clear();
    enterState(state, fighter, 0);
}

void resetTrainingCommandDemoScene(AppState& state, const CommandStateEntry& entry) {
    if (state.fighters.size() < 2) {
        return;
    }

    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;

    state.runtimeEffects.clear();
    state.helpers.clear();
    state.projectiles.clear();
    clearGlobalPause(state);
    clearEnvShake(state);
    clearPaletteRuntime(state);
    clearComboCounters(state);

    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    resetTrainingDemoFighter(state, p1);
    resetTrainingDemoFighter(state, p2);

    const float distance = trainingDemoFighterDistance(state, entry);
    const float halfDistance = distance * 0.5f;
    const float centerMin = stage.leftbound + halfDistance;
    const float centerMax = stage.rightbound - halfDistance;
    const float center = centerMin <= centerMax
        ? std::clamp(stage.cameraStartx, centerMin, centerMax)
        : stage.cameraStartx;

    p1.x = clampFighterOriginToStage(center - halfDistance, stage);
    p2.x = clampFighterOriginToStage(center + halfDistance, stage);
    p1.y = 0.0f;
    p2.y = 0.0f;
    p1.facing = 1;
    p2.facing = -1;
    state.cameraX = std::clamp(center, stage.cameraBoundleft, stage.cameraBoundright);
    state.cameraY = stage.cameraStarty;
}

void stopTrainingCommandDemo(AppState& state) {
    state.training.commandDemo.active = false;
    state.training.commandDemo.stepIndex = 0;
    state.training.commandDemo.stepTicks = 0;
    state.training.commandDemo.neutralTicks = 0;
    state.training.commandDemo.elapsedTicks = 0;
}

void beginTrainingCommandDemo(AppState& state) {
    int selected = -1;
    const CommandStateEntry* entry = selectedTrainingCommandEntry(state, &selected);
    if (!entry || state.fighters.size() < 2) {
        return;
    }

    auto& demo = state.training.commandDemo;
    demo.active = true;
    demo.selectedMoveListEntry = selected;
    demo.stepIndex = 0;
    demo.stepTicks = 0;
    demo.neutralTicks = 8;
    demo.elapsedTicks = 0;
    demo.flashTicks = 0;

    resetTrainingCommandDemoScene(state, *entry);

    auto& p2 = state.fighters[1];
    p2.ctrl = true;
    p2.vx = 0.0f;
    p2.vy = 0.0f;
    p2.y = 0.0f;
    p2.onGround = true;
    applyTrainingCommandDemoVariablePrereqs(p2, *entry);
    if (trainingCommandEntryNeedsFallRecoverySetup(*entry)) {
        prepareTrainingCommandDemoFallRecovery(state, p2);
    } else if (const int guardCancelState = trainingCommandEntryGuardCancelState(*entry); guardCancelState >= 0) {
        prepareTrainingCommandDemoGuardCancelSetup(state, p2, guardCancelState);
    } else if (const auto stateNo = commandEntryDemoStateNoPrereq(state, p2, *entry)) {
        prepareTrainingCommandDemoStateNoSetup(state, p2, *stateNo);
    } else if (trainingCommandEntryNeedsAirSetup(*entry)) {
        prepareTrainingCommandDemoAirSetup(state, *entry, p2);
    } else if (entry->requiredStateType == 'C') {
        prepareTrainingCommandDemoCrouchSetup(state, p2);
    }
    p2.power = std::max(p2.power, commandEntryRequiredPower(*entry));
    state.messages.lastHitText = "Demo: " + moveListEntryName(*entry);
    state.messages.lastHitTextTicks = 90;
}

