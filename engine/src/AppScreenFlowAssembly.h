#pragma once

// Internal App.cpp implementation shard.
// Arena setup, stage select, versus screen, world render, and training debug include wiring.

void drawStageSelectPreviewBackground(SDL_Renderer* renderer, const AppState& state);
void ensureSelectedStagePreviewBackground(SDL_Renderer* renderer, AppState& state);

void drawArenaSetup(SDL_Renderer* renderer, AppState& state) {
    ensureSelectedStagePreviewBackground(renderer, state);
    drawStageSelectPreviewBackground(renderer, state);

    ArenaSetupView view;
    view.title = state.arenaConfig.modeName;
    view.description = state.arenaConfig.description;
    view.fighterName = compactSettingText(selectedCharacterName(state.selection), 18);
    view.cpuCount = arenaCpuCount(state);
    for (int i = 0; i < static_cast<int>(view.cpuNames.size()); ++i) {
        view.cpuNames[static_cast<size_t>(i)] = compactSettingText(arenaCpuSlotLabel(state, i), 20);
    }
    view.modeLabel = "Free-for-all";
    view.stageName = compactSettingText(selectedStageName(state.selection), 22);
    view.timerLabel = arenaTimerLabel(state);
    view.zAxisEnabled = arenaZAxisEnabled(state);
    view.cameraRotationEnabled = arenaCameraRotationSelected(state);
    view.selectedOption = state.frontend.selectedArenaSetupOption;
    view.frame = state.frame;
    drawArenaSetupOverlay(uiRenderContext(renderer, state), view);
    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

void drawStageLayer(SDL_Renderer* renderer, const AppState& state, int layerNo);
void drawFallbackStage(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, float cameraY);

void ensureSelectedStagePreviewBackground(SDL_Renderer* renderer, AppState& state) {
    const int selectedIndex = state.selection.selectedStage;
    const StageSlot* selected = selectedStageSlot(state.selection);
    if (!selected) {
        destroyStageBackground(state.stageBackground);
        state.stageBackgroundStageIndex = -1;
        return;
    }

    state.cameraX = selected->cameraStartx;
    state.cameraY = selected->cameraStarty;
    if (state.stageBackgroundStageIndex == selectedIndex) {
        return;
    }

    destroyStageBackground(state.stageBackground);
    state.stageBackgroundStageIndex = selectedIndex;
    try {
        state.stageBackground = loadStageBackground(renderer, *selected);
    } catch (const std::exception& ex) {
        SDL_Log("Stage select preview load failed %s: %s", selected->displayName.c_str(), ex.what());
    }
}

void drawStageSelectPreviewBackground(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    if (!state.stageBackground.empty()) {
        drawStageLayer(renderer, state, 0);
        drawStageLayer(renderer, state, 1);
    } else {
        drawFallbackStage(renderer, state, stage, state.cameraY);
    }
}

void drawStageSelect(SDL_Renderer* renderer, AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Story) {
        ensureStoryBoardRouteLoaded(state);
        syncStorySelectedStageToBoardNode(state);
    }
    ensureSelectedStagePreviewBackground(renderer, state);
    drawStageSelectPreviewBackground(renderer, state);

    if (state.frontend.pendingMode == PendingMode::Story) {
        std::vector<StoryStageCardView> stages;
        stages.reserve(state.story.boardRoute.nodes.size());
        for (int i = 0; i < static_cast<int>(state.story.boardRoute.nodes.size()); ++i) {
            const StoryBoardNode& node = state.story.boardRoute.nodes[static_cast<std::size_t>(i)];
            const int stageIndex = storyStageIndexForNode(state, node);
            const StageSlot* stage = stageSlotAt(state.selection, stageIndex);
            stages.push_back(StoryStageCardView{
                node.title.empty() ? (stage ? stage->displayName : node.id) : node.title,
                node.id,
                stage && !stage->author.empty() ? stage->author : std::string("STORY BOARD"),
                storyBoardNodeKindLabel(node.kind),
                i == state.story.selectedBoardNode,
                stage ? (stage->openborScrolling || stage->legacyOpenBorSection) : false,
                node.kind == StoryBoardNodeKind::Shop,
                node.kind == StoryBoardNodeKind::MidBoss || node.kind == StoryBoardNodeKind::ArenaBoss,
            });
        }

        StoryStageSelectView view;
        view.stages = stages;
        view.fighterLabel = selectedCharacterName(state.selection);
        view.routeTitle = state.story.boardRoute.title;
        view.selectedIndex = state.story.selectedBoardNode;
        view.waveCount = storySelectedBoardWaveCount(state);
        view.frame = state.frame;
        view.difficultyLabel = std::string(storyDifficultyShortLabel(state.story.difficulty));
        if (const StoryBoardNode* node = selectedStoryBoardNode(state)) {
            view.selectedStageName = node->title.empty() ? node->id : node->title;
            view.selectedNodeKind = storyBoardNodeKindLabel(node->kind);
            view.selectedNodeTarget = !node->shopRef.empty() ? node->shopRef : node->enemyRef;
        }
        if (const StageSlot* selected = selectedStageSlot(state.selection)) {
            view.selectedStageAuthor = selected->displayName;
        }
        drawStoryStageSelectOverlay(uiRenderContext(renderer, state), view);
        drawFpsCounter(renderer, state);
        presentPresentationFrame(renderer, state);
        return;
    }

    std::vector<StageSelectRowView> rows;
    rows.reserve(state.selection.stages.size());
    for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
        const auto& stage = state.selection.stages[static_cast<std::size_t>(i)];
        rows.push_back(StageSelectRowView{
            stage.displayName,
            i == state.selection.selectedStage,
        });
    }

    StageSelectView view;
    view.rows = rows;
    view.modeLabel = std::string(pendingModeTitle(state.frontend.pendingMode));
    view.frame = state.frame;
    view.fighterLabel = selectedCharacterName(state.selection);
    view.opponentLabel = compactSettingText(opponentDisplayName(state), 11);
    view.hasStagePreview = true;
    if (const StageSlot* selected = selectedStageSlot(state.selection)) {
        view.selectedStageName = selected->displayName;
        view.selectedStageId = selected->id;
        view.selectedStageAuthor = selected->author;
    }

    drawStageSelectOverlay(uiRenderContext(renderer, state), view);
    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

void drawVersusScreen(SDL_Renderer* renderer, const AppState& state) {
    drawVersusScreenOverlay(
        uiRenderContext(renderer, state),
        versusScreenView(state));
    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

bool hasSelectedStageBackground(const AppState& state) {
    return !state.stageBackground.empty();
}

#include "WorldRender.h"

#include "TrainingDebugViewAssembly.h"
