#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local command/input runtime types and helpers.
// Include only from App.cpp after those dependencies are defined.
// Do not include from other translation units.

bool commandActive(const std::vector<std::string>& commands, std::string_view command) {
    return std::any_of(commands.begin(), commands.end(), [command](const std::string& active) {
        return active == command;
    });
}

void pushFighterInputFrame(FighterState& fighter, const FighterInputState& input, int tick) {
    fighter.inputHistory.push_back(CommandInputFrame{ input, tick });
    constexpr size_t maxInputHistory = 90;
    if (fighter.inputHistory.size() > maxInputHistory) {
        fighter.inputHistory.erase(fighter.inputHistory.begin(), fighter.inputHistory.end() - static_cast<std::ptrdiff_t>(maxInputHistory));
    }
}

bool previousFighterInputHeldUp(const FighterState& fighter) {
    if (fighter.inputHistory.size() < 2) {
        return false;
    }
    return fighter.inputHistory[fighter.inputHistory.size() - 2].input.up;
}

bool fighterPressedUpWithin(const FighterState& fighter, int currentTick, int maxAgeTicks) {
    if (maxAgeTicks < 0) {
        return false;
    }
    for (size_t historyIndex = fighter.inputHistory.size(); historyIndex > 0; --historyIndex) {
        const size_t index = historyIndex - 1;
        const CommandInputFrame& frame = fighter.inputHistory[index];
        if (currentTick - frame.tick > maxAgeTicks) {
            break;
        }
        if (!frame.input.up) {
            continue;
        }
        const bool previousHeldUp = index > 0 && fighter.inputHistory[index - 1].input.up;
        if (!previousHeldUp) {
            return true;
        }
    }
    return false;
}

bool buttonAtomActive(const FighterInputState& input, std::string_view symbol) {
    if (symbol == "x") {
        return input.x;
    }
    if (symbol == "y") {
        return input.y;
    }
    if (symbol == "z") {
        return input.z;
    }
    if (symbol == "a") {
        return input.a;
    }
    if (symbol == "b") {
        return input.b;
    }
    if (symbol == "c") {
        return input.c;
    }
    if (symbol == "s") {
        return input.s;
    }
    return false;
}

bool buttonAtomSymbol(std::string_view symbol) {
    return symbol == "x"
        || symbol == "y"
        || symbol == "z"
        || symbol == "a"
        || symbol == "b"
        || symbol == "c"
        || symbol == "s";
}

bool cardinalDirectionAtomSymbol(std::string_view symbol) {
    return symbol == "F" || symbol == "B" || symbol == "D" || symbol == "U";
}

bool directionAtomActive(const FighterInputState& input, std::string_view symbol, int facing, bool broadDirection) {
    const bool forward = facing >= 0 ? input.right : input.left;
    const bool back = facing >= 0 ? input.left : input.right;
    if (broadDirection) {
        if (symbol == "F") {
            return forward;
        }
        if (symbol == "B") {
            return back;
        }
        if (symbol == "D") {
            return input.down;
        }
        if (symbol == "U") {
            return input.up;
        }
    }

    if (symbol == "F") {
        return forward && !input.down && !input.up;
    }
    if (symbol == "B") {
        return back && !input.down && !input.up;
    }
    if (symbol == "D") {
        return input.down && !forward && !back;
    }
    if (symbol == "U") {
        return input.up && !forward && !back;
    }
    if (symbol == "DF") {
        return input.down && forward;
    }
    if (symbol == "DB") {
        return input.down && back;
    }
    if (symbol == "UF") {
        return input.up && forward;
    }
    if (symbol == "UB") {
        return input.up && back;
    }
    return false;
}

bool commandAtomBaseActive(const FighterInputState& input, const CommandAtom& atom, int facing, bool broadDirection) {
    if (buttonAtomActive(input, atom.symbol)) {
        return true;
    }
    return directionAtomActive(input, atom.symbol, facing, broadDirection);
}

bool commandAtomReleased(
    const std::vector<CommandInputFrame>& history,
    size_t index,
    const CommandAtom& atom,
    int facing,
    bool broadDirection) {
    if (index == 0) {
        return false;
    }
    return commandAtomBaseActive(history[index - 1].input, atom, facing, broadDirection)
        && !commandAtomBaseActive(history[index].input, atom, facing, broadDirection);
}

bool commandAtomActive(const FighterInputState& input, const CommandAtom& atom, int facing, bool terminalButtonStepLeniency = false) {
    const bool broadDirection = atom.broadDirection
        || (terminalButtonStepLeniency && !atom.hold && cardinalDirectionAtomSymbol(atom.symbol));
    return commandAtomBaseActive(input, atom, facing, broadDirection);
}

bool commandAtomFresh(
    const std::vector<CommandInputFrame>& history,
    size_t index,
    const CommandAtom& atom,
    int facing,
    bool terminalButtonStepLeniency = false) {
    if (!commandAtomActive(history[index].input, atom, facing, terminalButtonStepLeniency)) {
        const bool broadDirection = atom.broadDirection
            || (terminalButtonStepLeniency && !atom.hold && cardinalDirectionAtomSymbol(atom.symbol));
        if (!atom.release || !commandAtomReleased(history, index, atom, facing, broadDirection)) {
            return false;
        }
    }
    if (atom.hold) {
        return true;
    }
    if (atom.release && buttonAtomSymbol(atom.symbol)) {
        const bool broadDirection = atom.broadDirection
            || (terminalButtonStepLeniency && cardinalDirectionAtomSymbol(atom.symbol));
        return commandAtomReleased(history, index, atom, facing, broadDirection);
    }
    if (index == 0) {
        return true;
    }
    return !commandAtomActive(history[index - 1].input, atom, facing, terminalButtonStepLeniency);
}

bool commandStepMatches(const std::vector<CommandInputFrame>& history, size_t index, const CommandStep& step, int facing) {
    bool hasPressAtom = false;
    bool hasFreshPressAtom = false;
    const bool terminalButtonStepLeniency = std::any_of(step.atoms.begin(), step.atoms.end(), [](const CommandAtom& atom) {
        return buttonAtomSymbol(atom.symbol) && !atom.hold;
    });
    for (const auto& atom : step.atoms) {
        const bool broadDirection = atom.broadDirection
            || (terminalButtonStepLeniency && !atom.hold && cardinalDirectionAtomSymbol(atom.symbol));
        const bool active = commandAtomActive(history[index].input, atom, facing, terminalButtonStepLeniency);
        const bool released = atom.release && commandAtomReleased(history, index, atom, facing, broadDirection);
        if (!active && !released) {
            return false;
        }
        if (!atom.hold) {
            hasPressAtom = true;
            if (released || commandAtomFresh(history, index, atom, facing, terminalButtonStepLeniency)) {
                hasFreshPressAtom = true;
            }
        }
    }
    return !hasPressAtom || hasFreshPressAtom;
}

bool commandStepHasButtonAtom(const CommandStep& step) {
    return std::any_of(step.atoms.begin(), step.atoms.end(), [](const CommandAtom& atom) {
        return buttonAtomSymbol(atom.symbol);
    });
}

bool commandStepHasDirectionAtom(const CommandStep& step) {
    return std::any_of(step.atoms.begin(), step.atoms.end(), [](const CommandAtom& atom) {
        return !buttonAtomSymbol(atom.symbol);
    });
}

bool commandStepAllowsPreviousOverlap(const CommandStep& previousStep, const CommandStep& step) {
    return commandStepHasDirectionAtom(previousStep)
        && !commandStepHasButtonAtom(previousStep)
        && commandStepHasButtonAtom(step)
        && !commandStepHasDirectionAtom(step);
}

bool commandDefinitionActive(const CommandDefinition& definition, const FighterState& fighter) {
    if (definition.steps.empty() || fighter.inputHistory.empty()) {
        return false;
    }

    int searchIndex = static_cast<int>(fighter.inputHistory.size()) - 1;
    int newestTick = fighter.inputHistory.back().tick;
    int lastMatchedTick = -1;
    for (int stepIndex = static_cast<int>(definition.steps.size()) - 1; stepIndex >= 0; --stepIndex) {
        bool matched = false;
        for (int frameIndex = searchIndex; frameIndex >= 0; --frameIndex) {
            const int elapsed = newestTick - fighter.inputHistory[static_cast<size_t>(frameIndex)].tick;
            if (stepIndex == static_cast<int>(definition.steps.size()) - 1 && elapsed > definition.bufferTime) {
                break;
            }
            if (elapsed > definition.maxTime) {
                break;
            }
            if (commandStepMatches(fighter.inputHistory, static_cast<size_t>(frameIndex), definition.steps[static_cast<size_t>(stepIndex)], fighter.facing)) {
                matched = true;
                if (lastMatchedTick < 0) {
                    lastMatchedTick = fighter.inputHistory[static_cast<size_t>(frameIndex)].tick;
                }
                searchIndex = stepIndex > 0
                    && commandStepAllowsPreviousOverlap(
                        definition.steps[static_cast<size_t>(stepIndex - 1)],
                        definition.steps[static_cast<size_t>(stepIndex)])
                    ? frameIndex
                    : frameIndex - 1;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    return lastMatchedTick >= 0 && newestTick - lastMatchedTick <= definition.bufferTime;
}

void appendBufferedCommands(std::vector<std::string>& commands, const FighterState& fighter, const std::vector<CommandDefinition>& definitions) {
    for (const auto& definition : definitions) {
        if (commandDefinitionActive(definition, fighter)) {
            addUniqueCommand(commands, definition.name);
        }
    }
}

std::vector<std::string> collectFighterCommands(
    const FighterInputState& input,
    const FighterState& fighter,
    const std::vector<CommandDefinition>& definitions) {
    std::vector<std::string> commands;
    if (input.x) {
        commands.push_back("x");
    }
    if (input.y) {
        commands.push_back("y");
    }
    if (input.z) {
        commands.push_back("z");
    }
    if (input.a) {
        commands.push_back("a");
    }
    if (input.b) {
        commands.push_back("b");
    }
    if (input.c) {
        commands.push_back("c");
    }
    if (input.down) {
        commands.push_back("holddown");
    }
    if (input.up) {
        commands.push_back("holdup");
    }

    const bool holdingLeft = input.left;
    const bool holdingRight = input.right;
    if ((fighter.facing >= 0 && holdingRight) || (fighter.facing < 0 && holdingLeft)) {
        commands.push_back("holdfwd");
    }
    if ((fighter.facing >= 0 && holdingLeft) || (fighter.facing < 0 && holdingRight)) {
        commands.push_back("holdback");
    }
    appendBufferedCommands(commands, fighter, definitions);
    return commands;
}
