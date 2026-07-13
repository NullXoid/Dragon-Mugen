#pragma once

// Internal App.cpp implementation shard.
// Training move list, command HUD, options overlay, and fight-presentation include wiring.

std::string commandEntryTargetLabel(const CommandStateEntry& entry) {
    if (const auto literalTarget = parsePlainIntValue(entry.targetStateExpression)) {
        return std::to_string(*literalTarget);
    }
    return fitDebugText(entry.targetStateExpression.empty() ? std::to_string(entry.targetState) : entry.targetStateExpression, 17);
}

std::string moveListEntryName(const CommandStateEntry& entry) {
    if (!entry.displayLabel.empty()) {
        return entry.displayLabel;
    }
    return entry.label.empty() ? "State " + commandEntryTargetLabel(entry) : entry.label;
}

std::string commandEntrySelectionKey(const CommandStateEntry& entry) {
    std::string key = moveListEntryName(entry);
    key += '\n';
    key += entry.targetStateExpression;
    key += '\n';
    for (const auto& command : entry.requiredCommands) {
        key += command;
        key += ',';
    }
    key += '\n';
    for (const auto& optionGroup : entry.commandOptionGroups) {
        key += '[';
        for (const auto& command : optionGroup) {
            key += command;
            key += ',';
        }
        key += ']';
    }
    return key;
}

bool commandEntryTargetsEquivalentMove(const CommandStateEntry& lhs, const CommandStateEntry& rhs) {
    if (!lhs.targetStateExpression.empty()
        && !rhs.targetStateExpression.empty()
        && lhs.targetStateExpression == rhs.targetStateExpression) {
        return true;
    }
    return lhs.targetState != 0 && lhs.targetState == rhs.targetState;
}

bool commandEntriesShareCommandName(const CommandStateEntry& lhs, const CommandStateEntry& rhs) {
    return anyCommandEntryCommand(lhs, [&rhs](const std::string& command) {
        return commandEntryUsesCommand(rhs, command);
    });
}

bool commandEntriesRepresentSameTrainingMove(const CommandStateEntry& lhs, const CommandStateEntry& rhs) {
    if (&lhs == &rhs || commandEntrySelectionKey(lhs) == commandEntrySelectionKey(rhs)) {
        return true;
    }
    if (commandEntryTargetsEquivalentMove(lhs, rhs)) {
        return true;
    }
    if (!equalsNoCase(moveListEntryName(lhs), moveListEntryName(rhs))) {
        return false;
    }
    return commandEntryCategory(lhs) == commandEntryCategory(rhs)
        && commandEntriesShareCommandName(lhs, rhs);
}

bool commandEntryMatchesActiveTrainingMove(
    const CommandStateEntry& selectedEntry,
    const CommandStateEntry* activeEntry) {
    return activeEntry && commandEntriesRepresentSameTrainingMove(selectedEntry, *activeEntry);
}

bool displayableTrainingMoveEntriesContain(
    const std::vector<const CommandStateEntry*>& entries,
    const CommandStateEntry* candidate) {
    return candidate && std::any_of(entries.begin(), entries.end(), [&](const CommandStateEntry* entry) {
        return entry && commandEntriesRepresentSameTrainingMove(*entry, *candidate);
    });
}

int findMoveListEntryByKey(const std::vector<const CommandStateEntry*>& entries, const std::string& key) {
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        const auto* entry = entries[static_cast<size_t>(i)];
        if (entry && commandEntrySelectionKey(*entry) == key) {
            return i;
        }
    }
    return -1;
}

int trainingMoveListVisibleMoveCapacity() {
    return std::max(1, kTrainingMoveListRows);
}

struct TrainingMoveListScrollMetrics {
    int totalRows = 0;
    int selectedRow = 0;
};

TrainingMoveListScrollMetrics trainingMoveListScrollMetrics(
    const std::vector<const CommandStateEntry*>& entries,
    int selectedEntry) {
    TrainingMoveListScrollMetrics metrics;
    if (entries.empty()) {
        return metrics;
    }

    selectedEntry = std::clamp(selectedEntry, 0, static_cast<int>(entries.size()) - 1);
    std::string previousCategory;
    for (int index = 0; index < static_cast<int>(entries.size()); ++index) {
        const auto& entry = *entries[static_cast<std::size_t>(index)];
        const std::string category = commandEntryMoveListSectionLabel(entry);
        if (index == 0 || category != previousCategory) {
            ++metrics.totalRows;
        }
        if (index == selectedEntry) {
            metrics.selectedRow = metrics.totalRows;
        }
        ++metrics.totalRows;
        previousCategory = category;
    }
    return metrics;
}

void clampTrainingMoveListSelection(AppState& state) {
    const auto& entries = activeDisplayableMoveListEntries(state);
    if (entries.empty()) {
        state.training.options.selectedMoveListEntry = 0;
        state.training.options.moveListScroll = 0;
        return;
    }

    const int maxSelected = static_cast<int>(entries.size()) - 1;
    state.training.options.selectedMoveListEntry =
        std::clamp(state.training.options.selectedMoveListEntry, 0, maxSelected);
    const int visibleRows = trainingMoveListVisibleMoveCapacity();
    const TrainingMoveListScrollMetrics metrics =
        trainingMoveListScrollMetrics(entries, state.training.options.selectedMoveListEntry);
    const int maxScroll = std::max(0, metrics.totalRows - visibleRows);
    if (metrics.selectedRow < state.training.options.moveListScroll) {
        state.training.options.moveListScroll = metrics.selectedRow;
    } else if (metrics.selectedRow >= state.training.options.moveListScroll + visibleRows) {
        state.training.options.moveListScroll = metrics.selectedRow - visibleRows + 1;
    }
    state.training.options.moveListScroll = std::clamp(state.training.options.moveListScroll, 0, maxScroll);
}

void setTrainingMoveListTabPreservingSelection(AppState& state, TrainingMoveListTab tab) {
    if (state.training.options.moveListTab == tab) {
        clampTrainingMoveListSelection(state);
        return;
    }

    const auto& oldEntries = activeDisplayableMoveListEntries(state);
    std::string oldKey;
    if (!oldEntries.empty()) {
        const int oldSelected = std::clamp(
            state.training.options.selectedMoveListEntry,
            0,
            static_cast<int>(oldEntries.size()) - 1);
        oldKey = commandEntrySelectionKey(*oldEntries[static_cast<size_t>(oldSelected)]);
    }

    state.training.options.moveListTab = tab;
    const auto& newEntries = activeDisplayableMoveListEntries(state);
    if (!oldKey.empty()) {
        const int preserved = findMoveListEntryByKey(newEntries, oldKey);
        if (preserved >= 0) {
            state.training.options.selectedMoveListEntry = preserved;
        } else {
            state.training.options.selectedMoveListEntry = std::clamp(
                state.training.options.selectedMoveListEntry,
                0,
                std::max(0, static_cast<int>(newEntries.size()) - 1));
        }
    }
    clampTrainingMoveListSelection(state);
}

#include "TrainingCommandPracticeAssembly.h"

TrainingCommandHudView trainingCommandHudView(
    const AppState& state,
    std::vector<TrainingCommandRowView>& rows,
    std::vector<TrainingCommandStepView>& steps) {
    rows.clear();
    steps.clear();
    TrainingCommandHudView view;
    view.input.visible = state.training.options.showInputHud;
    view.commandsVisible = state.training.options.showCommandHud;
    view.commandIcons = state.commandInputIcons.view();
    view.completionCheck = uiSpriteView(&state.commandCompleteCheck);
    view.physicalDirections = true;
    view.paused = state.frontend.fightPauseOpen || state.frontend.screenshotFreeze;

    if (!view.input.visible && !view.commandsVisible) {
        return view;
    }
    if (state.fighters.empty()) {
        view.input.visible = false;
        view.commandsVisible = false;
        return view;
    }

    const auto& fighter = state.fighters[0];
    view.facing = fighter.facing;
    view.hasGuideAnchor = true;
    view.guideAnchorX = screenCenterX(state) + (fighter.x - state.cameraX);
    const CommandButtonPromptMode promptMode = commandButtonPromptModeForPlayer(state, 0);
    const FighterState* opponent = state.fighters.size() > 1 ? &state.fighters[1] : nullptr;
    const std::vector<std::string> commands = fighter.inputHistory.empty()
        ? std::vector<std::string>{}
        : collectFighterCommands(fighter.inputHistory.back().input, fighter, commandDefinitionsForActor(state, fighter));
    const CommandStateEntry* activeEntry = activeCommandEntry(state, fighter, opponent, commands);

    if (view.input.visible) {
        const std::string current = fighter.inputHistory.empty()
            ? "-"
            : physicalInputDisplayToken(fighter.inputHistory.back().input, promptMode);
        view.input.currentInput = fitDebugText(current, 18);
        view.input.recentInputs = fitDebugText(joinTokens(recentPhysicalInputDisplayTokens(fighter, 8, promptMode), " "), 27);
    }

    if (view.commandsVisible) {
        const auto& entries = activeDisplayableMoveListEntries(state);
        const CommandStateEntry* displayActiveEntry =
            displayableTrainingMoveEntriesContain(entries, activeEntry) ? activeEntry : nullptr;
        const int selected = entries.empty()
            ? -1
            : std::clamp(state.training.options.selectedMoveListEntry, 0, static_cast<int>(entries.size()) - 1);

        constexpr int visibleRows = 7;
        const int maxStart = std::max(0, static_cast<int>(entries.size()) - visibleRows);
        const int firstRow = selected < 0 ? 0 : std::clamp(selected - 2, 0, maxStart);

        rows.reserve(visibleRows);
        for (int row = 0; row < visibleRows; ++row) {
            const int index = firstRow + row;
            if (index >= static_cast<int>(entries.size())) {
                break;
            }
            const auto* entry = entries[static_cast<size_t>(index)];
            if (!entry) {
                continue;
            }
            const CommandDefinition* definition = practiceCommandDefinitionForEntry(state, *entry, commands);
            const std::string inputText = moveListInputText(*entry, promptMode);
            rows.push_back(TrainingCommandRowView{
                fitDebugText(moveListEntryName(*entry), 16),
                fitDebugText(inputText.empty() && definition ? commandDefinitionInputLabel(*definition, promptMode) : inputText, 10),
                commandEntryMatchesActiveTrainingMove(*entry, displayActiveEntry),
                index == selected,
            });
        }

        view.categoryLabel = state.training.options.moveListTab == TrainingMoveListTab::Main
            ? "MAIN"
            : std::string(trainingMoveCategoryStatus(state.training.options.moveCategory));
        view.pageLabel = entries.empty()
            ? "0/0"
            : std::to_string(selected + 1) + "/" + std::to_string(entries.size());
        view.showMeLabel = "SHOW:H/L3/R3";
        view.nextMoveLabel = "SEL NEXT/2S";
        view.demoActive = trainingCommandDemoActive(state);
        view.completionVisible = state.training.commandPractice.flashTicks > 0
            && !state.training.commandPractice.notification.empty();
        view.completionTicks = state.training.commandPractice.flashTicks;
        view.completionLabel = fitDebugText(state.training.commandPractice.notification, 21);
        if (displayActiveEntry) {
            view.activeCommandLabel = fitDebugText(moveListEntryName(*displayActiveEntry), 19);
        }
        if (selected >= 0) {
            const auto& entry = *entries[static_cast<size_t>(selected)];
            const bool complete = commandEntryMatchesActiveTrainingMove(entry, displayActiveEntry);
            const CommandDefinition* definition = practiceCommandDefinitionForEntry(state, entry, commands);
            const int matched = definition ? matchedPracticeStepCount(fighter, *definition) : 0;
            view.currentMoveName = fitDebugText(moveListEntryName(entry), 20);
            const std::string inputText = moveListInputText(entry, promptMode);
            view.currentMoveInput = fitDebugText(inputText, 25);
            view.completeFlash = complete || view.completionVisible;

            const bool definitionActive = definition && commandDefinitionActive(*definition, fighter);
            if (!entry.displayInput.empty()) {
                appendPresentationPracticeSteps(steps, inputText, matched, complete || definitionActive);
            } else if (definition) {
                appendDefinitionPracticeSteps(steps, fighter, *definition, complete || definitionActive, promptMode);
            } else {
                appendEntryPracticeSteps(steps, entry, commands, complete, promptMode);
            }
        } else {
            view.currentMoveName = "No loaded moves";
            view.currentMoveInput = "-";
        }
    }

    if (view.commandsVisible) {
        const FighterInputState currentInput = fighter.inputHistory.empty()
            ? FighterInputState{}
            : fighter.inputHistory.back().input;
        if (view.input.visible) {
            view.input.expectedInput = fitDebugText(currentTrainingCommandStepLabel(steps), 16);
        }
        view.buttonGuide = trainingCommandButtonGuideView(currentInput, promptMode, steps);
        view.directionGuide = trainingCommandDirectionGuideView(currentInput, fighter.facing, steps);
        if (view.paused || state.training.options.showHitboxes || state.freezeWatch.visible) {
            view.buttonGuide.visible = false;
            view.directionGuide.visible = false;
        }
    }

    view.commandRows = rows;
    view.practiceSteps = steps;
    return view;
}

void drawTrainingCommandHud(SDL_Renderer* renderer, const AppState& state) {
    std::vector<TrainingCommandRowView> rows;
    std::vector<TrainingCommandStepView> steps;
    const TrainingCommandHudView view = trainingCommandHudView(state, rows, steps);
    dragon::drawTrainingCommandOverlay(uiRenderContext(renderer, state), view);
}

TrainingOptionsMenuView trainingOptionsMenuView(const AppState& state, std::vector<TrainingOptionRowView>& rows) {
    rows.clear();
    const auto& options = state.training.options;
    const int pageCount = (kTrainingOptionCount + kTrainingOptionRows - 1) / kTrainingOptionRows;
    const int selected = std::clamp(options.selectedOption, 0, kTrainingOptionCount - 1);
    const int page = std::clamp(selected / kTrainingOptionRows, 0, pageCount - 1);
    const int firstOption = page * kTrainingOptionRows;
    const int lastOption = std::min(kTrainingOptionCount, firstOption + kTrainingOptionRows);

    rows.reserve(kTrainingOptionRows);
    for (int i = firstOption; i < lastOption; ++i) {
        rows.push_back(TrainingOptionRowView{
            std::string(trainingOptionLabel(i)),
            trainingOptionStatus(options, i),
            i == selected,
        });
    }

    TrainingOptionsMenuView view;
    view.rows = rows;
    view.pageLabel = std::to_string(page + 1) + "/" + std::to_string(pageCount);
    return view;
}

TrainingMoveListView trainingMoveListView(const AppState& state, std::vector<TrainingMoveRowView>& rows) {
    rows.clear();
    constexpr int visibleRows = kTrainingMoveListRows;
    const int visibleMoveRows = trainingMoveListVisibleMoveCapacity();

    const auto& entries = activeDisplayableMoveListEntries(state);
    const CommandButtonPromptMode promptMode = commandButtonPromptModeForPlayer(state, 0);
    const int selected = entries.empty()
        ? -1
        : std::clamp(state.training.options.selectedMoveListEntry, 0, static_cast<int>(entries.size()) - 1);

    std::vector<TrainingMoveRowView> allRows;
    allRows.reserve(entries.size() + 6);
    std::string previousCategory;
    for (int index = 0; index < static_cast<int>(entries.size()); ++index) {
        const auto& entry = *entries[static_cast<size_t>(index)];
        const std::string category = commandEntryMoveListSectionLabel(entry);
        if (index == 0 || category != previousCategory) {
            allRows.push_back(TrainingMoveRowView{
                "",
                category,
                "",
                false,
                category,
                true,
                true,
            });
        }
        allRows.push_back(TrainingMoveRowView{
            (index + 1 < 10 ? "0" : "") + std::to_string(index + 1),
            moveListEntryName(entry),
            moveListInputText(entry, promptMode),
            index == selected,
            category,
            index == 0 || category != previousCategory,
        });
        previousCategory = category;
    }

    const int maxScroll = std::max(0, static_cast<int>(allRows.size()) - visibleMoveRows);
    const int scroll = std::clamp(state.training.options.moveListScroll, 0, maxScroll);
    rows.reserve(visibleRows);
    for (int row = scroll; row < static_cast<int>(allRows.size()) && static_cast<int>(rows.size()) < visibleRows; ++row) {
        rows.push_back(allRows[static_cast<std::size_t>(row)]);
    }

    TrainingMoveListView view;
    view.rows = rows;
    view.commandIcons = state.commandInputIcons.view();
    view.selectedCharacterLabel = fitDebugText(selectedCharacterName(state.selection), 17);
    view.categoryLabel = std::string(trainingMoveCategoryStatus(state.training.options.moveCategory));
    view.physicalDirections = true;
    if (!state.fighters.empty()) {
        view.facing = state.fighters[0].facing;
    }
    view.pageLabel = entries.empty()
        ? "0/0"
        : std::to_string(selected + 1) + "/" + std::to_string(entries.size());
    view.activeTab = state.training.options.moveListTab;
    view.selectedIndex = selected;
    view.firstVisibleIndex = scroll;
    view.totalCount = static_cast<int>(allRows.size());
    view.visibleCapacity = visibleMoveRows;
    view.empty = entries.empty();

    if (selected >= 0) {
        const auto& entry = *entries[static_cast<size_t>(selected)];
        const std::string inputText = moveListInputText(entry, promptMode);
        const int requiredPower = commandEntryRequiredPower(entry);
        view.detail = TrainingMoveDetailView{
            fitDebugText(moveListEntryName(entry), 17),
            commandEntryTargetLabel(entry),
            fitDebugText(inputText, 16),
            commandEntryMoveListSectionLabel(entry),
            requiredPower > 0 ? "POWER " + std::to_string(requiredPower) : "LOADED",
            fitDebugText(inputText, 26),
            true,
        };
    }

    return view;
}

void drawTrainingMoveListPage(SDL_Renderer* renderer, const AppState& state) {
    std::vector<TrainingMoveRowView> rows;
    const TrainingMoveListView view = trainingMoveListView(state, rows);
    dragon::drawTrainingMoveListPage(uiRenderContext(renderer, state), view);
}

void drawTrainingOptionsMenu(SDL_Renderer* renderer, const AppState& state) {
    if (state.training.options.moveListOpen) {
        drawTrainingMoveListPage(renderer, state);
        return;
    }

    std::vector<TrainingOptionRowView> rows;
    const TrainingOptionsMenuView view = trainingOptionsMenuView(state, rows);
    dragon::drawTrainingOptionsMenu(uiRenderContext(renderer, state), view);
}
