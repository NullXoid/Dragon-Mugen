#include "VerificationScenarioCommon.h"

#include "DragonUi.h"
#include "MainMenuOverlay.h"
#include "UiMenuList.h"
#include "dragon/MugenText.h"

#include <fstream>
#include <iterator>
#include <optional>

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
            && videoRows[0].label == "RESOLUTION"
            && videoRows[1].label == "UI SCALE"
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

int runMainMenuResponsiveLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY main-menu-responsive-layout\n";

    struct Case {
        CanvasPreset preset;
        const char* name;
    };
    constexpr std::array<Case, 5> cases{ {
        { CanvasPreset::Classic320x240, "classic_320x240" },
        { CanvasPreset::Wide426x240, "wide_426x240" },
        { CanvasPreset::Extra480x240, "extra_480x240" },
        { CanvasPreset::Sd854x480, "sd_854x480" },
        { CanvasPreset::Hd1280x720, "hd_1280x720" },
    } };

    for (const Case& item : cases) {
        const CanvasDimensions dimensions = presentationDimensions();
        const DragonUiMetrics metrics = dragonUiMetricsForCanvas(dimensions, 1.0f);
        const UiRenderContext ui{
            nullptr,
            dimensions.width,
            dimensions.height,
            1.0f,
            dimensionsForPreset(item.preset).width,
            dimensionsForPreset(item.preset).height,
        };
        const SDL_FRect rect = dragon::mainMenuPanelRect(ui);
        const bool inside =
            rect.x >= -0.01f
            && rect.y >= metrics.topBarH - 0.01f
            && rect.x + rect.w <= static_cast<float>(dimensions.width) + 0.01f
            && rect.y + rect.h <= static_cast<float>(dimensions.height) + 0.01f;
        record(out, counts, inside ? Status::Pass : Status::Fail,
            std::string(item.name) + "_panel_inside_canvas",
            "x=" + std::to_string(rect.x)
                + " y=" + std::to_string(rect.y)
                + " w=" + std::to_string(rect.w)
                + " h=" + std::to_string(rect.h));

        const float availableH = static_cast<float>(dimensions.height) - metrics.topBarH;
        const bool heightReasonable = rect.h <= availableH * 0.84f;
        record(out, counts, heightReasonable ? Status::Pass : Status::Fail,
            std::string(item.name) + "_panel_height_reasonable",
            "height_ratio=" + std::to_string(rect.h / std::max(1.0f, availableH)));

        const float expectedW = 176.0f * metrics.pixelScale;
        const float expectedH = 110.0f * metrics.pixelScale;
        const bool normalizedScale = std::fabs(rect.w - expectedW) <= 0.01f
            && std::fabs(rect.h - expectedH) <= 0.01f;
        record(out, counts, normalizedScale ? Status::Pass : Status::Fail,
            std::string(item.name) + "_stable_presentation_grid_ui_size",
            "expected=" + std::to_string(expectedW) + "x" + std::to_string(expectedH)
                + " actual=" + std::to_string(rect.w) + "x" + std::to_string(rect.h));
    }

    summary(out, counts);
    return exitCode(counts);
}

std::string verificationUnquote(std::string value) {
    value = trim(value);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string verificationPropertyValue(const MugenSection* section, std::string_view key) {
    if (!section) {
        return {};
    }
    const auto* property = findProperty(*section, key);
    return property ? verificationUnquote(property->value) : std::string{};
}

int runMainMenuEditablePresentationData(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY main-menu-editable-presentation-data\n";

    const auto root = std::filesystem::path(runtime.rootText());
    const auto dragonDef = root / "data" / "dragon.def";
    const auto systemDef = root / "data" / "system.def";

    MugenDocument dragonDoc;
    bool dragonParsed = false;
    try {
        dragonDoc = parseMugenTextFile(dragonDef);
        dragonParsed = true;
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "dragon_def_parse", ex.what());
    }

    const MugenSection* dragonMenu = dragonParsed ? findSection(dragonDoc, "Dragon.MainMenu") : nullptr;
    record(out, counts, dragonMenu ? Status::Pass : Status::Fail,
        "dragon_main_menu_section",
        dragonDef.string());
    if (dragonMenu) {
        const std::string background = verificationPropertyValue(dragonMenu, "background");
        record(out, counts, !background.empty() ? Status::Pass : Status::Fail,
            "dragon_main_menu_background_configurable",
            "background=" + background);

        constexpr std::array<std::string_view, kMainMenuOptionCount> dragonLabelKeys{ {
            "label.training",
            "label.single_player",
            "label.vs_mode",
            "label.arena_mode",
            "label.story_mode",
            "label.shop_demo",
            "label.options",
            "label.exit",
        } };
        for (std::string_view key : dragonLabelKeys) {
            const std::string value = verificationPropertyValue(dragonMenu, key);
            record(out, counts, !value.empty() ? Status::Pass : Status::Fail,
                "dragon_main_menu_" + std::string(key) + "_editable",
                value);
        }
    }

    MugenDocument systemDoc;
    bool systemParsed = false;
    try {
        systemDoc = parseMugenTextFile(systemDef);
        systemParsed = true;
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "system_def_parse", ex.what());
    }

    const MugenSection* titleInfo = systemParsed ? findSection(systemDoc, "Title Info") : nullptr;
    record(out, counts, titleInfo ? Status::Pass : Status::Fail,
        "mugen_title_info_section",
        systemDef.string());
    if (titleInfo) {
        constexpr std::array<std::string_view, 5> motifLabelKeys{ {
            "menu.itemname.training",
            "menu.itemname.arcade",
            "menu.itemname.versus",
            "menu.itemname.options",
            "menu.itemname.exit",
        } };
        for (std::string_view key : motifLabelKeys) {
            const std::string value = verificationPropertyValue(titleInfo, key);
            record(out, counts, !value.empty() ? Status::Pass : Status::Fail,
                "mugen_title_" + std::string(key) + "_fallback",
                value);
        }
    }

    summary(out, counts);
    return exitCode(counts);
}

int runVideoResolutionStableVirtualLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY video-resolution-stable-virtual-layout\n";

    const CanvasDimensions presentation = presentationDimensions();
    record(out, counts,
        presentation.width == kDesignLogicalWidth && presentation.height == kDesignLogicalHeight
            ? Status::Pass : Status::Fail,
        "presentation_uses_fixed_design_canvas",
        std::to_string(presentation.width) + "x" + std::to_string(presentation.height));

    struct Case {
        CanvasPreset preset;
        const char* name;
    };
    constexpr std::array<Case, 5> cases{ {
        { CanvasPreset::Classic320x240, "classic_320x240" },
        { CanvasPreset::Wide426x240, "wide_426x240" },
        { CanvasPreset::Extra480x240, "extra_480x240" },
        { CanvasPreset::Sd854x480, "sd_854x480" },
        { CanvasPreset::Hd1280x720, "hd_1280x720" },
    } };

    std::optional<SDL_FRect> expectedMainMenuPanel;
    std::optional<std::string> expectedOptionsGeometry;

    for (const Case& item : cases) {
        const CanvasDimensions output = outputDimensionsForPreset(item.preset);
        const UiRenderContext ui{
            nullptr,
            presentation.width,
            presentation.height,
            1.0f,
            output.width,
            output.height,
        };

        record(out, counts,
            !canvasPresetChangesLayout(item.preset) ? Status::Pass : Status::Fail,
            std::string(item.name) + "_declares_output_only",
            std::to_string(output.width) + "x" + std::to_string(output.height));

        const SDL_FRect mainPanel = mainMenuPanelRect(ui);
        if (!expectedMainMenuPanel) {
            expectedMainMenuPanel = mainPanel;
        }
        const bool sameMainPanel =
            std::fabs(mainPanel.x - expectedMainMenuPanel->x) <= 0.01f
            && std::fabs(mainPanel.y - expectedMainMenuPanel->y) <= 0.01f
            && std::fabs(mainPanel.w - expectedMainMenuPanel->w) <= 0.01f
            && std::fabs(mainPanel.h - expectedMainMenuPanel->h) <= 0.01f;
        record(out, counts,
            sameMainPanel ? Status::Pass : Status::Fail,
            std::string(item.name) + "_main_menu_panel_stable",
            "panel=" + std::to_string(static_cast<int>(mainPanel.x)) + ","
                + std::to_string(static_cast<int>(mainPanel.y)) + " "
                + std::to_string(static_cast<int>(mainPanel.w)) + "x"
                + std::to_string(static_cast<int>(mainPanel.h)));

        MainSettings settings;
        settings.canvasPreset = item.preset;
        settings.optionsScreen = OptionsMenuScreen::Video;
        settings.selectedVideoOption = 0;
        auto rows = buildControlsOptionsRows(defaultControlsOptionsContext(settings));
        const OptionsMenuView view = buildControlsOptionsView(defaultControlsOptionsContext(settings), rows);
        std::vector<UiMenuListRowView> menuRows;
        menuRows.reserve(rows.size());
        for (const auto& row : rows) {
            menuRows.push_back(UiMenuListRowView{
                row.label,
                row.value,
                row.selected,
                row.adjustable,
                row.disabled,
            });
        }
        const UiMenuListGeometryReport optionsGeometry = verifyUiMenuListGeometry(
            UiMenuListView{
                menuRows,
                view.title,
                view.pageLabel,
                view.labelHeader.empty() ? "SETTING" : view.labelHeader,
                view.valueHeader,
                view.padSummary,
                view.footer,
            },
            static_cast<float>(presentation.width),
            UiMenuListStyle{
                40.0f,
                300.0f,
                406.0f,
                18.0f,
                true,
                1.0f,
            });
        if (!expectedOptionsGeometry) {
            expectedOptionsGeometry = optionsGeometry.detail;
        }
        record(out, counts,
            optionsGeometry.ok && optionsGeometry.detail == *expectedOptionsGeometry ? Status::Pass : Status::Fail,
            std::string(item.name) + "_options_geometry_stable",
            optionsGeometry.detail);
    }

    summary(out, counts);
    return exitCode(counts);
}

int runVideoHdFullscreenWindowPolicy(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY video-hd-fullscreen-window-policy\n";

    const MainSettings settings;
    record(out, counts,
        settings.canvasPreset == kStandardCanvasPreset
                && settings.canvasPreset == CanvasPreset::Hd1280x720
            ? Status::Pass
            : Status::Fail,
        "default_canvas_preset_is_hd_720p",
        canvasSizeSettingText(settings));

    record(out, counts,
        kWindowWidth == 1280 && kWindowHeight == 720 ? Status::Pass : Status::Fail,
        "standard_window_size_is_1280x720",
        std::to_string(kWindowWidth) + "x" + std::to_string(kWindowHeight));

    record(out, counts,
        WindowPresentationState{}.fullscreen ? Status::Pass : Status::Fail,
        "default_window_mode_is_fullscreen",
        "fullscreen=true");

    const auto repoRoot = std::filesystem::path(runtime.rootText()).parent_path();
    const auto loopPath = repoRoot / "engine" / "src" / "AppMainLoopAssembly.h";
    std::string loopText;
    if (std::ifstream in(loopPath); in) {
        loopText.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    record(out, counts,
        loopText.find("SDL_SetWindowFullscreen") != std::string::npos ? Status::Pass : Status::Fail,
        "fullscreen_toggle_uses_sdl_window_fullscreen",
        loopPath.string());

    record(out, counts,
        loopText.find("SDL_MinimizeWindow") != std::string::npos ? Status::Pass : Status::Fail,
        "minimize_shortcut_uses_sdl_minimize",
        loopPath.string());

    record(out, counts,
        loopText.find("SDLK_F11") != std::string::npos
                && loopText.find("SDLK_RETURN") != std::string::npos
                && loopText.find("SDL_KMOD_ALT") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "fullscreen_shortcuts_registered",
        "F11 and Alt+Enter");

    record(out, counts,
        loopText.find("SDLK_M") != std::string::npos
                && loopText.find("SDL_MinimizeWindow") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "minimize_shortcut_registered",
        "Alt+M");

    summary(out, counts);
    return exitCode(counts);
}



} // namespace dragon::verification
