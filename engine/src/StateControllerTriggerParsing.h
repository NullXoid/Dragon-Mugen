#pragma once

// Internal App.cpp trigger parser implementation header.
// Depends on App.cpp-local CNS/state-controller parse types and helpers.
// Include only through StateControllerParsing.h after required types/helpers are defined.
// Do not include from other translation units.

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
        int depth = 0;
        bool inQuote = false;
        bool wrapsWholeExpression = true;
        for (size_t i = 0; i < value.size(); ++i) {
            const char ch = value[i];
            if (ch == '"') {
                inQuote = !inQuote;
                continue;
            }
            if (inQuote) {
                continue;
            }
            if (ch == '(') {
                ++depth;
            } else if (ch == ')') {
                --depth;
                if (depth == 0 && i + 1 < value.size()) {
                    wrapsWholeExpression = false;
                    break;
                }
                if (depth < 0) {
                    wrapsWholeExpression = false;
                    break;
                }
            }
        }
        if (!wrapsWholeExpression || depth != 0) {
            break;
        }
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
            if (op == "+" || op == "-") {
                size_t previous = i;
                while (previous > 0 && std::isspace(static_cast<unsigned char>(expression[previous - 1]))) {
                    --previous;
                }
                if (previous == 0) {
                    continue;
                }
                const char before = expression[previous - 1];
                if (before == 'e' || before == 'E'
                    || before == '(' || before == '[' || before == ','
                    || before == '+' || before == '-' || before == '*'
                    || before == '/' || before == '%' || before == '='
                    || before == '!' || before == '<' || before == '>'
                    || before == '&' || before == '|') {
                    continue;
                }
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
    constexpr std::array<std::string_view, 54> supportedTokens{
        "var(", "fvar(", "sysvar(", "sysfvar(", "random", "ailevel", "teammode", "time", "animtime",
        "stateno", "roundstate", "roundno", "roundsexisted", "power", "powermax", "hitcount", "life", "p2life", "statetype", "movetype", "ctrl",
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
    if (startsWithNoCase(lowered, "anim") && (lowered.size() == 4 || !isIdentifierChar(lowered[4]))) {
        subjectEnd = 4;
        return StateTriggerSubject::Anim;
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
    if (equalsNoCase(clause, "!hitshakeover")) {
        group.floatConditions.push_back(StateFloatCondition{ StateTriggerSubject::HitShakeOver, CompareOp::Equal, 0.0f });
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
        if (tail.empty()) {
            group.floatConditions.push_back(StateFloatCondition{ *subject, CompareOp::NotEqual, 0.0f });
            return true;
        }
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

void appendStateTriggerGroup(StateTriggerGroup& target, const StateTriggerGroup& source) {
    target.floatConditions.insert(target.floatConditions.end(), source.floatConditions.begin(), source.floatConditions.end());
    target.rangeConditions.insert(target.rangeConditions.end(), source.rangeConditions.begin(), source.rangeConditions.end());
    target.stateTypeConditions.insert(target.stateTypeConditions.end(), source.stateTypeConditions.begin(), source.stateTypeConditions.end());
    target.expressionConditions.insert(target.expressionConditions.end(), source.expressionConditions.begin(), source.expressionConditions.end());
    target.booleanExpressions.insert(target.booleanExpressions.end(), source.booleanExpressions.begin(), source.booleanExpressions.end());
    target.animElemTimeConditions.insert(target.animElemTimeConditions.end(), source.animElemTimeConditions.begin(), source.animElemTimeConditions.end());
    target.requiredCommands.insert(target.requiredCommands.end(), source.requiredCommands.begin(), source.requiredCommands.end());
    target.forbiddenCommands.insert(target.forbiddenCommands.end(), source.forbiddenCommands.begin(), source.forbiddenCommands.end());
    target.requiresMoveContact = target.requiresMoveContact || source.requiresMoveContact;
}

std::optional<std::vector<StateTriggerGroup>> parseStateTriggerClauseAlternatives(const std::string& clause) {
    const std::string trimmed = stripOuterParens(clause);
    const auto orClauses = splitTopLevelClauses(trimmed, "||", true);
    std::vector<StateTriggerGroup> alternatives;
    alternatives.reserve(std::max<size_t>(1, orClauses.size()));
    for (const auto& orClause : orClauses) {
        StateTriggerGroup group;
        if (!parseStateTriggerClause(orClause, group)) {
            return std::nullopt;
        }
        alternatives.push_back(std::move(group));
    }
    if (alternatives.empty()) {
        StateTriggerGroup group;
        if (!parseStateTriggerClause(trimmed, group)) {
            return std::nullopt;
        }
        alternatives.push_back(std::move(group));
    }
    return alternatives;
}

std::optional<std::vector<StateTriggerGroup>> parseStateTriggerExpressionGroups(const std::string& expression) {
    std::vector<StateTriggerGroup> groups;
    for (const auto& orClause : splitTopLevelClauses(expression, "||", true)) {
        std::vector<StateTriggerGroup> andGroups{ StateTriggerGroup{} };
        for (const auto& andClause : splitTopLevelClauses(orClause, "&&")) {
            const auto alternatives = parseStateTriggerClauseAlternatives(andClause);
            if (!alternatives) {
                return std::nullopt;
            }
            std::vector<StateTriggerGroup> expanded;
            expanded.reserve(andGroups.size() * alternatives->size());
            for (const auto& base : andGroups) {
                for (const auto& alternative : *alternatives) {
                    StateTriggerGroup combined = base;
                    appendStateTriggerGroup(combined, alternative);
                    expanded.push_back(std::move(combined));
                }
            }
            andGroups = std::move(expanded);
        }
        groups.insert(groups.end(), andGroups.begin(), andGroups.end());
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
            expression = "(" + expression + ") && (" + value + ")";
            return;
        }
    }
    expressions.push_back({ loweredKey, value });
}

StateControllerTrigger parseStateControllerTriggerOptions(const MugenSection& section) {
    StateControllerTrigger trigger;
    if (const auto* persistent = findProperty(section, "persistent")) {
        trigger.persistent = std::max(0, parseIntValue(persistent->value, 1));
    }
    if (const auto* ignoreHitPause = findProperty(section, "ignorehitpause")) {
        trigger.ignoreHitPause = parseIntValue(ignoreHitPause->value, 0) != 0;
    }
    return trigger;
}

std::optional<StateControllerTrigger> parseStateControllerTrigger(const MugenSection& section) {
    StateControllerTrigger trigger = parseStateControllerTriggerOptions(section);

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

