#pragma once

// Internal App.cpp implementation header.
// Advances active command-demo playback.
// Include only through TrainingCommandPracticeAssembly.h.
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
    if (entry->requiredStateType == 'C') {
        input.down = true;
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
