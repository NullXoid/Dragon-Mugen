#pragma once

// Internal App.cpp implementation shard.
// Training command token/display/guide helpers.

enum class CommandButtonPromptMode {
    Strength,
    Xbox,
    Playstation,
};

CommandButtonPromptMode commandButtonPromptModeForPlayer(const AppState& state, int playerIndex) {
    if (!assignedGamepad(state, playerIndex)) {
        return CommandButtonPromptMode::Strength;
    }
    return effectiveGamepadPromptStyle(state, playerIndex) == GamepadPromptStyle::Playstation
        ? CommandButtonPromptMode::Playstation
        : CommandButtonPromptMode::Xbox;
}

std::string commandButtonDisplayToken(
    std::string_view command,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    if (mode == CommandButtonPromptMode::Xbox) {
        if (command == "x") return "X";
        if (command == "y") return "Y";
        if (command == "z") return "LB";
        if (command == "a") return "A";
        if (command == "b") return "BTN_B";
        if (command == "c") return "RB";
        if (command == "s") return "MENU";
    }
    if (mode == CommandButtonPromptMode::Playstation) {
        if (command == "x") return "SQ";
        if (command == "y") return "TRI";
        if (command == "z") return "L1";
        if (command == "a") return "X";
        if (command == "b") return "O";
        if (command == "c") return "R1";
        if (command == "s") return "OPT";
    }

    if (command == "x") return "LP";
    if (command == "y") return "MP";
    if (command == "z") return "SP";
    if (command == "a") return "LK";
    if (command == "b") return "MK";
    if (command == "c") return "SK";
    if (command == "s") return "START";
    return {};
}

bool isCommandButtonSymbol(char ch) {
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(ch)))) {
    case 'x':
    case 'y':
    case 'z':
    case 'a':
    case 'b':
    case 'c':
    case 's':
        return true;
    default:
        return false;
    }
}

std::string commandButtonGroupDisplayToken(std::string_view group, CommandButtonPromptMode mode) {
    if (mode == CommandButtonPromptMode::Xbox) {
        if (group == "p") return "X/Y/LB";
        if (group == "k") return "A/BTN_B/RB";
    }
    if (mode == CommandButtonPromptMode::Playstation) {
        if (group == "p") return "SQ/TRI/L1";
        if (group == "k") return "X/O/R1";
    }
    if (group == "p") return "P";
    if (group == "k") return "K";
    return {};
}

std::string moveListTokenForCommand(
    std::string_view command,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    if (const std::string button = commandButtonDisplayToken(command, mode); !button.empty()) {
        return button;
    }
    if (command.size() == 2
        && isCommandButtonSymbol(command[0])
        && isCommandButtonSymbol(command[1])) {
        return commandButtonDisplayToken(std::string_view(command).substr(0, 1), mode)
            + "+" + commandButtonDisplayToken(std::string_view(command).substr(1, 1), mode);
    }
    if (command == "holddown") {
        return "DOWN";
    }
    if (command == "holdup") {
        return "UP";
    }
    if (command == "holdfwd") {
        return "FWD";
    }
    if (command == "holdback") {
        return "BACK";
    }
    if (startsWithNoCase(command, "hold_") && command.size() == 6) {
        if (const std::string button = commandButtonDisplayToken(std::string_view(command).substr(5, 1), mode); !button.empty()) {
            return "HOLD " + button;
        }
    }

    std::string label(command);
    std::replace(label.begin(), label.end(), '_', '+');
    if (label.size() >= 2 && label[label.size() - 2] == '+') {
        const std::string button(1, static_cast<char>(std::tolower(static_cast<unsigned char>(label.back()))));
        if (const std::string display = commandButtonDisplayToken(button, mode); !display.empty()) {
            label.replace(label.size() - 1, 1, display);
        } else if (button == "p" || button == "k") {
            label.replace(label.size() - 1, 1, commandButtonGroupDisplayToken(button, mode));
        }
    }
    return label;
}

std::string remapStrengthTokenForPrompt(std::string_view rawToken, CommandButtonPromptMode mode) {
    const std::string token = uppercaseCopy(trim(rawToken));
    if (token.empty()) {
        return {};
    }
    if (mode == CommandButtonPromptMode::Strength) {
        return token;
    }

    if (token == "LP") return commandButtonDisplayToken("x", mode);
    if (token == "MP") return commandButtonDisplayToken("y", mode);
    if (token == "SP") return commandButtonDisplayToken("z", mode);
    if (token == "LK") return commandButtonDisplayToken("a", mode);
    if (token == "MK") return commandButtonDisplayToken("b", mode);
    if (token == "SK") return commandButtonDisplayToken("c", mode);
    if (token == "P") return commandButtonGroupDisplayToken("p", mode);
    if (token == "K") return commandButtonGroupDisplayToken("k", mode);
    return token;
}

std::string remapDisplayInputForPrompt(std::string_view input, CommandButtonPromptMode mode) {
    if (mode == CommandButtonPromptMode::Strength) {
        return std::string(input);
    }

    std::string out;
    std::string token;
    const auto flushToken = [&]() {
        if (!token.empty()) {
            out += remapStrengthTokenForPrompt(token, mode);
            token.clear();
        }
    };

    for (const char ch : input) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            token.push_back(ch);
            continue;
        }
        flushToken();
        out.push_back(ch);
    }
    flushToken();
    return out;
}

bool commandListContains(const std::vector<std::string>& commands, std::string_view command) {
    return std::any_of(commands.begin(), commands.end(), [command](const std::string& current) {
        return current == command;
    });
}

std::string moveListInputText(
    const CommandStateEntry& entry,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    if (!entry.displayInput.empty()) {
        return remapDisplayInputForPrompt(entry.displayInput, mode);
    }

    std::vector<std::string> parts;
    const auto appendIfRequired = [&parts, &entry, mode](std::string_view command) {
        if (commandListContains(entry.requiredCommands, command)) {
            parts.push_back(moveListTokenForCommand(command, mode));
        }
    };

    for (const auto& optionGroup : entry.commandOptionGroups) {
        std::string groupText;
        for (const auto& option : optionGroup) {
            if (!groupText.empty()) {
                groupText += "/";
            }
            groupText += moveListTokenForCommand(option, mode);
        }
        if (!groupText.empty()) {
            parts.push_back(groupText);
        }
    }

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
    appendIfRequired("s");

    for (const auto& command : entry.requiredCommands) {
        if (command == "holddown"
            || command == "holdfwd"
            || command == "holdback"
            || command == "holdup"
            || command == "x"
            || command == "y"
            || command == "z"
            || command == "a"
            || command == "b"
            || command == "c"
            || command == "s") {
            continue;
        }
        parts.push_back(moveListTokenForCommand(command, mode));
    }

    if (parts.empty()) {
        return "-";
    }

    std::string text = parts.front();
    for (size_t i = 1; i < parts.size(); ++i) {
        text += "+";
        text += parts[i];
    }
    return text;
}

void accumulateCommandPowerRequirement(const CommandStateEntry::IntCondition& condition, int& requiredPower) {
    if (condition.subject != CommandConditionSubject::Power) {
        return;
    }
    if (condition.op == CompareOp::GreaterEqual) {
        requiredPower = std::max(requiredPower, condition.value);
    } else if (condition.op == CompareOp::Greater) {
        requiredPower = std::max(requiredPower, condition.value + 1);
    }
}

void accumulateCommandPowerRequirementFromExpression(const std::string& expression, int& requiredPower) {
    for (const auto& orClause : splitTopLevelClauses(expression, "||", true)) {
        for (const auto& andClause : splitTopLevelClauses(orClause, "&&")) {
            if (const auto condition = parseCommandIntCondition(andClause)) {
                accumulateCommandPowerRequirement(*condition, requiredPower);
            }
        }
    }
}

int commandEntryRequiredPower(const CommandStateEntry& entry) {
    int requiredPower = 0;
    for (const auto& condition : entry.intConditions) {
        accumulateCommandPowerRequirement(condition, requiredPower);
    }
    for (const auto& expression : entry.booleanExpressions) {
        accumulateCommandPowerRequirementFromExpression(expression, requiredPower);
    }
    return requiredPower;
}

TrainingMoveCategory commandEntryCategory(const CommandStateEntry& entry) {
    const int requiredPower = commandEntryRequiredPower(entry);
    if (requiredPower >= 1000) {
        return TrainingMoveCategory::Supers;
    }
    if (const auto targetState = parsePlainIntValue(entry.targetStateExpression)) {
        if (*targetState >= 3000) {
            return TrainingMoveCategory::Supers;
        }
        if (*targetState >= 200 && *targetState < 700) {
            return TrainingMoveCategory::Normals;
        }
    }
    return TrainingMoveCategory::Specials;
}

enum class TrainingMoveListSection {
    StandingNormals,
    CrouchingNormals,
    AirNormals,
    Specials,
    Supers,
    Throws,
    Counters,
};

int commandEntryTargetStateNumber(const CommandStateEntry& entry) {
    if (const auto targetState = parsePlainIntValue(entry.targetStateExpression)) {
        return *targetState;
    }
    return entry.targetState;
}

std::string commandEntryMoveListNameLower(const CommandStateEntry& entry) {
    return lowercaseCopy(entry.displayLabel.empty() ? entry.label : entry.displayLabel);
}

template <typename Fn>
bool anyCommandEntryCommand(const CommandStateEntry& entry, Fn&& fn) {
    for (const auto& command : entry.requiredCommands) {
        if (fn(command)) {
            return true;
        }
    }
    for (const auto& group : entry.commandOptionGroups) {
        for (const auto& command : group) {
            if (fn(command)) {
                return true;
            }
        }
    }
    return false;
}

bool commandEntryUsesCommand(const CommandStateEntry& entry, std::string_view wanted) {
    return anyCommandEntryCommand(entry, [wanted](const std::string& command) {
        return equalsNoCase(command, wanted);
    });
}

bool commandEntryLooksLikeCounter(const CommandStateEntry& entry) {
    const std::string label = commandEntryMoveListNameLower(entry);
    return label.find("counter") != std::string::npos
        || label.find("alpha") != std::string::npos
        || label.find("guard cancel") != std::string::npos;
}

bool commandEntryLooksLikeThrow(const CommandStateEntry& entry) {
    const std::string label = commandEntryMoveListNameLower(entry);
    if (label.find("throw") != std::string::npos) {
        return true;
    }
    const int targetState = commandEntryTargetStateNumber(entry);
    return targetState >= 800 && targetState < 1000;
}

bool commandEntryLooksAirNormal(const CommandStateEntry& entry) {
    const int targetState = commandEntryTargetStateNumber(entry);
    const std::string label = commandEntryMoveListNameLower(entry);
    return entry.requiredStateType == 'A'
        || (targetState >= 600 && targetState < 700)
        || label.find("air ") != std::string::npos
        || label.find("j.") != std::string::npos
        || label.find("jump") != std::string::npos;
}

bool commandEntryLooksCrouchingNormal(const CommandStateEntry& entry) {
    const int targetState = commandEntryTargetStateNumber(entry);
    const std::string label = commandEntryMoveListNameLower(entry);
    return entry.requiredStateType == 'C'
        || (targetState >= 400 && targetState < 500)
        || label.find("crouch") != std::string::npos
        || label.find("c.") != std::string::npos
        || commandEntryUsesCommand(entry, "holddown");
}

TrainingMoveListSection commandEntryMoveListSection(const CommandStateEntry& entry) {
    if (commandEntryLooksLikeCounter(entry)) {
        return TrainingMoveListSection::Counters;
    }
    if (commandEntryLooksLikeThrow(entry)) {
        return TrainingMoveListSection::Throws;
    }
    const TrainingMoveCategory category = commandEntryCategory(entry);
    if (category == TrainingMoveCategory::Supers) {
        return TrainingMoveListSection::Supers;
    }
    if (category == TrainingMoveCategory::Normals) {
        if (commandEntryLooksAirNormal(entry)) {
            return TrainingMoveListSection::AirNormals;
        }
        if (commandEntryLooksCrouchingNormal(entry)) {
            return TrainingMoveListSection::CrouchingNormals;
        }
        return TrainingMoveListSection::StandingNormals;
    }
    return TrainingMoveListSection::Specials;
}

int moveListSectionSortOrder(TrainingMoveListSection section) {
    switch (section) {
    case TrainingMoveListSection::StandingNormals:
        return 0;
    case TrainingMoveListSection::CrouchingNormals:
        return 1;
    case TrainingMoveListSection::AirNormals:
        return 2;
    case TrainingMoveListSection::Specials:
        return 3;
    case TrainingMoveListSection::Supers:
        return 4;
    case TrainingMoveListSection::Throws:
        return 5;
    case TrainingMoveListSection::Counters:
        return 6;
    default:
        return 3;
    }
}

std::string_view commandEntryMoveListSectionLabel(TrainingMoveListSection section) {
    switch (section) {
    case TrainingMoveListSection::StandingNormals:
        return "STANDING NORMAL";
    case TrainingMoveListSection::CrouchingNormals:
        return "CROUCHING NORMAL";
    case TrainingMoveListSection::AirNormals:
        return "AIR NORMAL";
    case TrainingMoveListSection::Specials:
        return "SPECIAL MOVE";
    case TrainingMoveListSection::Supers:
        return "SUPER MOVE";
    case TrainingMoveListSection::Throws:
        return "THROW";
    case TrainingMoveListSection::Counters:
        return "COUNTER";
    default:
        return "SPECIAL MOVE";
    }
}

std::string commandEntryMoveListSectionLabel(const CommandStateEntry& entry) {
    return std::string(commandEntryMoveListSectionLabel(commandEntryMoveListSection(entry)));
}

int commandEntryPrimaryButtonSortOrder(const CommandStateEntry& entry) {
    const auto buttonOrder = [](char button) {
        switch (static_cast<char>(std::tolower(static_cast<unsigned char>(button)))) {
        case 'x':
            return 0;
        case 'y':
            return 1;
        case 'z':
            return 2;
        case 'a':
            return 3;
        case 'b':
            return 4;
        case 'c':
            return 5;
        case 's':
            return 6;
        default:
            return 20;
        }
    };

    int best = 20;
    anyCommandEntryCommand(entry, [&](const std::string& command) {
        if (command.size() == 1) {
            best = std::min(best, buttonOrder(command.front()));
        }
        if (!command.empty()) {
            best = std::min(best, buttonOrder(command.back()));
        }
        return false;
    });
    return best;
}

int commandEntryMoveListSortKey(const CommandStateEntry& entry) {
    return moveListSectionSortOrder(commandEntryMoveListSection(entry)) * 100000
        + commandEntryPrimaryButtonSortOrder(entry) * 1000
        + std::clamp(commandEntryTargetStateNumber(entry), 0, 999);
}

bool commandEntryMatchesMoveCategory(const CommandStateEntry& entry, TrainingMoveCategory category) {
    if (category == TrainingMoveCategory::All) {
        return true;
    }
    return commandEntryCategory(entry) == category;
}

std::vector<const CommandStateEntry*> displayableMoveListEntries(const AppState& state) {
    std::vector<const CommandStateEntry*> entries;
    const auto sameDisplayMove = [](const CommandStateEntry& lhs, const CommandStateEntry& rhs) {
        return lhs.label == rhs.label
            && lhs.targetStateExpression == rhs.targetStateExpression
            && lhs.requiredCommands == rhs.requiredCommands
            && lhs.commandOptionGroups == rhs.commandOptionGroups;
    };
    const auto displayPriority = [](const CommandStateEntry& entry) {
        int priority = 0;
        if (entry.requiresMoveContact) {
            priority += 8;
        }
        if (commandEntryHasSelfStateNoGate(entry)) {
            priority += 4;
        }
        if (!entry.requiresCtrl) {
            priority += 1;
        }
        return priority;
    };
    for (const auto& entry : state.commandEntries) {
        if (entry.requiredCommands.empty() && entry.commandOptionGroups.empty()) {
            continue;
        }
        if (!commandEntryMatchesMoveCategory(entry, state.training.options.moveCategory)) {
            continue;
        }
        if (const auto literalTarget = parsePlainIntValue(entry.targetStateExpression)) {
            const StateDefinition* stateDef = findStateDefinition(state, *literalTarget);
            if (!stateDef || (stateDef->hasAnim && !findExactClip(state, stateDef->anim))) {
                continue;
            }
        }
        const auto duplicate = std::find_if(entries.begin(), entries.end(), [&entry, &sameDisplayMove](const CommandStateEntry* existing) {
            return existing && sameDisplayMove(*existing, entry);
        });
        if (duplicate != entries.end()) {
            if (displayPriority(entry) < displayPriority(**duplicate)) {
                *duplicate = &entry;
            }
            continue;
        }
        entries.push_back(&entry);
    }
    std::stable_sort(entries.begin(), entries.end(), [](const CommandStateEntry* lhs, const CommandStateEntry* rhs) {
        if (!lhs || !rhs) {
            return lhs != nullptr;
        }
        const int leftKey = commandEntryMoveListSortKey(*lhs);
        const int rightKey = commandEntryMoveListSortKey(*rhs);
        if (leftKey != rightKey) {
            return leftKey < rightKey;
        }
        return commandEntryMoveListNameLower(*lhs) < commandEntryMoveListNameLower(*rhs);
    });
    return entries;
}

bool commandEntryNameContains(const CommandStateEntry& entry, std::string_view needle) {
    const std::string label = lowercaseCopy(entry.displayLabel.empty() ? entry.label : entry.displayLabel);
    return label.find(lowercaseCopy(needle)) != std::string::npos;
}

bool commandEntryLooksLikeDebugUtility(const CommandStateEntry& entry) {
    const std::string label = lowercaseCopy(entry.displayLabel.empty() ? entry.label : entry.displayLabel);
    static constexpr std::array<std::string_view, 10> kDebugNeedles{
        "compatibility",
        "stress",
        "helper",
        "lifecycle",
        "bounds",
        "fallback",
        "demo",
        "test",
        "audit",
        "debug",
    };
    return std::any_of(kDebugNeedles.begin(), kDebugNeedles.end(), [&label](std::string_view needle) {
        return label.find(needle) != std::string::npos;
    });
}

bool commandEntryIsMainFallback(const CommandStateEntry& entry) {
    const TrainingMoveCategory category = commandEntryCategory(entry);
    if (category == TrainingMoveCategory::Specials || category == TrainingMoveCategory::Supers) {
        return true;
    }
    if (commandEntryRequiredPower(entry) > 0 || commandEntryNameContains(entry, "throw")) {
        return true;
    }
    if (entry.label.empty() || commandEntryLooksLikeDebugUtility(entry)) {
        return false;
    }
    return !entry.requiredCommands.empty() || !entry.commandOptionGroups.empty();
}

std::vector<const CommandStateEntry*> displayableMoveListEntriesForTab(const AppState& state, TrainingMoveListTab tab) {
    const auto entries = displayableMoveListEntries(state);
    if (tab == TrainingMoveListTab::All || entries.empty()) {
        return entries;
    }

    std::vector<const CommandStateEntry*> mainEntries;
    mainEntries.reserve(entries.size());
    bool hasPresentationEntries = false;
    for (const auto* entry : entries) {
        if (entry && entry->presentationOverride) {
            hasPresentationEntries = true;
            break;
        }
    }
    if (hasPresentationEntries) {
        for (const auto* entry : entries) {
            if (!entry) {
                continue;
            }
            const TrainingMoveListSection section = commandEntryMoveListSection(*entry);
            if (entry->presentationOverride
                || commandEntryCategory(*entry) == TrainingMoveCategory::Normals
                || section == TrainingMoveListSection::Throws
                || section == TrainingMoveListSection::Counters) {
                mainEntries.push_back(entry);
            }
        }
        return mainEntries;
    }

    for (const auto* entry : entries) {
        if (entry && commandEntryIsMainFallback(*entry)) {
            mainEntries.push_back(entry);
        }
    }
    return mainEntries.empty() ? entries : mainEntries;
}

const std::vector<const CommandStateEntry*>& activeDisplayableMoveListEntries(const AppState& state) {
    const CommandStateEntry* data = state.commandEntries.empty() ? nullptr : state.commandEntries.data();
    const size_t count = state.commandEntries.size();
    const TrainingMoveCategory category = state.training.options.moveCategory;
    const TrainingMoveListTab tab = state.training.options.moveListTab;
    if (!state.trainingMoveListCacheValid
        || state.trainingMoveListCacheData != data
        || state.trainingMoveListCacheCount != count
        || state.trainingMoveListCacheCategory != category
        || state.trainingMoveListCacheTab != tab) {
        state.trainingMoveListEntriesCache = displayableMoveListEntriesForTab(state, tab);
        state.trainingMoveListCacheData = data;
        state.trainingMoveListCacheCount = count;
        state.trainingMoveListCacheCategory = category;
        state.trainingMoveListCacheTab = tab;
        state.trainingMoveListCacheValid = true;
    }
    return state.trainingMoveListEntriesCache;
}

std::string joinTokens(const std::vector<std::string>& tokens, std::string_view separator) {
    if (tokens.empty()) {
        return "-";
    }
    std::string result = tokens.front();
    for (size_t i = 1; i < tokens.size(); ++i) {
        result += separator;
        result += tokens[i];
    }
    return result;
}

std::string inputDirectionToken(const FighterInputState& input, int facing) {
    const bool forward = facing >= 0 ? input.right : input.left;
    const bool back = facing >= 0 ? input.left : input.right;
    if (input.down && forward) {
        return "DF";
    }
    if (input.down && back) {
        return "DB";
    }
    if (input.up && forward) {
        return "UF";
    }
    if (input.up && back) {
        return "UB";
    }
    if (forward) {
        return "F";
    }
    if (back) {
        return "B";
    }
    if (input.down) {
        return "D";
    }
    if (input.up) {
        return "U";
    }
    return "";
}

std::string physicalInputDirectionToken(const FighterInputState& input) {
    if (input.down && input.right) {
        return "DF";
    }
    if (input.down && input.left) {
        return "DB";
    }
    if (input.up && input.right) {
        return "UF";
    }
    if (input.up && input.left) {
        return "UB";
    }
    if (input.right) {
        return "F";
    }
    if (input.left) {
        return "B";
    }
    if (input.down) {
        return "D";
    }
    if (input.up) {
        return "U";
    }
    return "";
}

std::string inputDisplayToken(
    const FighterInputState& input,
    int facing,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::vector<std::string> tokens;
    const std::string direction = inputDirectionToken(input, facing);
    if (!direction.empty()) {
        tokens.push_back(direction);
    }
    if (input.x) {
        tokens.push_back(commandButtonDisplayToken("x", mode));
    }
    if (input.y) {
        tokens.push_back(commandButtonDisplayToken("y", mode));
    }
    if (input.z) {
        tokens.push_back(commandButtonDisplayToken("z", mode));
    }
    if (input.a) {
        tokens.push_back(commandButtonDisplayToken("a", mode));
    }
    if (input.b) {
        tokens.push_back(commandButtonDisplayToken("b", mode));
    }
    if (input.c) {
        tokens.push_back(commandButtonDisplayToken("c", mode));
    }
    if (input.s) {
        tokens.push_back(commandButtonDisplayToken("s", mode));
    }
    return joinTokens(tokens, "+");
}

std::string physicalInputDisplayToken(
    const FighterInputState& input,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::vector<std::string> tokens;
    const std::string direction = physicalInputDirectionToken(input);
    if (!direction.empty()) {
        tokens.push_back(direction);
    }
    if (input.x) {
        tokens.push_back(commandButtonDisplayToken("x", mode));
    }
    if (input.y) {
        tokens.push_back(commandButtonDisplayToken("y", mode));
    }
    if (input.z) {
        tokens.push_back(commandButtonDisplayToken("z", mode));
    }
    if (input.a) {
        tokens.push_back(commandButtonDisplayToken("a", mode));
    }
    if (input.b) {
        tokens.push_back(commandButtonDisplayToken("b", mode));
    }
    if (input.c) {
        tokens.push_back(commandButtonDisplayToken("c", mode));
    }
    if (input.s) {
        tokens.push_back(commandButtonDisplayToken("s", mode));
    }
    return joinTokens(tokens, "+");
}

std::vector<std::string> recentInputDisplayTokens(
    const FighterState& fighter,
    int maxTokens,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::vector<std::string> tokens;
    std::string lastToken;
    for (auto it = fighter.inputHistory.rbegin(); it != fighter.inputHistory.rend() && static_cast<int>(tokens.size()) < maxTokens; ++it) {
        std::string token = inputDisplayToken(it->input, fighter.facing, mode);
        if (token == "-" || token == lastToken) {
            continue;
        }
        tokens.push_back(std::move(token));
        lastToken = tokens.back();
    }
    std::reverse(tokens.begin(), tokens.end());
    return tokens;
}

std::vector<std::string> recentPhysicalInputDisplayTokens(
    const FighterState& fighter,
    int maxTokens,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::vector<std::string> tokens;
    std::string lastToken;
    for (auto it = fighter.inputHistory.rbegin(); it != fighter.inputHistory.rend() && static_cast<int>(tokens.size()) < maxTokens; ++it) {
        std::string token = physicalInputDisplayToken(it->input, mode);
        if (token == "-" || token == lastToken) {
            continue;
        }
        tokens.push_back(std::move(token));
        lastToken = tokens.back();
    }
    std::reverse(tokens.begin(), tokens.end());
    return tokens;
}

bool commandInputHasAnyToken(const std::string& input, std::initializer_list<std::string_view> aliases) {
    for (const auto& token : commandInputTokens(input)) {
        if (token.kind == CommandInputTokenKind::Space) {
            continue;
        }
        const std::string id = commandInputIconId(token.text);
        for (std::string_view alias : aliases) {
            if (id == commandInputIconId(alias)) {
                return true;
            }
        }
    }
    return false;
}

std::string uppercaseTrimmed(std::string_view value) {
    std::string out = uppercaseCopy(trim(value));
    return out;
}

bool explicitCommandButtonDisplayToken(std::string_view token) {
    return uppercaseTrimmed(token).rfind("BTN_", 0) == 0;
}

bool commandInputHasCommandButton(
    const std::string& input,
    std::string_view command,
    CommandButtonPromptMode mode) {
    const std::string displayed = commandButtonDisplayToken(command, mode);
    const std::string strength = commandButtonDisplayToken(command, CommandButtonPromptMode::Strength);
    const bool punchGroup = command == "x" || command == "y" || command == "z";
    const bool kickGroup = command == "a" || command == "b" || command == "c";

    for (const auto& token : commandInputTokens(input)) {
        if (token.kind == CommandInputTokenKind::Space) {
            continue;
        }
        const std::string id = commandInputIconId(token.text);
        if (!displayed.empty()
            && ((explicitCommandButtonDisplayToken(displayed) && uppercaseTrimmed(token.text) == uppercaseTrimmed(displayed))
                || (!explicitCommandButtonDisplayToken(displayed) && id == commandInputIconId(displayed)))) {
            return true;
        }
        if (!strength.empty() && id == commandInputIconId(strength)) {
            return true;
        }
        if ((punchGroup && id == "P") || (kickGroup && id == "K")) {
            return true;
        }
    }
    return false;
}

std::array<bool, 4> requiredPhysicalDirectionsForStep(const std::string& expected, int facing) {
    std::array<bool, 4> required{ false, false, false, false };
    if (expected.empty()) {
        return required;
    }

    CommandInputRenderOptions options;
    options.directionPresentation = CommandInputDirectionPresentation::Physical;
    options.facing = facing;
    for (const auto& token : commandInputTokens(expected)) {
        if (token.kind == CommandInputTokenKind::Space) {
            continue;
        }
        const std::string id = commandInputPresentedIconId(token.text, options);
        if (id.find('B') != std::string::npos) {
            required[0] = true;
        }
        if (id.find('U') != std::string::npos) {
            required[1] = true;
        }
        if (id.find('D') != std::string::npos) {
            required[2] = true;
        }
        if (id.find('F') != std::string::npos) {
            required[3] = true;
        }
    }
    return required;
}

std::string currentTrainingCommandStepLabel(std::span<const TrainingCommandStepView> steps) {
    for (const auto& step : steps) {
        if (step.status == TrainingCommandStepStatus::Current) {
            return step.label;
        }
    }
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        if (it->status == TrainingCommandStepStatus::Matched) {
            return it->label;
        }
    }
    return {};
}

TrainingCommandButtonGuideView trainingCommandButtonGuideView(
    const FighterInputState& input,
    CommandButtonPromptMode mode,
    std::span<const TrainingCommandStepView> steps) {
    TrainingCommandButtonGuideView guide;
    guide.visible = true;
    const std::string expected = currentTrainingCommandStepLabel(steps);

    const auto makeButton = [&expected, mode](std::string label, bool pressed, std::string_view command) {
        const bool required = !expected.empty() && commandInputHasCommandButton(expected, command, mode);
        return TrainingCommandButtonGuideButtonView{
            std::move(label),
            pressed,
            required,
            pressed && required,
        };
    };

    const std::string west = commandButtonDisplayToken("x", mode);
    const std::string north = commandButtonDisplayToken("y", mode);
    const std::string south = commandButtonDisplayToken("a", mode);
    const std::string east = commandButtonDisplayToken("b", mode);
    const std::string system = commandButtonDisplayToken("s", mode);

    guide.buttons = {
        makeButton(west.empty() ? "LP" : west, input.x, "x"),
        makeButton(north.empty() ? "MP" : north, input.y, "y"),
        makeButton(south.empty() ? "LK" : south, input.a, "a"),
        makeButton(east.empty() ? "MK" : east, input.b, "b"),
    };
    guide.systemButton = makeButton(system.empty() ? "START" : system, input.s, "s");
    guide.systemButtonVisible = guide.systemButton.pressed
        || guide.systemButton.required
        || guide.systemButton.matched;
    return guide;
}

TrainingCommandDirectionGuideView trainingCommandDirectionGuideView(
    const FighterInputState& input,
    int facing,
    std::span<const TrainingCommandStepView> steps) {
    TrainingCommandDirectionGuideView guide;
    guide.visible = true;
    const std::string expected = currentTrainingCommandStepLabel(steps);
    const std::array<bool, 4> required = requiredPhysicalDirectionsForStep(expected, facing);
    const std::array<bool, 4> pressed{ input.left, input.up, input.down, input.right };
    const std::array<std::string, 4> labels{ "<", "^", "v", ">" };

    for (size_t i = 0; i < guide.directions.size(); ++i) {
        guide.directions[i] = TrainingCommandDirectionGuideButtonView{
            labels[i],
            pressed[i],
            required[i],
            pressed[i] && required[i],
        };
    }
    return guide;
}
