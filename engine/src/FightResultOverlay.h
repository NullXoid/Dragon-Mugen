#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

std::string_view matchResultLabel(int option) {
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "REMATCH",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

void drawRoundCalloutBand(SDL_Renderer* renderer, const AppState& state, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    setColor(renderer, 6, 8, 14, 210);
    fillRect(renderer, centerX - 116.0f, 82, 232, 34);
    setColor(renderer, r, g, b);
    fillRect(renderer, centerX - 114.0f, 84, 228, 2);
    fillRect(renderer, centerX - 114.0f, 114, 228, 2);
    debugTextCentered(renderer, centerX, 96, fitDebugText(text, 28));
}

void drawRoundStartOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (state.matchPhaseTicks < state.fightRoundSettings.startWaitTime) {
        return;
    }

    const bool fightText = state.matchPhaseTicks >= singleFightRoundDisplayEndTick(state);
    drawRoundCalloutBand(renderer, state, fightText ? "FIGHT" : roundStartCalloutText(state), 230, 220, 172);
}

void drawRoundFinishOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (state.matchPhaseTicks < singleFightRoundFinishCalloutTicks(state)) {
        drawRoundCalloutBand(renderer, state, roundFinishCalloutText(state), 230, 190, 105);
        return;
    }
    if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
        drawRoundCalloutBand(renderer, state, roundResultText(state), 222, 226, 232);
    }
}

void drawRoundResultOverlay(SDL_Renderer* renderer, const AppState& state) {
    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    setColor(renderer, 6, 8, 14, 224);
    fillRect(renderer, centerX - 98.0f, 76, 196, 70);
    setColor(renderer, 230, 190, 105);
    drawRect(renderer, centerX - 98.0f, 76, 196, 70);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, centerX - 96.0f, 78, 192, 2);

    setColor(renderer, 222, 226, 232);
    debugTextCentered(renderer, centerX, 94, fitDebugText(roundResultText(state), 24));
    setColor(renderer, 230, 220, 172);
    drawRoundWinPips(renderer, centerX - 42.0f, 112, state.roundWins[0], matchWinsRequired(state), false, 6.0f);
    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, centerX, 111, "-");
    drawRoundWinPips(renderer, centerX + 42.0f, 112, state.roundWins[1], matchWinsRequired(state), true, 6.0f);
    setColor(renderer, 130, 142, 156);
    debugTextCentered(renderer, centerX, 130, state.matchComplete ? "MATCH COMPLETE" : "NEXT ROUND");
}

void drawMatchResultScreen(SDL_Renderer* renderer, const AppState& state) {
    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    setColor(renderer, 6, 8, 14, 238);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    setColor(renderer, 34, 40, 52);
    fillRect(renderer, 0, 0, widthF, 52);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 0, 52, widthF, 2);

    const int winner = matchWinner(state);
    const std::string winnerText = winner == 0 ? "DRAW GAME" : uppercaseCopy(fighterResultName(state, winner));

    setColor(renderer, 230, 220, 172);
    debugText(renderer, 22, 18, "MATCH COMPLETE");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 198, 18, isMatchMode(state) ? std::string(pendingModeTitle(state.pendingMode)) : "");

    setColor(renderer, 222, 226, 232);
    debugTextCentered(renderer, centerX, 72, fitDebugText(winnerText, 28));
    setColor(renderer, 230, 190, 105);
    debugTextCentered(renderer, centerX, 94, singleFightScoreText(state));
    setColor(renderer, 174, 184, 196);
    debugTextCentered(renderer, centerX, 112, matchWinMethodText(state));
    const std::string quote = winner > 0 && winner <= static_cast<int>(state.fighters.size())
        ? selectedVictoryQuoteText(state, state.fighters[static_cast<size_t>(winner - 1)])
        : std::string{};
    if (!quote.empty()) {
        debugTextCentered(renderer, centerX, 128, fitDebugText("\"" + quote + "\"", 40));
        debugTextCentered(renderer, centerX, 142, fitDebugText("Stage: " + selectedStageName(state.selection), 34));
    } else {
        debugTextCentered(renderer, centerX, 128, fitDebugText("Stage: " + selectedStageName(state.selection), 34));
    }

    for (int i = 0; i < kMatchResultOptionCount; ++i) {
        const float y = (quote.empty() ? 154.0f : 166.0f) + static_cast<float>(i * 16);
        if (i == state.selectedMatchResultOption) {
            setColor(renderer, 74, 170, 134);
            fillRect(renderer, centerX - 64.0f, y - 3.0f, 128, 13);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 174, 184, 196);
        }
        debugTextCentered(renderer, centerX, y, std::string(matchResultLabel(i)));
    }

    setColor(renderer, 130, 142, 156);
    debugTextCentered(renderer, centerX, 224, "ENTER select");
}
