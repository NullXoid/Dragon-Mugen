    std::vector<verification::RosterCharacterInfo> selectableCharacters() const override {
        const std::vector<CharacterSlot> characters = state_.selection.characters.empty()
            ? loadCharacters(gameRoot_)
            : state_.selection.characters;
        std::vector<verification::RosterCharacterInfo> out;
        out.reserve(characters.size());
        for (const auto& character : characters) {
            out.push_back(verification::RosterCharacterInfo{
                character.id,
                character.displayName,
                character.defPath.string(),
                compatibilityProfileName(character.compatibilityProfile),
            });
        }
        return out;
    }

    std::vector<verification::TrainingMoveInfo> trainingMovesForMode(CommandButtonPromptMode mode) const {
        std::vector<verification::TrainingMoveInfo> moves;
        const auto& entries = activeDisplayableMoveListEntries(state_);
        moves.reserve(entries.size());
        for (const auto* entry : entries) {
            if (!entry) {
                continue;
            }
            int targetState = -1;
            std::vector<std::string> commandNames = entry->requiredCommands;
            for (const auto& optionGroup : entry->commandOptionGroups) {
                for (const auto& command : optionGroup) {
                    if (!commandListContains(commandNames, command)) {
                        commandNames.push_back(command);
                    }
                }
            }
            if (const auto literalTarget = parsePlainIntValue(entry->targetStateExpression)) {
                targetState = *literalTarget;
            } else if (!state_.fighters.empty()) {
                std::vector<std::string> commands = entry->requiredCommands;
                for (const auto& optionGroup : entry->commandOptionGroups) {
                    if (!optionGroup.empty()) {
                        commands.push_back(optionGroup.front());
                    }
                }
                const FighterState& demoFighter = state_.fighters.size() > 1 ? state_.fighters[1] : state_.fighters[0];
                const FighterState* opponent = state_.fighters.size() > 1 ? &state_.fighters[0] : nullptr;
                if (const auto resolvedTarget = resolveCommandTargetState(state_, demoFighter, opponent, *entry, commands)) {
                    targetState = *resolvedTarget;
                }
            }
            moves.push_back(verification::TrainingMoveInfo{
                moveListEntryName(*entry),
                moveListInputText(*entry, mode),
                targetState,
                entry->requiredStateType,
                commandEntryRequiredPower(*entry),
                std::move(commandNames),
                commandEntryMoveListSectionLabel(*entry),
            });
        }
        return moves;
    }

    std::vector<verification::TrainingMoveInfo> trainingMoves() const override {
        return trainingMovesForMode(CommandButtonPromptMode::Strength);
    }

    std::vector<verification::TrainingMoveInfo> trainingMovesForPromptStyle(std::string_view style) const override {
        if (style == "playstation") {
            return trainingMovesForMode(CommandButtonPromptMode::Playstation);
        }
        if (style == "xbox") {
            return trainingMovesForMode(CommandButtonPromptMode::Xbox);
        }
        return trainingMovesForMode(CommandButtonPromptMode::Strength);
    }

    void setTrainingMoveCategory(std::string_view category) override {
        if (category == "normal" || category == "normals") {
            state_.training.options.moveCategory = TrainingMoveCategory::Normals;
        } else if (category == "special" || category == "specials") {
            state_.training.options.moveCategory = TrainingMoveCategory::Specials;
        } else if (category == "super" || category == "supers") {
            state_.training.options.moveCategory = TrainingMoveCategory::Supers;
        } else {
            state_.training.options.moveCategory = TrainingMoveCategory::All;
        }
        state_.trainingMoveListCacheValid = false;
        state_.training.options.selectedMoveListEntry = 0;
        state_.training.options.moveListScroll = 0;
        clampTrainingMoveListSelection(state_);
    }

    std::string trainingMoveListTab() const override {
        return state_.training.options.moveListTab == TrainingMoveListTab::Main ? "main" : "all";
    }

    void setTrainingMoveListTab(std::string_view tab) override {
        if (tab == "main") {
            setTrainingMoveListTabPreservingSelection(state_, TrainingMoveListTab::Main);
            return;
        }
        setTrainingMoveListTabPreservingSelection(state_, TrainingMoveListTab::All);
    }

    bool trainingMoveListSelectedRowVisible() const override {
        std::vector<TrainingMoveRowView> rows;
        const TrainingMoveListView view = trainingMoveListView(state_, rows);
        if (view.empty) {
            return false;
        }
        return std::any_of(rows.begin(), rows.end(), [](const TrainingMoveRowView& row) {
            return row.selected;
        });
    }

    bool commandIconAtlasLoaded() const override {
        return commandInputIconAtlasReady(state_.commandInputIcons.view());
    }

    std::string trainingCurrentInputDisplay() const override {
        if (state_.fighters.empty() || state_.fighters[0].inputHistory.empty()) {
            return "-";
        }
        return physicalInputDisplayToken(
            state_.fighters[0].inputHistory.back().input,
            commandButtonPromptModeForPlayer(state_, 0));
    }

    std::string trainingExpectedInputDisplay() const override {
        std::vector<TrainingCommandRowView> rows;
        std::vector<TrainingCommandStepView> steps;
        const TrainingCommandHudView view = trainingCommandHudView(state_, rows, steps);
        return view.input.expectedInput;
    }

    std::string trainingDirectionGuideState() const override {
        std::vector<TrainingCommandRowView> rows;
        std::vector<TrainingCommandStepView> steps;
        const TrainingCommandHudView view = trainingCommandHudView(state_, rows, steps);
        std::string out;
        for (size_t i = 0; i < view.directionGuide.directions.size(); ++i) {
            const auto& direction = view.directionGuide.directions[i];
            if (i > 0) {
                out += ";";
            }
            out += direction.label;
            out += ":";
            out += direction.pressed ? "p" : "-";
            out += direction.required ? "r" : "-";
            out += direction.matched ? "m" : "-";
        }
        return out;
    }

    std::string trainingButtonGuideState() const override {
        std::vector<TrainingCommandRowView> rows;
        std::vector<TrainingCommandStepView> steps;
        const TrainingCommandHudView view = trainingCommandHudView(state_, rows, steps);
        std::string out;
        for (size_t i = 0; i < view.buttonGuide.buttons.size(); ++i) {
            const auto& button = view.buttonGuide.buttons[i];
            if (i > 0) {
                out += ";";
            }
            out += button.label;
            out += ":";
            out += button.pressed ? "p" : "-";
            out += button.required ? "r" : "-";
            out += button.matched ? "m" : "-";
        }
        out += ";SYSTEM:";
        out += view.buttonGuide.systemButtonVisible ? "v" : "-";
        out += ":";
        out += view.buttonGuide.systemButton.label;
        out += ":";
        out += view.buttonGuide.systemButton.pressed ? "p" : "-";
        out += view.buttonGuide.systemButton.required ? "r" : "-";
        out += view.buttonGuide.systemButton.matched ? "m" : "-";
        return out;
    }

    bool trainingCommandCompleteFlash() const override {
        std::vector<TrainingCommandRowView> rows;
        std::vector<TrainingCommandStepView> steps;
        const TrainingCommandHudView view = trainingCommandHudView(state_, rows, steps);
        return view.completeFlash;
    }

    verification::UiGeometryProbe trainingCommandHudGeometry(int logicalWidth) const override {
        std::vector<TrainingCommandRowView> rows;
        std::vector<TrainingCommandStepView> steps;
        const TrainingCommandHudView view = trainingCommandHudView(state_, rows, steps);
        const TrainingCommandHudGeometryReport report = verifyTrainingCommandHudGeometry(view, logicalWidth);
        return verification::UiGeometryProbe{
            report.ok,
            report.objectiveVisible,
            report.bottomLegendVisible,
            report.commandIconsVisible,
            report.detail,
        };
    }

    verification::UiGeometryProbe trainingPauseHelpGeometry(int logicalWidth, bool optionsOpen) const override {
        const TrainingPauseHelpView view{ !optionsOpen };
        const TrainingPauseHelpGeometryReport report = verifyTrainingPauseHelpGeometry(view, logicalWidth);
        return verification::UiGeometryProbe{
            report.ok,
            report.visible,
            report.legendVisible,
            false,
            report.detail,
        };
    }

    bool selectTrainingMoveIndex(int index) override {
        const auto& entries = activeDisplayableMoveListEntries(state_);
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            return false;
        }
        state_.training.options.selectedMoveListEntry = index;
        clampTrainingMoveListSelection(state_);
        return true;
    }

    bool selectTrainingMove(std::string_view label) override {
        const std::string wanted = lowercaseCopy(label);
        const auto& entries = activeDisplayableMoveListEntries(state_);
        for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
            if (lowercaseCopy(moveListEntryName(*entries[static_cast<size_t>(i)])) != wanted) {
                continue;
            }
            state_.training.options.selectedMoveListEntry = i;
            clampTrainingMoveListSelection(state_);
            return true;
        }
        return false;
    }

    void startTrainingCommandDemo() override {
        beginTrainingCommandDemo(state_);
    }

    void pressTrainingShowShortcut() override {
        triggerTrainingShowShortcut(state_);
    }
