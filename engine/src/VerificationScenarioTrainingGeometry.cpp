#include "VerificationScenarioCommon.h"

namespace dragon::verification {

int runTrainingOptionsMenuGeometry(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY training-options-menu-geometry\n";

    const int pageCount = (kTrainingOptionCount + kTrainingOptionRows - 1) / kTrainingOptionRows;
    record(out, counts, pageCount == 2 ? Status::Pass : Status::Fail, "page_count",
        "page_count=" + std::to_string(pageCount)
        + " option_count=" + std::to_string(kTrainingOptionCount)
        + " page_rows=" + std::to_string(kTrainingOptionRows));

    for (int page = 0; page < pageCount; ++page) {
        TrainingOptions options;
        options.selectedOption = page * kTrainingOptionRows;
        options.dummyGuardMode = DummyGuardMode::Crouch;
        options.moveCategory = TrainingMoveCategory::Specials;

        std::vector<TrainingOptionRowView> rows;
        const int firstOption = page * kTrainingOptionRows;
        const int lastOption = std::min(kTrainingOptionCount, firstOption + kTrainingOptionRows);
        rows.reserve(kTrainingOptionRows);
        for (int option = firstOption; option < lastOption; ++option) {
            rows.push_back(TrainingOptionRowView{
                std::string(trainingOptionLabel(option)),
                trainingOptionStatus(options, option),
                option == options.selectedOption,
            });
        }

        TrainingOptionsMenuView view;
        view.rows = rows;
        view.pageLabel = std::to_string(page + 1) + "/" + std::to_string(pageCount);

        const auto selectedRows = std::count_if(rows.begin(), rows.end(), [](const TrainingOptionRowView& row) {
            return row.selected;
        });
        const auto geometry = verifyTrainingOptionsMenuGeometry(view);
        const std::string pageName = "page_" + std::to_string(page + 1);
        record(out, counts, rows.size() == static_cast<std::size_t>(kTrainingOptionRows) ? Status::Pass : Status::Fail,
            pageName + "_row_count",
            "rows=" + std::to_string(rows.size())
            + " first_option=" + std::to_string(firstOption)
            + " last_option=" + std::to_string(lastOption - 1));
        record(out, counts, selectedRows == 1 ? Status::Pass : Status::Fail,
            pageName + "_selected_row",
            "selected_rows=" + std::to_string(selectedRows)
            + " selected_option=" + std::to_string(options.selectedOption));
        record(out, counts, geometry.ok ? Status::Pass : Status::Fail,
            pageName + "_geometry_fits",
            geometry.detail);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingMoveListGeometry(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY training-move-list-geometry\n";

    std::vector<TrainingMoveRowView> firstPageRows{
        { "", "Special Move", "", false, "SPECIAL", true, true },
        { "01", "Ground Recovery Roll", "B+DB+D+A", true },
        { "02", "Shoryureppa", "D+DF+F+A", false },
        { "03", "Shinryuken", "D+F+D+F+Z", false },
        { "04", "Shippu Jinrai Kyaku", "D+DB+B+X", false },
        { "05", "Kyouja Renbu", "D+DF+F+C", false },
        { "06", "Kuuchuu Shakunetsu Hadouken", "D+B+D+B+Z", false },
        { "07", "Punch Throw", "A+S", false },
        { "08", "Stand Kick Throw", "Z+X", false },
        { "09", "Shouki Hatsudou", "B+D+F+A+S", false },
    };
    TrainingMoveListView firstPage;
    firstPage.rows = firstPageRows;
    firstPage.detail = TrainingMoveDetailView{
        "Ground Recovery Roll",
        "5200",
        "B+DB+D+A",
        "SPECIALS",
        "LOADED",
        "B+DB+D+A",
        true,
    };
    firstPage.selectedCharacterLabel = "Evil Ken";
    firstPage.categoryLabel = "ALL";
    firstPage.pageLabel = "1/18";
    firstPage.activeTab = TrainingMoveListTab::All;

    std::vector<TrainingMoveRowView> secondPageRows{
        { "", "Super Move", "", false, "SUPER", true, true },
        { "11", "Hadouken", "D+DF+F+A", false },
        { "12", "Shakunetsu Hadouken", "B+DB+D+DF+F+Z", false },
        { "13", "Tatsumaki Senpuukyaku", "D+DB+B+X", false },
        { "14", "Air Tatsumaki Senpuukyaku", "D+DB+B+X", false },
        { "15", "Shin Shoryuken", "D+F+D+F+Z", false },
        { "16", "Raging Demon State 3890", "A+A+F+Z+D", false },
        { "17", "Long Compatibility Stress Command Entry", "D+DF+F+D+DF+F+A+S", false },
        { "18", "Arena Forward Dash Bounds Recovery", "F+F", false },
        { "20", "Command Training Demo Finish Advance", "D+DB+B+D+DB+B+C", true },
    };
    TrainingMoveListView secondPage;
    secondPage.rows = secondPageRows;
    secondPage.detail = TrainingMoveDetailView{
        "Command Training Demo Finish Advance",
        "3000",
        "D+DB+B+D+DB+B+C",
        "SUPERS",
        "POWER 3000",
        "D+DB+B+D+DB+B+C",
        true,
    };
    secondPage.selectedCharacterLabel = "Evil Ken";
    secondPage.categoryLabel = "SUPERS";
    secondPage.pageLabel = "20/20";
    secondPage.activeTab = TrainingMoveListTab::Main;

    const TrainingMoveListView pages[] = { firstPage, secondPage };
    for (int i = 0; i < 2; ++i) {
        const auto& view = pages[i];
        const auto selectedRows = std::count_if(view.rows.begin(), view.rows.end(), [](const TrainingMoveRowView& row) {
            return row.selected;
        });
        const auto geometry = verifyTrainingMoveListGeometry(view);
        const std::string pageName = "sample_" + std::to_string(i + 1);
        record(out, counts, view.rows.size() == static_cast<std::size_t>(kTrainingMoveListRows) ? Status::Pass : Status::Fail,
            pageName + "_row_count",
            "rows=" + std::to_string(view.rows.size())
            + " expected=" + std::to_string(kTrainingMoveListRows));
        record(out, counts, selectedRows == 1 ? Status::Pass : Status::Fail,
            pageName + "_selected_row",
            "selected_rows=" + std::to_string(selectedRows)
            + " page=" + view.pageLabel);
        record(out, counts, geometry.ok ? Status::Pass : Status::Fail,
            pageName + "_geometry_fits",
            geometry.detail);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandHudLayout(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-hud-layout");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const bool selected = runtime.selectTrainingMove("Ground Recovery Roll") || runtime.selectTrainingMoveIndex(0);
    record(out, counts, selected ? Status::Pass : Status::Fail, "selected_training_move",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    runtime.step({}, 1);

    for (int width : { 320, 426, 480 }) {
        const UiGeometryProbe geometry = runtime.trainingCommandHudGeometry(width);
        const std::string prefix = "width_" + std::to_string(width);
        record(out, counts, geometry.ok ? Status::Pass : Status::Fail,
            prefix + "_layout_fits",
            geometry.detail);
        record(out, counts, geometry.visible ? Status::Pass : Status::Fail,
            prefix + "_objective_visible",
            geometry.detail);
        record(out, counts, !geometry.secondaryVisible ? Status::Pass : Status::Fail,
            prefix + "_no_live_bottom_legend",
            geometry.detail);
        record(out, counts, geometry.tertiaryVisible ? Status::Pass : Status::Fail,
            prefix + "_command_icons_visible",
            geometry.detail);
    }

    if (const char* screenshotPath = std::getenv("DRAGON_SCREENSHOT_PATH"); screenshotPath && *screenshotPath) {
        const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
        record(out, counts, captured ? Status::Pass : Status::Fail, "screenshot_captured", screenshotPath);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingPauseHelpLegend(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-pause-help-legend");

    for (int width : { 320, 426, 480 }) {
        const UiGeometryProbe paused = runtime.trainingPauseHelpGeometry(width, false);
        const std::string prefix = "width_" + std::to_string(width);
        record(out, counts, paused.ok ? Status::Pass : Status::Fail,
            prefix + "_pause_help_fits",
            paused.detail);
        record(out, counts, paused.visible && paused.secondaryVisible ? Status::Pass : Status::Fail,
            prefix + "_pause_legend_visible",
            paused.detail);

        const UiGeometryProbe optionsOpen = runtime.trainingPauseHelpGeometry(width, true);
        record(out, counts,
            optionsOpen.ok && !optionsOpen.visible && !optionsOpen.secondaryVisible ? Status::Pass : Status::Fail,
            prefix + "_options_hides_pause_help",
            optionsOpen.detail);
    }

    if (const char* screenshotPath = std::getenv("DRAGON_PAUSE_SCREENSHOT_PATH"); screenshotPath && *screenshotPath) {
        runtime.setFightPaused(true);
        const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
        record(out, counts, captured ? Status::Pass : Status::Fail, "pause_screenshot_captured", screenshotPath);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandListTabs(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-command-list-tabs");

    record(out, counts, runtime.trainingMoveListTab() == "all" ? Status::Pass : Status::Fail,
        "default_tab_all",
        "tab=" + runtime.trainingMoveListTab());

    const auto allMoves = runtime.trainingMoves();
    record(out, counts, !allMoves.empty() ? Status::Pass : Status::Fail,
        "all_tab_has_moves",
        "moves=" + std::to_string(allMoves.size()));
    if (allMoves.empty()) {
        summary(out, counts);
        return exitCode(counts);
    }
    const auto sectionRank = [](const std::string& section) {
        if (section == "STANDING NORMAL") return 0;
        if (section == "CROUCHING NORMAL") return 1;
        if (section == "AIR NORMAL") return 2;
        if (section == "SPECIAL MOVE") return 3;
        if (section == "SUPER MOVE") return 4;
        if (section == "THROW") return 5;
        if (section == "COUNTER") return 6;
        return 7;
    };
    const auto sectionOrderValid = [&sectionRank](const std::vector<TrainingMoveInfo>& moves) {
        int previous = -1;
        for (const auto& move : moves) {
            const int rank = sectionRank(move.section);
            if (rank < previous) {
                return false;
            }
            previous = rank;
        }
        return true;
    };
    const auto containsSection = [](const std::vector<TrainingMoveInfo>& moves, const std::string& section) {
        for (const auto& move : moves) {
            if (move.section == section) {
                return true;
            }
        }
        return false;
    };
    const auto normalFirstOrUnavailable = [&containsSection](const std::vector<TrainingMoveInfo>& moves) {
        return !containsSection(moves, "STANDING NORMAL") || moves.front().section == "STANDING NORMAL";
    };
    const auto firstMoveDetail = [&containsSection](const std::vector<TrainingMoveInfo>& moves) {
        return "first=\"" + moves.front().label + "\" section=\"" + moves.front().section + "\""
            + " standing_normals_present=" + (containsSection(moves, "STANDING NORMAL") ? "true" : "false");
    };
    record(out, counts, normalFirstOrUnavailable(allMoves) ? Status::Pass : Status::Fail,
        "all_tab_starts_on_standard_normals",
        firstMoveDetail(allMoves));
    record(out, counts, sectionOrderValid(allMoves) ? Status::Pass : Status::Fail,
        "all_tab_section_order",
        "first=\"" + allMoves.front().section + "\" last=\"" + allMoves.back().section + "\"");

    runtime.setTrainingMoveListTab("main");
    const auto mainMoves = runtime.trainingMoves();
    record(out, counts, runtime.trainingMoveListTab() == "main" ? Status::Pass : Status::Fail,
        "switch_to_main",
        "tab=" + runtime.trainingMoveListTab());
    record(out, counts, !mainMoves.empty() && mainMoves.size() <= allMoves.size() ? Status::Pass : Status::Fail,
        "main_subset_valid",
        "main=" + std::to_string(mainMoves.size()) + " all=" + std::to_string(allMoves.size()));
    if (!mainMoves.empty()) {
        record(out, counts, normalFirstOrUnavailable(mainMoves) ? Status::Pass : Status::Fail,
            "main_tab_starts_on_standard_normals",
            firstMoveDetail(mainMoves));
        record(out, counts, sectionOrderValid(mainMoves) ? Status::Pass : Status::Fail,
            "main_tab_section_order",
            "first=\"" + mainMoves.front().section + "\" last=\"" + mainMoves.back().section + "\"");
    }

    if (!mainMoves.empty()) {
        const bool selectedLastMain = runtime.selectTrainingMoveIndex(static_cast<int>(mainMoves.size()) - 1);
        record(out, counts, selectedLastMain && runtime.trainingMoveListSelectedRowVisible() ? Status::Pass : Status::Fail,
            "last_main_command_visible_after_scroll",
            "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");

        const std::string selectedLabel = mainMoves.front().label;
        const bool selected = runtime.selectTrainingMove(selectedLabel);
        runtime.setTrainingMoveListTab("all");
        const auto afterAll = runtime.snapshot();
        record(out, counts, selected && afterAll.selectedTrainingMoveLabel == selectedLabel ? Status::Pass : Status::Fail,
            "selection_preserved_to_all",
            "wanted=\"" + selectedLabel + "\" got=\"" + afterAll.selectedTrainingMoveLabel + "\"");

        runtime.setTrainingMoveListTab("main");
        const auto afterMain = runtime.snapshot();
        record(out, counts, afterMain.selectedTrainingMoveLabel == selectedLabel ? Status::Pass : Status::Fail,
            "selection_preserved_back_to_main",
            "wanted=\"" + selectedLabel + "\" got=\"" + afterMain.selectedTrainingMoveLabel + "\"");
    }

    runtime.setTrainingMoveListTab("all");
    const bool selectedLast = runtime.selectTrainingMoveIndex(static_cast<int>(allMoves.size()) - 1);
    record(out, counts, selectedLast && runtime.trainingMoveListSelectedRowVisible() ? Status::Pass : Status::Fail,
        "last_command_visible_after_scroll",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingCommandIconAtlas(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY training-command-icon-atlas\n";

    const CommandInputRenderOptions fallbackOptions;
    const float fallbackWidth = commandInputWidth("DOWN+DF+F+SQ/TRI/L1", fallbackOptions);
    CommandInputRenderOptions missingAtlasOptions;
    missingAtlasOptions.preferBitmapIcons = true;
    missingAtlasOptions.iconAtlas = CommandInputIconAtlasView{};
    const float missingAtlasWidth = commandInputWidth("DOWN+DF+F+SQ/TRI/L1", missingAtlasOptions);
    record(out, counts, fallbackWidth > 0.0f && missingAtlasWidth == fallbackWidth ? Status::Pass : Status::Fail,
        "text_fallback_width",
        "fallback=" + std::to_string(fallbackWidth)
            + " missing_atlas=" + std::to_string(missingAtlasWidth));
    record(out, counts, commandInputIconId("DOWN") == "D"
            && commandInputIconId("FWD") == "F"
            && commandInputIconId("SQUARE") == "SQ"
            && commandInputIconId("BTN_B") == "B"
            && commandInputIconId("TRIANGLE") == "TRI"
            && commandInputIconId("...") == ".."
            ? Status::Pass : Status::Fail,
        "token_normalization",
        "DOWN=" + commandInputIconId("DOWN")
            + " FWD=" + commandInputIconId("FWD")
            + " SQUARE=" + commandInputIconId("SQUARE"));

    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    record(out, counts, runtime.commandIconAtlasLoaded() ? Status::Pass : Status::Fail,
        "png_atlas_loaded",
        runtime.commandIconAtlasLoaded() ? "loaded" : "not loaded");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


} // namespace dragon::verification
