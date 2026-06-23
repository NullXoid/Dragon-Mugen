#pragma once

// Internal App.cpp implementation header.
// Tracks selected practice entries and completion progress.
// Include only through TrainingCommandPracticeAssembly.h.
const CommandStateEntry* selectedTrainingCommandEntry(const AppState& state, int* selectedIndex = nullptr) {
    const auto& entries = activeDisplayableMoveListEntries(state);
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

std::optional<float> trainingCommandDirectVariableGateValue(
    const FighterState& fighter,
    std::string_view expression,
    bool& usesVariable) {
    if (const auto ref = parseMugenVariableRef(expression)) {
        usesVariable = true;
        return fighterVariableValue(fighter, *ref);
    }
    return parsePlainFloatValue(std::string(expression));
}

std::optional<bool> trainingCommandDirectVariableGateSatisfied(
    const FighterState& fighter,
    const MugenExpressionCondition& condition) {
    bool lhsUsesVariable = false;
    bool rhsUsesVariable = false;
    const auto lhs = trainingCommandDirectVariableGateValue(fighter, trim(condition.lhs), lhsUsesVariable);
    const auto rhs = trainingCommandDirectVariableGateValue(fighter, trim(condition.rhs), rhsUsesVariable);
    if (!lhs || !rhs || (!lhsUsesVariable && !rhsUsesVariable)) {
        return std::nullopt;
    }
    return compareFloatValue(*lhs, condition.op, *rhs);
}

bool trainingCommandBooleanVariableGatesSatisfied(const FighterState& fighter, const std::string& expression) {
    bool sawVariableBranch = false;
    for (const auto& orClause : splitTopLevelClauses(expression, "||", true)) {
        bool branchUsesVariable = false;
        bool branchVariablesSatisfied = true;
        for (const auto& andClause : splitTopLevelClauses(orClause, "&&")) {
            const auto condition = parseMugenExpressionCondition(stripOuterParens(andClause));
            if (!condition) {
                continue;
            }
            const auto satisfied = trainingCommandDirectVariableGateSatisfied(fighter, *condition);
            if (!satisfied) {
                continue;
            }
            branchUsesVariable = true;
            branchVariablesSatisfied = branchVariablesSatisfied && *satisfied;
        }
        if (!branchUsesVariable) {
            return true;
        }
        sawVariableBranch = true;
        if (branchVariablesSatisfied) {
            return true;
        }
    }
    return !sawVariableBranch;
}

bool trainingCommandEntryVariableGatesSatisfied(const FighterState& fighter, const CommandStateEntry& entry) {
    for (const auto& condition : entry.expressionConditions) {
        const auto satisfied = trainingCommandDirectVariableGateSatisfied(fighter, condition);
        if (satisfied && !*satisfied) {
            return false;
        }
    }
    for (const auto& expression : entry.booleanExpressions) {
        if (!trainingCommandBooleanVariableGatesSatisfied(fighter, expression)) {
            return false;
        }
    }
    return true;
}

bool trainingCommandEntryAutoAdvanceCandidate(const AppState& state, int fighterIndex, const CommandStateEntry& entry) {
    if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state.fighters.size())) {
        return true;
    }
    return trainingCommandEntryVariableGatesSatisfied(state.fighters[static_cast<size_t>(fighterIndex)], entry);
}

bool cycleSelectedTrainingCommandEntry(
    AppState& state,
    int direction,
    bool announce = true,
    bool skipUnavailablePracticeEntries = false,
    int fighterIndex = 0) {
    const auto& entries = activeDisplayableMoveListEntries(state);
    if (entries.empty() || direction == 0) {
        return false;
    }

    const int count = static_cast<int>(entries.size());
    const int current = std::clamp(state.training.options.selectedMoveListEntry, 0, count - 1);
    int selected = (current + direction + count) % count;
    if (skipUnavailablePracticeEntries) {
        for (int step = 1; step <= count; ++step) {
            const int candidate = (current + direction * step + count * step) % count;
            const auto* entry = entries[static_cast<size_t>(candidate)];
            if (entry && trainingCommandEntryAutoAdvanceCandidate(state, fighterIndex, *entry)) {
                selected = candidate;
                break;
            }
        }
    }
    state.training.options.selectedMoveListEntry = selected;

    const int visibleRows = trainingMoveListVisibleMoveCapacity();
    const int maxScroll = std::max(0, count - visibleRows);
    if (selected < state.training.options.moveListScroll) {
        state.training.options.moveListScroll = selected;
    } else if (selected >= state.training.options.moveListScroll + visibleRows) {
        state.training.options.moveListScroll = selected - visibleRows + 1;
    }
    state.training.options.moveListScroll = std::clamp(state.training.options.moveListScroll, 0, maxScroll);

    if (announce) {
        state.messages.lastHitText = "Move: " + moveListEntryName(*entries[static_cast<size_t>(selected)]);
        state.messages.lastHitTextTicks = 90;
    }
    return true;
}

bool trainingCommandDemoActive(const AppState& state) {
    return state.frontend.pendingMode == PendingMode::Training && state.training.commandDemo.active;
}

void updateTrainingCommandPracticeTimers(AppState& state) {
    auto& practice = state.training.commandPractice;
    if (practice.flashTicks > 0) {
        --practice.flashTicks;
        if (practice.flashTicks == 0) {
            practice.notification.clear();
        }
    }
    if (practice.cooldownTicks > 0) {
        --practice.cooldownTicks;
    }
}

bool trainingCommandPracticeActive(const AppState& state) {
    return state.frontend.pendingMode == PendingMode::Training
        && state.training.options.showCommandHud
        && !trainingCommandDemoActive(state);
}

void completeTrainingCommandPracticeMove(AppState& state, const CommandStateEntry& entry) {
    auto& practice = state.training.commandPractice;
    const std::string completedName = moveListEntryName(entry);
    practice.flashTicks = 72;
    practice.cooldownTicks = 24;
    playMenuCursorDoneSound(state);

    cycleSelectedTrainingCommandEntry(state, 1, false, true, 0);

    int nextSelected = -1;
    const CommandStateEntry* nextEntry = selectedTrainingCommandEntry(state, &nextSelected);
    const std::string nextName = nextEntry ? moveListEntryName(*nextEntry) : "-";
    practice.notification = "OK  NEXT " + fitDebugText(nextName, 12);

    state.messages.lastHitText = "OK: " + completedName;
    state.messages.lastHitTextTicks = 90;
}

void updateTrainingCommandPracticeProgress(
    AppState& state,
    const FighterState& fighterBeforeUpdate,
    const FighterState& fighterAfterUpdate,
    const FighterState* opponent) {
    if (!trainingCommandPracticeActive(state) || state.training.commandPractice.cooldownTicks > 0) {
        return;
    }
    if (fighterAfterUpdate.inputHistory.empty()) {
        return;
    }

    const CommandStateEntry* selectedEntry = selectedTrainingCommandEntry(state, nullptr);
    if (!selectedEntry) {
        return;
    }

    const auto& commandDefinitions = commandDefinitionsForActor(state, fighterBeforeUpdate);
    const std::vector<std::string> commands =
        collectFighterCommands(fighterAfterUpdate.inputHistory.back().input, fighterAfterUpdate, commandDefinitions);

    const CommandStateEntry* completedEntry = selectedEntry;
    auto targetState = resolveCommandTargetState(state, fighterBeforeUpdate, opponent, *selectedEntry, commands);
    if (!canEnterCommandState(state, fighterBeforeUpdate, opponent, *selectedEntry, commands)) {
        const CommandStateEntry* activeEntry = activeCommandEntry(state, fighterBeforeUpdate, opponent, commands);
        if (!commandEntryMatchesActiveTrainingMove(*selectedEntry, activeEntry)) {
            return;
        }
        completedEntry = activeEntry;
        targetState = resolveCommandTargetState(state, fighterBeforeUpdate, opponent, *completedEntry, commands);
    }
    if (!targetState || fighterAfterUpdate.stateNo != *targetState) {
        return;
    }
    if (fighterBeforeUpdate.stateNo == fighterAfterUpdate.stateNo) {
        return;
    }

    completeTrainingCommandPracticeMove(state, *selectedEntry);
}

