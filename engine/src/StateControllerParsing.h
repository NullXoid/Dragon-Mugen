#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local CNS/state-controller parse types and helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

std::optional<int> parseStateNumber(std::string_view sectionName, std::string_view prefix) {
    if (!startsWithNoCase(sectionName, prefix)) {
        return std::nullopt;
    }

    try {
        return std::stoi(trim(sectionName.substr(prefix.size())));
    } catch (...) {
        return std::nullopt;
    }
}

char parseStateChar(const MugenSection* section, std::string_view key, char fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    const auto value = trim(property->value);
    if (value.empty()) {
        return fallback;
    }
    return static_cast<char>(SDL_toupper(static_cast<unsigned char>(value.front())));
}

std::string stripOuterParens(std::string value);

std::optional<int> parseTimeTrigger(const std::string& trigger) {
    const std::string condition = stripOuterParens(trigger);
    if (!startsWithNoCase(condition, "Time")) {
        return std::nullopt;
    }
    const auto equals = condition.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }
    return parseIntValue(trim(std::string_view(condition).substr(equals + 1)), 0);
}

std::optional<int> parseAnimElemTrigger(const std::string& trigger) {
    const std::string condition = stripOuterParens(trigger);
    if (!startsWithNoCase(condition, "AnimElem") || startsWithNoCase(condition, "AnimElemTime")) {
        return std::nullopt;
    }
    const auto equals = condition.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }
    const auto valuePart = trim(std::string_view(condition).substr(equals + 1));
    const auto values = splitCsv(valuePart);
    if (values.empty()) {
        return std::nullopt;
    }
    return parseIntValue(values.front(), 0);
}

std::optional<std::pair<CompareOp, size_t>> findCompareOp(std::string_view value) {
    const std::array<std::pair<std::string_view, CompareOp>, 6> ops{ {
        { ">=", CompareOp::GreaterEqual },
        { "<=", CompareOp::LessEqual },
        { "!=", CompareOp::NotEqual },
        { "=", CompareOp::Equal },
        { ">", CompareOp::Greater },
        { "<", CompareOp::Less },
    } };

    for (const auto& [token, op] : ops) {
        if (const auto pos = value.find(token); pos != std::string_view::npos) {
            return std::pair<CompareOp, size_t>{ op, pos };
        }
    }
    return std::nullopt;
}

std::string stripOuterParens(std::string value) {
    value = trim(value);
    while (value.size() >= 2 && value.front() == '(' && value.back() == ')') {
        value = trim(std::string_view(value).substr(1, value.size() - 2));
    }
    return value;
}

std::vector<std::string> splitAndClauses(const std::string& trigger) {
    std::vector<std::string> clauses;
    size_t start = 0;
    while (start <= trigger.size()) {
        const auto delimiter = trigger.find("&&", start);
        const auto end = delimiter == std::string::npos ? trigger.size() : delimiter;
        clauses.push_back(stripOuterParens(trim(std::string_view(trigger).substr(start, end - start))));
        if (delimiter == std::string::npos) {
            break;
        }
        start = delimiter + 2;
    }
    return clauses;
}

std::optional<AnimElemTimeCondition> parseAnimElemTimeCondition(const std::string& trigger) {
    const std::string condition = stripOuterParens(trigger);
    if (!startsWithNoCase(condition, "AnimElemTime")) {
        return std::nullopt;
    }

    const auto open = condition.find('(');
    const auto close = condition.find(')', open == std::string::npos ? 0 : open + 1);
    if (open == std::string::npos || close == std::string::npos || close <= open + 1) {
        return std::nullopt;
    }

    const int elem = parseIntValue(trim(std::string_view(condition).substr(open + 1, close - open - 1)), 0);
    const auto tail = trim(std::string_view(condition).substr(close + 1));
    const auto compare = findCompareOp(tail);
    if (!compare) {
        return std::nullopt;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    return AnimElemTimeCondition{
        elem,
        op,
        parseIntValue(trim(std::string_view(tail).substr(pos + opLength)), 0),
    };
}

bool triggerIsMoveContact(const std::string& trigger) {
    const auto trimmed = stripOuterParens(trigger);
    return startsWithNoCase(trimmed, "movecontact") && trimmed.size() == std::string_view("movecontact").size();
}

struct ParsedCommandCondition {
    std::string command;
    bool negated = false;
};

std::vector<ParsedCommandCondition> findCommandConditions(const std::string& value) {
    std::vector<ParsedCommandCondition> commands;
    const std::string condition = stripOuterParens(value);
    size_t cursor = 0;
    while (cursor < condition.size()) {
        const size_t commandPos = findNoCase(condition, "command", cursor);
        if (commandPos == std::string::npos) {
            break;
        }

        const bool leftBoundary = commandPos == 0 || !isIdentifierChar(condition[commandPos - 1]);
        const size_t afterName = commandPos + std::string_view("command").size();
        const bool rightBoundary = afterName >= condition.size() || !isIdentifierChar(condition[afterName]);
        if (!leftBoundary || !rightBoundary) {
            cursor = afterName;
            continue;
        }

        cursor = afterName;
        while (cursor < condition.size() && std::isspace(static_cast<unsigned char>(condition[cursor]))) {
            ++cursor;
        }

        bool negated = false;
        if (cursor + 1 < condition.size() && condition[cursor] == '!' && condition[cursor + 1] == '=') {
            negated = true;
            cursor += 2;
        } else if (cursor < condition.size() && condition[cursor] == '=') {
            ++cursor;
        } else {
            continue;
        }

        while (cursor < condition.size() && std::isspace(static_cast<unsigned char>(condition[cursor]))) {
            ++cursor;
        }
        if (cursor >= condition.size() || condition[cursor] != '"') {
            continue;
        }

        const size_t closingQuote = condition.find('"', cursor + 1);
        if (closingQuote == std::string::npos) {
            break;
        }

        commands.push_back(ParsedCommandCondition{
            std::string(std::string_view(condition).substr(cursor + 1, closingQuote - cursor - 1)),
            negated,
        });
        cursor = closingQuote + 1;
    }
    return commands;
}

bool isSupportedStateType(char value);
void addUniqueCommand(std::vector<std::string>& commands, const std::string& command);

std::vector<std::string> splitTopLevelClauses(const std::string& expression, std::string_view delimiter, bool allowSinglePipe = false) {
    std::vector<std::string> clauses;
    size_t start = 0;
    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;

    for (size_t i = 0; i < expression.size(); ++i) {
        const char ch = expression[i];
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) {
            continue;
        }
        if (ch == '(') {
            ++parenDepth;
        } else if (ch == ')' && parenDepth > 0) {
            --parenDepth;
        } else if (ch == '[') {
            ++bracketDepth;
        } else if (ch == ']' && bracketDepth > 0) {
            --bracketDepth;
        }

        if (parenDepth != 0 || bracketDepth != 0) {
            continue;
        }

        bool matched = false;
        size_t delimiterLength = delimiter.size();
        if (!delimiter.empty() && i + delimiterLength <= expression.size()) {
            matched = std::string_view(expression).substr(i, delimiterLength) == delimiter;
        }
        if (!matched && allowSinglePipe && ch == '|') {
            matched = true;
            delimiterLength = i + 1 < expression.size() && expression[i + 1] == '|' ? 2 : 1;
        }
        if (!matched) {
            continue;
        }

        clauses.push_back(stripOuterParens(trim(std::string_view(expression).substr(start, i - start))));
        start = i + delimiterLength;
        i += delimiterLength - 1;
    }

    clauses.push_back(stripOuterParens(trim(std::string_view(expression).substr(start))));
    return clauses;
}

std::optional<size_t> findTopLevelBinaryOperator(
    std::string_view expression,
    const std::vector<std::string_view>& operators) {
    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;

    for (size_t i = expression.size(); i-- > 0;) {
        const char ch = expression[i];
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) {
            continue;
        }
        if (ch == ')') {
            ++parenDepth;
            continue;
        }
        if (ch == '(' && parenDepth > 0) {
            --parenDepth;
            continue;
        }
        if (ch == ']') {
            ++bracketDepth;
            continue;
        }
        if (ch == '[' && bracketDepth > 0) {
            --bracketDepth;
            continue;
        }
        if (parenDepth != 0 || bracketDepth != 0) {
            continue;
        }

        for (const auto op : operators) {
            if (i + op.size() > expression.size()
                || expression.substr(i, op.size()) != op) {
                continue;
            }
            if ((op == "+" || op == "-") && i == 0) {
                continue;
            }
            if ((op == "+" || op == "-")
                && i > 0
                && (expression[i - 1] == 'e' || expression[i - 1] == 'E')) {
                continue;
            }
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::pair<CompareOp, size_t>> findTopLevelCompareOp(std::string_view expression) {
    const std::array<std::pair<std::string_view, CompareOp>, 6> ops{ {
        { ">=", CompareOp::GreaterEqual },
        { "<=", CompareOp::LessEqual },
        { "!=", CompareOp::NotEqual },
        { "=", CompareOp::Equal },
        { ">", CompareOp::Greater },
        { "<", CompareOp::Less },
    } };

    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;
    for (size_t i = 0; i < expression.size(); ++i) {
        const char ch = expression[i];
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) {
            continue;
        }
        if (ch == '(') {
            ++parenDepth;
            continue;
        }
        if (ch == ')' && parenDepth > 0) {
            --parenDepth;
            continue;
        }
        if (ch == '[') {
            ++bracketDepth;
            continue;
        }
        if (ch == ']' && bracketDepth > 0) {
            --bracketDepth;
            continue;
        }
        if (parenDepth != 0 || bracketDepth != 0) {
            continue;
        }

        for (const auto& [token, op] : ops) {
            if (i + token.size() <= expression.size()
                && expression.substr(i, token.size()) == token) {
                return std::pair<CompareOp, size_t>{ op, i };
            }
        }
    }
    return std::nullopt;
}

std::optional<MugenVariableRef> parseMugenVariableRef(std::string_view value) {
    const std::string trimmed = trim(value);
    const struct {
        std::string_view prefix;
        MugenVariableBank bank;
    } prefixes[] = {
        { "sysfvar(", MugenVariableBank::SysFVar },
        { "sysvar(", MugenVariableBank::SysVar },
        { "fvar(", MugenVariableBank::FVar },
        { "var(", MugenVariableBank::Var },
    };

    for (const auto& candidate : prefixes) {
        if (!startsWithNoCase(trimmed, candidate.prefix)) {
            continue;
        }
        const auto close = trimmed.find(')', candidate.prefix.size());
        if (close == std::string::npos) {
            return std::nullopt;
        }
        const auto index = parsePlainIntValue(trim(std::string_view(trimmed).substr(candidate.prefix.size(), close - candidate.prefix.size())));
        if (!index || *index < 0) {
            return std::nullopt;
        }
        return MugenVariableRef{ candidate.bank, *index };
    }
    return std::nullopt;
}

std::optional<MugenExpressionCondition> parseMugenExpressionCondition(const std::string& clause) {
    const std::string condition = stripOuterParens(clause);
    const auto compare = findTopLevelCompareOp(condition);
    if (!compare) {
        return std::nullopt;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    const std::string lhs = trim(std::string_view(condition).substr(0, pos));
    const std::string rhs = trim(std::string_view(condition).substr(pos + opLength));
    if (lhs.empty() || rhs.empty()) {
        return std::nullopt;
    }
    return MugenExpressionCondition{ lhs, op, rhs };
}

bool expressionLooksSupported(std::string_view value) {
    const std::string lowered = lowercaseCopy(value);
    constexpr std::array<std::string_view, 53> supportedTokens{
        "var(", "fvar(", "sysvar(", "sysfvar(", "random", "ailevel", "teammode", "time", "animtime",
        "stateno", "roundstate", "roundno", "roundsexisted", "power", "powermax", "hitcount", "life", "p2life", "statetype", "movetype",
        "pos", "vel", "p2bodydist", "p2dist", "p2stateno", "p2statetype", "p2movetype", "movecontact", "movehit", "moveguarded",
        "numtarget", "frontedgedist", "backedgedist", "frontedgebodydist", "backedgebodydist", "facing", "alive", "selfanimexist", "numhelper", "numproj", "numprojid",
        "projhit", "projcontact", "projguarded", "projcontacttime", "projguardedtime", "gethitvar", "parent,", "root,",
        "enemy,", "enemynear,", "helper(", "target,",
    };
    return std::any_of(supportedTokens.begin(), supportedTokens.end(), [&lowered](std::string_view token) {
        return lowered.find(token) != std::string::npos;
    });
}

std::optional<StateTriggerSubject> parseStateTriggerSubject(const std::string& clause, size_t& subjectEnd) {
    const std::string lowered = lowercaseCopy(clause);
    if (startsWithNoCase(lowered, "time") && (lowered.size() == 4 || !isIdentifierChar(lowered[4]))) {
        subjectEnd = 4;
        return StateTriggerSubject::Time;
    }
    if (startsWithNoCase(lowered, "animtime") && (lowered.size() == 8 || !isIdentifierChar(lowered[8]))) {
        subjectEnd = 8;
        return StateTriggerSubject::AnimTime;
    }
    if (startsWithNoCase(lowered, "vel")) {
        size_t cursor = 3;
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::VelX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::VelY;
        }
    }
    if (startsWithNoCase(lowered, "pos")) {
        size_t cursor = 3;
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::PosX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::PosY;
        }
    }
    if (startsWithNoCase(lowered, "p2bodydist")) {
        size_t cursor = std::string_view("p2bodydist").size();
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2BodyDistX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2BodyDistY;
        }
    }
    if (startsWithNoCase(lowered, "p2dist")) {
        size_t cursor = std::string_view("p2dist").size();
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2DistX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2DistY;
        }
    }
    if (startsWithNoCase(lowered, "frontedgebodydist") || startsWithNoCase(lowered, "frontedgedist")) {
        subjectEnd = startsWithNoCase(lowered, "frontedgebodydist")
            ? std::string_view("frontedgebodydist").size()
            : std::string_view("frontedgedist").size();
        return StateTriggerSubject::FrontEdgeBodyDist;
    }
    if (startsWithNoCase(lowered, "backedgebodydist") || startsWithNoCase(lowered, "backedgedist")) {
        subjectEnd = startsWithNoCase(lowered, "backedgebodydist")
            ? std::string_view("backedgebodydist").size()
            : std::string_view("backedgedist").size();
        return StateTriggerSubject::BackEdgeBodyDist;
    }
    if (startsWithNoCase(lowered, "hitshakeover")) {
        subjectEnd = std::string_view("hitshakeover").size();
        return StateTriggerSubject::HitShakeOver;
    }
    return std::nullopt;
}

std::optional<StateRangeCondition> parseStateRangeCondition(StateTriggerSubject subject, const std::string& rhs) {
    const std::string value = trim(rhs);
    if (value.size() < 5 || value.front() != '[' || value.back() != ']') {
        return std::nullopt;
    }
    const auto parts = splitCsv(std::string(std::string_view(value).substr(1, value.size() - 2)));
    if (parts.size() != 2) {
        return std::nullopt;
    }
    const auto minValue = parsePlainFloatValue(parts[0]);
    const auto maxValue = parsePlainFloatValue(parts[1]);
    if (!minValue || !maxValue) {
        return std::nullopt;
    }
    return StateRangeCondition{ subject, *minValue, *maxValue };
}

bool parseStateFloatCondition(const std::string& clause, StateTriggerGroup& group) {
    if (equalsNoCase(clause, "!time")) {
        group.floatConditions.push_back(StateFloatCondition{ StateTriggerSubject::Time, CompareOp::Equal, 0.0f });
        return true;
    }
    if (equalsNoCase(clause, "!animtime")) {
        group.floatConditions.push_back(StateFloatCondition{ StateTriggerSubject::AnimTime, CompareOp::Equal, 0.0f });
        return true;
    }

    size_t subjectEnd = 0;
    const auto subject = parseStateTriggerSubject(clause, subjectEnd);
    if (!subject) {
        return false;
    }

    const std::string tail = trim(std::string_view(clause).substr(subjectEnd));
    const auto compare = findCompareOp(tail);
    if (!compare) {
        return false;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    const std::string rhs = trim(std::string_view(tail).substr(pos + opLength));
    if (op == CompareOp::Equal) {
        if (const auto range = parseStateRangeCondition(*subject, rhs)) {
            group.rangeConditions.push_back(*range);
            return true;
        }
    }

    const auto value = parsePlainFloatValue(rhs);
    if (!value) {
        return false;
    }
    group.floatConditions.push_back(StateFloatCondition{ *subject, op, *value });
    return true;
}

bool parseStateTypeTriggerClause(const std::string& clause, StateTriggerGroup& group) {
    const std::string condition = stripOuterParens(clause);
    if (!startsWithNoCase(condition, "statetype")) {
        return false;
    }

    std::string tail = trim(std::string_view(condition).substr(std::string_view("statetype").size()));
    bool negated = false;
    if (startsWithNoCase(tail, "!=")) {
        negated = true;
        tail = trim(std::string_view(tail).substr(2));
    } else if (startsWithNoCase(tail, "=")) {
        tail = trim(std::string_view(tail).substr(1));
    } else {
        return false;
    }

    if (tail.empty()) {
        return false;
    }

    const char stateType = static_cast<char>(SDL_toupper(static_cast<unsigned char>(tail.front())));
    if (!isSupportedStateType(stateType)) {
        return false;
    }
    group.stateTypeConditions.push_back(StateTypeTriggerCondition{ stateType, negated });
    return true;
}

bool parseStateAnimElemTriggerClause(const std::string& clause, StateTriggerGroup& group) {
    const std::string condition = stripOuterParens(clause);
    if (!startsWithNoCase(condition, "AnimElem") || startsWithNoCase(condition, "AnimElemTime")) {
        return false;
    }

    const auto equals = condition.find('=');
    if (equals == std::string::npos) {
        return false;
    }

    const auto values = splitCsv(trim(std::string_view(condition).substr(equals + 1)));
    if (values.empty()) {
        return false;
    }

    const int elem = parseIntValue(values.front(), 0);
    if (elem <= 0) {
        return false;
    }

    CompareOp op = CompareOp::Equal;
    int offset = 0;
    if (values.size() >= 2) {
        const std::string offsetClause = trim(values[1]);
        if (const auto compare = findCompareOp(offsetClause)) {
            const auto [parsedOp, pos] = *compare;
            const size_t opLength = parsedOp == CompareOp::GreaterEqual
                || parsedOp == CompareOp::LessEqual
                || parsedOp == CompareOp::NotEqual
                ? 2
                : 1;
            op = parsedOp;
            offset = parseIntValue(trim(std::string_view(offsetClause).substr(pos + opLength)), 0);
        } else {
            offset = parseIntValue(offsetClause, 0);
        }
    }
    group.animElemTimeConditions.push_back(AnimElemTimeCondition{ elem, op, offset });
    return true;
}

bool parseStateTriggerClause(const std::string& clause, StateTriggerGroup& group) {
    const std::string trimmed = stripOuterParens(clause);
    if (trimmed.empty() || trimmed == "1") {
        return true;
    }
    const auto commands = findCommandConditions(trimmed);
    if (!commands.empty()) {
        for (const auto& command : commands) {
            if (command.negated) {
                addUniqueCommand(group.forbiddenCommands, command.command);
            } else {
                addUniqueCommand(group.requiredCommands, command.command);
            }
        }
        return true;
    }
    if (triggerIsMoveContact(trimmed)) {
        group.requiresMoveContact = true;
        return true;
    }
    if (parseStateFloatCondition(trimmed, group)) {
        return true;
    }
    if (parseStateTypeTriggerClause(trimmed, group)) {
        return true;
    }
    if (parseStateAnimElemTriggerClause(trimmed, group)) {
        return true;
    }
    if (const auto condition = parseAnimElemTimeCondition(trimmed)) {
        group.animElemTimeConditions.push_back(*condition);
        return true;
    }
    if (const auto expression = parseMugenExpressionCondition(trimmed)) {
        if (expressionLooksSupported(expression->lhs) || expressionLooksSupported(expression->rhs)) {
            group.expressionConditions.push_back(*expression);
            return true;
        }
    }
    if (expressionLooksSupported(trimmed)) {
        group.booleanExpressions.push_back(trimmed);
        return true;
    }
    return false;
}

std::optional<std::vector<StateTriggerGroup>> parseStateTriggerExpressionGroups(const std::string& expression) {
    std::vector<StateTriggerGroup> groups;
    for (const auto& orClause : splitTopLevelClauses(expression, "||", true)) {
        StateTriggerGroup group;
        for (const auto& andClause : splitTopLevelClauses(orClause, "&&")) {
            if (!parseStateTriggerClause(andClause, group)) {
                return std::nullopt;
            }
        }
        groups.push_back(std::move(group));
    }
    if (groups.empty()) {
        groups.push_back({});
    }
    return groups;
}

void appendNumberedTriggerExpression(std::vector<std::pair<std::string, std::string>>& expressions, const std::string& key, const std::string& value) {
    const std::string loweredKey = lowercaseCopy(key);
    for (auto& [currentKey, expression] : expressions) {
        if (currentKey == loweredKey) {
            expression += " && ";
            expression += value;
            return;
        }
    }
    expressions.push_back({ loweredKey, value });
}

std::optional<StateControllerTrigger> parseStateControllerTrigger(const MugenSection& section) {
    StateControllerTrigger trigger;
    if (const auto* persistent = findProperty(section, "persistent")) {
        trigger.persistent = std::max(0, parseIntValue(persistent->value, 1));
    }

    std::vector<std::pair<std::string, std::string>> numberedExpressions;
    for (const auto& property : section.properties) {
        if (!startsWithNoCase(property.key, "trigger")) {
            continue;
        }

        trigger.hasTrigger = true;
        if (startsWithNoCase(property.key, "triggerall")) {
            const auto groups = parseStateTriggerExpressionGroups(property.value);
            if (!groups) {
                return std::nullopt;
            }
            trigger.triggerAllExpressions.push_back(*groups);
        } else {
            appendNumberedTriggerExpression(numberedExpressions, property.key, property.value);
        }
    }

    for (const auto& [key, expression] : numberedExpressions) {
        (void)key;
        const auto groups = parseStateTriggerExpressionGroups(expression);
        if (!groups) {
            return std::nullopt;
        }
        trigger.triggerGroups.insert(trigger.triggerGroups.end(), groups->begin(), groups->end());
    }

    if (!trigger.hasTrigger) {
        return std::nullopt;
    }
    return trigger;
}

std::optional<float> parseControllerFloatProperty(
    const MugenSection& section,
    std::string_view key,
    const CharacterConstants& constants) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    return parseControllerFloatValue(property->value, constants);
}

std::optional<std::pair<float, float>> parseControllerFloatPairProperty(
    const MugenSection& section,
    std::string_view key,
    const CharacterConstants& constants,
    float fallbackY = 0.0f) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    return parseControllerFloatPairValue(property->value, constants, fallbackY);
}

float parseExplodPositionComponent(const std::string& value, const CharacterConstants& constants, float fallback) {
    if (const auto parsed = parseControllerFloatValue(value, constants)) {
        return *parsed;
    }

    const std::string lowered = lowercaseCopy(value);
    if (findNoCase(lowered, "screenpos y", 0) != std::string::npos) {
        if (const auto minus = lowered.rfind('-'); minus != std::string::npos) {
            return -parseFloatValue(trim(std::string_view(lowered).substr(minus + 1)), -fallback);
        }
        if (const auto plus = lowered.rfind('+'); plus != std::string::npos) {
            return parseFloatValue(trim(std::string_view(lowered).substr(plus + 1)), fallback);
        }
        return 0.0f;
    }
    return fallback;
}

std::pair<float, float> parseExplodPosition(const std::string& value, const CharacterConstants& constants) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { 0.0f, 0.0f };
    }
    const float x = parseExplodPositionComponent(parts[0], constants, 0.0f);
    const float y = parts.size() >= 2 ? parseExplodPositionComponent(parts[1], constants, 0.0f) : 0.0f;
    return { x, y };
}

EnvShakeSpec parseEnvShakeSpec(const MugenSection& section) {
    EnvShakeSpec shake;
    if (const auto* time = findProperty(section, "time")) {
        shake.time = std::max(0, parseIntValue(time->value, shake.time));
        shake.timeExpression = trim(time->value);
    }
    if (const auto* frequency = findProperty(section, "freq")) {
        shake.frequency = std::max(1, parseIntValue(frequency->value, shake.frequency));
        shake.frequencyExpression = trim(frequency->value);
    }
    if (const auto* amplitude = findProperty(section, "ampl")) {
        shake.amplitude = parseFloatValue(amplitude->value, shake.amplitude);
        shake.amplitudeExpression = trim(amplitude->value);
    }
    if (const auto* phase = findProperty(section, "phase")) {
        shake.phase = parseIntValue(phase->value, shake.phase);
        shake.phaseExpression = trim(phase->value);
    }
    shake.enabled = shake.time > 0 && std::abs(shake.amplitude) > 0.001f;
    return shake;
}

std::array<int, 3> parseIntTripleValue(const std::string& value, int fallbackR, int fallbackG, int fallbackB) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB };
    }
    return {
        parseIntValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseIntValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseIntValue(parts[2], fallbackB) : fallbackB,
    };
}

std::array<std::string, 3> parseExpressionTripleValue(
    const std::string& value,
    std::array<std::string, 3> fallback = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return fallback;
    }
    std::array<std::string, 3> out = fallback;
    for (size_t i = 0; i < std::min<size_t>(parts.size(), out.size()); ++i) {
        out[i] = trim(parts[i]);
    }
    return out;
}

std::array<int, 4> parseIntQuadValue(const std::string& value, int fallbackR, int fallbackG, int fallbackB, int fallbackPeriod) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB, fallbackPeriod };
    }
    return {
        parseIntValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseIntValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseIntValue(parts[2], fallbackB) : fallbackB,
        parts.size() >= 4 ? parseIntValue(parts[3], fallbackPeriod) : fallbackPeriod,
    };
}

std::array<std::string, 4> parseExpressionQuadValue(
    const std::string& value,
    std::array<std::string, 4> fallback = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return fallback;
    }
    std::array<std::string, 4> out = fallback;
    for (size_t i = 0; i < std::min<size_t>(parts.size(), out.size()); ++i) {
        out[i] = trim(parts[i]);
    }
    return out;
}

PaletteEffectSpec parsePaletteEffectSpec(const MugenSection& section, std::string_view prefix = "") {
    PaletteEffectSpec effect;
    const std::string keyPrefix(prefix);
    auto prefixed = [&keyPrefix](std::string_view key) {
        return keyPrefix.empty() ? std::string(key) : keyPrefix + "." + std::string(key);
    };

    if (const auto* time = findProperty(section, prefixed("time"))) {
        effect.time = parseIntValue(time->value, effect.time);
        effect.timeExpression = trim(time->value);
    }
    if (const auto* add = findProperty(section, prefixed("add"))) {
        const auto values = parseIntTripleValue(add->value, 0, 0, 0);
        effect.addR = values[0];
        effect.addG = values[1];
        effect.addB = values[2];
        effect.addExpressions = parseExpressionTripleValue(add->value);
    }
    if (const auto* mul = findProperty(section, prefixed("mul"))) {
        const auto values = parseIntTripleValue(mul->value, 256, 256, 256);
        effect.mulR = values[0];
        effect.mulG = values[1];
        effect.mulB = values[2];
        effect.mulExpressions = parseExpressionTripleValue(mul->value, { "256", "256", "256" });
    }
    if (const auto* sinAdd = findProperty(section, prefixed("sinadd"))) {
        const auto values = parseIntQuadValue(sinAdd->value, 0, 0, 0, 0);
        effect.sinAddR = values[0];
        effect.sinAddG = values[1];
        effect.sinAddB = values[2];
        effect.sinPeriod = values[3];
        effect.sinAddExpressions = parseExpressionQuadValue(sinAdd->value);
    }
    if (const auto* color = findProperty(section, prefixed("color"))) {
        effect.color = parseIntValue(color->value, effect.color);
        effect.colorExpression = trim(color->value);
    }
    if (const auto* invertAll = findProperty(section, prefixed("invertall"))) {
        effect.invertAll = parseIntValue(invertAll->value, 0) != 0;
        effect.invertAllExpression = trim(invertAll->value);
    }

    effect.enabled = effect.time != 0
        && (effect.addR != 0 || effect.addG != 0 || effect.addB != 0
            || effect.mulR != 256 || effect.mulG != 256 || effect.mulB != 256
            || effect.sinAddR != 0 || effect.sinAddG != 0 || effect.sinAddB != 0
            || effect.color != 256 || effect.invertAll);
    return effect;
}

std::optional<char> parseControllerCharProperty(const MugenSection& section, std::string_view key) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    const std::string value = trim(property->value);
    if (value.empty()) {
        return std::nullopt;
    }
    return static_cast<char>(SDL_toupper(static_cast<unsigned char>(value.front())));
}

std::string normalizeAssertSpecialFlag(std::string_view value) {
    std::string normalized = lowercaseCopy(trim(value));
    normalized.erase(
        std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0 || ch == '_' || ch == '-';
        }),
        normalized.end());
    return normalized;
}

std::vector<std::string> parseAssertSpecialFlags(const MugenSection& section) {
    std::vector<std::string> flags;
    for (const auto& property : section.properties) {
        if (!startsWithNoCase(property.key, "flag")) {
            continue;
        }

        const std::string flag = normalizeAssertSpecialFlag(property.value);
        if (!flag.empty()
            && std::find(flags.begin(), flags.end(), flag) == flags.end()) {
            flags.push_back(flag);
        }
    }
    return flags;
}

std::optional<std::pair<MugenVariableRef, std::string>> parseDirectVariableAssignment(const MugenSection& section) {
    for (const auto& property : section.properties) {
        if (const auto target = parseMugenVariableRef(property.key)) {
            return std::pair<MugenVariableRef, std::string>{ *target, trim(property.value) };
        }
    }
    return std::nullopt;
}

std::optional<MugenVariableRef> parseVariableControllerTarget(const MugenSection& section) {
    if (const auto direct = parseDirectVariableAssignment(section)) {
        return direct->first;
    }
    if (const auto* fv = findProperty(section, "fv")) {
        const auto index = parsePlainIntValue(fv->value);
        if (index && *index >= 0) {
            return MugenVariableRef{ MugenVariableBank::FVar, *index };
        }
    }
    if (const auto* v = findProperty(section, "v")) {
        const auto index = parsePlainIntValue(v->value);
        if (index && *index >= 0) {
            return MugenVariableRef{ MugenVariableBank::Var, *index };
        }
    }
    return std::nullopt;
}

bool isSupportedMoveType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'I' || value == 'A' || value == 'H';
}

bool isSupportedPhysicsType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'S' || value == 'C' || value == 'A' || value == 'N';
}

std::vector<MugenDocument> loadCharacterStateDocuments(const CharacterFiles& files) {
    std::vector<MugenDocument> documents;
    documents.reserve(files.stateFiles.size());
    for (const auto& path : files.stateFiles) {
        documents.push_back(parseMugenTextFile(path));
    }
    return documents;
}

std::vector<std::string> loadVictoryQuotes(const CharacterFiles& files) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<std::string> quotes;
    for (const auto& document : documents) {
        for (const auto& section : document.sections) {
            if (!equalsNoCase(section.name, "Quotes")) {
                continue;
            }
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "victory")) {
                    continue;
                }
                const int index = parseIntValue(std::string(std::string_view(property.key).substr(7)), 0);
                if (index <= 0 || index > 99) {
                    continue;
                }
                if (quotes.size() < static_cast<size_t>(index)) {
                    quotes.resize(static_cast<size_t>(index));
                }
                quotes[static_cast<size_t>(index - 1)] = unquote(trim(property.value));
            }
        }
    }
    return quotes;
}

std::vector<StateDefinition> loadStateDefinitions(const CharacterFiles& files, const CharacterConstants& constants) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<StateDefinition> states;
    int nextSoundControllerId = 1;
    int nextCtrlControllerId = 1;
    int nextPosAddControllerId = 1;
    int nextChangeAnimControllerId = 1;
    int nextRuntimeControllerId = 1;

    for (const auto& cns : documents) {
        int currentStateIndex = -1;
        for (const auto& section : cns.sections) {
            const auto stateNo = parseStateNumber(section.name, "Statedef ");
            if (stateNo) {
                StateDefinition state;
                state.stateNo = *stateNo;
                state.stateType = parseStateChar(&section, "type", state.stateType);
                state.moveType = parseStateChar(&section, "movetype", state.moveType);
                state.physics = parseStateChar(&section, "physics", state.physics);
                if (const auto* anim = findProperty(section, "anim")) {
                    state.hasAnim = true;
                    state.anim = parseIntValue(anim->value, state.anim);
                } else {
                    state.anim = state.stateNo;
                }
                if (const auto* ctrl = findProperty(section, "ctrl")) {
                    state.ctrl = parseIntValue(ctrl->value, state.ctrl ? 1 : 0) != 0;
                }
                if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                    state.sprPriority = parseIntValue(sprPriority->value, state.sprPriority);
                }
                if (const auto* velset = findProperty(section, "velset")) {
                    if (const auto values = parseControllerFloatPairValue(velset->value, constants)) {
                        state.hasVelset = true;
                        state.velsetX = values->first;
                        state.velsetY = values->second;
                    }
                }
                states.push_back(std::move(state));
                currentStateIndex = static_cast<int>(states.size()) - 1;
                continue;
            }

            if (currentStateIndex < 0 || !startsWithNoCase(section.name, "State ")) {
                continue;
            }

            const auto* type = findProperty(section, "type");
            if (!type) {
                continue;
            }

            const std::string controllerType = trim(type->value);
            bool hasTriggerProperties = false;
            int triggerTime = -1;
            int triggerAnimElem = -1;
            bool requiresMoveContact = false;
            std::vector<AnimElemTimeCondition> animElemTimeConditions;
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "trigger")) {
                    continue;
                }
                hasTriggerProperties = true;
                const auto trigger = trim(property.value);
                for (const auto& clause : splitAndClauses(trigger)) {
                    if (const auto time = parseTimeTrigger(clause)) {
                        triggerTime = *time;
                    } else if (const auto elem = parseAnimElemTrigger(clause)) {
                        triggerAnimElem = *elem;
                    } else if (const auto condition = parseAnimElemTimeCondition(clause)) {
                        animElemTimeConditions.push_back(*condition);
                    } else if (triggerIsMoveContact(clause)) {
                        requiresMoveContact = true;
                    }
                }
            }
            const auto controllerTrigger = parseStateControllerTrigger(section);

            if (startsWithNoCase(controllerType, "ChangeState") || startsWithNoCase(controllerType, "SelfState")) {
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }

                const auto targetState = parsePlainIntValue(value->value);
                const std::string targetStateExpression = trim(value->value);
                const int parsedTargetState = targetState.value_or(0);

                bool animEndTrigger = false;
                for (const auto& property : section.properties) {
                    if (!startsWithNoCase(property.key, "trigger")) {
                        continue;
                    }
                    const auto trigger = trim(property.value);
                    if (equalsNoCase(stripOuterParens(trigger), "!animtime")) {
                        animEndTrigger = true;
                        continue;
                    }
                    if (!startsWithNoCase(trigger, "AnimTime")) {
                        continue;
                    }
                    if (const auto equals = trigger.find('='); equals != std::string::npos) {
                        animEndTrigger = parseIntValue(trim(std::string_view(trigger).substr(equals + 1)), 1) == 0;
                    }
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                if (animEndTrigger) {
                    state.hasAnimEndChangeState = true;
                    state.animEndChangeState = parsedTargetState;
                    state.animEndChangeStateExpression = targetStateExpression;
                    if (const auto* ctrl = findProperty(section, "ctrl")) {
                        state.hasAnimEndCtrl = true;
                        state.animEndCtrl = parseIntValue(ctrl->value, 0) != 0;
                    }
                    continue;
                }

                if (!controllerTrigger) {
                    continue;
                }

                StateChangeStateController changeState;
                changeState.id = nextRuntimeControllerId++;
                changeState.trigger = *controllerTrigger;
                changeState.targetState = parsedTargetState;
                changeState.targetStateExpression = targetStateExpression;
                if (const auto* ctrl = findProperty(section, "ctrl")) {
                    changeState.hasCtrl = true;
                    changeState.ctrl = parseIntValue(ctrl->value, 0) != 0;
                }
                state.changeStates.push_back(std::move(changeState));
                continue;
            }

        if (startsWithNoCase(controllerType, "PlaySnd")) {
            const auto* value = findProperty(section, "value");
            if (!value
                || (hasTriggerProperties && !controllerTrigger)
                || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }
            const auto sound = parseSoundValue(value->value);
            if (!sound) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateSoundController soundController;
            soundController.id = nextSoundControllerId++;
            if (controllerTrigger) {
                soundController.trigger = *controllerTrigger;
            }
            soundController.triggerTime = triggerTime;
            soundController.triggerAnimElem = triggerAnimElem;
            soundController.group = sound->group;
            soundController.index = sound->index;
            soundController.forceCommon = sound->forceCommon;
            if (const auto* channel = findProperty(section, "channel")) {
                soundController.channel = parseIntValue(channel->value, soundController.channel);
            }
            if (const auto* lowPriority = findProperty(section, "lowpriority")) {
                soundController.lowPriority = parseIntValue(lowPriority->value, 0) != 0;
            }
            if (const auto* volume = findProperty(section, "volume")) {
                soundController.gain = mugenVolumeToGain(volume->value);
            }
            if (const auto* loop = findProperty(section, "loop")) {
                soundController.loop = parseIntValue(loop->value, 0) != 0;
            }
            state.sounds.push_back(std::move(soundController));
            state.audioControllers.push_back(StateAudioControllerRef{
                StateAudioControllerKind::PlaySnd,
                state.sounds.size() - 1,
            });
            continue;
        }

        if (startsWithNoCase(controllerType, "StopSnd")) {
            const auto* channel = findProperty(section, "channel");
            if (!channel
                || (hasTriggerProperties && !controllerTrigger)
                || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }

            const int parsedChannel = parseIntValue(channel->value, -1);
            if (parsedChannel < 0) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateStopSoundController stopSound;
            stopSound.id = nextRuntimeControllerId++;
            if (controllerTrigger) {
                stopSound.trigger = *controllerTrigger;
            }
            stopSound.triggerTime = triggerTime;
            stopSound.triggerAnimElem = triggerAnimElem;
            stopSound.channel = parsedChannel;
            state.stopSounds.push_back(std::move(stopSound));
            state.audioControllers.push_back(StateAudioControllerRef{
                StateAudioControllerKind::StopSnd,
                state.stopSounds.size() - 1,
            });
            continue;
        }

        if (startsWithNoCase(controllerType, "CtrlSet")) {
            const auto* value = findProperty(section, "value");
            if (!value || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateCtrlController ctrlSet;
            ctrlSet.id = nextCtrlControllerId++;
            if (controllerTrigger) {
                ctrlSet.trigger = *controllerTrigger;
            }
            ctrlSet.triggerTime = triggerTime;
            ctrlSet.triggerAnimElem = triggerAnimElem;
            ctrlSet.value = parseIntValue(value->value, 0) != 0;
            state.ctrlSets.push_back(std::move(ctrlSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "VarSet")
            || startsWithNoCase(controllerType, "VarAdd")
            || startsWithNoCase(controllerType, "VarRandom")) {
            if (!controllerTrigger) {
                continue;
            }

            const auto target = parseVariableControllerTarget(section);
            if (!target) {
                continue;
            }

            StateVariableController variable;
            variable.id = nextRuntimeControllerId++;
            variable.trigger = *controllerTrigger;
            variable.target = *target;
            if (startsWithNoCase(controllerType, "VarAdd")) {
                variable.operation = StateVariableOperation::Add;
            } else if (startsWithNoCase(controllerType, "VarRandom")) {
                variable.operation = StateVariableOperation::Random;
            } else {
                variable.operation = StateVariableOperation::Set;
            }

            if (variable.operation == StateVariableOperation::Random) {
                const auto* range = findProperty(section, "range");
                if (!range) {
                    continue;
                }
                const auto parts = splitCsv(range->value);
                if (parts.empty()) {
                    continue;
                }
                variable.rangeMinExpression = trim(parts[0]);
                variable.rangeMaxExpression = parts.size() >= 2 ? trim(parts[1]) : variable.rangeMinExpression;
            } else if (const auto direct = parseDirectVariableAssignment(section)) {
                variable.valueExpression = direct->second;
            } else if (const auto* value = findProperty(section, "value")) {
                variable.valueExpression = trim(value->value);
            } else {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.variableControllers.push_back(std::move(variable));
            continue;
        }

        if (startsWithNoCase(controllerType, "VelSet")
            || startsWithNoCase(controllerType, "VelAdd")
            || startsWithNoCase(controllerType, "VelMul")) {
            if (!controllerTrigger) {
                continue;
            }

            StateVelocityController velocity;
            velocity.id = nextRuntimeControllerId++;
            velocity.trigger = *controllerTrigger;
            if (startsWithNoCase(controllerType, "VelAdd")) {
                velocity.operation = StateVelocityOperation::Add;
            } else if (startsWithNoCase(controllerType, "VelMul")) {
                velocity.operation = StateVelocityOperation::Mul;
            } else {
                velocity.operation = StateVelocityOperation::Set;
            }
            if (const auto x = parseControllerFloatProperty(section, "x", constants)) {
                velocity.hasX = true;
                velocity.x = *x;
            }
            if (const auto y = parseControllerFloatProperty(section, "y", constants)) {
                velocity.hasY = true;
                velocity.y = *y;
            }
            if (!velocity.hasX && !velocity.hasY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.velocityControllers.push_back(std::move(velocity));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePosSetController posSet;
            posSet.id = nextRuntimeControllerId++;
            posSet.trigger = *controllerTrigger;
            if (const auto x = parseControllerFloatProperty(section, "x", constants)) {
                posSet.hasX = true;
                posSet.x = *x;
            }
            if (const auto y = parseControllerFloatProperty(section, "y", constants)) {
                posSet.hasY = true;
                posSet.y = *y;
            }
            if (!posSet.hasX && !posSet.hasY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.posSets.push_back(std::move(posSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "StateTypeSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StateTypeSetController typeSet;
            typeSet.id = nextRuntimeControllerId++;
            typeSet.trigger = *controllerTrigger;
            if (const auto stateType = parseControllerCharProperty(section, "statetype")) {
                if (isSupportedStateType(*stateType)) {
                    typeSet.hasStateType = true;
                    typeSet.stateType = *stateType;
                }
            }
            if (const auto moveType = parseControllerCharProperty(section, "movetype")) {
                if (isSupportedMoveType(*moveType)) {
                    typeSet.hasMoveType = true;
                    typeSet.moveType = *moveType;
                }
            }
            if (const auto physics = parseControllerCharProperty(section, "physics")) {
                if (isSupportedPhysicsType(*physics)) {
                    typeSet.hasPhysics = true;
                    typeSet.physics = *physics;
                }
            }
            if (!typeSet.hasStateType && !typeSet.hasMoveType && !typeSet.hasPhysics) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.stateTypeSets.push_back(std::move(typeSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "ScreenBound")) {
            if (!controllerTrigger) {
                continue;
            }

            StateScreenBoundController screenBound;
            screenBound.id = nextRuntimeControllerId++;
            screenBound.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                screenBound.value = parseIntValue(value->value, screenBound.value ? 1 : 0) != 0;
            }
            if (const auto* moveCamera = findProperty(section, "movecamera")) {
                const auto values = parseIntPairValue(moveCamera->value);
                screenBound.moveCameraX = values.first != 0;
                screenBound.moveCameraY = values.second != 0;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.screenBounds.push_back(std::move(screenBound));
            continue;
        }

        if (startsWithNoCase(controllerType, "Width")) {
            if (!controllerTrigger) {
                continue;
            }

            StateWidthController width;
            width.id = nextRuntimeControllerId++;
            width.trigger = *controllerTrigger;
            const bool hasExplicitEdge = findProperty(section, "edge") != nullptr;
            const bool hasExplicitPlayer = findProperty(section, "player") != nullptr;

            if (const auto edge = parseControllerFloatPairProperty(section, "edge", constants)) {
                width.hasEdge = true;
                width.edgeFront = edge->first;
                width.edgeBack = edge->second;
            }
            if (const auto player = parseControllerFloatPairProperty(section, "player", constants)) {
                width.hasPlayer = true;
                width.playerFront = player->first;
                width.playerBack = player->second;
            }
            if (!hasExplicitEdge && !hasExplicitPlayer) {
                if (const auto value = parseControllerFloatPairProperty(section, "value", constants)) {
                    width.hasEdge = true;
                    width.edgeFront = value->first;
                    width.edgeBack = value->second;
                    width.hasPlayer = true;
                    width.playerFront = value->first;
                    width.playerBack = value->second;
                }
            }
            if (!width.hasEdge && !width.hasPlayer) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.widths.push_back(std::move(width));
            continue;
        }

        if (startsWithNoCase(controllerType, "PlayerPush")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePlayerPushController playerPush;
            playerPush.id = nextRuntimeControllerId++;
            playerPush.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                playerPush.value = parseIntValue(value->value, playerPush.value ? 1 : 0) != 0;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.playerPushes.push_back(std::move(playerPush));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallDamage")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallDamageController hitFallDamage;
            hitFallDamage.id = nextRuntimeControllerId++;
            hitFallDamage.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallDamages.push_back(std::move(hitFallDamage));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallVel")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallVelController hitFallVel;
            hitFallVel.id = nextRuntimeControllerId++;
            hitFallVel.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallVels.push_back(std::move(hitFallVel));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallSetController hitFallSet;
            hitFallSet.id = nextRuntimeControllerId++;
            hitFallSet.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                hitFallSet.value = parseIntValue(value->value, hitFallSet.value);
            }
            if (const auto xvel = parseControllerFloatProperty(section, "xvel", constants)) {
                hitFallSet.hasXVelocity = true;
                hitFallSet.xVelocity = *xvel;
            }
            if (const auto yvel = parseControllerFloatProperty(section, "yvel", constants)) {
                hitFallSet.hasYVelocity = true;
                hitFallSet.yVelocity = *yvel;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallSets.push_back(std::move(hitFallSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "SprPriority")) {
            if (!controllerTrigger) {
                continue;
            }

            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }

            StateSprPriorityController sprPriority;
            sprPriority.id = nextRuntimeControllerId++;
            sprPriority.trigger = *controllerTrigger;
            sprPriority.value = parseIntValue(value->value, 0);

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.sprPriorities.push_back(std::move(sprPriority));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosFreeze")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePosFreezeController posFreeze;
            posFreeze.id = nextRuntimeControllerId++;
            posFreeze.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                posFreeze.freezeX = parseIntValue(x->value, 0) != 0;
            }
            if (const auto* y = findProperty(section, "y")) {
                posFreeze.freezeY = parseIntValue(y->value, 0) != 0;
            }
            if (!posFreeze.freezeX && !posFreeze.freezeY) {
                posFreeze.freezeX = true;
                posFreeze.freezeY = true;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.posFreezes.push_back(std::move(posFreeze));
            continue;
        }

            if (startsWithNoCase(controllerType, "HitVelSet")) {
                if (!controllerTrigger) {
                    continue;
                }

            StateHitVelSetController hitVelSet;
            hitVelSet.id = nextRuntimeControllerId++;
            hitVelSet.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                hitVelSet.applyX = parseIntValue(x->value, 0) != 0;
            }
            if (const auto* y = findProperty(section, "y")) {
                hitVelSet.applyY = parseIntValue(y->value, 0) != 0;
            }
            if (!hitVelSet.applyX && !hitVelSet.applyY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.hitVelSets.push_back(std::move(hitVelSet));
                continue;
            }

            if (startsWithNoCase(controllerType, "NotHitBy") || startsWithNoCase(controllerType, "HitBy")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    value = findProperty(section, "value2");
                }
                if (!value) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateHitProtectionController protection;
                protection.id = nextRuntimeControllerId++;
                protection.trigger = *controllerTrigger;
                protection.notHitBy = startsWithNoCase(controllerType, "NotHitBy");
                protection.value = trim(value->value);
                if (const auto* time = findProperty(section, "time")) {
                    protection.time = std::max(1, parseIntValue(time->value, protection.time));
                }
                state.hitProtections.push_back(std::move(protection));
                continue;
            }

            if (startsWithNoCase(controllerType, "HitOverride")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* attr = findProperty(section, "attr");
                const auto* stateNo = findProperty(section, "stateno");
                if (!attr || !stateNo) {
                    continue;
                }
                const auto parsedState = parsePlainIntValue(stateNo->value);
                if (!parsedState) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateHitOverrideController overrideController;
                overrideController.id = nextRuntimeControllerId++;
                overrideController.trigger = *controllerTrigger;
                overrideController.attr = trim(attr->value);
                overrideController.stateNo = *parsedState;
                if (const auto* time = findProperty(section, "time")) {
                    overrideController.time = parseIntValue(time->value, overrideController.time);
                }
                state.hitOverrides.push_back(std::move(overrideController));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetState")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                const auto parsedValue = parsePlainIntValue(value->value);
                if (!parsedValue) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetStateController targetState;
                targetState.id = nextRuntimeControllerId++;
                targetState.trigger = *controllerTrigger;
                targetState.value = *parsedValue;
                if (const auto* id = findProperty(section, "id")) {
                    targetState.targetId = parseIntValue(id->value, targetState.targetId);
                }
                state.targetStates.push_back(std::move(targetState));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetBind")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetBindController targetBind;
                targetBind.id = nextRuntimeControllerId++;
                targetBind.trigger = *controllerTrigger;
                if (const auto pos = parseControllerFloatPairProperty(section, "pos", constants)) {
                    targetBind.x = pos->first;
                    targetBind.y = pos->second;
                }
                if (const auto* time = findProperty(section, "time")) {
                    targetBind.time = parseIntValue(time->value, targetBind.time);
                }
                if (const auto* id = findProperty(section, "id")) {
                    targetBind.targetId = parseIntValue(id->value, targetBind.targetId);
                }
                state.targetBinds.push_back(std::move(targetBind));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetDrop")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetDropController targetDrop;
                targetDrop.id = nextRuntimeControllerId++;
                targetDrop.trigger = *controllerTrigger;
                if (const auto* excludeId = findProperty(section, "excludeID")) {
                    targetDrop.excludeId = parseIntValue(excludeId->value, targetDrop.excludeId);
                }
                state.targetDrops.push_back(std::move(targetDrop));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetFacing")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetFacingController targetFacing;
                targetFacing.id = nextRuntimeControllerId++;
                targetFacing.trigger = *controllerTrigger;
                targetFacing.value = parseIntValue(value->value, targetFacing.value);
                if (const auto* id = findProperty(section, "id")) {
                    targetFacing.targetId = parseIntValue(id->value, targetFacing.targetId);
                }
                state.targetFacings.push_back(std::move(targetFacing));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetLifeAdd")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetLifeAddController targetLifeAdd;
                targetLifeAdd.id = nextRuntimeControllerId++;
                targetLifeAdd.trigger = *controllerTrigger;
                targetLifeAdd.valueExpression = trim(value->value);
                if (const auto* id = findProperty(section, "id")) {
                    targetLifeAdd.targetId = parseIntValue(id->value, targetLifeAdd.targetId);
                }
                if (const auto* kill = findProperty(section, "kill")) {
                    targetLifeAdd.kill = parseIntValue(kill->value, targetLifeAdd.kill ? 1 : 0) != 0;
                }
                if (const auto* absolute = findProperty(section, "absolute")) {
                    targetLifeAdd.absolute = parseIntValue(absolute->value, targetLifeAdd.absolute ? 1 : 0) != 0;
                }
                state.targetLifeAdds.push_back(std::move(targetLifeAdd));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetPowerAdd")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetPowerAddController targetPowerAdd;
                targetPowerAdd.id = nextRuntimeControllerId++;
                targetPowerAdd.trigger = *controllerTrigger;
                targetPowerAdd.valueExpression = trim(value->value);
                if (const auto* id = findProperty(section, "id")) {
                    targetPowerAdd.targetId = parseIntValue(id->value, targetPowerAdd.targetId);
                }
                state.targetPowerAdds.push_back(std::move(targetPowerAdd));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetVelAdd") || startsWithNoCase(controllerType, "TargetVelSet")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetVelocityController targetVelocity;
                targetVelocity.id = nextRuntimeControllerId++;
                targetVelocity.trigger = *controllerTrigger;
                targetVelocity.add = startsWithNoCase(controllerType, "TargetVelAdd");
                if (const auto* x = findProperty(section, "x")) {
                    targetVelocity.xExpression = trim(x->value);
                    targetVelocity.hasX = true;
                }
                if (const auto* y = findProperty(section, "y")) {
                    targetVelocity.yExpression = trim(y->value);
                    targetVelocity.hasY = true;
                }
                if (const auto* id = findProperty(section, "id")) {
                    targetVelocity.targetId = parseIntValue(id->value, targetVelocity.targetId);
                }
                if (!targetVelocity.hasX && !targetVelocity.hasY) {
                    continue;
                }
                state.targetVelocities.push_back(std::move(targetVelocity));
                continue;
            }

            if (startsWithNoCase(controllerType, "Turn")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTurnController turn;
                turn.id = nextRuntimeControllerId++;
                turn.trigger = *controllerTrigger;
                state.turns.push_back(std::move(turn));
                continue;
            }

            if (equalsNoCase(controllerType, "Pause") || equalsNoCase(controllerType, "SuperPause")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StatePauseController pause;
                pause.id = nextRuntimeControllerId++;
                pause.trigger = *controllerTrigger;
                pause.superPause = equalsNoCase(controllerType, "SuperPause");
                pause.time = pause.superPause ? 30 : 1;
                if (const auto* time = findProperty(section, "time")) {
                    pause.time = std::max(1, parseIntValue(time->value, pause.time));
                }
                if (const auto* moveTime = findProperty(section, "movetime")) {
                    pause.moveTime = std::max(0, parseIntValue(moveTime->value, pause.moveTime));
                }
                if (const auto* sound = findProperty(section, "sound")) {
                    if (const auto parsed = parseSoundValue(sound->value)) {
                        pause.soundGroup = parsed->group;
                        pause.soundIndex = parsed->index;
                        pause.soundForceCommon = parsed->forceCommon;
                    }
                }
                state.pauses.push_back(std::move(pause));
                continue;
            }

            if (startsWithNoCase(controllerType, "Null")) {
                continue;
            }

            if (startsWithNoCase(controllerType, "PalFX") || startsWithNoCase(controllerType, "BGPalFX")) {
                if (!controllerTrigger) {
                    continue;
                }

                StatePaletteEffectController paletteEffect;
                paletteEffect.id = nextRuntimeControllerId++;
                paletteEffect.trigger = *controllerTrigger;
                paletteEffect.background = startsWithNoCase(controllerType, "BGPalFX");
                paletteEffect.effect = parsePaletteEffectSpec(section);
                if (!paletteEffect.effect.enabled) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.paletteEffects.push_back(std::move(paletteEffect));
                continue;
            }

            if (startsWithNoCase(controllerType, "EnvColor")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateEnvColorController envColor;
                envColor.id = nextRuntimeControllerId++;
                envColor.trigger = *controllerTrigger;
                if (const auto* value = findProperty(section, "value")) {
                    const auto values = parseIntTripleValue(value->value, 255, 255, 255);
                    envColor.r = values[0];
                    envColor.g = values[1];
                    envColor.b = values[2];
                }
                if (const auto* time = findProperty(section, "time")) {
                    envColor.time = std::max(0, parseIntValue(time->value, envColor.time));
                }
                if (envColor.time <= 0) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.envColors.push_back(std::move(envColor));
                continue;
            }

            if (startsWithNoCase(controllerType, "EnvShake")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateEnvShakeController envShake;
                envShake.id = nextRuntimeControllerId++;
                envShake.trigger = *controllerTrigger;
                envShake.shake = parseEnvShakeSpec(section);
                if (!envShake.shake.enabled) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.envShakes.push_back(std::move(envShake));
                continue;
            }

            if (startsWithNoCase(controllerType, "FallEnvShake")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateFallEnvShakeController fallEnvShake;
                fallEnvShake.id = nextRuntimeControllerId++;
                fallEnvShake.trigger = *controllerTrigger;

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.fallEnvShakes.push_back(std::move(fallEnvShake));
                continue;
            }

            if (startsWithNoCase(controllerType, "ModifyExplod")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateModifyExplodController modifyExplod;
                modifyExplod.id = nextRuntimeControllerId++;
                modifyExplod.trigger = *controllerTrigger;
                if (const auto* explodId = findProperty(section, "id")) {
                    modifyExplod.explodId = parseIntValue(explodId->value, -1);
                }
                if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                    modifyExplod.hasSprPriority = true;
                    modifyExplod.sprPriority = parseIntValue(sprPriority->value, 0);
                }
                if (const auto* scale = findProperty(section, "scale")) {
                    const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                    modifyExplod.hasScale = true;
                    modifyExplod.scaleX = values.first;
                    modifyExplod.scaleY = values.second;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.modifyExplods.push_back(std::move(modifyExplod));
                continue;
            }

            if (startsWithNoCase(controllerType, "RemoveExplod")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateRemoveExplodController removeExplod;
                removeExplod.id = nextRuntimeControllerId++;
                removeExplod.trigger = *controllerTrigger;
                if (const auto* explodId = findProperty(section, "id")) {
                    removeExplod.explodId = parseIntValue(explodId->value, -1);
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.removeExplods.push_back(std::move(removeExplod));
                continue;
            }

            if (startsWithNoCase(controllerType, "Explod")) {
                if (!controllerTrigger) {
                    continue;
                }

            const auto* anim = findProperty(section, "anim");
            if (!anim) {
                continue;
            }

            std::string animValue = trim(anim->value);
            StateExplodController explod;
            explod.id = nextRuntimeControllerId++;
            explod.trigger = *controllerTrigger;
            if (const auto* explodId = findProperty(section, "id")) {
                explod.explodId = parseIntValue(explodId->value, -1);
            }
            if (!animValue.empty() && (animValue.front() == 'F' || animValue.front() == 'f')) {
                explod.fromFightFx = true;
                animValue = trim(std::string_view(animValue).substr(1));
            }
            explod.anim = parseIntValue(animValue, -1);
            if (explod.anim < 0) {
                continue;
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                explod.x = values.first;
                explod.y = values.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                explod.postype = lowercaseCopy(trim(postype->value));
            }
            if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                explod.sprPriority = parseIntValue(sprPriority->value, 0);
            }
            if (const auto* bindTime = findProperty(section, "bindtime")) {
                explod.bindTime = parseIntValue(bindTime->value, explod.bindTime);
            }
            if (const auto* removeTime = findProperty(section, "removetime")) {
                explod.removeTime = parseIntValue(removeTime->value, explod.removeTime);
            }
            if (const auto* scale = findProperty(section, "scale")) {
                const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                explod.scaleX = values.first;
                explod.scaleY = values.second;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.explods.push_back(std::move(explod));
            continue;
        }

        if (startsWithNoCase(controllerType, "AssertSpecial")) {
            if (!controllerTrigger) {
                continue;
            }

            StateAssertSpecialController assertSpecial;
            assertSpecial.id = nextRuntimeControllerId++;
            assertSpecial.trigger = *controllerTrigger;
            assertSpecial.flags = parseAssertSpecialFlags(section);
            if (assertSpecial.flags.empty()) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.assertSpecials.push_back(std::move(assertSpecial));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosAdd")) {
            if (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StatePosAddController posAdd;
            posAdd.id = controllerTrigger ? nextRuntimeControllerId++ : nextPosAddControllerId++;
            if (controllerTrigger) {
                posAdd.trigger = *controllerTrigger;
            }
            posAdd.triggerTime = triggerTime;
            posAdd.triggerAnimElem = triggerAnimElem;
            posAdd.x = findProperty(section, "x") ? parseFloatValue(findProperty(section, "x")->value, 0.0f) : 0.0f;
            posAdd.y = findProperty(section, "y") ? parseFloatValue(findProperty(section, "y")->value, 0.0f) : 0.0f;
            state.posAdds.push_back(std::move(posAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "ChangeAnim")) {
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateChangeAnimController changeAnim;
            changeAnim.id = controllerTrigger ? nextRuntimeControllerId++ : nextChangeAnimControllerId++;
            if (controllerTrigger) {
                changeAnim.trigger = *controllerTrigger;
            }
            changeAnim.triggerTime = triggerTime;
            changeAnim.triggerAnimElem = triggerAnimElem;
            changeAnim.value = parseIntValue(value->value, state.anim);
            changeAnim.elem = findProperty(section, "elem") ? parseIntValue(findProperty(section, "elem")->value, 1) : 1;
            changeAnim.requiresMoveContact = requiresMoveContact;
            changeAnim.animElemTimeConditions = std::move(animElemTimeConditions);
            state.changeAnims.push_back(std::move(changeAnim));
        }

        if (startsWithNoCase(controllerType, "Helper")) {
            const auto* stateNo = findProperty(section, "stateno");
            if (!controllerTrigger || !stateNo) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateHelperController helper;
            helper.id = nextRuntimeControllerId++;
            helper.trigger = *controllerTrigger;
            helper.stateNo = parseIntValue(stateNo->value, 0);
            helper.helperId = findProperty(section, "id") ? parseIntValue(findProperty(section, "id")->value, helper.stateNo) : helper.stateNo;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                helper.x = values.first;
                helper.y = values.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                helper.postype = lowercaseCopy(trim(postype->value));
            }
            if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                helper.sprPriority = parseIntValue(sprPriority->value, helper.sprPriority);
            }
            if (const auto* facing = findProperty(section, "facing")) {
                helper.facing = parseIntValue(facing->value, helper.facing);
            }
            state.helpers.push_back(std::move(helper));
            continue;
        }

        if (startsWithNoCase(controllerType, "DestroySelf")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateDestroySelfController destroySelf;
            destroySelf.id = nextRuntimeControllerId++;
            destroySelf.trigger = *controllerTrigger;
            state.destroySelfs.push_back(std::move(destroySelf));
            continue;
        }

        if (startsWithNoCase(controllerType, "BindToParent")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateBindToParentController bind;
            bind.id = nextRuntimeControllerId++;
            bind.trigger = *controllerTrigger;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                bind.x = values.first;
                bind.y = values.second;
            }
            state.bindToParents.push_back(std::move(bind));
            continue;
        }

        if (startsWithNoCase(controllerType, "BindToRoot")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateBindToRootController bind;
            bind.id = nextRuntimeControllerId++;
            bind.trigger = *controllerTrigger;
            if (const auto* time = findProperty(section, "time")) {
                bind.time = std::max(1, parseIntValue(time->value, bind.time));
            }
            if (const auto* facing = findProperty(section, "facing")) {
                bind.facing = std::clamp(parseIntValue(facing->value, bind.facing), -1, 1);
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                bind.x = values.first;
                bind.y = values.second;
            }
            state.bindToRoots.push_back(std::move(bind));
            continue;
        }

        if (startsWithNoCase(controllerType, "ParentVarAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto assignment = parseDirectVariableAssignment(section);
            if (!assignment) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateParentVarAddController parentVarAdd;
            parentVarAdd.id = nextRuntimeControllerId++;
            parentVarAdd.trigger = *controllerTrigger;
            parentVarAdd.target = assignment->first;
            parentVarAdd.valueExpression = assignment->second;
            state.parentVarAdds.push_back(std::move(parentVarAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "VarRangeSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            const auto* fvalue = findProperty(section, "fvalue");
            if (!value && !fvalue) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateVarRangeSetController rangeSet;
            rangeSet.id = nextRuntimeControllerId++;
            rangeSet.trigger = *controllerTrigger;
            rangeSet.floatBank = fvalue != nullptr;
            rangeSet.valueExpression = trim(rangeSet.floatBank ? fvalue->value : value->value);
            rangeSet.first = findProperty(section, "first") ? parseIntValue(findProperty(section, "first")->value, 0) : 0;
            rangeSet.last = findProperty(section, "last")
                ? parseIntValue(findProperty(section, "last")->value, rangeSet.floatBank ? 39 : 59)
                : (rangeSet.floatBank ? 39 : 59);
            state.varRangeSets.push_back(std::move(rangeSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "PowerAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StatePowerAddController powerAdd;
            powerAdd.id = nextRuntimeControllerId++;
            powerAdd.trigger = *controllerTrigger;
            powerAdd.valueExpression = trim(value->value);
            state.powerAdds.push_back(std::move(powerAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "LifeAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateLifeAddController lifeAdd;
            lifeAdd.id = nextRuntimeControllerId++;
            lifeAdd.trigger = *controllerTrigger;
            lifeAdd.valueExpression = trim(value->value);
            if (const auto* kill = findProperty(section, "kill")) {
                lifeAdd.kill = parseIntValue(kill->value, 1) != 0;
            }
            state.lifeAdds.push_back(std::move(lifeAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateHitAddController hitAdd;
            hitAdd.id = nextRuntimeControllerId++;
            hitAdd.trigger = *controllerTrigger;
            hitAdd.valueExpression = trim(value->value);
            state.hitAdds.push_back(std::move(hitAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "AttackDist")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAttackDistController attackDist;
            attackDist.id = nextRuntimeControllerId++;
            attackDist.trigger = *controllerTrigger;
            attackDist.valueExpression = trim(value->value);
            state.attackDists.push_back(std::move(attackDist));
            continue;
        }

        if (startsWithNoCase(controllerType, "DefenceMulSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateDefenceMulSetController defenceMulSet;
            defenceMulSet.id = nextRuntimeControllerId++;
            defenceMulSet.trigger = *controllerTrigger;
            defenceMulSet.valueExpression = trim(value->value);
            state.defenceMulSets.push_back(std::move(defenceMulSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "AttackMulSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAttackMulSetController attackMulSet;
            attackMulSet.id = nextRuntimeControllerId++;
            attackMulSet.trigger = *controllerTrigger;
            attackMulSet.valueExpression = trim(value->value);
            state.attackMulSets.push_back(std::move(attackMulSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "AngleSet")
            || startsWithNoCase(controllerType, "AngleAdd")
            || startsWithNoCase(controllerType, "AngleMul")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            StateAngleController angle;
            angle.id = nextRuntimeControllerId++;
            angle.trigger = *controllerTrigger;
            angle.valueExpression = trim(value->value);
            if (startsWithNoCase(controllerType, "AngleAdd")) {
                angle.operation = StateAngleOperation::Add;
            } else if (startsWithNoCase(controllerType, "AngleMul")) {
                angle.operation = StateAngleOperation::Mul;
            } else {
                angle.operation = StateAngleOperation::Set;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.angleControllers.push_back(std::move(angle));
            continue;
        }

        if (startsWithNoCase(controllerType, "AngleDraw")) {
            if (!controllerTrigger) {
                continue;
            }
            StateAngleDrawController angleDraw;
            angleDraw.id = nextRuntimeControllerId++;
            angleDraw.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.angleDraws.push_back(std::move(angleDraw));
            continue;
        }

        if (startsWithNoCase(controllerType, "Offset")) {
            if (!controllerTrigger) {
                continue;
            }
            StateOffsetController offset;
            offset.id = nextRuntimeControllerId++;
            offset.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                offset.hasX = true;
                offset.xExpression = trim(x->value);
            }
            if (const auto* y = findProperty(section, "y")) {
                offset.hasY = true;
                offset.yExpression = trim(y->value);
            }
            if (!offset.hasX && !offset.hasY) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.offsets.push_back(std::move(offset));
            continue;
        }

        if (startsWithNoCase(controllerType, "ForceFeedback")) {
            if (!controllerTrigger) {
                continue;
            }
            StateForceFeedbackController forceFeedback;
            forceFeedback.id = nextRuntimeControllerId++;
            forceFeedback.trigger = *controllerTrigger;
            if (const auto* waveform = findProperty(section, "waveform")) {
                forceFeedback.waveform = lowercaseCopy(trim(waveform->value));
            }
            if (const auto* time = findProperty(section, "time")) {
                forceFeedback.time = std::max(0, parseIntValue(time->value, forceFeedback.time));
            }
            if (const auto* ampl = findProperty(section, "ampl")) {
                const auto values = splitCsv(ampl->value);
                if (!values.empty()) {
                    forceFeedback.amplitude = std::clamp(parseIntValue(values.front(), forceFeedback.amplitude), 0, 255);
                }
            }
            if (const auto* self = findProperty(section, "self")) {
                forceFeedback.self = parseIntValue(self->value, forceFeedback.self ? 1 : 0) != 0;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.forceFeedbacks.push_back(std::move(forceFeedback));
            continue;
        }

        if (startsWithNoCase(controllerType, "GameMakeAnim")) {
            if (!controllerTrigger) {
                continue;
            }
            StateGameMakeAnimController gameMakeAnim;
            gameMakeAnim.id = nextRuntimeControllerId++;
            gameMakeAnim.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                gameMakeAnim.valueExpression = trim(value->value);
            }
            if (const auto* under = findProperty(section, "under")) {
                gameMakeAnim.under = parseIntValue(under->value, 0) != 0;
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseFloatPairValue(pos->value);
                gameMakeAnim.x = values.first;
                gameMakeAnim.y = values.second;
            }
            if (const auto* random = findProperty(section, "random")) {
                gameMakeAnim.random = std::max(0, parseIntValue(random->value, 0));
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.gameMakeAnims.push_back(std::move(gameMakeAnim));
            continue;
        }

        if (startsWithNoCase(controllerType, "DisplayToClipboard")
            || startsWithNoCase(controllerType, "AppendToClipboard")) {
            if (!controllerTrigger) {
                continue;
            }
            StateClipboardController clipboard;
            clipboard.id = nextRuntimeControllerId++;
            clipboard.trigger = *controllerTrigger;
            clipboard.append = startsWithNoCase(controllerType, "AppendToClipboard");
            if (const auto* text = findProperty(section, "text")) {
                clipboard.text = unquote(trim(text->value));
            }
            if (const auto* params = findProperty(section, "params")) {
                clipboard.params = splitCsv(params->value);
                for (auto& param : clipboard.params) {
                    param = trim(param);
                }
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.clipboards.push_back(std::move(clipboard));
            continue;
        }

        if (startsWithNoCase(controllerType, "VictoryQuote")) {
            if (!controllerTrigger) {
                continue;
            }
            StateVictoryQuoteController victoryQuote;
            victoryQuote.id = nextRuntimeControllerId++;
            victoryQuote.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                victoryQuote.valueExpression = trim(value->value);
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.victoryQuotes.push_back(std::move(victoryQuote));
            continue;
        }

        if (startsWithNoCase(controllerType, "RemapPal")) {
            if (!controllerTrigger) {
                continue;
            }
            StateRemapPalController remapPal;
            remapPal.id = nextRuntimeControllerId++;
            remapPal.trigger = *controllerTrigger;
            if (const auto* source = findProperty(section, "source")) {
                const auto values = splitCsv(source->value);
                if (!values.empty()) {
                    remapPal.remap.sourceGroupExpression = trim(values[0]);
                }
                if (values.size() >= 2) {
                    remapPal.remap.sourceItemExpression = trim(values[1]);
                }
            }
            if (const auto* dest = findProperty(section, "dest")) {
                const auto values = splitCsv(dest->value);
                if (!values.empty()) {
                    remapPal.remap.destGroupExpression = trim(values[0]);
                }
                if (values.size() >= 2) {
                    remapPal.remap.destItemExpression = trim(values[1]);
                }
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.remapPals.push_back(std::move(remapPal));
            continue;
        }

        if (startsWithNoCase(controllerType, "Trans")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateTransController trans;
            trans.id = nextRuntimeControllerId++;
            trans.trigger = *controllerTrigger;
            if (const auto* transValue = findProperty(section, "trans")) {
                trans.trans = lowercaseCopy(trim(transValue->value));
            }
            if (const auto* alpha = findProperty(section, "alpha")) {
                const auto values = splitCsv(alpha->value);
                if (!values.empty()) {
                    trans.alphaSourceExpression = values[0];
                }
                if (values.size() >= 2) {
                    trans.alphaDestExpression = values[1];
                }
            }
            state.transControllers.push_back(std::move(trans));
            continue;
        }

        if (startsWithNoCase(controllerType, "AfterImageTime")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* time = findProperty(section, "time");
            if (!time) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAfterImageTimeController afterImageTime;
            afterImageTime.id = nextRuntimeControllerId++;
            afterImageTime.trigger = *controllerTrigger;
            afterImageTime.timeExpression = trim(time->value);
            state.afterImageTimes.push_back(std::move(afterImageTime));
            continue;
        }

        if (startsWithNoCase(controllerType, "AfterImage")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAfterImageController afterImage;
            afterImage.id = nextRuntimeControllerId++;
            afterImage.trigger = *controllerTrigger;
            if (const auto* time = findProperty(section, "time")) {
                afterImage.time = parseIntValue(time->value, afterImage.time);
            }
            if (const auto* length = findProperty(section, "length")) {
                afterImage.length = std::max(1, parseIntValue(length->value, afterImage.length));
            }
            if (const auto* timeGap = findProperty(section, "timegap")) {
                afterImage.timeGap = std::max(1, parseIntValue(timeGap->value, afterImage.timeGap));
            }
            if (const auto* frameGap = findProperty(section, "framegap")) {
                afterImage.frameGap = std::max(1, parseIntValue(frameGap->value, afterImage.frameGap));
            }
            if (const auto* trans = findProperty(section, "trans")) {
                const std::string mode = lowercaseCopy(trim(trans->value));
                afterImage.additive = mode.find("add") != std::string::npos;
            }
            state.afterImages.push_back(std::move(afterImage));
            continue;
        }

        if (startsWithNoCase(controllerType, "Projectile")) {
            if (!controllerTrigger) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateProjectileController projectile;
            projectile.id = nextRuntimeControllerId++;
            projectile.trigger = *controllerTrigger;
            projectile.projectileId = findProperty(section, "projid") ? parseIntValue(findProperty(section, "projid")->value, 0) : projectile.id;
            projectile.anim = findProperty(section, "projanim") ? parseIntValue(findProperty(section, "projanim")->value, -1) : -1;
            projectile.hitAnim = findProperty(section, "projhitanim") ? parseIntValue(findProperty(section, "projhitanim")->value, projectile.anim) : projectile.anim;
            projectile.removeAnim = findProperty(section, "projremanim") ? parseIntValue(findProperty(section, "projremanim")->value, projectile.hitAnim) : projectile.hitAnim;
            projectile.cancelAnim = findProperty(section, "projcancelanim") ? parseIntValue(findProperty(section, "projcancelanim")->value, projectile.removeAnim) : projectile.removeAnim;
            projectile.hits = findProperty(section, "projhits") ? std::max(1, parseIntValue(findProperty(section, "projhits")->value, 1)) : 1;
            projectile.missTime = findProperty(section, "projmisstime") ? std::max(0, parseIntValue(findProperty(section, "projmisstime")->value, 0)) : 0;
            projectile.removeTime = findProperty(section, "projremovetime") ? parseIntValue(findProperty(section, "projremovetime")->value, -1) : -1;
            projectile.removeWhenHit = findProperty(section, "projremove") ? parseIntValue(findProperty(section, "projremove")->value, projectile.removeWhenHit) : projectile.removeWhenHit;
            projectile.pauseMoveTime = findProperty(section, "pausemovetime") ? std::max(0, parseIntValue(findProperty(section, "pausemovetime")->value, 0)) : 0;
            projectile.superMoveTime = findProperty(section, "supermovetime") ? std::max(0, parseIntValue(findProperty(section, "supermovetime")->value, projectile.pauseMoveTime)) : projectile.pauseMoveTime;
            if (const auto* priority = findProperty(section, "projpriority")) {
                const auto values = parseIntPairValue(priority->value, projectile.priority, projectile.cancelPriority);
                projectile.priority = values.first;
                projectile.cancelPriority = values.second;
            } else if (const auto* priority = findProperty(section, "priority")) {
                const auto values = parseIntPairValue(priority->value, projectile.priority, projectile.cancelPriority);
                projectile.priority = values.first;
                projectile.cancelPriority = values.second;
            }
            if (const auto* edgeBound = findProperty(section, "projedgebound")) {
                projectile.projEdgeBound = parseFloatValue(edgeBound->value, projectile.projEdgeBound);
            }
            if (const auto* screenBound = findProperty(section, "projscreenbound")) {
                projectile.projEdgeBound = parseFloatValue(screenBound->value, projectile.projEdgeBound);
            }
            if (const auto* stageBound = findProperty(section, "projstagebound")) {
                projectile.projStageBound = parseFloatValue(stageBound->value, projectile.projStageBound);
            }
            if (const auto* heightBound = findProperty(section, "projheightbound")) {
                const auto values = parseFloatPairValue(heightBound->value, projectile.projHeightBoundLow, projectile.projHeightBoundHigh);
                projectile.projHeightBoundLow = values.first;
                projectile.projHeightBoundHigh = values.second;
            }
            if (const auto* scale = findProperty(section, "projscale")) {
                const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                projectile.scaleX = values.first;
                projectile.scaleY = values.second;
                const auto expressions = parseExpressionPairValue(scale->value, "1", "1");
                projectile.scaleXExpression = expressions.first;
                projectile.scaleYExpression = expressions.second;
            }
            if (const auto* shadow = findProperty(section, "projshadow")) {
                const auto parts = splitCsv(shadow->value);
                if (!parts.empty() && parseIntValue(parts[0], 0) == -1) {
                    projectile.shadowEnabled = true;
                    projectile.shadowR = 96;
                    projectile.shadowG = 96;
                    projectile.shadowB = 96;
                } else {
                    const auto values = parseIntTripleValue(shadow->value, 0, 0, 0);
                    projectile.shadowEnabled = values[0] != 0 || values[1] != 0 || values[2] != 0;
                    projectile.shadowR = values[0];
                    projectile.shadowG = values[1];
                    projectile.shadowB = values[2];
                }
            }
            if (const auto* velocity = findProperty(section, "velocity")) {
                const auto values = parseFloatPairValue(velocity->value);
                projectile.vx = values.first;
                projectile.vy = values.second;
                const auto expressions = parseExpressionPairValue(velocity->value);
                projectile.vxExpression = expressions.first;
                projectile.vyExpression = expressions.second;
            }
            if (const auto* removeVelocity = findProperty(section, "remvelocity")) {
                const auto values = parseFloatPairValue(removeVelocity->value);
                projectile.removeVx = values.first;
                projectile.removeVy = values.second;
                const auto expressions = parseExpressionPairValue(removeVelocity->value);
                projectile.removeVxExpression = expressions.first;
                projectile.removeVyExpression = expressions.second;
            }
            if (const auto* accel = findProperty(section, "accel")) {
                const auto values = parseFloatPairValue(accel->value);
                projectile.ax = values.first;
                projectile.ay = values.second;
                const auto expressions = parseExpressionPairValue(accel->value);
                projectile.axExpression = expressions.first;
                projectile.ayExpression = expressions.second;
            }
            if (const auto* velMul = findProperty(section, "velmul")) {
                const auto values = parseFloatPairValue(velMul->value, 1.0f, 1.0f);
                projectile.velMulX = values.first;
                projectile.velMulY = values.second;
                const auto expressions = parseExpressionPairValue(velMul->value, "1", "1");
                projectile.velMulXExpression = expressions.first;
                projectile.velMulYExpression = expressions.second;
            }
            if (const auto* offset = findProperty(section, "offset")) {
                const auto values = parseFloatPairValue(offset->value);
                projectile.x = values.first;
                projectile.y = values.second;
                const auto expressions = parseExpressionPairValue(offset->value);
                projectile.xExpression = expressions.first;
                projectile.yExpression = expressions.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                projectile.postype = lowercaseCopy(trim(postype->value));
            }

            projectile.hitDef.id = projectile.id;
            projectile.hitDef.targetId = projectile.projectileId;
            projectile.hitDef.stateNo = state.stateNo;
            if (const auto* attr = findProperty(section, "attr")) {
                projectile.hitDef.attr = trim(attr->value);
            }
            if (const auto* damage = findProperty(section, "damage")) {
                const auto values = parseIntPairValue(damage->value);
                projectile.hitDef.damage = values.first;
                projectile.hitDef.guardDamage = values.second;
                const auto expressions = parseExpressionPairValue(damage->value);
                projectile.hitDef.damageExpression = expressions.first;
                projectile.hitDef.guardDamageExpression = expressions.second;
            }
            if (const auto* animtype = findProperty(section, "animtype")) {
                projectile.hitDef.animtype = trim(animtype->value);
            }
            if (const auto* hitflag = findProperty(section, "hitflag")) {
                projectile.hitDef.hitflag = trim(hitflag->value);
            }
            if (const auto* guardflag = findProperty(section, "guardflag")) {
                projectile.hitDef.guardflag = trim(guardflag->value);
            }
            if (const auto* guardDistance = findProperty(section, "guard.dist")) {
                projectile.hitDef.guardDistance = parseIntValue(guardDistance->value, projectile.hitDef.guardDistance);
            }
            if (const auto* pausetime = findProperty(section, "pausetime")) {
                const auto values = parseIntPairValue(pausetime->value);
                projectile.hitDef.pausetimeP1 = values.first;
                projectile.hitDef.pausetimeP2 = values.second;
                const auto expressions = parseExpressionPairValue(pausetime->value);
                projectile.hitDef.pausetimeP1Expression = expressions.first;
                projectile.hitDef.pausetimeP2Expression = expressions.second;
            }
            if (const auto* sparkNo = findProperty(section, "sparkno")) {
                projectile.hitDef.sparkNo = parseIntValue(sparkNo->value, projectile.hitDef.sparkNo);
                projectile.hitDef.sparkNoExpression = trim(sparkNo->value);
            }
            if (const auto* guardSparkNo = findProperty(section, "guard.sparkno")) {
                projectile.hitDef.guardSparkNo = parseIntValue(guardSparkNo->value, projectile.hitDef.guardSparkNo);
                projectile.hitDef.guardSparkNoExpression = trim(guardSparkNo->value);
            }
            if (const auto* sparkxy = findProperty(section, "sparkxy")) {
                const auto values = parseFloatPairValue(sparkxy->value);
                projectile.hitDef.sparkX = values.first;
                projectile.hitDef.sparkY = values.second;
                const auto expressions = parseExpressionPairValue(sparkxy->value);
                projectile.hitDef.sparkXExpression = expressions.first;
                projectile.hitDef.sparkYExpression = expressions.second;
            }
            if (const auto* hitSound = findProperty(section, "hitsound")) {
                if (const auto values = parseSoundValue(hitSound->value)) {
                    projectile.hitDef.hitSoundGroup = values->group;
                    projectile.hitDef.hitSoundIndex = values->index;
                    projectile.hitDef.hitSoundForceCommon = values->forceCommon;
                    projectile.hitDef.hitSoundGroupExpression = values->groupExpression;
                    projectile.hitDef.hitSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* guardSound = findProperty(section, "guardsound")) {
                if (const auto values = parseSoundValue(guardSound->value)) {
                    projectile.hitDef.guardSoundGroup = values->group;
                    projectile.hitDef.guardSoundIndex = values->index;
                    projectile.hitDef.guardSoundForceCommon = values->forceCommon;
                    projectile.hitDef.guardSoundGroupExpression = values->groupExpression;
                    projectile.hitDef.guardSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* groundType = findProperty(section, "ground.type")) {
                projectile.hitDef.groundType = trim(groundType->value);
            }
            if (const auto* groundSlideTime = findProperty(section, "ground.slidetime")) {
                projectile.hitDef.groundSlideTime = parseIntValue(groundSlideTime->value, projectile.hitDef.groundSlideTime);
                projectile.hitDef.groundSlideTimeExpression = trim(groundSlideTime->value);
            }
            if (const auto* groundHitTime = findProperty(section, "ground.hittime")) {
                projectile.hitDef.groundHitTime = parseIntValue(groundHitTime->value, projectile.hitDef.groundHitTime);
                projectile.hitDef.groundHitTimeExpression = trim(groundHitTime->value);
            }
            if (const auto* groundVelocity = findProperty(section, "ground.velocity")) {
                const auto values = parseFloatPairValue(groundVelocity->value);
                projectile.hitDef.groundVelocityX = values.first;
                projectile.hitDef.groundVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(groundVelocity->value);
                projectile.hitDef.groundVelocityXExpression = expressions.first;
                projectile.hitDef.groundVelocityYExpression = expressions.second;
            }
            if (const auto* airVelocity = findProperty(section, "air.velocity")) {
                const auto values = parseFloatPairValue(airVelocity->value);
                projectile.hitDef.hasAirVelocity = true;
                projectile.hitDef.airVelocityX = values.first;
                projectile.hitDef.airVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(airVelocity->value);
                projectile.hitDef.airVelocityXExpression = expressions.first;
                projectile.hitDef.airVelocityYExpression = expressions.second;
            }
            if (const auto* airHitTime = findProperty(section, "air.hittime")) {
                projectile.hitDef.airHitTime = parseIntValue(airHitTime->value, projectile.hitDef.airHitTime);
                projectile.hitDef.airHitTimeExpression = trim(airHitTime->value);
            }
            if (const auto* fall = findProperty(section, "fall")) {
                projectile.hitDef.fall = parseIntValue(fall->value, 0) != 0;
                projectile.hitDef.fallExpression = trim(fall->value);
            }
            if (const auto* airFall = findProperty(section, "air.fall")) {
                projectile.hitDef.airFall = parseIntValue(airFall->value, 0) != 0;
                projectile.hitDef.airFallExpression = trim(airFall->value);
            }
            if (const auto* fallRecover = findProperty(section, "fall.recover")) {
                projectile.hitDef.fallRecover = parseIntValue(fallRecover->value, 1) != 0;
                projectile.hitDef.fallRecoverExpression = trim(fallRecover->value);
            }
            if (const auto* fallRecoverTime = findProperty(section, "fall.recovertime")) {
                projectile.hitDef.fallRecoverTime = parseIntValue(fallRecoverTime->value, projectile.hitDef.fallRecoverTime);
                projectile.hitDef.fallRecoverTimeExpression = trim(fallRecoverTime->value);
            }
            if (const auto* fallDamage = findProperty(section, "fall.damage")) {
                projectile.hitDef.fallDamage = parseIntValue(fallDamage->value, 0);
                projectile.hitDef.fallDamageExpression = trim(fallDamage->value);
            }
            if (const auto* fallXVelocity = findProperty(section, "fall.xvelocity")) {
                projectile.hitDef.hasFallXVelocity = true;
                projectile.hitDef.fallXVelocity = parseFloatValue(fallXVelocity->value, projectile.hitDef.fallXVelocity);
                projectile.hitDef.fallXVelocityExpression = trim(fallXVelocity->value);
            }
            if (const auto* fallYVelocity = findProperty(section, "fall.yvelocity")) {
                projectile.hitDef.hasFallYVelocity = true;
                projectile.hitDef.fallYVelocity = parseFloatValue(fallYVelocity->value, projectile.hitDef.fallYVelocity);
                projectile.hitDef.fallYVelocityExpression = trim(fallYVelocity->value);
            }
            if (const auto* yAccel = findProperty(section, "yaccel")) {
                projectile.hitDef.hasYAccel = true;
                projectile.hitDef.yAccel = parseFloatValue(yAccel->value, 0.0f);
                projectile.hitDef.yAccelExpression = trim(yAccel->value);
            }
            if (const auto* guardVelocity = findProperty(section, "guard.velocity")) {
                const auto values = parseFloatPairValue(guardVelocity->value);
                projectile.hitDef.hasGuardVelocity = true;
                projectile.hitDef.guardVelocityX = values.first;
                projectile.hitDef.guardVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(guardVelocity->value);
                projectile.hitDef.guardVelocityXExpression = expressions.first;
                projectile.hitDef.guardVelocityYExpression = expressions.second;
            } else {
                projectile.hitDef.guardVelocityX = projectile.hitDef.groundVelocityX;
                projectile.hitDef.guardVelocityY = projectile.hitDef.groundVelocityY;
            }
            if (const auto* envShakeTime = findProperty(section, "envshake.time")) {
                projectile.hitDef.envShake.time = std::max(0, parseIntValue(envShakeTime->value, projectile.hitDef.envShake.time));
                projectile.hitDef.envShake.timeExpression = trim(envShakeTime->value);
                projectile.hitDef.envShake.enabled = projectile.hitDef.envShake.time > 0;
            }
            if (const auto* envShakeFrequency = findProperty(section, "envshake.freq")) {
                projectile.hitDef.envShake.frequency = std::max(1, parseIntValue(envShakeFrequency->value, projectile.hitDef.envShake.frequency));
                projectile.hitDef.envShake.frequencyExpression = trim(envShakeFrequency->value);
            }
            if (const auto* envShakeAmplitude = findProperty(section, "envshake.ampl")) {
                projectile.hitDef.envShake.amplitude = parseFloatValue(envShakeAmplitude->value, projectile.hitDef.envShake.amplitude);
                projectile.hitDef.envShake.amplitudeExpression = trim(envShakeAmplitude->value);
                projectile.hitDef.envShake.enabled = projectile.hitDef.envShake.time > 0 && std::abs(projectile.hitDef.envShake.amplitude) > 0.001f;
            }
            if (const auto* envShakePhase = findProperty(section, "envshake.phase")) {
                projectile.hitDef.envShake.phase = parseIntValue(envShakePhase->value, projectile.hitDef.envShake.phase);
                projectile.hitDef.envShake.phaseExpression = trim(envShakePhase->value);
            }
            if (const auto* fallEnvShakeTime = findProperty(section, "fall.envshake.time")) {
                projectile.hitDef.fallEnvShake.time = std::max(0, parseIntValue(fallEnvShakeTime->value, projectile.hitDef.fallEnvShake.time));
                projectile.hitDef.fallEnvShake.timeExpression = trim(fallEnvShakeTime->value);
                projectile.hitDef.fallEnvShake.enabled = projectile.hitDef.fallEnvShake.time > 0;
            }
            if (const auto* fallEnvShakeFrequency = findProperty(section, "fall.envshake.freq")) {
                projectile.hitDef.fallEnvShake.frequency = std::max(1, parseIntValue(fallEnvShakeFrequency->value, projectile.hitDef.fallEnvShake.frequency));
                projectile.hitDef.fallEnvShake.frequencyExpression = trim(fallEnvShakeFrequency->value);
            }
            if (const auto* fallEnvShakeAmplitude = findProperty(section, "fall.envshake.ampl")) {
                projectile.hitDef.fallEnvShake.amplitude = parseFloatValue(fallEnvShakeAmplitude->value, projectile.hitDef.fallEnvShake.amplitude);
                projectile.hitDef.fallEnvShake.amplitudeExpression = trim(fallEnvShakeAmplitude->value);
                projectile.hitDef.fallEnvShake.enabled = projectile.hitDef.fallEnvShake.time > 0 && std::abs(projectile.hitDef.fallEnvShake.amplitude) > 0.001f;
            }
            if (const auto* fallEnvShakePhase = findProperty(section, "fall.envshake.phase")) {
                projectile.hitDef.fallEnvShake.phase = parseIntValue(fallEnvShakePhase->value, projectile.hitDef.fallEnvShake.phase);
                projectile.hitDef.fallEnvShake.phaseExpression = trim(fallEnvShakePhase->value);
            }
            projectile.hitDef.palFx = parsePaletteEffectSpec(section);
            if (projectile.anim >= 0) {
                state.projectiles.push_back(std::move(projectile));
            }
            continue;
        }

        if (startsWithNoCase(controllerType, "MakeDust")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateMakeDustController makeDust;
            makeDust.id = nextRuntimeControllerId++;
            makeDust.trigger = *controllerTrigger;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                makeDust.x = values.first;
                makeDust.y = values.second;
            }
            if (const auto* pos2 = findProperty(section, "pos2")) {
                const auto values = parseExplodPosition(pos2->value, constants);
                makeDust.hasPos2 = true;
                makeDust.x2 = values.first;
                makeDust.y2 = values.second;
            }
            if (const auto* spacing = findProperty(section, "spacing")) {
                makeDust.spacing = std::max(1, parseIntValue(spacing->value, makeDust.spacing));
            }
            state.makeDusts.push_back(std::move(makeDust));
            continue;
        }
        }
    }

    return states;
}

