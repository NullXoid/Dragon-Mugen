#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local command/CNS/runtime helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

int commandConditionValue(const FighterState& fighter, CommandConditionSubject subject) {
    switch (subject) {
    case CommandConditionSubject::StateNo:
        return fighter.stateNo;
    case CommandConditionSubject::Time:
        return fighter.stateTime;
    case CommandConditionSubject::Power:
        return fighter.power;
    case CommandConditionSubject::RoundState:
        return 2;
    case CommandConditionSubject::AiLevel:
        return 0;
    default:
        return 0;
    }
}

std::string replaceCommandComparisonsInExpression(std::string expression, const std::vector<std::string>& commands) {
    size_t searchFrom = 0;
    while (true) {
        const std::string lowered = lowercaseCopy(expression);
        const size_t commandPos = lowered.find("command", searchFrom);
        if (commandPos == std::string::npos) {
            break;
        }

        size_t opPos = commandPos + std::string_view("command").size();
        while (opPos < expression.size() && std::isspace(static_cast<unsigned char>(expression[opPos]))) {
            ++opPos;
        }

        bool negate = false;
        if (opPos + 1 < expression.size() && expression[opPos] == '!' && expression[opPos + 1] == '=') {
            negate = true;
            opPos += 2;
        } else if (opPos < expression.size() && expression[opPos] == '=') {
            ++opPos;
        } else {
            searchFrom = commandPos + 1;
            continue;
        }

        while (opPos < expression.size() && std::isspace(static_cast<unsigned char>(expression[opPos]))) {
            ++opPos;
        }
        if (opPos >= expression.size() || expression[opPos] != '"') {
            searchFrom = commandPos + 1;
            continue;
        }

        const size_t quoteStart = opPos + 1;
        const size_t quoteEnd = expression.find('"', quoteStart);
        if (quoteEnd == std::string::npos) {
            break;
        }

        const std::string commandName = expression.substr(quoteStart, quoteEnd - quoteStart);
        const bool active = commandActive(commands, commandName);
        const std::string replacement = (negate ? !active : active) ? "1" : "0";
        expression.replace(commandPos, quoteEnd + 1 - commandPos, replacement);
        searchFrom = commandPos + replacement.size();
    }
    return expression;
}

std::optional<int> resolveCommandTargetState(
    const AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const CommandStateEntry& entry,
    const std::vector<std::string>& commands) {
    std::string expression = entry.targetStateExpression.empty()
        ? std::to_string(entry.targetState)
        : entry.targetStateExpression;
    expression = replaceCommandComparisonsInExpression(std::move(expression), commands);
    const auto value = evalMugenExpression(state, fighter, expression, opponent, nullptr);
    if (!value) {
        return std::nullopt;
    }
    return static_cast<int>(std::lround(*value));
}

bool canEnterCommandState(
    const AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const CommandStateEntry& entry,
    const std::vector<std::string>& commands) {
    for (const auto& required : entry.requiredCommands) {
        if (!commandActive(commands, required)) {
            return false;
        }
    }
    for (const auto& optionGroup : entry.commandOptionGroups) {
        bool matched = false;
        for (const auto& option : optionGroup) {
            if (commandActive(commands, option)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    for (const auto& forbidden : entry.forbiddenCommands) {
        if (commandActive(commands, forbidden)) {
            return false;
        }
    }
    if (entry.requiredStateType != 0 && fighter.stateType != entry.requiredStateType) {
        return false;
    }
    for (const char forbiddenStateType : entry.forbiddenStateTypes) {
        if (fighter.stateType == forbiddenStateType) {
            return false;
        }
    }
    if (entry.requiresCtrl && !fighter.ctrl) {
        return false;
    }
    if (entry.requiresMoveContact && !fighter.moveContact) {
        return false;
    }
    for (const auto& condition : entry.intConditions) {
        if (!compareIntValue(commandConditionValue(fighter, condition.subject), condition.op, condition.value)) {
            return false;
        }
    }
    for (const auto& condition : entry.intRangeConditions) {
        const int value = commandConditionValue(fighter, condition.subject);
        const bool inRange = value >= condition.minValue && value <= condition.maxValue;
        if (condition.op == CompareOp::Equal && !inRange) {
            return false;
        }
        if (condition.op == CompareOp::NotEqual && inRange) {
            return false;
        }
    }
    for (const auto& condition : entry.expressionConditions) {
        if (!evalMugenExpressionCondition(state, fighter, condition, opponent, nullptr)) {
            return false;
        }
    }
    for (const auto& expression : entry.booleanExpressions) {
        const auto value = evalMugenExpression(state, fighter, expression, opponent, nullptr);
        if (!value || *value == 0.0f) {
            return false;
        }
    }

    const auto targetState = resolveCommandTargetState(state, fighter, opponent, entry, commands);
    if (!targetState) {
        return false;
    }
    const StateDefinition* stateDef = findStateDefinitionForActor(state, fighter, *targetState);
    return stateDef && (!stateDef->hasAnim || findExactClipForActor(state, fighter, stateDef->anim));
}

const CommandStateEntry* activeCommandEntry(
    const AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const std::vector<std::string>& commands) {
    for (const auto& entry : commandEntriesForActor(state, fighter)) {
        if (canEnterCommandState(state, fighter, opponent, entry, commands)) {
            return &entry;
        }
    }
    return nullptr;
}
