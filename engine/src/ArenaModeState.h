#pragma once

// Internal App.cpp implementation header for Arena DLC state helpers.
// Include only from App.cpp after AppState and selection/string helpers exist.

bool isArenaMode(const AppState& state) {
    return state.frontend.pendingMode == PendingMode::Arena;
}

int nearestLivingEnemyIndex(const AppState& state, int ownerIndex);

int arenaCpuCount(const AppState& state) {
    return std::clamp(
        state.selection.sessionSlots.arenaCpuCount,
        state.arenaConfig.cpuCountMin,
        state.arenaConfig.cpuCountMax);
}

int arenaFighterCount(const AppState& state) {
    return 1 + arenaCpuCount(state);
}

int arenaFighterCharacterIndex(const AppState& state, size_t fighterIndex) {
    if (fighterIndex == 0) {
        return sessionP1CharacterIndex(state.selection);
    }
    const size_t cpuSlot = fighterIndex - 1;
    if (cpuSlot < state.selection.sessionSlots.arenaCpuCharacters.size()) {
        return state.selection.sessionSlots.arenaCpuCharacters[cpuSlot];
    }
    return -1;
}

std::string arenaFighterName(const AppState& state, size_t fighterIndex) {
    if (const CharacterSlot* character = characterSlotAt(state.selection, arenaFighterCharacterIndex(state, fighterIndex))) {
        return character->displayName;
    }
    if (fighterIndex == 0) {
        return selectedCharacterName(state.selection);
    }
    return "CPU " + std::to_string(fighterIndex);
}

bool pathEndsWithNoCase(const std::filesystem::path& path, const std::filesystem::path& suffix) {
    const std::string pathText = lowercaseCopy(path.lexically_normal().generic_string());
    const std::string suffixText = lowercaseCopy(suffix.lexically_normal().generic_string());
    return pathText.size() >= suffixText.size()
        && pathText.compare(pathText.size() - suffixText.size(), suffixText.size(), suffixText) == 0;
}

void selectArenaDefaultStage(AppState& state) {
    if (state.selection.stages.empty()) {
        state.selection.selectedStage = 0;
        return;
    }

    const std::filesystem::path configured = state.arenaConfig.defaultStage.is_absolute()
        ? state.arenaConfig.defaultStage
        : state.gameRoot / state.arenaConfig.defaultStage;
    int selected = findStageIndexByDefPath(state.selection, configured);
    if (selected < 0) {
        for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
            if (pathEndsWithNoCase(state.selection.stages[static_cast<size_t>(i)].defPath, state.arenaConfig.defaultStage)) {
                selected = i;
                break;
            }
        }
    }
    state.selection.selectedStage = selected >= 0 ? selected : 0;
}

void setArenaCpuCount(AppState& state, int count) {
    state.selection.sessionSlots.arenaCpuCount = std::clamp(
        count,
        state.arenaConfig.cpuCountMin,
        state.arenaConfig.cpuCountMax);
}

void chooseArenaCpuCharacters(AppState& state) {
    const int characterCount = static_cast<int>(state.selection.characters.size());
    const int playerIndex = sessionP1CharacterIndex(state.selection);
    std::vector<int> candidates;
    candidates.reserve(static_cast<size_t>(std::max(0, characterCount)));
    for (int i = 0; i < characterCount; ++i) {
        if (characterCount > 1 && i == playerIndex) {
            continue;
        }
        candidates.push_back(i);
    }
    if (candidates.empty() && characterCount > 0) {
        candidates.push_back(std::clamp(playerIndex, 0, characterCount - 1));
    }

    unsigned seed = static_cast<unsigned>(state.frame + 17 + std::max(0, playerIndex) * 131);
    for (int i = 0; i < static_cast<int>(state.selection.sessionSlots.arenaCpuCharacters.size()); ++i) {
        if (i >= arenaCpuCount(state) || candidates.empty()) {
            state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(i)] = -1;
            continue;
        }
        seed = seed * 1103515245u + 12345u;
        const size_t picked = static_cast<size_t>(seed) % candidates.size();
        state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(i)] = candidates[picked];
    }
}

int livingArenaFighterCount(const AppState& state) {
    int living = 0;
    for (const auto& fighter : state.fighters) {
        if (fighter.life > 0) {
            ++living;
        }
    }
    return living;
}

int arenaWinnerIndex(const AppState& state) {
    int winner = -1;
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (state.fighters[i].life <= 0) {
            continue;
        }
        if (winner >= 0) {
            return -1;
        }
        winner = static_cast<int>(i);
    }
    return winner;
}
