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

int arenaTimerSeconds(const AppState& state) {
    const int seconds = state.selection.sessionSlots.arenaTimerSeconds;
    return (seconds == 30 || seconds == 60 || seconds == 99) ? seconds : 0;
}

std::string arenaTimerLabel(const AppState& state) {
    const int seconds = arenaTimerSeconds(state);
    return seconds <= 0 ? "INF" : std::to_string(seconds);
}

void cycleArenaTimer(AppState& state, int direction) {
    static constexpr std::array<int, 4> timers{ 0, 30, 60, 99 };
    const int current = arenaTimerSeconds(state);
    auto it = std::find(timers.begin(), timers.end(), current);
    int index = it == timers.end() ? 0 : static_cast<int>(std::distance(timers.begin(), it));
    index = (index + (direction >= 0 ? 1 : -1) + static_cast<int>(timers.size())) % static_cast<int>(timers.size());
    state.selection.sessionSlots.arenaTimerSeconds = timers[static_cast<size_t>(index)];
}

bool arenaZAxisEnabled(const AppState& state) {
    return state.selection.sessionSlots.arenaZAxisEnabled;
}

bool arenaCameraRotationSelected(const AppState& state) {
    return state.selection.sessionSlots.arenaCameraRotationEnabled;
}

void setArenaDefaultsFromConfig(AppState& state) {
    setArenaCpuCount(state, state.arenaConfig.cpuCountDefault);
    state.selection.sessionSlots.arenaTimerSeconds = state.arenaConfig.timerDefault;
    state.selection.sessionSlots.arenaZAxisEnabled = state.arenaConfig.zAxisDefault;
    state.selection.sessionSlots.arenaCameraRotationEnabled = state.arenaConfig.cameraRotationDefault;
}

int cycleArenaCpuCharacter(AppState& state, int slot, int direction) {
    if (slot < 0 || slot >= static_cast<int>(state.selection.sessionSlots.arenaCpuCharacters.size())) {
        return -1;
    }

    const int characterCount = static_cast<int>(state.selection.characters.size());
    const int playerIndex = sessionP1CharacterIndex(state.selection);
    std::vector<int> choices{ -1 };
    choices.reserve(static_cast<size_t>(std::max(0, characterCount)) + 1);
    for (int i = 0; i < characterCount; ++i) {
        if (characterCount > 1 && i == playerIndex) {
            continue;
        }
        choices.push_back(i);
    }
    if (choices.size() == 1 && characterCount > 0) {
        choices.push_back(std::clamp(playerIndex, 0, characterCount - 1));
    }

    int current = state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(slot)];
    auto it = std::find(choices.begin(), choices.end(), current);
    int index = it == choices.end() ? 0 : static_cast<int>(std::distance(choices.begin(), it));
    index = (index + (direction >= 0 ? 1 : -1) + static_cast<int>(choices.size())) % static_cast<int>(choices.size());
    current = choices[static_cast<size_t>(index)];
    state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(slot)] = current;
    return current;
}

std::string arenaCpuSlotLabel(const AppState& state, int slot) {
    if (slot < 0 || slot >= static_cast<int>(state.selection.sessionSlots.arenaCpuCharacters.size())) {
        return "Random";
    }
    const int characterIndex = state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(slot)];
    if (characterIndex < 0) {
        return "Random";
    }
    if (const CharacterSlot* character = characterSlotAt(state.selection, characterIndex)) {
        return character->displayName;
    }
    return "Random";
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
    std::vector<int> pickedCharacters;
    for (int i = 0; i < static_cast<int>(state.selection.sessionSlots.arenaCpuCharacters.size()); ++i) {
        if (i >= arenaCpuCount(state) || candidates.empty()) {
            state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(i)] = -1;
            continue;
        }
        int chosen = state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(i)];
        if (!characterSlotAt(state.selection, chosen) || (characterCount > 1 && chosen == playerIndex)) {
            seed = seed * 1103515245u + 12345u;
            std::vector<int> available = candidates;
            if (static_cast<int>(candidates.size()) >= arenaCpuCount(state)) {
                available.erase(
                    std::remove_if(available.begin(), available.end(), [&pickedCharacters](int index) {
                        return std::find(pickedCharacters.begin(), pickedCharacters.end(), index) != pickedCharacters.end();
                    }),
                    available.end());
            }
            if (available.empty()) {
                available = candidates;
            }
            const size_t picked = static_cast<size_t>(seed) % available.size();
            chosen = available[picked];
        }
        state.selection.sessionSlots.arenaCpuCharacters[static_cast<size_t>(i)] = chosen;
        pickedCharacters.push_back(chosen);
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
