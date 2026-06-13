#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local command/runtime helpers and types.
// Include only from App.cpp after command display helpers are defined.
// Do not include from other translation units.

bool simpleTrainingCommandToken(std::string_view command) {
    return command == "x"
        || command == "y"
        || command == "z"
        || command == "a"
        || command == "b"
        || command == "c"
        || command == "s";
}

bool holdTrainingCommandToken(std::string_view command) {
    return command == "holddown"
        || command == "holdup"
        || command == "holdfwd"
        || command == "holdback"
        || command == "hold_x"
        || command == "hold_y"
        || command == "hold_z"
        || command == "hold_a"
        || command == "hold_b"
        || command == "hold_c";
}

std::string commandAtomDisplayLabel(const CommandAtom& atom) {
    std::string label = moveListTokenForCommand(atom.symbol);
    if (atom.hold) {
        label = "HOLD " + label;
    }
    return fitDebugText(label, 10);
}

std::string commandStepDisplayLabel(const CommandStep& step) {
    std::string label;
    for (const auto& atom : step.atoms) {
        if (!label.empty()) {
            label += "+";
        }
        label += commandAtomDisplayLabel(atom);
    }
    return fitDebugText(label.empty() ? "-" : label, 12);
}

std::string commandDefinitionInputLabel(const CommandDefinition& definition) {
    if (definition.steps.empty()) {
        return "-";
    }

    std::string label;
    for (const auto& step : definition.steps) {
        if (!label.empty()) {
            label += " > ";
        }
        label += commandStepDisplayLabel(step);
    }
    return label;
}

const CommandDefinition* findCommandDefinitionByName(const AppState& state, std::string_view name) {
    const auto it = std::find_if(state.commandDefinitions.begin(), state.commandDefinitions.end(), [name](const CommandDefinition& definition) {
        return definition.name == name;
    });
    return it == state.commandDefinitions.end() ? nullptr : &*it;
}

void appendEntryCommandNames(const CommandStateEntry& entry, std::vector<std::string_view>& names) {
    for (const auto& command : entry.requiredCommands) {
        names.push_back(command);
    }
    for (const auto& group : entry.commandOptionGroups) {
        for (const auto& command : group) {
            names.push_back(command);
        }
    }
}

const CommandDefinition* practiceCommandDefinitionForEntry(
    const AppState& state,
    const CommandStateEntry& entry,
    const std::vector<std::string>& activeCommands) {
    std::vector<std::string_view> names;
    appendEntryCommandNames(entry, names);

    for (const auto name : names) {
        if (commandListContains(activeCommands, name)) {
            if (const CommandDefinition* definition = findCommandDefinitionByName(state, name)) {
                return definition;
            }
        }
    }
    for (const auto name : names) {
        if (!holdTrainingCommandToken(name) && !simpleTrainingCommandToken(name)) {
            if (const CommandDefinition* definition = findCommandDefinitionByName(state, name)) {
                return definition;
            }
        }
    }
    for (const auto name : names) {
        if (const CommandDefinition* definition = findCommandDefinitionByName(state, name)) {
            return definition;
        }
    }
    return nullptr;
}

int matchedPracticeStepCount(const FighterState& fighter, const CommandDefinition& definition) {
    if (fighter.inputHistory.empty() || definition.steps.empty()) {
        return 0;
    }

    const int newestTick = fighter.inputHistory.back().tick;
    int firstMatchedTick = -1;
    size_t searchFrom = 0;
    int matched = 0;
    for (const auto& step : definition.steps) {
        bool found = false;
        for (size_t frameIndex = searchFrom; frameIndex < fighter.inputHistory.size(); ++frameIndex) {
            const int tick = fighter.inputHistory[frameIndex].tick;
            if (newestTick - tick > definition.maxTime + definition.bufferTime + 12) {
                continue;
            }
            if (firstMatchedTick >= 0 && tick - firstMatchedTick > definition.maxTime) {
                return matched;
            }
            if (!commandStepMatches(fighter.inputHistory, frameIndex, step, fighter.facing)) {
                continue;
            }
            if (firstMatchedTick < 0) {
                firstMatchedTick = tick;
            }
            searchFrom = frameIndex + 1;
            ++matched;
            found = true;
            break;
        }
        if (!found) {
            break;
        }
    }
    return matched;
}

void appendDefinitionPracticeSteps(
    std::vector<TrainingCommandStepView>& steps,
    const FighterState& fighter,
    const CommandDefinition& definition,
    bool complete) {
    const int matched = complete
        ? static_cast<int>(definition.steps.size())
        : matchedPracticeStepCount(fighter, definition);
    for (int i = 0; i < static_cast<int>(definition.steps.size()); ++i) {
        TrainingCommandStepStatus status = TrainingCommandStepStatus::Pending;
        if (i < matched) {
            status = TrainingCommandStepStatus::Matched;
        } else if (i == matched) {
            status = TrainingCommandStepStatus::Current;
        }
        steps.push_back(TrainingCommandStepView{
            commandStepDisplayLabel(definition.steps[static_cast<size_t>(i)]),
            status,
        });
    }
}

void appendEntryPracticeSteps(
    std::vector<TrainingCommandStepView>& steps,
    const CommandStateEntry& entry,
    const std::vector<std::string>& activeCommands,
    bool complete) {
    struct PracticeCommandChip {
        std::string command;
        std::string label;
    };

    std::vector<PracticeCommandChip> chips;
    const auto appendIfRequired = [&chips, &entry](std::string_view command) {
        if (commandListContains(entry.requiredCommands, command)) {
            chips.push_back(PracticeCommandChip{
                std::string(command),
                moveListTokenForCommand(command),
            });
        }
    };

    appendIfRequired("holddown");
    appendIfRequired("holdfwd");
    appendIfRequired("holdback");
    appendIfRequired("holdup");
    appendIfRequired("x");
    appendIfRequired("y");
    appendIfRequired("z");
    appendIfRequired("a");
    appendIfRequired("b");
    appendIfRequired("c");

    for (const auto& group : entry.commandOptionGroups) {
        std::string label;
        bool groupActive = false;
        for (const auto& option : group) {
            if (!label.empty()) {
                label += "/";
            }
            label += moveListTokenForCommand(option);
            groupActive = groupActive || commandListContains(activeCommands, option);
        }
        if (!label.empty()) {
            steps.push_back(TrainingCommandStepView{
                fitDebugText(label, 12),
                complete || groupActive ? TrainingCommandStepStatus::Matched : TrainingCommandStepStatus::Current,
            });
        }
    }

    bool waitingMarked = false;
    for (const auto& chip : chips) {
        const bool matched = commandListContains(activeCommands, chip.command) || complete;
        TrainingCommandStepStatus status = TrainingCommandStepStatus::Pending;
        if (matched) {
            status = TrainingCommandStepStatus::Matched;
        } else if (!waitingMarked) {
            status = TrainingCommandStepStatus::Current;
            waitingMarked = true;
        }
        steps.push_back(TrainingCommandStepView{ fitDebugText(chip.label, 12), status });
    }

    if (steps.empty()) {
        steps.push_back(TrainingCommandStepView{ "-", TrainingCommandStepStatus::Current });
    }
}

void applyTrainingDemoDirection(FighterInputState& input, std::string_view symbol, int facing) {
    const auto holdForward = [&input, facing]() {
        if (facing >= 0) {
            input.right = true;
        } else {
            input.left = true;
        }
    };
    const auto holdBack = [&input, facing]() {
        if (facing >= 0) {
            input.left = true;
        } else {
            input.right = true;
        }
    };

    if (symbol == "F") {
        holdForward();
    } else if (symbol == "B") {
        holdBack();
    } else if (symbol == "D") {
        input.down = true;
    } else if (symbol == "U") {
        input.up = true;
    } else if (symbol == "DF") {
        input.down = true;
        holdForward();
    } else if (symbol == "DB") {
        input.down = true;
        holdBack();
    } else if (symbol == "UF") {
        input.up = true;
        holdForward();
    } else if (symbol == "UB") {
        input.up = true;
        holdBack();
    }
}

void applyTrainingDemoAtom(FighterInputState& input, const CommandAtom& atom, int facing) {
    if (atom.release) {
        CommandAtom held = atom;
        held.release = false;
        applyTrainingDemoAtom(input, held, facing);
        return;
    }
    if (atom.symbol == "x") {
        input.x = true;
    } else if (atom.symbol == "y") {
        input.y = true;
    } else if (atom.symbol == "z") {
        input.z = true;
    } else if (atom.symbol == "a") {
        input.a = true;
    } else if (atom.symbol == "b") {
        input.b = true;
    } else if (atom.symbol == "c") {
        input.c = true;
    } else if (atom.symbol == "s") {
        input.s = true;
    } else {
        applyTrainingDemoDirection(input, atom.symbol, facing);
    }
}

void applyTrainingDemoDirectionAtoms(FighterInputState& input, const CommandStep& step, int facing) {
    for (const auto& atom : step.atoms) {
        if (buttonAtomSymbol(atom.symbol)) {
            continue;
        }
        CommandAtom directionAtom = atom;
        directionAtom.release = false;
        applyTrainingDemoAtom(input, directionAtom, facing);
    }
}

FighterInputState trainingDemoInputForStep(const CommandStep& step, int facing) {
    FighterInputState input;
    for (const auto& atom : step.atoms) {
        applyTrainingDemoAtom(input, atom, facing);
    }
    return input;
}

FighterInputState trainingDemoInputForEntry(const CommandStateEntry& entry, int facing) {
    FighterInputState input;
    const auto applyRequired = [&input, &entry, facing](std::string_view command) {
        if (!commandListContains(entry.requiredCommands, command)) {
            return;
        }
        CommandAtom atom;
        atom.symbol = std::string(command);
        if (holdTrainingCommandToken(command)) {
            if (command == "holddown") {
                atom.symbol = "D";
            } else if (command == "holdup") {
                atom.symbol = "U";
            } else if (command == "holdfwd") {
                atom.symbol = "F";
            } else if (command == "holdback") {
                atom.symbol = "B";
            } else if (command == "hold_x") {
                atom.symbol = "x";
            } else if (command == "hold_y") {
                atom.symbol = "y";
            } else if (command == "hold_z") {
                atom.symbol = "z";
            } else if (command == "hold_a") {
                atom.symbol = "a";
            } else if (command == "hold_b") {
                atom.symbol = "b";
            } else if (command == "hold_c") {
                atom.symbol = "c";
            }
        }
        applyTrainingDemoAtom(input, atom, facing);
    };

    applyRequired("holddown");
    applyRequired("holdfwd");
    applyRequired("holdback");
    applyRequired("holdup");
    applyRequired("hold_x");
    applyRequired("hold_y");
    applyRequired("hold_z");
    applyRequired("hold_a");
    applyRequired("hold_b");
    applyRequired("hold_c");
    applyRequired("x");
    applyRequired("y");
    applyRequired("z");
    applyRequired("a");
    applyRequired("b");
    applyRequired("c");
    for (const auto& optionGroup : entry.commandOptionGroups) {
        for (const auto& option : optionGroup) {
            if (!holdTrainingCommandToken(option) && !simpleTrainingCommandToken(option)) {
                continue;
            }
            CommandAtom atom;
            atom.symbol = option;
            if (holdTrainingCommandToken(option)) {
                if (option == "holddown") {
                    atom.symbol = "D";
                } else if (option == "holdup") {
                    atom.symbol = "U";
                } else if (option == "holdfwd") {
                    atom.symbol = "F";
                } else if (option == "holdback") {
                    atom.symbol = "B";
                } else if (option == "hold_x") {
                    atom.symbol = "x";
                } else if (option == "hold_y") {
                    atom.symbol = "y";
                } else if (option == "hold_z") {
                    atom.symbol = "z";
                } else if (option == "hold_a") {
                    atom.symbol = "a";
                } else if (option == "hold_b") {
                    atom.symbol = "b";
                } else if (option == "hold_c") {
                    atom.symbol = "c";
                }
            }
            applyTrainingDemoAtom(input, atom, facing);
            break;
        }
    }
    return input;
}

void mergeTrainingDemoInput(FighterInputState& target, const FighterInputState& source) {
    target.left = target.left || source.left;
    target.right = target.right || source.right;
    target.up = target.up || source.up;
    target.down = target.down || source.down;
    target.x = target.x || source.x;
    target.y = target.y || source.y;
    target.z = target.z || source.z;
    target.a = target.a || source.a;
    target.b = target.b || source.b;
    target.c = target.c || source.c;
    target.s = target.s || source.s;
}

const CommandStateEntry* selectedTrainingCommandEntry(const AppState& state, int* selectedIndex = nullptr) {
    const auto entries = displayableMoveListEntries(state);
    if (entries.empty()) {
        if (selectedIndex) {
            *selectedIndex = -1;
        }
        return nullptr;
    }
    const int selected = std::clamp(
        state.training.options.selectedMoveListEntry,
        0,
        static_cast<int>(entries.size()) - 1);
    if (selectedIndex) {
        *selectedIndex = selected;
    }
    return entries[static_cast<size_t>(selected)];
}

bool cycleSelectedTrainingCommandEntry(AppState& state, int direction) {
    const auto entries = displayableMoveListEntries(state);
    if (entries.empty() || direction == 0) {
        return false;
    }

    const int count = static_cast<int>(entries.size());
    const int current = std::clamp(state.training.options.selectedMoveListEntry, 0, count - 1);
    const int selected = (current + direction + count) % count;
    state.training.options.selectedMoveListEntry = selected;

    constexpr int visibleRows = 7;
    const int maxScroll = std::max(0, count - visibleRows);
    if (selected < state.training.options.moveListScroll) {
        state.training.options.moveListScroll = selected;
    } else if (selected >= state.training.options.moveListScroll + visibleRows) {
        state.training.options.moveListScroll = selected - visibleRows + 1;
    }
    state.training.options.moveListScroll = std::clamp(state.training.options.moveListScroll, 0, maxScroll);

    state.messages.lastHitText = "Move: " + moveListEntryName(*entries[static_cast<size_t>(selected)]);
    state.messages.lastHitTextTicks = 90;
    return true;
}

bool trainingCommandDemoActive(const AppState& state) {
    return state.frontend.pendingMode == PendingMode::Training && state.training.commandDemo.active;
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
    int minState = -1000000;
    int maxState = 1000000;
    bool hasStateNoGate = false;
    for (const auto& condition : entry.intConditions) {
        if (condition.subject != CommandConditionSubject::StateNo) {
            continue;
        }
        hasStateNoGate = true;
        if (condition.op == CompareOp::Equal) {
            minState = std::max(minState, condition.value);
            maxState = std::min(maxState, condition.value);
        } else if (condition.op == CompareOp::GreaterEqual) {
            minState = std::max(minState, condition.value);
        } else if (condition.op == CompareOp::Greater) {
            minState = std::max(minState, condition.value + 1);
        } else if (condition.op == CompareOp::LessEqual) {
            maxState = std::min(maxState, condition.value);
        } else if (condition.op == CompareOp::Less) {
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
    fighter.life = 1000;
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
    } else if (trainingCommandEntryNeedsAirSetup(*entry)) {
        prepareTrainingCommandDemoAirSetup(state, *entry, p2);
    }
    p2.power = std::max(p2.power, commandEntryRequiredPower(*entry));
    state.messages.lastHitText = "Demo: " + moveListEntryName(*entry);
    state.messages.lastHitTextTicks = 90;
}

FighterInputState nextTrainingCommandDemoInput(AppState& state, FighterState& demoFighter) {
    FighterInputState input;
    auto& demo = state.training.commandDemo;
    if (!demo.active) {
        return input;
    }

    ++demo.elapsedTicks;
    int selected = -1;
    const CommandStateEntry* entry = selectedTrainingCommandEntry(state, &selected);
    if (!entry || selected != demo.selectedMoveListEntry) {
        stopTrainingCommandDemo(state);
        return input;
    }
    applyTrainingCommandDemoVariablePrereqs(demoFighter, *entry);

    const CommandDefinition* definition = practiceCommandDefinitionForEntry(state, *entry, {});
    const int stepCount = definition ? static_cast<int>(definition->steps.size()) : 1;
    constexpr int kTrainingCommandDemoMaxTicks = 900;
    if (stepCount <= 0 || demo.elapsedTicks > kTrainingCommandDemoMaxTicks) {
        stopTrainingCommandDemo(state);
        return input;
    }

    if (demo.stepIndex >= stepCount) {
        const bool recovered =
            demoFighter.ctrl
            && demoFighter.onGround
            && demoFighter.moveType == 'I'
            && (demoFighter.stateNo == 0 || demoFighter.stateNo == 20);
        if (recovered && demo.elapsedTicks > 24) {
            stopTrainingCommandDemo(state);
        }
        return input;
    }

    if (demo.neutralTicks > 0) {
        --demo.neutralTicks;
        return input;
    }

    if (definition) {
        const auto& step = definition->steps[static_cast<size_t>(demo.stepIndex)];
        input = trainingDemoInputForStep(step, demoFighter.facing);
        if (commandStepHasButtonAtom(step)
            && !commandStepHasDirectionAtom(step)
            && demo.stepIndex > 0) {
            applyTrainingDemoDirectionAtoms(
                input,
                definition->steps[static_cast<size_t>(demo.stepIndex - 1)],
                demoFighter.facing);
        }
        mergeTrainingDemoInput(input, trainingDemoInputForEntry(*entry, demoFighter.facing));
    } else {
        input = trainingDemoInputForEntry(*entry, demoFighter.facing);
    }

    ++demo.stepTicks;
    const bool finalStep = demo.stepIndex == stepCount - 1;
    const int activeTicks = finalStep ? 3 : 2;
    if (demo.stepTicks >= activeTicks) {
        ++demo.stepIndex;
        demo.stepTicks = 0;
        demo.neutralTicks = finalStep ? 0 : 1;
    }
    return input;
}
