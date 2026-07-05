#pragma once

// Internal App.cpp implementation header for Story Mode selection/state helpers.
// Include only from App.cpp after AppState, SelectionState helpers, and ArenaModeState helpers exist.

#include <filesystem>

bool isStoryMode(const AppState& state) {
    return state.frontend.pendingMode == PendingMode::Story;
}

int storyWaveEnemyCount(int waveIndex) {
    return std::clamp(waveIndex + 1, 1, kStoryMaxEnemies);
}

int findStoryDefaultStageIndex(const AppState& state);

const StoryBoardNode* storyBoardNodeAt(const AppState& state, int index) {
    if (index < 0 || index >= static_cast<int>(state.story.boardRoute.nodes.size())) {
        return nullptr;
    }
    return &state.story.boardRoute.nodes[static_cast<size_t>(index)];
}

const StoryBoardNode* selectedStoryBoardNode(const AppState& state) {
    return storyBoardNodeAt(state, state.story.selectedBoardNode);
}

const StoryBoardNode* activeStoryBoardNode(const AppState& state) {
    return storyBoardNodeAt(state, state.story.activeBoardNode);
}

const StoryBoardNode* nextStoryBoardNode(const AppState& state) {
    return storyBoardNodeAt(state, state.story.activeBoardNode + 1);
}

const StoryBoardNode* nextStoryShopBoardNode(const AppState& state) {
    const StoryBoardNode* node = nextStoryBoardNode(state);
    if (!node || node->kind != StoryBoardNodeKind::Shop) {
        return nullptr;
    }
    return node;
}

bool storyBoardNodeStartsFight(const StoryBoardNode& node) {
    return node.kind != StoryBoardNodeKind::Shop;
}

int nextStoryPlayableBoardNodeIndex(const AppState& state, int startIndex) {
    for (int i = startIndex + 1; i < static_cast<int>(state.story.boardRoute.nodes.size()); ++i) {
        if (storyBoardNodeStartsFight(state.story.boardRoute.nodes[static_cast<size_t>(i)])) {
            return i;
        }
    }
    return -1;
}

StoryBoardRoute fallbackStoryBoardRoute(const AppState& state) {
    StoryBoardRoute route;
    route.id = "select_def_stages";
    route.title = "STORY BOARDS";
    route.nodes.reserve(state.selection.stages.size());
    for (const StageSlot& stage : state.selection.stages) {
        StoryBoardNode node;
        node.id = stage.id;
        node.title = stage.displayName;
        node.stageRef = stage.defPath.generic_string();
        node.kind = stage.openborScrolling || stage.legacyOpenBorSection
            ? StoryBoardNodeKind::SideScroller
            : StoryBoardNodeKind::ArenaBoss;
        node.waves = kStoryWaveCount;
        route.nodes.push_back(std::move(node));
    }
    return route;
}

std::filesystem::path storyBoardRoutePath(const AppState& state) {
    return state.gameRoot / "data" / "story_boards.def";
}

void ensureStoryBoardRouteLoaded(AppState& state) {
    if (state.story.boardRouteLoaded) {
        return;
    }

    StoryBoardRoute route;
    const std::filesystem::path routePath = storyBoardRoutePath(state);
    if (std::filesystem::exists(routePath)) {
        try {
            route = loadStoryBoardRouteFile(routePath);
        } catch (const std::exception& ex) {
            SDL_Log("Story board route parse failed %s: %s", routePath.string().c_str(), ex.what());
        }
    }
    if (route.nodes.empty()) {
        route = fallbackStoryBoardRoute(state);
    }
    state.story.boardRoute = std::move(route);
    state.story.selectedBoardNode = std::clamp(
        state.story.selectedBoardNode,
        0,
        std::max(0, static_cast<int>(state.story.boardRoute.nodes.size()) - 1));
    state.story.activeBoardNode = std::clamp(
        state.story.activeBoardNode,
        0,
        std::max(0, static_cast<int>(state.story.boardRoute.nodes.size()) - 1));
    state.story.boardRouteLoaded = true;
}

bool storyStageRefMatches(const AppState& state, const StageSlot& stage, std::string_view stageRef) {
    const std::string wanted = lowercaseCopy(trim(stageRef));
    if (wanted.empty()) {
        return false;
    }
    if (lowercaseCopy(stage.id) == wanted || lowercaseCopy(stage.displayName) == wanted) {
        return true;
    }

    const std::filesystem::path refPath = std::filesystem::path(std::string(stageRef)).lexically_normal();
    const std::filesystem::path gameRelative = (state.gameRoot / refPath).lexically_normal();
    const std::string refText = lowercaseCopy(refPath.generic_string());
    const std::string gameRefText = lowercaseCopy(gameRelative.generic_string());
    const std::string stagePathText = lowercaseCopy(stage.defPath.lexically_normal().generic_string());
    const std::string stageFileText = lowercaseCopy(stage.defPath.filename().generic_string());
    return stagePathText == refText
        || stagePathText == gameRefText
        || stageFileText == refText
        || stagePathText.ends_with("/" + refText)
        || stagePathText.ends_with("\\" + refText);
}

int storyStageIndexForNode(const AppState& state, const StoryBoardNode& node) {
    if (node.stageRef.empty()) {
        return findStoryDefaultStageIndex(state);
    }
    for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
        if (storyStageRefMatches(state, state.selection.stages[static_cast<size_t>(i)], node.stageRef)) {
            return i;
        }
    }
    return findStoryDefaultStageIndex(state);
}

void syncStorySelectedStageToBoardNode(AppState& state) {
    ensureStoryBoardRouteLoaded(state);
    if (const StoryBoardNode* node = selectedStoryBoardNode(state)) {
        const int stageIndex = storyStageIndexForNode(state, *node);
        if (stageIndex >= 0) {
            state.selection.selectedStage = stageIndex;
        }
    }
}

void selectStoryDefaultBoardNode(AppState& state) {
    ensureStoryBoardRouteLoaded(state);
    int selected = 0;
    for (int i = 0; i < static_cast<int>(state.story.boardRoute.nodes.size()); ++i) {
        const StoryBoardNode& node = state.story.boardRoute.nodes[static_cast<size_t>(i)];
        if (node.kind != StoryBoardNodeKind::Shop) {
            selected = i;
            break;
        }
    }
    state.story.selectedBoardNode = selected;
    state.story.activeBoardNode = selected;
    syncStorySelectedStageToBoardNode(state);
}

void moveStoryBoardNodeSelection(AppState& state, int direction) {
    ensureStoryBoardRouteLoaded(state);
    const int count = static_cast<int>(state.story.boardRoute.nodes.size());
    if (count <= 0) {
        return;
    }
    state.story.selectedBoardNode = (state.story.selectedBoardNode + direction + count) % count;
    syncStorySelectedStageToBoardNode(state);
}

void commitSelectedStoryBoardNode(AppState& state) {
    ensureStoryBoardRouteLoaded(state);
    state.story.activeBoardNode = std::clamp(
        state.story.selectedBoardNode,
        0,
        std::max(0, static_cast<int>(state.story.boardRoute.nodes.size()) - 1));
    syncStorySelectedStageToBoardNode(state);
}

int storyWaveCount(const AppState& state) {
    if (const StoryBoardNode* node = activeStoryBoardNode(state)) {
        return std::max(1, node->waves);
    }
    return kStoryWaveCount;
}

int storySelectedBoardWaveCount(const AppState& state) {
    if (const StoryBoardNode* node = selectedStoryBoardNode(state)) {
        return std::max(1, node->waves);
    }
    return kStoryWaveCount;
}

int storyTotalEnemyCount(const AppState& state) {
    int total = 0;
    const int waves = storyWaveCount(state);
    for (int wave = 0; wave < waves; ++wave) {
        total += storyWaveEnemyCount(wave);
    }
    return total;
}

bool storyEnemySlotActive(const AppState& state, size_t fighterIndex) {
    if (!isStoryMode(state) || fighterIndex == 0 || fighterIndex > static_cast<size_t>(kStoryMaxEnemies)) {
        return false;
    }
    return static_cast<int>(fighterIndex) <= state.story.activeWaveEnemyCount;
}

int storyFighterCount() {
    return 1 + kStoryMaxEnemies;
}

int storyFighterCharacterIndex(const AppState& state, size_t fighterIndex) {
    if (fighterIndex == 0) {
        return sessionP1CharacterIndex(state.selection);
    }
    const size_t enemySlot = fighterIndex - 1;
    if (enemySlot < state.story.enemyCharacterIndices.size()) {
        return state.story.enemyCharacterIndices[enemySlot];
    }
    return -1;
}

std::string storyFighterName(const AppState& state, size_t fighterIndex) {
    if (const CharacterSlot* character = characterSlotAt(state.selection, storyFighterCharacterIndex(state, fighterIndex))) {
        return character->displayName;
    }
    if (fighterIndex == 0) {
        return selectedCharacterName(state.selection);
    }
    return "Enemy " + std::to_string(fighterIndex);
}

bool storyCharacterNameMatches(const CharacterSlot& character, std::string_view wanted) {
    const std::string target = lowercaseCopy(wanted);
    return lowercaseCopy(character.id) == target
        || lowercaseCopy(character.displayName) == target
        || lowercaseCopy(character.folder.filename().string()) == target
        || lowercaseCopy(character.defPath.generic_string()).find(target) != std::string::npos;
}

int findStoryPreferredEnemy(const AppState& state, std::string_view wanted, const std::vector<int>& alreadyPicked) {
    const int p1Index = sessionP1CharacterIndex(state.selection);
    for (int i = 0; i < static_cast<int>(state.selection.characters.size()); ++i) {
        if (i == p1Index && state.selection.characters.size() > 1) {
            continue;
        }
        if (std::find(alreadyPicked.begin(), alreadyPicked.end(), i) != alreadyPicked.end()) {
            continue;
        }
        if (storyCharacterNameMatches(state.selection.characters[static_cast<size_t>(i)], wanted)) {
            return i;
        }
    }
    return -1;
}

void chooseStoryEnemyCharacters(AppState& state) {
    const int characterCount = static_cast<int>(state.selection.characters.size());
    const int p1Index = sessionP1CharacterIndex(state.selection);
    std::vector<int> picked;
    picked.reserve(kStoryMaxEnemies);

    if (const StoryBoardNode* node = activeStoryBoardNode(state)) {
        if (!node->enemyRef.empty()) {
            const int index = findStoryPreferredEnemy(state, node->enemyRef, picked);
            if (index >= 0) {
                picked.push_back(index);
            }
        }
    }

    static constexpr std::array<std::string_view, 5> preferredEnemies{
        "I.Chie",
        "A.Ben",
        "ichie",
        "aben",
        "shopkeeper",
    };
    for (std::string_view preferred : preferredEnemies) {
        if (static_cast<int>(picked.size()) >= kStoryMaxEnemies) {
            break;
        }
        const int index = findStoryPreferredEnemy(state, preferred, picked);
        if (index >= 0) {
            picked.push_back(index);
        }
    }

    for (int i = 0; i < characterCount && static_cast<int>(picked.size()) < kStoryMaxEnemies; ++i) {
        if (i == p1Index && characterCount > 1) {
            continue;
        }
        if (std::find(picked.begin(), picked.end(), i) == picked.end()) {
            picked.push_back(i);
        }
    }
    if (picked.empty() && characterCount > 0) {
        picked.push_back(std::clamp(p1Index, 0, characterCount - 1));
    }
    while (!picked.empty() && static_cast<int>(picked.size()) < kStoryMaxEnemies) {
        picked.push_back(picked[picked.size() % picked.size()]);
    }

    for (size_t i = 0; i < state.story.enemyCharacterIndices.size(); ++i) {
        state.story.enemyCharacterIndices[i] = i < picked.size() ? picked[i] : -1;
    }
}

int findStoryDefaultStageIndex(const AppState& state) {
    if (state.selection.stages.empty()) {
        return 0;
    }

    for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
        const auto& stage = state.selection.stages[static_cast<size_t>(i)];
        const std::string id = lowercaseCopy(stage.id);
        const std::string name = lowercaseCopy(stage.displayName);
        const std::string path = lowercaseCopy(stage.defPath.generic_string());
        if ((id.find("tmnt") != std::string::npos
                || name.find("tmnt") != std::string::npos
                || path.find("tmnt") != std::string::npos)
            && stage.openborScrolling) {
            return i;
        }
    }

    for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
        if (state.selection.stages[static_cast<size_t>(i)].openborScrolling) {
            return i;
        }
    }

    return std::clamp(state.selection.selectedStage, 0, static_cast<int>(state.selection.stages.size()) - 1);
}

void selectStoryDefaultStage(AppState& state) {
    ensureStoryBoardRouteLoaded(state);
    if (state.story.boardRoute.nodes.empty()) {
        state.selection.selectedStage = findStoryDefaultStageIndex(state);
        return;
    }
    syncStorySelectedStageToBoardNode(state);
}

int livingStoryEnemyCount(const AppState& state) {
    if (!isStoryMode(state)) {
        return 0;
    }
    int living = 0;
    const int active = std::clamp(state.story.activeWaveEnemyCount, 0, kStoryMaxEnemies);
    for (int i = 1; i <= active && i < static_cast<int>(state.fighters.size()); ++i) {
        if (state.fighters[static_cast<size_t>(i)].life > 0) {
            ++living;
        }
    }
    return living;
}

std::string storyPlayerGoldStatusSuffix(const AppState& state) {
    if (!state.progression.loaded || !state.progression.data.config.enabled) {
        return {};
    }
    const std::string profileId = dragonProgressionPlayerProfileId(state.progression.save, 0);
    if (isDragonProgressionGuestProfile(profileId)) {
        return {};
    }
    return "  G " + std::to_string(dragonProgressionGoldForProfile(state.progression.save, profileId));
}

int storyNearestLivingEnemyIndex(const AppState& state, int ownerIndex) {
    if (!isStoryMode(state) || ownerIndex < 0 || ownerIndex >= static_cast<int>(state.fighters.size())) {
        return -1;
    }
    if (ownerIndex > 0) {
        return !state.fighters.empty() && state.fighters[0].life > 0 ? 0 : -1;
    }

    const FighterState& owner = state.fighters[0];
    int nearest = -1;
    float nearestDistance = 0.0f;
    const int active = std::clamp(state.story.activeWaveEnemyCount, 0, kStoryMaxEnemies);
    for (int i = 1; i <= active && i < static_cast<int>(state.fighters.size()); ++i) {
        const FighterState& enemy = state.fighters[static_cast<size_t>(i)];
        if (enemy.life <= 0) {
            continue;
        }
        const float dx = enemy.x - owner.x;
        const float dz = arenaDepthActive(state) ? enemy.depthZ - owner.depthZ : 0.0f;
        const float distance = dx * dx + dz * dz;
        if (nearest < 0 || distance < nearestDistance) {
            nearest = i;
            nearestDistance = distance;
        }
    }
    return nearest;
}

int storyProjectileDefenderIndex(const AppState& state, int ownerIndex) {
    if (ownerIndex < 0 || ownerIndex >= static_cast<int>(state.fighters.size())) {
        return -1;
    }
    if (ownerIndex == 0) {
        return storyNearestLivingEnemyIndex(state, 0);
    }
    return !state.fighters.empty() && state.fighters[0].life > 0 ? 0 : -1;
}

std::string storyStatusLine(const AppState& state) {
    if (state.story.stageClear) {
        return "Stage clear  Enemies: "
            + std::to_string(state.story.totalEnemies)
            + "/"
            + std::to_string(state.story.totalEnemies)
            + storyPlayerGoldStatusSuffix(state);
    }
    if (state.story.stageFailed) {
        return "Mission failed" + storyPlayerGoldStatusSuffix(state);
    }
    const int waves = storyWaveCount(state);
    return "Wave "
        + std::to_string(std::clamp(state.story.waveIndex + 1, 1, waves))
        + "/"
        + std::to_string(waves)
        + "  "
        + std::string(storyDifficultyShortLabel(state.story.difficulty))
        + "  Defeated: "
        + std::to_string(std::clamp(state.story.enemiesDefeated, 0, state.story.totalEnemies))
        + "/"
        + std::to_string(std::max(1, state.story.totalEnemies))
        + "  Living: "
        + std::to_string(livingStoryEnemyCount(state))
        + storyPlayerGoldStatusSuffix(state);
}
