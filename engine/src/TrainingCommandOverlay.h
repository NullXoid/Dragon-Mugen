#pragma once

// Internal App.cpp extraction: render-only training command/input HUD.
// Display helpers remain read-only and drive no gameplay state changes.

void drawTrainingCommandHud(SDL_Renderer* renderer, const AppState& state) {
    if (!state.trainingOptions.showCommandHud && !state.trainingOptions.showInputHud) {
        return;
    }
    if (state.fighters.empty()) {
        return;
    }

    const auto& fighter = state.fighters[0];
    const FighterState* opponent = state.fighters.size() > 1 ? &state.fighters[1] : nullptr;
    const std::vector<std::string> commands = fighter.inputHistory.empty()
        ? std::vector<std::string>{}
        : collectFighterCommands(fighter.inputHistory.back().input, fighter, state.commandDefinitions);
    const CommandStateEntry* activeEntry = activeCommandEntry(state, fighter, opponent, commands);
    const float scale = uiScale(state);
    const float widthF = logicalWidthF(state);
    const float panelW = std::min(194.0f, widthF - 16.0f);
    const float panelX = widthF - panelW - 8.0f;
    float panelY = 42.0f;
    float panelH = state.trainingOptions.showInputHud && state.trainingOptions.showCommandHud ? 112.0f : 62.0f;

    setColor(renderer, 6, 8, 14, 216);
    fillScaledRect(renderer, scale, panelX, panelY, panelW, panelH);
    setColor(renderer, 54, 70, 98);
    drawScaledRect(renderer, scale, panelX, panelY, panelW, panelH);

    float y = panelY + 8.0f;
    if (state.trainingOptions.showInputHud) {
        const std::string current = fighter.inputHistory.empty()
            ? "-"
            : inputDisplayToken(fighter.inputHistory.back().input, fighter.facing);
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, "INPUT");
        setColor(renderer, 222, 226, 232);
        scaledDebugText(renderer, scale, panelX + 58.0f, y, fitDebugText(current, 18));
        y += 14.0f;

        const auto recent = recentInputDisplayTokens(fighter, 8);
        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, fitDebugText(joinTokens(recent, " "), 27));
        y += 20.0f;
    }

    if (state.trainingOptions.showCommandHud) {
        const auto entries = displayableMoveListEntries(state);
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, "COMMANDS");
        y += 13.0f;

        int drawn = 0;
        for (const auto* entry : entries) {
            if (!entry || drawn >= 5) {
                break;
            }
            const bool active = activeEntry == entry;
            if (active) {
                setColor(renderer, 74, 170, 134);
                fillScaledRect(renderer, scale, panelX + 6.0f, y - 2.0f, panelW - 12.0f, 11.0f);
                setColor(renderer, 8, 12, 16);
            } else {
                setColor(renderer, 174, 184, 196);
            }
            const std::string label = entry->label.empty() ? "State " + commandEntryTargetLabel(*entry) : entry->label;
            scaledDebugText(renderer, scale, panelX + 10.0f, y, fitDebugText(label, 15));
            scaledDebugText(renderer, scale, panelX + 116.0f, y, fitDebugText(moveListInputText(*entry), 10));
            ++drawn;
            y += 12.0f;
        }

        if (activeEntry) {
            setColor(renderer, 230, 190, 105);
            scaledDebugText(renderer, scale, panelX + 8.0f, panelY + panelH - 14.0f, "MATCH " + fitDebugText(activeEntry->label, 19));
        }
    }
}
