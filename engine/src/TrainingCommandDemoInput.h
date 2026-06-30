#pragma once

// Internal App.cpp implementation header.
// Synthesizes command-demo directional and button input.
// Include only through TrainingCommandPracticeAssembly.h.
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
    applyRequired("s");
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

