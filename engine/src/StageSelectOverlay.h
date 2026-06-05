#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

void drawStageSelect(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    setColor(renderer, 36, 34, 30);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    setColor(renderer, 24, 30, 38);
    fillRect(renderer, 0, 176, widthF, 64);
    setColor(renderer, 94, 78, 54);
    fillRect(renderer, 0, 174, widthF, 1);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 220, 178, 112);
    debugText(renderer, 20, 30, "STAGE SELECT");
    setColor(renderer, 155, 164, 174);
    debugText(renderer, 210, 30, "2/2 ARENA");

    drawPanel(renderer, 18, 52, 284, 108);

    if (state.selection.stages.empty()) {
        setColor(renderer, 230, 130, 120);
        debugText(renderer, 32, 72, "No stages found in game/stages");
    } else {
        for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
            const float y = 66.0f + static_cast<float>(i * 20);
            const auto& stage = state.selection.stages[static_cast<size_t>(i)];
            if (i == state.selection.selectedStage) {
                const int pulse = 150 + ((state.frame / 8) % 55);
                setColor(renderer, static_cast<Uint8>(pulse), 124, 58);
                fillRect(renderer, 28, y - 3, 140, 15);
                setColor(renderer, 8, 12, 16);
                debugText(renderer, 36, y, stage.displayName);
            } else {
                setColor(renderer, 184, 178, 168);
                debugText(renderer, 36, y, stage.displayName);
            }
        }

        const auto& selected = state.selection.stages[static_cast<size_t>(state.selection.selectedStage)];
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 182, 68, selected.displayName);
        setColor(renderer, 155, 164, 174);
        debugText(renderer, 182, 84, "id: " + selected.id);
        debugText(renderer, 182, 96, "author: " + selected.author);
        debugText(renderer, 182, 116, "fighter: " + selectedCharacterName(state.selection));
        debugText(renderer, 182, 132, "opponent: " + compactSettingText(opponentDisplayName(state), 11));
    }

    setColor(renderer, 118, 126, 138);
    debugText(renderer, 20, 204, "UP/DOWN choose  ENTER start  ESC fighter select");
    SDL_RenderPresent(renderer);
}
