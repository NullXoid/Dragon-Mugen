#include "VerificationScenarioCommon.h"

namespace dragon::verification {

ControlsOptionsContext defaultControlsOptionsContext(MainSettings settings = {}) {
    static ControlsSettings controls;
    controls = {};
    controls.gamepadAssignments = { 0, 0, 0, 0 };
    controls.profiles.push_back(makeDefaultControlProfile("player1", 0));
    controls.profiles.push_back(makeDefaultControlProfile("player2", 1));
    controls.profiles.push_back(makeDefaultControlProfile("player3", 2));
    controls.profiles.push_back(makeDefaultControlProfile("player4", 3));

    ControlsOptionsContext context;
    context.settings = settings;
    context.controls = &controls;
    context.playerProfileIds = { "player1", "player2", "player3", "player4" };
    context.playerProfileNames = { "Player 1", "Player 2", "Player 3", "Player 4" };
    context.gamepadAssignmentText = { "AUTO NONE", "AUTO NONE", "AUTO NONE", "AUTO NONE" };
    context.padSummary = "PADS NONE";
    context.promptStyle = GamepadPromptStyle::Playstation;
    return context;
}

int runOptionsCategoryNavigation(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY options-category-navigation\n";

    MainSettings settings;
    settings.optionsScreen = OptionsMenuScreen::Root;
    auto rootRows = buildControlsOptionsRows(defaultControlsOptionsContext(settings));
    record(out, counts, rootRows.size() == kOptionsRootCount ? Status::Pass : Status::Fail,
        "root_category_count",
        "rows=" + std::to_string(rootRows.size()));
    const bool rootHasCategories =
        rootRows.size() >= 4
        && rootRows[0].label == "GAMEPLAY"
        && rootRows[1].label == "VIDEO"
        && rootRows[2].label == "CONTROLS"
        && rootRows[3].label == "BACK";
    record(out, counts, rootHasCategories ? Status::Pass : Status::Fail,
        "root_categories_present",
        rootHasCategories ? "" : "expected Gameplay/Video/Controls/Back");

    settings.optionsScreen = OptionsMenuScreen::Gameplay;
    auto gameplayRows = buildControlsOptionsRows(defaultControlsOptionsContext(settings));
    record(out, counts,
        gameplayRows.size() == kOptionsGameplayCount
            && gameplayRows[0].label == "MATCH TIMER"
            && gameplayRows[1].label == "P1 PROFILE"
            && gameplayRows[2].label == "CREATE P1 PROFILE"
            && gameplayRows[3].label == "P2 PROFILE"
            && gameplayRows[4].label == "CREATE P2 PROFILE" ? Status::Pass : Status::Fail,
        "gameplay_rows",
        "rows=" + std::to_string(gameplayRows.size()));

    settings.optionsScreen = OptionsMenuScreen::Video;
    auto videoRows = buildControlsOptionsRows(defaultControlsOptionsContext(settings));
    record(out, counts,
        videoRows.size() == kOptionsVideoCount
            && videoRows[0].label == "CANVAS SIZE"
            && videoRows[2].label == "FPS CAP"
            && videoRows[3].label == "PERFORMANCE HUD" ? Status::Pass : Status::Fail,
        "video_rows",
        "rows=" + std::to_string(videoRows.size()));

    settings.optionsScreen = OptionsMenuScreen::Controls;
    auto controlRows = buildControlsOptionsRows(defaultControlsOptionsContext(settings));
    record(out, counts,
        controlRows.size() == kOptionsControlsCount
            && controlRows[0].label == "P1 CONTROLS"
            && controlRows[3].label == "P4 CONTROLS" ? Status::Pass : Status::Fail,
        "controls_rows",
        "rows=" + std::to_string(controlRows.size()));

    summary(out, counts);
    return exitCode(counts);
}

int runControlsPlayerOneToFourNavigation(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-player-1-4-navigation\n";
    for (int player = 0; player < kControlPlayerCount; ++player) {
        MainSettings settings;
        settings.optionsScreen = OptionsMenuScreen::PlayerControls;
        settings.selectedControlPlayer = player;
        const auto rows = buildControlsOptionsRows(defaultControlsOptionsContext(settings));
        const bool hasCoreRows =
            rows.size() >= kControlPlayerStaticRows + fightingInputActions().size() + 3
            && rows[0].label == "PROFILE"
            && rows[1].label == "DEVICE"
            && rows[2].label == "PRESET"
            && rows[3].label == "ACTION SET";
        record(out, counts, hasCoreRows ? Status::Pass : Status::Fail,
            "player_" + std::to_string(player + 1) + "_control_rows",
            "rows=" + std::to_string(rows.size()));
    }
    summary(out, counts);
    return exitCode(counts);
}

int runControlsGuidedSetup(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-guided-setup\n";
    ControlProfileBinding profile = makeDefaultControlProfile("player1", 0);
    setPrimaryActionBinding(profile, InputAction::MoveLeft, keyBinding(SDL_SCANCODE_F));
    setPrimaryActionBinding(profile, InputAction::LP, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_SOUTH));
    record(out, counts,
        actionBindingLabel(profile, InputAction::MoveLeft).find("F") != std::string::npos ? Status::Pass : Status::Fail,
        "keyboard_rebind_applied",
        actionBindingLabel(profile, InputAction::MoveLeft));
    record(out, counts,
        actionBindingLabel(profile, InputAction::LP, GamepadPromptStyle::Xbox).find("A") != std::string::npos ? Status::Pass : Status::Fail,
        "gamepad_rebind_applied",
        actionBindingLabel(profile, InputAction::LP, GamepadPromptStyle::Xbox));
    record(out, counts,
        missingRequiredControlActions(profile).empty() ? Status::Pass : Status::Fail,
        "required_actions_still_satisfied",
        "missing=" + std::to_string(missingRequiredControlActions(profile).size()));
    summary(out, counts);
    return exitCode(counts);
}

int runControlsManualEditConflicts(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-manual-edit-conflicts\n";
    ControlProfileBinding profile = makeDefaultControlProfile("player1", 0);
    setPrimaryActionBinding(profile, InputAction::Pause, keyBinding(SDL_SCANCODE_SPACE));
    setPrimaryActionBinding(profile, InputAction::Taunt, keyBinding(SDL_SCANCODE_SPACE));
    const auto conflicts = controlBindingConflicts(profile);
    record(out, counts, !conflicts.empty() ? Status::Pass : Status::Fail,
        "pause_taunt_conflict_detected",
        conflicts.empty() ? "no conflicts" : conflicts.front());
    setPrimaryActionBinding(profile, InputAction::Pause, keyBinding(SDL_SCANCODE_RETURN));
    const auto fixed = controlBindingConflicts(profile);
    record(out, counts, fixed.empty() ? Status::Pass : Status::Fail,
        "conflict_clears_after_rebind",
        fixed.empty() ? "OK" : fixed.front());
    summary(out, counts);
    return exitCode(counts);
}

int runControlsPresets(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-presets\n";
    ControlProfileBinding profile = makeDefaultControlProfile("player1", 0);
    applyControlPreset(profile, ControlPreset::BeatEmUpModern, 0);
    record(out, counts, profile.actionSet == InputActionSet::BeatEmUp ? Status::Pass : Status::Fail,
        "beat_em_up_preset_sets_action_set",
        std::string(inputActionSetLabel(profile.actionSet)));
    record(out, counts, findActionBinding(profile, InputAction::Jump) != nullptr ? Status::Pass : Status::Fail,
        "beat_em_up_jump_binding",
        actionBindingLabel(profile, InputAction::Jump, GamepadPromptStyle::Xbox));
    const ControlPreset next = cycleControlPreset(profile.presetName, 1);
    record(out, counts, !std::string(controlPresetName(next)).empty() ? Status::Pass : Status::Fail,
        "preset_cycle_available",
        std::string(controlPresetName(next)));
    summary(out, counts);
    return exitCode(counts);
}

int runControlsProfilePersistence(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-profile-persistence\n";
    ControlsSettings settings;
    settings.gamepadAssignments = { 1, -1, 2, 0 };
    ControlProfileBinding profile = makeDefaultControlProfile("player1", 0);
    setPrimaryActionBinding(profile, InputAction::MoveRight, keyBinding(SDL_SCANCODE_G));
    settings.profiles.push_back(std::move(profile));

    const auto path = std::filesystem::temp_directory_path() / "dragon_mugen_controls_verify.def";
    saveControlsSettings(path, settings);
    const ControlsSettings loaded = loadControlsSettings(path);
    std::error_code ec;
    std::filesystem::remove(path, ec);

    const auto* loadedProfile = findControlProfile(loaded, "player1");
    record(out, counts, loaded.gamepadAssignments[0] == 1 && loaded.gamepadAssignments[1] == -1 ? Status::Pass : Status::Fail,
        "assignments_round_trip",
        "p1=" + std::to_string(loaded.gamepadAssignments[0]) + " p2=" + std::to_string(loaded.gamepadAssignments[1]));
    record(out, counts,
        loadedProfile && actionBindingLabel(*loadedProfile, InputAction::MoveRight).find("G") != std::string::npos ? Status::Pass : Status::Fail,
        "profile_binding_round_trip",
        loadedProfile ? actionBindingLabel(*loadedProfile, InputAction::MoveRight) : "missing profile");
    summary(out, counts);
    return exitCode(counts);
}

int runControlsInputTestLive(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-input-test-live\n";
    MainSettings inputTestSettings;
    inputTestSettings.optionsScreen = OptionsMenuScreen::InputTest;
    inputTestSettings.selectedInputTestOption = 0;
    const auto inputRows = buildControlsOptionsRows(defaultControlsOptionsContext(inputTestSettings));
    record(out, counts,
        inputRows.size() == kOptionsInputTestCount
            && inputRows.size() == 1
            && inputRows.front().label == "PRESS INPUT" ? Status::Pass : Status::Fail,
        "input_test_capture_only_row",
        "rows=" + std::to_string(inputRows.size()));
    setCurrentOptionsSelection(
        inputTestSettings,
        moveCurrentOptionsSelection(inputTestSettings, FrontendKey::Down));
    const bool downStayedCaptured = currentOptionsSelection(inputTestSettings) == 0;
    setCurrentOptionsSelection(
        inputTestSettings,
        moveCurrentOptionsSelection(inputTestSettings, FrontendKey::Up));
    const bool upStayedCaptured = currentOptionsSelection(inputTestSettings) == 0;
    record(out, counts,
        downStayedCaptured && upStayedCaptured ? Status::Pass : Status::Fail,
        "input_test_direction_keys_do_not_navigate",
        "selection=" + std::to_string(currentOptionsSelection(inputTestSettings)));

    ControlProfileBinding profile = makeDefaultControlProfile("player1", 0);
    std::array<bool, SDL_SCANCODE_COUNT> keys{};
    keys[SDL_SCANCODE_A] = true;
    record(out, counts,
        controlActionDown(keys.data(), nullptr, profile, InputAction::LP) ? Status::Pass : Status::Fail,
        "keyboard_action_detected",
        "A -> LP");
    keys[SDL_SCANCODE_A] = false;
    keys[SDL_SCANCODE_RETURN] = true;
    record(out, counts,
        controlActionDown(keys.data(), nullptr, profile, InputAction::Pause) ? Status::Pass : Status::Fail,
        "pause_action_detected",
        "Return -> Pause");
    summary(out, counts);
    return exitCode(counts);
}

int runControlsGlyphDeviceDetection(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-glyph-device-detection\n";
    record(out, counts,
        isPlaystationGamepad(SDL_GAMEPAD_TYPE_PS5) && isXboxGamepad(SDL_GAMEPAD_TYPE_XBOXONE) ? Status::Pass : Status::Fail,
        "device_family_detection",
        "ps5/xboxone");
    record(out, counts,
        physicalInputLabel(gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST), GamepadPromptStyle::Playstation) == "SQ"
            && physicalInputLabel(gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST), GamepadPromptStyle::Xbox) == "X" ? Status::Pass : Status::Fail,
        "west_button_glyph_style",
        physicalInputLabel(gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST), GamepadPromptStyle::Playstation)
            + "/"
            + physicalInputLabel(gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST), GamepadPromptStyle::Xbox));
    record(out, counts,
        physicalInputLabel(gamepadButtonBinding(SDL_GAMEPAD_BUTTON_TOUCHPAD), GamepadPromptStyle::Playstation) == "TP" ? Status::Pass : Status::Fail,
        "touchpad_glyph",
        physicalInputLabel(gamepadButtonBinding(SDL_GAMEPAD_BUTTON_TOUCHPAD), GamepadPromptStyle::Playstation));
    summary(out, counts);
    return exitCode(counts);
}

int runControlsPauseTauntSeparation(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY controls-pause-taunt-separation\n";
    const ControlProfileBinding profile = makeDefaultControlProfile("player1", 0);
    const auto* pause = findActionBinding(profile, InputAction::Pause);
    const auto* taunt = findActionBinding(profile, InputAction::Taunt);
    const bool hasPauseStart = pause && std::any_of(pause->bindings.begin(), pause->bindings.end(), [](const auto& binding) {
        return samePhysicalInput(binding, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_START));
    });
    const bool hasTauntTouchpad = taunt && std::any_of(taunt->bindings.begin(), taunt->bindings.end(), [](const auto& binding) {
        return samePhysicalInput(binding, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_TOUCHPAD));
    });
    const bool separated = pause && taunt && std::none_of(pause->bindings.begin(), pause->bindings.end(), [&](const auto& lhs) {
        return std::any_of(taunt->bindings.begin(), taunt->bindings.end(), [&](const auto& rhs) {
            return samePhysicalInput(lhs, rhs);
        });
    });
    record(out, counts, hasPauseStart ? Status::Pass : Status::Fail, "pause_defaults_to_start", "");
    record(out, counts, hasTauntTouchpad ? Status::Pass : Status::Fail, "taunt_defaults_to_touchpad", "");
    record(out, counts, separated ? Status::Pass : Status::Fail, "pause_and_taunt_separate", "");
    summary(out, counts);
    return exitCode(counts);
}



} // namespace dragon::verification
