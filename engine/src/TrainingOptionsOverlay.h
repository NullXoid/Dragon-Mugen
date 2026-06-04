#pragma once

// Internal App.cpp extraction: render-only F2 options and move-list pages.
// Key handling and option mutation stay in App.cpp.

void drawMoveListPage(SDL_Renderer* renderer, const AppState& state) {
    const auto entries = displayableMoveListEntries(state);
    constexpr int visibleRows = 7;
    const int maxScroll = std::max(0, static_cast<int>(entries.size()) - visibleRows);
    const int scroll = std::clamp(state.trainingOptions.moveListScroll, 0, maxScroll);
    const int selected = entries.empty()
        ? -1
        : std::clamp(state.trainingOptions.selectedMoveListEntry, 0, static_cast<int>(entries.size()) - 1);

    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    setColor(renderer, 6, 8, 14, 244);
    fillRect(renderer, 10, 16, 300, 216);
    setColor(renderer, 54, 70, 98);
    drawRect(renderer, 10, 16, 300, 216);

    setColor(renderer, 28, 42, 74);
    fillRect(renderer, 12, 18, 296, 24);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 12, 42, 296, 3);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, 18, 48, 282, 1);

    setColor(renderer, 230, 220, 172);
    debugText(renderer, 20, 24, "COMMAND TRAINING");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 184, 24, fitDebugText(selectedCharacterName(state), 17));
    setColor(renderer, 174, 184, 196);
    debugText(renderer, 20, 38, std::string(trainingMoveCategoryStatus(state.trainingOptions.moveCategory)));

    setColor(renderer, 16, 20, 28, 232);
    fillRect(renderer, 18, 54, 150, 116);
    setColor(renderer, 78, 94, 124);
    drawRect(renderer, 18, 54, 150, 116);
    setColor(renderer, 208, 168, 92);
    debugText(renderer, 26, 62, "TECHNIQUE");

    if (entries.empty()) {
        setColor(renderer, 184, 178, 168);
        debugText(renderer, 26, 84, "No loaded moves");
    } else {
        for (int row = 0; row < visibleRows; ++row) {
            const int index = scroll + row;
            if (index >= static_cast<int>(entries.size())) {
                break;
            }
            const auto& entry = *entries[static_cast<size_t>(index)];
            const float y = 78.0f + static_cast<float>(row * 13);
            if (index == selected) {
                setColor(renderer, 74, 170, 134);
                fillRect(renderer, 24, y - 3.0f, 136, 12);
                setColor(renderer, 8, 12, 16);
            } else {
                setColor(renderer, row % 2 == 0 ? 174 : 138, row % 2 == 0 ? 184 : 150, row % 2 == 0 ? 196 : 164);
            }
            const std::string number = (index + 1 < 10 ? "0" : "") + std::to_string(index + 1);
            debugText(renderer, 28, y, number);
            debugText(renderer, 48, y, fitDebugText(entry.label.empty() ? "State " + commandEntryTargetLabel(entry) : entry.label, 19));
        }
    }

    setColor(renderer, 16, 20, 28, 232);
    fillRect(renderer, 176, 54, 124, 116);
    setColor(renderer, 78, 94, 124);
    drawRect(renderer, 176, 54, 124, 116);
    setColor(renderer, 208, 168, 92);
    debugText(renderer, 184, 62, "DETAIL");

    if (selected >= 0) {
        const auto& entry = *entries[static_cast<size_t>(selected)];
        const std::string moveName = entry.label.empty() ? "State " + commandEntryTargetLabel(entry) : entry.label;
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 184, 82, fitDebugText(moveName, 17));
        setColor(renderer, 128, 171, 225);
        debugText(renderer, 184, 100, "STATE");
        setColor(renderer, 230, 190, 105);
        debugText(renderer, 236, 100, commandEntryTargetLabel(entry));
        setColor(renderer, 128, 171, 225);
        debugText(renderer, 184, 116, "INPUT");
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 184, 130, fitDebugText(moveListInputText(entry), 16));
        setColor(renderer, 128, 171, 225);
        debugText(renderer, 184, 146, "TYPE");
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 236, 146, std::string(trainingMoveCategoryStatus(commandEntryCategory(entry))));
        setColor(renderer, 116, 190, 154);
        debugText(renderer, 184, 158, commandEntryRequiredPower(entry) > 0 ? "POWER " + std::to_string(commandEntryRequiredPower(entry)) : "LOADED");
    }

    setColor(renderer, 34, 26, 30, 238);
    fillRect(renderer, 18, 176, 282, 32);
    setColor(renderer, 158, 64, 58);
    drawRect(renderer, 18, 176, 282, 32);
    setColor(renderer, 230, 220, 172);
    if (selected >= 0) {
        const auto& entry = *entries[static_cast<size_t>(selected)];
        debugText(renderer, 28, 184, "PERFORM");
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 90, 184, fitDebugText(moveListInputText(entry), 26));
    } else {
        debugText(renderer, 28, 184, "No executable commands loaded");
    }

    setColor(renderer, 130, 142, 156);
    const std::string page = entries.empty()
        ? "0/0"
        : std::to_string(selected + 1) + "/" + std::to_string(entries.size());
    debugText(renderer, 20, 216, "UP/DOWN technique  ESC back");
    debugText(renderer, 266, 216, page);
}

void drawTrainingOptionsMenu(SDL_Renderer* renderer, const AppState& state) {
    if (state.trainingOptions.moveListOpen) {
        drawMoveListPage(renderer, state);
        return;
    }

    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    const auto& display = state.trainingOptions;
    setColor(renderer, 8, 10, 12, 236);
    fillRect(renderer, 34, 24, 286, 202);
    setColor(renderer, 92, 104, 124);
    drawRect(renderer, 34, 24, 286, 202);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 46, 34, "TRAINING OPTIONS");

    for (int i = 0; i < kTrainingOptionCount; ++i) {
        const int column = i / kTrainingOptionRows;
        const int row = i % kTrainingOptionRows;
        const float x = column == 0 ? 48.0f : 176.0f;
        const float y = 52.0f + static_cast<float>(row * 14);
        const bool selected = i == display.selectedOption;
        if (selected) {
            setColor(renderer, 74, 170, 134);
            fillRect(renderer, x - 6.0f, y - 3.0f, 116.0f, 12.0f);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 172, 182, 194);
        }
        debugText(renderer, x, y, std::string(trainingOptionLabel(i)));
        debugText(renderer, x + 86.0f, y, trainingOptionStatus(display, i));
    }

    setColor(renderer, 130, 142, 156);
    debugText(renderer, 48, 204, "ENTER toggle");
    debugText(renderer, 176, 204, "RESET POS runs now");
    debugText(renderer, 48, 216, "F2/ESC close");
}
