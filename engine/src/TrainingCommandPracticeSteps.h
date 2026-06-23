#pragma once

// Internal App.cpp implementation header.
// Builds command labels and practice step views.
// Include only through TrainingCommandPracticeAssembly.h.
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

std::string trainingRequiredCommandDisplayToken(
    std::string_view command,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    if (command == "holddown"
        || command == "holdup"
        || command == "holdfwd"
        || command == "holdback") {
        return "HOLD " + moveListTokenForCommand(command, mode);
    }
    return moveListTokenForCommand(command, mode);
}

std::string commandAtomDisplayLabel(
    const CommandAtom& atom,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::string label = moveListTokenForCommand(atom.symbol, mode);
    if (atom.hold) {
        label = "HOLD " + label;
    }
    return fitDebugText(label, 10);
}

std::string commandStepDisplayLabel(
    const CommandStep& step,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::string label;
    for (const auto& atom : step.atoms) {
        if (!label.empty()) {
            label += "+";
        }
        label += commandAtomDisplayLabel(atom, mode);
    }
    return fitDebugText(label.empty() ? "-" : label, 12);
}

std::string commandDefinitionInputLabel(
    const CommandDefinition& definition,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    if (definition.steps.empty()) {
        return "-";
    }

    std::string label;
    for (const auto& step : definition.steps) {
        if (!label.empty()) {
            label += " > ";
        }
        label += commandStepDisplayLabel(step, mode);
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

    const auto namedMotionDefinition = [](std::string_view name) {
        return !holdTrainingCommandToken(name) && !simpleTrainingCommandToken(name);
    };

    for (const auto name : names) {
        if (!namedMotionDefinition(name)) {
            continue;
        }
        if (commandListContains(activeCommands, name)) {
            if (const CommandDefinition* definition = findCommandDefinitionByName(state, name)) {
                return definition;
            }
        }
    }
    for (const auto name : names) {
        if (!namedMotionDefinition(name)) {
            continue;
        }
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
    bool complete,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
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
            commandStepDisplayLabel(definition.steps[static_cast<size_t>(i)], mode),
            status,
        });
    }
}

std::vector<std::string> presentationInputTokens(std::string_view displayInput) {
    std::vector<std::string> tokens;
    std::string current;
    for (const char ch : displayInput) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

void appendPresentationPracticeSteps(
    std::vector<TrainingCommandStepView>& steps,
    std::string_view displayInput,
    int matched,
    bool complete) {
    const auto tokens = presentationInputTokens(displayInput);
    for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
        TrainingCommandStepStatus status = TrainingCommandStepStatus::Pending;
        if (complete || i < matched) {
            status = TrainingCommandStepStatus::Matched;
        } else if (i == matched) {
            status = TrainingCommandStepStatus::Current;
        }
        steps.push_back(TrainingCommandStepView{ fitDebugText(tokens[static_cast<size_t>(i)], 12), status });
    }
}

void appendEntryPracticeSteps(
    std::vector<TrainingCommandStepView>& steps,
    const CommandStateEntry& entry,
    const std::vector<std::string>& activeCommands,
    bool complete,
    CommandButtonPromptMode mode = CommandButtonPromptMode::Strength) {
    std::string label;
    bool allActive = complete;
    const auto appendPart = [&label](std::string part) {
        part = trim(part);
        if (part.empty()) {
            return;
        }
        if (!label.empty()) {
            label += "+";
        }
        label += std::move(part);
    };

    if (!complete) {
        allActive = true;
    }

    const auto appendIfRequired = [&](std::string_view command) {
        if (!commandListContains(entry.requiredCommands, command)) {
            return;
        }
        appendPart(trainingRequiredCommandDisplayToken(command, mode));
        allActive = allActive && commandListContains(activeCommands, command);
    };

    appendIfRequired("holddown");
    appendIfRequired("holdfwd");
    appendIfRequired("holdback");
    appendIfRequired("holdup");
    appendIfRequired("hold_x");
    appendIfRequired("hold_y");
    appendIfRequired("hold_z");
    appendIfRequired("hold_a");
    appendIfRequired("hold_b");
    appendIfRequired("hold_c");
    appendIfRequired("x");
    appendIfRequired("y");
    appendIfRequired("z");
    appendIfRequired("a");
    appendIfRequired("b");
    appendIfRequired("c");
    appendIfRequired("s");

    for (const auto& group : entry.commandOptionGroups) {
        std::string groupLabel;
        bool groupActive = false;
        for (const auto& option : group) {
            if (!groupLabel.empty()) {
                groupLabel += "/";
            }
            groupLabel += trainingRequiredCommandDisplayToken(option, mode);
            groupActive = groupActive || commandListContains(activeCommands, option);
        }
        if (!groupLabel.empty()) {
            appendPart(groupLabel);
            allActive = allActive && (complete || groupActive);
        }
    }

    for (const auto& command : entry.requiredCommands) {
        if (holdTrainingCommandToken(command)
            || simpleTrainingCommandToken(command)
            || commandListContains(activeCommands, command)) {
            continue;
        }
        appendPart(moveListTokenForCommand(command, mode));
        allActive = false;
    }

    if (label.empty()) {
        label = "-";
    }
    steps.push_back(TrainingCommandStepView{
        fitDebugText(label, 20),
        allActive ? TrainingCommandStepStatus::Matched : TrainingCommandStepStatus::Current,
    });
}

