#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local command/CNS parse types and helpers.
// Include only from App.cpp after those dependencies are defined.

void addUniqueStateType(std::vector<char>& stateTypes, char stateType) {
    stateType = static_cast<char>(SDL_toupper(static_cast<unsigned char>(stateType)));
    if (std::find(stateTypes.begin(), stateTypes.end(), stateType) == stateTypes.end()) {
        stateTypes.push_back(stateType);
    }
}

struct StateTypeCondition {
    char stateType = 0;
    bool negated = false;
};

bool expressionHasOr(const std::string& value) {
    return value.find("||") != std::string::npos;
}

std::optional<StateTypeCondition> parseStateTypeCondition(const std::string& value) {
    const auto condition = stripOuterParens(value);
    if (!startsWithNoCase(condition, "statetype")) {
        return std::nullopt;
    }

    std::string tail = trim(std::string_view(condition).substr(std::string_view("statetype").size()));
    bool negated = false;
    if (startsWithNoCase(tail, "!=")) {
        negated = true;
        tail = trim(std::string_view(tail).substr(2));
    } else if (startsWithNoCase(tail, "=")) {
        tail = trim(std::string_view(tail).substr(1));
    } else {
        return std::nullopt;
    }

    if (tail.empty()) {
        return std::nullopt;
    }

    const char stateType = static_cast<char>(SDL_toupper(static_cast<unsigned char>(tail.front())));
    if (!isSupportedStateType(stateType)) {
        return std::nullopt;
    }
    return StateTypeCondition{ stateType, negated };
}

std::optional<CommandConditionSubject> commandConditionSubjectFromName(std::string_view name) {
    if (equalsNoCase(name, "stateno")) {
        return CommandConditionSubject::StateNo;
    }
    if (equalsNoCase(name, "time")) {
        return CommandConditionSubject::Time;
    }
    if (equalsNoCase(name, "power")) {
        return CommandConditionSubject::Power;
    }
    if (equalsNoCase(name, "roundstate")) {
        return CommandConditionSubject::RoundState;
    }
    if (equalsNoCase(name, "ailevel")) {
        return CommandConditionSubject::AiLevel;
    }
    return std::nullopt;
}

std::optional<CommandStateEntry::IntCondition> parseCommandIntCondition(const std::string& value) {
    const std::string condition = stripOuterParens(value);
    if (equalsNoCase(condition, "!AILevel")) {
        return CommandStateEntry::IntCondition{ CommandConditionSubject::AiLevel, CompareOp::Equal, 0 };
    }

    constexpr std::array<std::string_view, 5> subjectNames{ "stateno", "time", "power", "roundstate", "ailevel" };
    for (const std::string_view subjectName : subjectNames) {
        if (!startsWithNoCase(condition, subjectName)) {
            continue;
        }
        const size_t nameLength = subjectName.size();
        if (condition.size() > nameLength && isIdentifierChar(condition[nameLength])) {
            continue;
        }

        const auto subject = commandConditionSubjectFromName(subjectName);
        if (!subject) {
            continue;
        }

        const std::string tail = trim(std::string_view(condition).substr(nameLength));
        const auto compare = findCompareOp(tail);
        if (!compare) {
            return std::nullopt;
        }

        const auto [op, pos] = *compare;
        const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
        const auto rhs = parsePlainIntValue(trim(std::string_view(tail).substr(pos + opLength)));
        if (!rhs) {
            return std::nullopt;
        }
        return CommandStateEntry::IntCondition{ *subject, op, *rhs };
    }

    return std::nullopt;
}

std::optional<CommandStateEntry::IntRangeCondition> parseCommandIntRangeCondition(const std::string& value) {
    const std::string condition = stripOuterParens(value);
    constexpr std::array<std::string_view, 5> subjectNames{ "stateno", "time", "power", "roundstate", "ailevel" };
    for (const std::string_view subjectName : subjectNames) {
        if (!startsWithNoCase(condition, subjectName)) {
            continue;
        }
        const size_t nameLength = subjectName.size();
        if (condition.size() > nameLength && isIdentifierChar(condition[nameLength])) {
            continue;
        }

        const auto subject = commandConditionSubjectFromName(subjectName);
        if (!subject) {
            continue;
        }

        const std::string tail = trim(std::string_view(condition).substr(nameLength));
        const auto compare = findCompareOp(tail);
        if (!compare) {
            return std::nullopt;
        }

        const auto [op, pos] = *compare;
        if (op != CompareOp::Equal && op != CompareOp::NotEqual) {
            return std::nullopt;
        }
        const size_t opLength = op == CompareOp::NotEqual ? 2 : 1;
        const std::string rhs = trim(std::string_view(tail).substr(pos + opLength));
        if (rhs.size() < 5 || rhs.front() != '[' || rhs.back() != ']') {
            return std::nullopt;
        }
        const auto parts = splitCsv(std::string(std::string_view(rhs).substr(1, rhs.size() - 2)));
        if (parts.empty()) {
            return std::nullopt;
        }
        const auto minValue = parsePlainIntValue(parts[0]);
        const auto maxValue = parts.size() >= 2 ? parsePlainIntValue(parts[1]) : minValue;
        if (!minValue || !maxValue) {
            return std::nullopt;
        }
        return CommandStateEntry::IntRangeCondition{
            *subject,
            op,
            std::min(*minValue, *maxValue),
            std::max(*minValue, *maxValue),
        };
    }

    return std::nullopt;
}

bool triggerRequiresCtrl(const std::string& value) {
    const auto trigger = stripOuterParens(value);
    if (!startsWithNoCase(trigger, "ctrl")) {
        return false;
    }
    if (trigger.size() == std::string_view("ctrl").size()) {
        return true;
    }
    const char next = trigger[std::string_view("ctrl").size()];
    return std::isspace(static_cast<unsigned char>(next)) || next == '=' || next == '|' || next == '&';
}

void applyCommandTriggerClause(CommandStateEntry& entry, const std::string& clause) {
    if (clause.empty()) {
        return;
    }

    const bool hasOr = expressionHasOr(clause);
    const auto commands = findCommandConditions(clause);
    if (hasOr && !commands.empty()) {
        std::vector<std::string> options;
        for (const auto& command : commands) {
            if (command.negated) {
                options.clear();
                break;
            }
            addUniqueCommand(options, command.command);
        }
        if (!options.empty()) {
            entry.commandOptionGroups.push_back(std::move(options));
            return;
        }
    }

    for (const auto& command : commands) {
        addUniqueCommand(command.negated ? entry.forbiddenCommands : entry.requiredCommands, command.command);
    }

    if (hasOr) {
        if (expressionLooksSupported(clause)) {
            entry.booleanExpressions.push_back(stripOuterParens(clause));
        }
        return;
    }

    if (const auto stateType = parseStateTypeCondition(clause)) {
        if (stateType->negated) {
            addUniqueStateType(entry.forbiddenStateTypes, stateType->stateType);
        } else if (entry.requiredStateType == 0) {
            entry.requiredStateType = stateType->stateType;
        }
    }
    if (const auto condition = parseCommandIntCondition(clause)) {
        entry.intConditions.push_back(*condition);
    }
    if (const auto rangeCondition = parseCommandIntRangeCondition(clause)) {
        entry.intRangeConditions.push_back(*rangeCondition);
        return;
    }
    if (const auto expression = parseMugenExpressionCondition(clause)) {
        if (expressionLooksSupported(expression->lhs) || expressionLooksSupported(expression->rhs)) {
            entry.expressionConditions.push_back(*expression);
        }
    } else if (expressionLooksSupported(clause)) {
        entry.booleanExpressions.push_back(stripOuterParens(clause));
    }
    if (triggerRequiresCtrl(clause)) {
        entry.requiresCtrl = true;
    }
    if (triggerIsMoveContact(clause)) {
        entry.requiresMoveContact = true;
    }
}

void applyCommandTriggerExpression(CommandStateEntry& entry, const std::string& value) {
    for (const auto& clause : splitTopLevelClauses(value, "&&")) {
        applyCommandTriggerClause(entry, clause);
    }
}

std::vector<CommandStateEntry> loadCommandStateEntries(const CharacterFiles& files) {
    if (files.cmd.empty() || !std::filesystem::exists(files.cmd)) {
        return {};
    }

    const auto cmd = parseMugenTextFile(files.cmd);
    std::vector<CommandStateEntry> entries;
    bool inStateMinusOne = false;

    for (const auto& section : cmd.sections) {
        if (const auto stateNo = parseStateNumber(section.name, "Statedef ")) {
            inStateMinusOne = *stateNo == -1;
            continue;
        }
        if (!inStateMinusOne || !startsWithNoCase(section.name, "State -1")) {
            continue;
        }

        const auto* type = findProperty(section, "type");
        const auto* value = findProperty(section, "value");
        if (!type || !value || !startsWithNoCase(trim(type->value), "ChangeState")) {
            continue;
        }

        const auto targetState = parsePlainIntValue(value->value);
        const std::string targetStateExpression = trim(value->value);

        CommandStateEntry entry;
        entry.targetState = targetState.value_or(0);
        entry.targetStateExpression = targetStateExpression;
        if (const auto comma = section.name.find(','); comma != std::string::npos) {
            entry.label = trim(std::string_view(section.name).substr(comma + 1));
        }

        for (const auto& property : section.properties) {
            const bool isTriggerAll = equalsNoCase(property.key, "triggerall");
            const bool isPrimaryTrigger = equalsNoCase(property.key, "trigger1");
            if (!isTriggerAll && !isPrimaryTrigger) {
                continue;
            }

            applyCommandTriggerExpression(entry, property.value);
        }

        if (!entry.requiredCommands.empty() || !entry.commandOptionGroups.empty()) {
            entries.push_back(std::move(entry));
        }
    }

    return entries;
}

std::string normalizeCommandAtomSymbol(std::string token) {
    token = trim(token);
    while (!token.empty() && token.front() == '>') {
        token = trim(std::string_view(token).substr(1));
    }
    return token;
}

CommandAtom parseCommandAtom(std::string token) {
    CommandAtom atom;
    token = normalizeCommandAtomSymbol(token);

    while (!token.empty()) {
        if (token.front() == '/') {
            atom.hold = true;
            token = trim(std::string_view(token).substr(1));
            continue;
        }
        if (token.front() == '~') {
            atom.release = true;
            token = trim(std::string_view(token).substr(1));
            while (!token.empty() && std::isdigit(static_cast<unsigned char>(token.front()))) {
                token.erase(token.begin());
            }
            continue;
        }
        if (token.front() == '$') {
            atom.broadDirection = true;
            token = trim(std::string_view(token).substr(1));
            continue;
        }
        break;
    }

    if (token.size() == 1) {
        const char ch = token.front();
        if (ch == 'a' || ch == 'b' || ch == 'c' || ch == 'x' || ch == 'y' || ch == 'z' || ch == 's') {
            atom.symbol = lowercaseCopy(token);
        } else {
            atom.symbol = uppercaseCopy(token);
        }
    } else {
        atom.symbol = uppercaseCopy(token);
    }
    return atom;
}

CommandStep parseCommandStep(const std::string& token) {
    CommandStep step;
    size_t start = 0;
    while (start <= token.size()) {
        const auto delimiter = token.find('+', start);
        const auto end = delimiter == std::string::npos ? token.size() : delimiter;
        CommandAtom atom = parseCommandAtom(std::string(std::string_view(token).substr(start, end - start)));
        if (!atom.symbol.empty()) {
            step.atoms.push_back(std::move(atom));
        }
        if (delimiter == std::string::npos) {
            break;
        }
        start = delimiter + 1;
    }
    return step;
}

CommandDefinition parseCommandDefinitionSection(const MugenSection& section, int defaultTime, int defaultBufferTime) {
    CommandDefinition definition;
    definition.maxTime = std::max(1, defaultTime);
    definition.bufferTime = std::max(1, defaultBufferTime);
    if (const auto* name = findProperty(section, "name")) {
        definition.name = unquote(name->value);
    }
    if (const auto* time = findProperty(section, "time")) {
        definition.maxTime = std::max(1, parseIntValue(time->value, definition.maxTime));
    }
    const auto* command = findProperty(section, "command");
    if (!command) {
        return definition;
    }

    for (const auto& token : splitCsv(command->value)) {
        CommandStep step = parseCommandStep(token);
        if (!step.atoms.empty()) {
            definition.steps.push_back(std::move(step));
        }
    }
    return definition;
}

std::vector<CommandDefinition> loadCommandDefinitions(const CharacterFiles& files) {
    if (files.cmd.empty() || !std::filesystem::exists(files.cmd)) {
        return {};
    }

    std::vector<CommandDefinition> definitions;
    const auto cmd = parseMugenTextFile(files.cmd);
    int defaultCommandTime = 15;
    int defaultCommandBufferTime = 1;
    for (const auto& section : cmd.sections) {
        if (!equalsNoCase(section.name, "Defaults")) {
            continue;
        }
        if (const auto* time = findProperty(section, "command.time")) {
            defaultCommandTime = std::max(1, parseIntValue(time->value, defaultCommandTime));
        }
        if (const auto* bufferTime = findProperty(section, "command.buffer.time")) {
            defaultCommandBufferTime = std::max(1, parseIntValue(bufferTime->value, defaultCommandBufferTime));
        }
    }

    for (const auto& section : cmd.sections) {
        if (!equalsNoCase(section.name, "Command")) {
            continue;
        }

        CommandDefinition definition = parseCommandDefinitionSection(section, defaultCommandTime, defaultCommandBufferTime);
        if (!definition.name.empty() && !definition.steps.empty()) {
            definitions.push_back(std::move(definition));
        }
    }
    return definitions;
}
