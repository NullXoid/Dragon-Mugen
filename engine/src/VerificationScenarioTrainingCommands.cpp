#include "VerificationScenarioCommon.h"

namespace dragon::verification {

int runTrainingCommandFacingAwareDisplay(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY training-command-facing-aware-display\n";

    CommandInputRenderOptions facingRight;
    facingRight.directionPresentation = CommandInputDirectionPresentation::Physical;
    facingRight.facing = 1;
    CommandInputRenderOptions facingLeft = facingRight;
    facingLeft.facing = -1;

    const std::string input = "D+DF+F+X/B";
    const std::string right = commandInputPresentedInput(input, facingRight);
    const std::string left = commandInputPresentedInput(input, facingLeft);
    record(out, counts, right == "D+DF+F+X/B" ? Status::Pass : Status::Fail,
        "facing_right_keeps_forward_right",
        "presented=\"" + right + "\"");
    record(out, counts, left == "D+DB+B+X/F" ? Status::Pass : Status::Fail,
        "facing_left_flips_forward_to_left",
        "presented=\"" + left + "\"");
    record(out, counts, commandInputPresentedIconId("DF", facingLeft) == "DB"
            && commandInputPresentedIconId("B", facingLeft) == "F"
            ? Status::Pass : Status::Fail,
        "direction_icon_ids_flip",
        "DF->" + commandInputPresentedIconId("DF", facingLeft)
            + " B->" + commandInputPresentedIconId("B", facingLeft));
    record(out, counts, commandInputPresentedIconId("BTN_B", facingLeft) == "B"
            && commandInputPresentedText("BTN_B", facingLeft) == "B"
            ? Status::Pass : Status::Fail,
        "explicit_xbox_b_does_not_flip",
        "BTN_B->" + commandInputPresentedIconId("BTN_B", facingLeft));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandSideSwitchHighlight(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-side-switch-highlight");

    waitForControllableIdle(runtime, 420);
    runtime.positionFighters(120.0f, -120.0f);
    const auto sideSwitched = runtime.snapshot();
    record(out, counts, sideSwitched.p1.facing < 0 ? Status::Pass : Status::Fail,
        "p1_faces_left_after_side_switch",
        "facing=" + std::to_string(sideSwitched.p1.facing));

    runtime.step(SymbolicInput{ .right = true }, 1);
    const std::string rightDisplay = runtime.trainingCurrentInputDisplay();
    runtime.step(SymbolicInput{ .left = true }, 1);
    const std::string leftDisplay = runtime.trainingCurrentInputDisplay();
    CommandInputRenderOptions liveInputOptions;
    liveInputOptions.facing = sideSwitched.p1.facing;
    record(out, counts, rightDisplay == "F" ? Status::Pass : Status::Fail,
        "physical_right_still_displays_right",
        "display=\"" + rightDisplay + "\"");
    record(out, counts, leftDisplay == "B" ? Status::Pass : Status::Fail,
        "physical_left_still_displays_left",
        "display=\"" + leftDisplay + "\"");
    record(out, counts, commandInputPresentedInput(rightDisplay, liveInputOptions) == "F"
            && commandInputPresentedInput(leftDisplay, liveInputOptions) == "B"
            ? Status::Pass : Status::Fail,
        "live_input_render_does_not_flip",
        "right=\"" + commandInputPresentedInput(rightDisplay, liveInputOptions)
            + "\" left=\"" + commandInputPresentedInput(leftDisplay, liveInputOptions) + "\"");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandPhysicalDirectionGuide(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-physical-direction-guide");

    waitForControllableIdle(runtime, 420);
    runtime.positionFighters(120.0f, -120.0f);
    runtime.step(SymbolicInput{ .right = true }, 1);
    const std::string rightGuide = runtime.trainingDirectionGuideState();
    runtime.step(SymbolicInput{ .left = true }, 1);
    const std::string leftGuide = runtime.trainingDirectionGuideState();

    record(out, counts, rightGuide.find(">:p") != std::string::npos ? Status::Pass : Status::Fail,
        "guide_lights_physical_right",
        rightGuide);
    record(out, counts, leftGuide.find("<:p") != std::string::npos ? Status::Pass : Status::Fail,
        "guide_lights_physical_left",
        leftGuide);

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandStartButtonGuide(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-start-button-guide");

    record(out, counts,
        gamepadButtonMapsToFighterStart(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_BUTTON_TOUCHPAD)
            ? Status::Pass
            : Status::Fail,
        "ps_touchpad_maps_to_fighter_start",
        "PS5 touchpad should feed ST/Taunt");
    record(out, counts,
        !gamepadButtonMapsToFighterStart(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_BUTTON_START)
            ? Status::Pass
            : Status::Fail,
        "ps_start_reserved_for_pause",
        "PS5 Start/Options should stay system pause");
    record(out, counts,
        !gamepadButtonMapsToFighterStart(SDL_GAMEPAD_TYPE_XBOXONE, SDL_GAMEPAD_BUTTON_TOUCHPAD)
            ? Status::Pass
            : Status::Fail,
        "non_ps_touchpad_not_fighter_start",
        "touchpad taunt is PlayStation-only");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const bool selected = runtime.selectTrainingMove("Taunt");
    record(out, counts, selected ? Status::Pass : Status::Blocked, "select_taunt_move",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    const std::string expected = runtime.trainingExpectedInputDisplay();
    const std::string initialGuide = runtime.trainingButtonGuideState();
    record(out, counts,
        expected.find("START") != std::string::npos || expected.find("ST") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "expected_input_contains_start",
        "expected=\"" + expected + "\"");
    record(out, counts,
        initialGuide.find("SYSTEM:v:START:-r-") != std::string::npos
            || initialGuide.find("SYSTEM:v:MENU:-r-") != std::string::npos
            || initialGuide.find("SYSTEM:v:OPT:-r-") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "start_button_required_visible",
        initialGuide);

    runtime.step(SymbolicInput{ .s = true }, 1);
    const std::string pressedDisplay = runtime.trainingCurrentInputDisplay();
    const std::string pressedGuide = runtime.trainingButtonGuideState();
    record(out, counts,
        pressedDisplay.find("START") != std::string::npos
            || pressedDisplay.find("ST") != std::string::npos
            || pressedDisplay.find("MENU") != std::string::npos
            || pressedDisplay.find("OPT") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "start_input_reaches_history",
        "display=\"" + pressedDisplay + "\"");
    record(out, counts,
        pressedGuide.find("SYSTEM:v:START:p") != std::string::npos
            || pressedGuide.find("SYSTEM:v:MENU:p") != std::string::npos
            || pressedGuide.find("SYSTEM:v:OPT:p") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "start_button_pressed",
        pressedGuide);

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandCompleteBlink(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-complete-blink");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-260.0f, 260.0f);
    const bool selected = runtime.selectTrainingMove("S.Jab");
    record(out, counts, selected ? Status::Pass : Status::Blocked, "select_simple_move",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    constexpr std::array<char, 6> buttons{ 'x', 'y', 'z', 'a', 'b', 'c' };
    bool sawFlash = false;
    char workingButton = '?';
    for (const char button : buttons) {
        waitForControllableIdle(runtime, 180);
        runtime.step({}, 6);
        runtime.step(withButton(button), 2);
        for (int i = 0; i < 24; ++i) {
            sawFlash = runtime.trainingCommandCompleteFlash();
            if (sawFlash) {
                workingButton = button;
                break;
            }
            runtime.step({}, 1);
        }
        if (sawFlash) {
            break;
        }
    }

    record(out, counts, sawFlash ? Status::Pass : Status::Fail,
        "complete_flash_reaches_hud",
        "button=" + std::string(1, workingButton)
            + " last_hit=\"" + runtime.snapshot().lastHitText + "\"");
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingPaletteSlotSeparation(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-palette-slot-separation");

    const auto snapshot = runtime.snapshot();
    record(out, counts, snapshot.p1.paletteNo == 1 ? Status::Pass : Status::Fail,
        "p1_uses_palette_1",
        "p1_pal=" + std::to_string(snapshot.p1.paletteNo));
    record(out, counts, snapshot.p2.paletteNo == 2 ? Status::Pass : Status::Fail,
        "dummy_uses_palette_2",
        "p2_pal=" + std::to_string(snapshot.p2.paletteNo));
    record(out, counts, snapshot.p1.paletteNo != snapshot.p2.paletteNo ? Status::Pass : Status::Fail,
        "slots_are_visually_separated",
        "p1_pal=" + std::to_string(snapshot.p1.paletteNo)
            + " p2_pal=" + std::to_string(snapshot.p2.paletteNo));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingShowSelectHold(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-show-select-hold");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto moves = runtime.trainingMoves();
    record(out, counts, moves.size() >= 2 ? Status::Pass : Status::Fail, "training_moves_available",
        "moves=" + std::to_string(moves.size()));
    if (moves.size() < 2) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto shippu = std::find_if(moves.begin(), moves.end(), [](const TrainingMoveInfo& move) {
        return move.label == "Shippu Jinrai Kyaku";
    });
    const bool shippuUsesHumanNotation =
        shippu != moves.end()
        && shippu->input.find("DB") != std::string::npos
        && shippu->input.find("MASH") != std::string::npos;
    record(out, counts, shippuUsesHumanNotation ? Status::Pass : Status::Fail,
        "movelist_dat_input_notation_used",
        shippu == moves.end() ? "Shippu Jinrai Kyaku missing" : "input=\"" + shippu->input + "\"");

    const auto sJab = std::find_if(moves.begin(), moves.end(), [](const TrainingMoveInfo& move) {
        return move.label == "S.Jab";
    });
    const bool sJabUsesStrengthNotation = sJab != moves.end() && sJab->input == "LP";
    record(out, counts, sJabUsesStrengthNotation ? Status::Pass : Status::Fail,
        "fallback_buttons_use_strength_notation",
        sJab == moves.end() ? "S.Jab missing" : "input=\"" + sJab->input + "\"");

    const auto xboxMoves = runtime.trainingMovesForPromptStyle("xbox");
    const auto xboxShippu = std::find_if(xboxMoves.begin(), xboxMoves.end(), [](const TrainingMoveInfo& move) {
        return move.label == "Shippu Jinrai Kyaku";
    });
    const bool xboxUsesPadNotation =
        xboxShippu != xboxMoves.end()
        && xboxShippu->input.find("A/BTN_B/RB") != std::string::npos
        && xboxShippu->input.find("LK") == std::string::npos;
    record(out, counts, xboxUsesPadNotation ? Status::Pass : Status::Fail,
        "xbox_controller_prompt_notation_used",
        xboxShippu == xboxMoves.end() ? "Shippu Jinrai Kyaku missing" : "input=\"" + xboxShippu->input + "\"");

    const auto psMoves = runtime.trainingMovesForPromptStyle("playstation");
    const auto psSJab = std::find_if(psMoves.begin(), psMoves.end(), [](const TrainingMoveInfo& move) {
        return move.label == "S.Jab";
    });
    const bool psUsesPadNotation = psSJab != psMoves.end() && psSJab->input == "SQ";
    record(out, counts, psUsesPadNotation ? Status::Pass : Status::Fail,
        "playstation_controller_prompt_notation_used",
        psSJab == psMoves.end() ? "S.Jab missing" : "input=\"" + psSJab->input + "\"");

    runtime.selectTrainingMoveIndex(0);
    const std::string firstLabel = moves[0].label;
    const std::string secondLabel = moves[1].label;
    runtime.holdTrainingShowSelect(true, 30);
    runtime.holdTrainingShowSelect(false, 1);
    const auto tap = runtime.snapshot();
    record(out, counts, tap.selectedTrainingMoveLabel == secondLabel ? Status::Pass : Status::Fail,
        "short_select_tap_advances_move",
        "before=\"" + firstLabel + "\" after=\"" + tap.selectedTrainingMoveLabel + "\" expected=\"" + secondLabel + "\"");

    const bool selectedHadouken = runtime.selectTrainingMove("Hadouken");
    record(out, counts, selectedHadouken ? Status::Pass : Status::Fail, "selected_hadouken_for_hold_demo",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selectedHadouken) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.holdTrainingShowSelect(true, 119);
    const auto beforeThreshold = runtime.snapshot();
    record(out, counts, beforeThreshold.p2.stateNo != 1000 ? Status::Pass : Status::Fail,
        "hold_before_two_seconds_does_not_start_demo",
        "p2_state=" + std::to_string(beforeThreshold.p2.stateNo)
        + " selected=\"" + beforeThreshold.selectedTrainingMoveLabel + "\"");

    runtime.holdTrainingShowSelect(true, 1);
    runtime.holdTrainingShowSelect(false, 1);

    bool sawHadouken = false;
    FighterSnapshot finalP2;
    std::string commands;
    for (int frame = 0; frame < 90; ++frame) {
        const auto snap = runtime.snapshot();
        finalP2 = snap.p2;
        commands = snap.p2Commands;
        sawHadouken = sawHadouken || snap.p2.stateNo == 1000 || snap.p2.action == 1000;
        if (sawHadouken) {
            break;
        }
        runtime.step({}, 1);
    }
    const auto afterHold = runtime.snapshot();
    record(out, counts, afterHold.selectedTrainingMoveLabel == "Hadouken" ? Status::Pass : Status::Fail,
        "long_select_hold_keeps_selected_move",
        "selected=\"" + afterHold.selectedTrainingMoveLabel + "\"");
    record(out, counts, sawHadouken ? Status::Pass : Status::Fail,
        "long_select_hold_starts_show_command_demo",
        "p2_state=" + std::to_string(finalP2.stateNo)
        + " p2_action=" + std::to_string(finalP2.action)
        + " commands=\"" + commands + "\"");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingShowControllerShortcut(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-show-controller-shortcut");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const bool selectedHadouken = runtime.selectTrainingMove("Hadouken");
    record(out, counts, selectedHadouken ? Status::Pass : Status::Fail, "selected_hadouken_for_shortcut_demo",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selectedHadouken) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.pressTrainingShowShortcut();
    bool sawHadouken = false;
    FighterSnapshot finalP2;
    std::string commands;
    for (int frame = 0; frame < 90; ++frame) {
        const auto snap = runtime.snapshot();
        finalP2 = snap.p2;
        commands = snap.p2Commands;
        sawHadouken = sawHadouken || snap.p2.stateNo == 1000 || snap.p2.action == 1000;
        if (sawHadouken) {
            break;
        }
        runtime.step({}, 1);
    }
    record(out, counts, sawHadouken ? Status::Pass : Status::Fail,
        "controller_shortcut_starts_show_command_demo",
        "p2_state=" + std::to_string(finalP2.stateNo)
        + " p2_action=" + std::to_string(finalP2.action)
        + " commands=\"" + commands + "\"");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandFilteredComplete(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-filtered-complete");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-260.0f, 260.0f);
    runtime.setTrainingMoveCategory("special");
    const bool selectedHadouken = runtime.selectTrainingMove("Hadouken");
    record(out, counts, selectedHadouken ? Status::Pass : Status::Blocked, "select_filtered_special_hadouken",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selectedHadouken) {
        summary(out, counts);
        return exitCode(counts);
    }

    performQcfButton(runtime, 'x');
    bool sawFlash = runtime.trainingCommandCompleteFlash();
    for (int i = 0; i < 36 && !sawFlash; ++i) {
        runtime.step({}, 1);
        sawFlash = runtime.trainingCommandCompleteFlash();
    }

    const auto snap = runtime.snapshot();
    record(out, counts, snap.p1.stateNo == 1000 || snap.p1.action == 1000 ? Status::Pass : Status::Fail,
        "hadouken_entered",
        "state=" + std::to_string(snap.p1.stateNo)
            + " action=" + std::to_string(snap.p1.action)
            + " commands=\"" + snap.p1Commands + "\"");
    record(out, counts, sawFlash ? Status::Pass : Status::Fail,
        "filtered_special_completion_flash",
        "selected=\"" + snap.selectedTrainingMoveLabel
            + "\" last_hit=\"" + snap.lastHitText + "\"");

    const bool recovered = waitForControllableIdle(runtime, 420);
    const auto afterFirstOk = runtime.snapshot();
    record(out, counts, recovered ? Status::Pass : Status::Fail,
        "recovered_after_first_ok",
        "state=" + std::to_string(afterFirstOk.p1.stateNo)
            + " selected=\"" + afterFirstOk.selectedTrainingMoveLabel + "\"");
    record(out, counts, afterFirstOk.selectedTrainingMoveLabel != "Seoi Shoryuken" ? Status::Pass : Status::Fail,
        "auto_advance_skips_unavailable_followup",
        "selected=\"" + afterFirstOk.selectedTrainingMoveLabel + "\"");
    if (recovered) {
        performDpButton(runtime, 'x');
    }
    bool sawSecondFlash = runtime.trainingCommandCompleteFlash();
    for (int i = 0; i < 36 && !sawSecondFlash; ++i) {
        runtime.step({}, 1);
        sawSecondFlash = runtime.trainingCommandCompleteFlash();
    }
    const auto afterSecondMove = runtime.snapshot();
    record(out, counts, afterSecondMove.p1.stateNo == 1500 || afterSecondMove.p1.action == 1500 ? Status::Pass : Status::Fail,
        "second_special_entered",
        "state=" + std::to_string(afterSecondMove.p1.stateNo)
            + " action=" + std::to_string(afterSecondMove.p1.action)
            + " selected=\"" + afterFirstOk.selectedTrainingMoveLabel + "\"");
    record(out, counts, sawSecondFlash ? Status::Pass : Status::Fail,
        "second_filtered_special_completion_flash",
        "selected=\"" + afterSecondMove.selectedTrainingMoveLabel
            + "\" last_hit=\"" + afterSecondMove.lastHitText + "\"");

    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "evilryu_setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    header(out, runtime, "training-command-filtered-complete-evilryu");

    const bool evilRyuIdle = waitForControllableIdle(runtime, 420);
    record(out, counts, evilRyuIdle ? Status::Pass : Status::Fail, "evilryu_controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!evilRyuIdle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-260.0f, 260.0f);
    runtime.setTrainingMoveCategory("special");
    const bool selectedZankuu = runtime.selectTrainingMove("Zankuu Hadouken");
    record(out, counts, selectedZankuu ? Status::Pass : Status::Blocked, "select_filtered_special_zankuu",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selectedZankuu) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.forceFighterState(0, 50);
    runtime.setFighterPosition(0, -260.0f, -80.0f);
    runtime.setFighterControl(0, true);
    runtime.step({}, 1);
    performQcbButton(runtime, 'x');
    bool sawEvilRyuFlash = runtime.trainingCommandCompleteFlash();
    for (int i = 0; i < 36 && !sawEvilRyuFlash; ++i) {
        runtime.step({}, 1);
        sawEvilRyuFlash = runtime.trainingCommandCompleteFlash();
    }

    const auto evilRyuAfterZankuu = runtime.snapshot();
    const bool zankuuEntered =
        evilRyuAfterZankuu.p1.stateNo == 1865 || evilRyuAfterZankuu.p1.action == 1865;
    record(out, counts, zankuuEntered ? Status::Pass : Status::Fail,
        "evilryu_zankuu_entered",
        "state=" + std::to_string(evilRyuAfterZankuu.p1.stateNo)
            + " action=" + std::to_string(evilRyuAfterZankuu.p1.action)
            + " commands=\"" + evilRyuAfterZankuu.p1Commands + "\"");
    record(out, counts, sawEvilRyuFlash ? Status::Pass : Status::Fail,
        "evilryu_zankuu_completion_flash",
        "selected=\"" + evilRyuAfterZankuu.selectedTrainingMoveLabel
            + "\" last_hit=\"" + evilRyuAfterZankuu.lastHitText + "\"");

    const bool evilRyuRecovered = waitForControllableIdle(runtime, 420);
    const auto evilRyuAfterFirstOk = runtime.snapshot();
    record(out, counts, evilRyuRecovered ? Status::Pass : Status::Fail,
        "evilryu_recovered_after_zankuu_ok",
        "state=" + std::to_string(evilRyuAfterFirstOk.p1.stateNo)
            + " selected=\"" + evilRyuAfterFirstOk.selectedTrainingMoveLabel + "\"");
    record(out, counts, evilRyuAfterFirstOk.selectedTrainingMoveLabel != "Seoi Zankuu Hadouken" ? Status::Pass : Status::Fail,
        "evilryu_auto_advance_skips_unavailable_followup",
        "selected=\"" + evilRyuAfterFirstOk.selectedTrainingMoveLabel + "\"");
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandHeldButtonPrompt(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-held-button-prompt");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto moves = runtime.trainingMoves();
    const auto closeRoundhouse = std::find_if(moves.begin(), moves.end(), [](const TrainingMoveInfo& move) {
        return move.label == "Close S.RoundHouse" || move.label == "Close S.Roundhouse";
    });
    const bool listShowsHeldKick =
        closeRoundhouse != moves.end()
        && closeRoundhouse->input.find("BACK") != std::string::npos
        && closeRoundhouse->input.find("SK") != std::string::npos;
    record(out, counts, listShowsHeldKick ? Status::Pass : Status::Fail,
        "close_roundhouse_list_shows_back_and_strong_kick",
        closeRoundhouse == moves.end() ? "Close S.RoundHouse missing" : "input=\"" + closeRoundhouse->input + "\"");

    const bool selected = runtime.selectTrainingMove("Close S.RoundHouse")
        || runtime.selectTrainingMove("Close S.Roundhouse");
    record(out, counts, selected ? Status::Pass : Status::Fail,
        "selected_close_roundhouse",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.step({}, 1);
    const std::string expected = runtime.trainingExpectedInputDisplay();
    const bool hudShowsHeldKick =
        expected.find("BACK") != std::string::npos
        && (expected.find("SK") != std::string::npos
            || expected.find("R1") != std::string::npos
            || expected.find("RB") != std::string::npos);
    record(out, counts, hudShowsHeldKick ? Status::Pass : Status::Fail,
        "hud_expected_shows_back_and_strong_kick",
        "expected=\"" + expected + "\"");

    runtime.step(SymbolicInput{ .left = true }, 6);
    const auto backOnly = runtime.snapshot();
    record(out, counts, backOnly.p1.stateNo != 209 ? Status::Pass : Status::Fail,
        "back_only_does_not_fire_close_roundhouse",
        "state=" + std::to_string(backOnly.p1.stateNo)
        + " action=" + std::to_string(backOnly.p1.action));

    waitForControllableIdle(runtime, 180);
    runtime.step(SymbolicInput{ .left = true, .c = true }, 8);
    const auto backKick = runtime.snapshot();
    record(out, counts, backKick.p1.stateNo == 209 || backKick.p1.action == 209 ? Status::Pass : Status::Fail,
        "back_plus_strong_kick_fires_close_roundhouse",
        "state=" + std::to_string(backKick.p1.stateNo)
        + " action=" + std::to_string(backKick.p1.action));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
