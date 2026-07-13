#pragma once

// Internal App.cpp implementation shard.
// Fight HUD, results, pause, screenshot, and frontend flow include wiring.

FightComboCounterView fightComboCounterView(const AppState& state, size_t attackerIndex) {
    FightComboCounterView view;
    if (attackerIndex >= state.display.comboCounters.size()) {
        return view;
    }

    const auto& combo = state.display.comboCounters[attackerIndex];
    const auto& settings = state.fightRoundSettings.combo;
    view.displayHits = combo.displayHits;
    view.displayTicks = combo.displayTicks;
    view.displayTime = std::max(1, settings.displayTime);
    view.frame = state.frame;
    view.posX = settings.posX;
    view.posY = settings.posY;
    view.startX = settings.startX;
    view.counterFontPalette = settings.counterFontPalette;
    view.counterShake = settings.counterShake;
    view.text = settings.text;
    view.textFontPalette = settings.textFontPalette;
    view.textOffsetX = settings.textOffsetX;
    view.textOffsetY = settings.textOffsetY;
    return view;
}

FightPowerGaugeView fightPowerGaugeView(const AppState& state, size_t fighterIndex) {
    FightPowerGaugeView view;
    if (fighterIndex >= state.fighters.size()) {
        return view;
    }

    const bool p2 = fighterIndex == 1;
    const auto& settings = state.fightRoundSettings.powerbar;
    view.value = state.fighters[fighterIndex].power;
    view.maxValue = std::max(1, characterConstantsForActor(state, state.fighters[fighterIndex]).maxPower);
    view.anchorX = motifOriginX(state) + (p2 ? settings.p2PosX : settings.p1PosX);
    view.y = p2 ? settings.p2PosY : settings.p1PosY;
    view.rangeStart = p2 ? settings.p2RangeStart : settings.p1RangeStart;
    view.rangeEnd = p2 ? settings.p2RangeEnd : settings.p1RangeEnd;
    return view;
}

std::string singleFightStatusLine(const AppState& state) {
    if (state.matchPhase == MatchPhase::RoundStart) {
        return roundStartCalloutText(state);
    }
    if (state.matchPhase == MatchPhase::RoundFinish) {
        if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
            return roundResultText(state);
        }
        return roundFinishCalloutText(state);
    }
    if (state.matchPhase == MatchPhase::RoundResult) {
        return roundResultText(state);
    }
    if (state.matchPhase == MatchPhase::MatchResult) {
        return "MATCH COMPLETE";
    }
    if (!state.gamepads.empty()) {
        return "Pads " + gamepadActionLayoutText(state, 0) + "  Start pause";
    }
    if (state.frontend.pendingMode == PendingMode::SinglePlayer) {
        return "P1 arrows A/S/D Z/X/C  CPU opponent";
    }
    return "P1 arrows A/S/D Z/X/C  P2 I/J/K/L U/O/P N/M/,";
}

FightHudView fightHudView(const AppState& state) {
    FightHudView view;
    view.p1.name = selectedCharacterName(state.selection);
    if (state.progression.loaded && state.progression.data.config.enabled) {
        if (const CharacterSlot* p1 = sessionP1CharacterSlot(state.selection)) {
            const std::string p1ProfileId = dragonProgressionPlayerProfileId(state.progression.save, 0);
            view.p1.progressionLabel = compactSettingText(
                dragonProgressionCharacterSummaryForProfile(
                    state.progression.data,
                    state.progression.save,
                    p1ProfileId,
                    p1->id),
                18);
        }
    }
    view.p1.life = state.fighters[0].life;
    view.p1.maxLife = characterMaxLifeForFighterIndex(state, 0);
    view.p1.power = fightPowerGaugeView(state, 0);

    view.p2.name = compactSettingText(opponentDisplayName(state), 12);
    view.p2.life = state.fighters[1].life;
    view.p2.maxLife = characterMaxLifeForFighterIndex(state, 1);
    view.p2.power = fightPowerGaugeView(state, 1);
    if (state.progression.loaded
        && state.progression.data.config.enabled
        && activeOpponentType(state) == OpponentType::LocalP2) {
        const std::string p2ProfileId = dragonProgressionPlayerProfileId(state.progression.save, 1);
        if (isDragonProgressionGuestProfile(p2ProfileId)) {
            view.p2.progressionLabel = "GUEST";
        } else if (const CharacterSlot* p2 = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
            view.p2.progressionLabel = compactSettingText(
                dragonProgressionCharacterSummaryForProfile(
                    state.progression.data,
                    state.progression.save,
                    p2ProfileId,
                    p2->id),
                18);
        }
    }
    if (state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story) {
        view.arenaMode = true;
        const size_t visibleFighters = state.frontend.pendingMode == PendingMode::Story
            ? static_cast<size_t>(1 + std::clamp(state.story.activeWaveEnemyCount, 0, kStoryMaxEnemies))
            : state.fighters.size();
        view.arenaFighterCount = static_cast<int>(std::min(view.arenaFighters.size(), visibleFighters));
        for (int i = 0; i < view.arenaFighterCount; ++i) {
            auto& fighterView = view.arenaFighters[static_cast<size_t>(i)];
            std::string prefix = i == 0
                ? "P1 "
                : (state.frontend.pendingMode == PendingMode::Story ? "E" + std::to_string(i) + " " : "P" + std::to_string(i + 1) + " ");
            if (state.frontend.pendingMode == PendingMode::Story && i > 0) {
                prefix += std::string(storyDifficultyShortLabel(state.story.difficulty)) + " ";
            }
            const FighterState& fighter = state.fighters[static_cast<size_t>(i)];
            const bool storyEnemy = state.frontend.pendingMode == PendingMode::Story && i > 0;
            const std::string name = storyEnemy
                ? std::string(storyWaveRoleLabel(storyWaveRole(state, state.story.waveIndex)))
                : (state.frontend.pendingMode == PendingMode::Story
                    ? storyFighterName(state, static_cast<size_t>(i))
                    : arenaFighterName(state, static_cast<size_t>(i)));
            const int defaultMaxLife = characterMaxLifeForFighterIndex(state, static_cast<size_t>(i));
            fighterView.name = compactSettingText(prefix + name, 13);
            fighterView.life = fighter.life;
            fighterView.maxLife = fighter.maxLifeOverride > 0 ? fighter.maxLifeOverride : defaultMaxLife;
            fighterView.power = fightPowerGaugeView(state, static_cast<size_t>(i));
        }
    }

    view.comboCounters[0] = fightComboCounterView(state, 0);
    view.comboCounters[1] = fightComboCounterView(state, 1);
    view.showMatchTimer = isMatchMode(state);
    view.currentRound = state.currentRound;
    if (state.frontend.pendingMode != PendingMode::Training) {
        if (state.frontend.pendingMode == PendingMode::Arena) {
            view.versusLine =
                "P1 " + compactSettingText(selectedCharacterName(state.selection), 11)
                + " vs " + std::to_string(arenaCpuCount(state)) + " CPU FFA";
        } else if (state.frontend.pendingMode == PendingMode::Story) {
            view.versusLine =
                "P1 " + compactSettingText(selectedCharacterName(state.selection), 11)
                + " vs Enemy Waves";
        } else {
            view.versusLine =
                "P1 " + compactSettingText(selectedCharacterName(state.selection), 11)
                + " vs " + compactSettingText(opponentDisplayName(state), 9);
        }
    }

    if (view.showMatchTimer) {
        const int winsRequired = matchWinsRequired(state);
        view.timerSeconds = std::max(0, (state.matchTimerTicks + 59) / 60);
        const int configuredTimer = state.frontend.pendingMode == PendingMode::Arena
            ? arenaTimerSeconds(state)
            : (state.frontend.pendingMode == PendingMode::Story ? 0 : state.mainSettings.matchTimerSeconds);
        view.timerText = configuredTimer <= 0 ? "INF" : std::to_string(view.timerSeconds);
        view.p1.roundPips = FightRoundPipsView{ state.roundWins[0], winsRequired };
        view.p2.roundPips = FightRoundPipsView{ state.roundWins[1], winsRequired, true };
    }

    if (state.frontend.pendingMode == PendingMode::Training
        && state.training.options.showHitLog
        && state.messages.lastHitTextTicks > 0
        && !state.messages.lastHitText.empty()) {
        view.bottomLine = state.messages.lastHitText;
        view.bottomLineHighlighted = true;
    } else if (isMatchMode(state)) {
        if (state.frontend.pendingMode == PendingMode::Arena && state.matchPhase == MatchPhase::Fight) {
            view.bottomLine = "Last fighter standing  Living: " + std::to_string(livingArenaFighterCount(state));
        } else if (state.frontend.pendingMode == PendingMode::Story) {
            view.bottomLine = storyStatusLine(state);
        } else {
            view.bottomLine = singleFightStatusLine(state);
        }
        view.bottomLineHighlighted = isSingleFightResultPhase(state);
    } else if (state.frontend.pendingMode != PendingMode::Training) {
        view.bottomLine = "A/S/D Z/X/C  R reset  F1 boxes  F2 options";
    }
    return view;
}

void drawFightHudView(SDL_Renderer* renderer, const AppState& state) {
    drawFightHud(uiRenderContext(renderer, state), fightHudView(state));
}

std::string_view matchResultLabel(int option) {
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "REMATCH",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

std::string_view arenaMatchResultLabel(int option) {
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "PLAY AGAIN",
        "ARENA SETUP",
        "FIGHTER SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

std::string_view storyMatchResultLabel(const AppState& state, int option) {
    if (option == 0 && storyCanContinueRoute(state)) {
        return "CONTINUE";
    }
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "TRY AGAIN",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

FightRoundCalloutView roundStartOverlayView(const AppState& state) {
    FightRoundCalloutView view;
    if (state.matchPhaseTicks < state.fightRoundSettings.startWaitTime) {
        return view;
    }

    const bool fightText = state.matchPhaseTicks >= singleFightRoundDisplayEndTick(state);
    view.visible = true;
    view.text = fightText ? "FIGHT" : roundStartCalloutText(state);
    view.r = 230;
    view.g = 220;
    view.b = 172;
    view.frame = state.matchPhaseTicks;
    return view;
}

FightRoundCalloutView roundFinishOverlayView(const AppState& state) {
    FightRoundCalloutView view;
    if (state.matchPhaseTicks < singleFightRoundFinishCalloutTicks(state)) {
        view.visible = true;
        view.text = roundFinishCalloutText(state);
        view.r = 230;
        view.g = 190;
        view.b = 105;
        view.frame = state.matchPhaseTicks;
        return view;
    }
    if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
        view.visible = true;
        view.text = roundResultText(state);
        view.r = 222;
        view.g = 226;
        view.b = 232;
        view.frame = state.matchPhaseTicks - state.fightRoundSettings.winTime;
    }
    return view;
}

FightRoundResultView roundResultOverlayView(const AppState& state) {
    const int winsRequired = matchWinsRequired(state);
    FightRoundResultView view;
    view.visible = true;
    view.resultText = roundResultText(state);
    view.p1RoundPips = FightRoundPipsView{ state.roundWins[0], winsRequired, false, 6.0f };
    view.p2RoundPips = FightRoundPipsView{ state.roundWins[1], winsRequired, true, 6.0f };
    view.footerText = state.matchComplete ? "MATCH COMPLETE" : "NEXT ROUND";
    if (state.frontend.pendingMode == PendingMode::Arena) {
        view.p1RoundPips = FightRoundPipsView{ 0, 0 };
        view.p2RoundPips = FightRoundPipsView{ 0, 0 };
        view.footerText = state.arenaConfig.endTitle;
    } else if (state.frontend.pendingMode == PendingMode::Story) {
        view.p1RoundPips = FightRoundPipsView{ 0, 0 };
        view.p2RoundPips = FightRoundPipsView{ 0, 0 };
        view.footerText = state.roundWinner == 1 ? "STAGE CLEAR" : "MISSION FAILED";
    }
    view.frame = state.matchPhaseTicks;
    return view;
}

FightMatchResultView matchResultScreenView(const AppState& state) {
    FightMatchResultView view;
    const int winner = matchWinner(state);
    view.modeLabel = isMatchMode(state) ? std::string(pendingModeTitle(state.frontend.pendingMode)) : "";
    view.winnerText = winner == 0 ? "DRAW GAME" : uppercaseCopy(fighterResultName(state, winner));
    if (state.frontend.pendingMode == PendingMode::Arena) {
        view.winnerText = state.arenaConfig.endTitle;
    } else if (state.frontend.pendingMode == PendingMode::Story) {
        view.winnerText = state.roundWinner == 1 ? "STAGE CLEAR" : "MISSION FAILED";
    }
    view.scoreText = singleFightScoreText(state);
    view.methodText = matchWinMethodText(state);
    view.quoteText = winner > 0 && winner <= static_cast<int>(state.fighters.size())
        ? selectedVictoryQuoteText(state, state.fighters[static_cast<size_t>(winner - 1)])
        : std::string{};
    if (state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story) {
        view.quoteText.clear();
    }
    view.stageText = "Stage: " + selectedStageName(state.selection);
    view.progressionText = state.progression.lastAwardText;
    view.menuRowCount = kMatchResultOptionCount;
    view.frame = state.matchPhaseTicks;
    for (int i = 0; i < kMatchResultOptionCount; ++i) {
        auto& row = view.menuRows[static_cast<size_t>(i)];
        if (state.frontend.pendingMode == PendingMode::Arena) {
            row.label = std::string(arenaMatchResultLabel(i));
        } else if (state.frontend.pendingMode == PendingMode::Story) {
            row.label = std::string(storyMatchResultLabel(state, i));
        } else {
            row.label = std::string(matchResultLabel(i));
        }
        row.selected = i == state.frontend.selectedMatchResultOption;
    }
    return view;
}

void drawRoundStartOverlay(SDL_Renderer* renderer, const AppState& state) {
    drawRoundStartOverlay(uiRenderContext(renderer, state), roundStartOverlayView(state));
}

void drawRoundFinishOverlay(SDL_Renderer* renderer, const AppState& state) {
    drawRoundFinishOverlay(uiRenderContext(renderer, state), roundFinishOverlayView(state));
}

void drawRoundResultOverlay(SDL_Renderer* renderer, const AppState& state) {
    drawRoundResultOverlay(uiRenderContext(renderer, state), roundResultOverlayView(state));
}

void drawMatchResultScreen(SDL_Renderer* renderer, const AppState& state) {
    drawMatchResultScreen(uiRenderContext(renderer, state), matchResultScreenView(state));
}

void drawLightFightPauseOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (!state.frontend.fightPauseOpen
        || state.training.options.menuOpen
        || state.frontend.singleFightPauseOpen
        || state.matchPhase == MatchPhase::MatchResult) {
        return;
    }

    const bool trainingMode = state.frontend.pendingMode == PendingMode::Training;
    setColor(renderer, 0, 0, 0, trainingMode ? 104 : 84);
    fillRect(renderer, 0, 0, logicalWidthF(state), logicalHeightF(state));

    if (trainingMode) {
        drawTrainingPauseHelpOverlay(uiRenderContext(renderer, state), TrainingPauseHelpView{ true });
        return;
    }

    const UiRenderContext ui = uiRenderContext(renderer, state);
    constexpr bool hdCanvas = true;
    constexpr float canvasW = 640.0f;
    constexpr float canvasH = 360.0f;
    ScopedVirtualCanvas virtualCanvas(ui, canvasW, canvasH);
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const auto& tokens = dragonUiTokens();
    const float s = hdCanvas ? 1.0f : metrics.pixelScale;
    const float panelW = (hdCanvas ? 220.0f : 172.0f) * s;
    const float panelH = (hdCanvas ? 108.0f : 70.0f) * s;
    const float x = std::floor((canvasW - panelW) * 0.5f);
    const float y = std::floor((canvasH - panelH) * 0.5f);
    setColor(renderer, tokens.panelBase, 226);
    fillRect(renderer, x, y, panelW, panelH);
    setColor(renderer, tokens.primaryTeal, 230);
    drawRect(renderer, x, y, panelW, panelH);
    setColor(renderer, tokens.separatorRed, 230);
    fillRect(renderer, x + 2.0f * s, y + (hdCanvas ? 34.0f : 23.0f) * s, panelW - 4.0f * s, metrics.border);
    setColor(renderer, tokens.mutedGold, 245);
    scaledDebugText(renderer, s, x + (hdCanvas ? 82.0f : 55.0f) * s, y + (hdCanvas ? 13.0f : 8.0f) * s, "PAUSED");
    setColor(renderer, tokens.primaryText, 235);
    scaledDebugText(renderer, s, x + (hdCanvas ? 42.0f : 25.0f) * s, y + (hdCanvas ? 52.0f : 34.0f) * s, "START  RESUME");
    setColor(renderer, tokens.mutedText, 230);
    scaledDebugText(renderer, s, x + (hdCanvas ? 42.0f : 25.0f) * s, y + (hdCanvas ? 72.0f : 48.0f) * s, "SELECT OPTIONS");
}

void drawScreenshotFreezeOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (!state.frontend.screenshotFreeze || state.frontend.screenshotFreezeNoticeTicks <= 0) {
        return;
    }

    const float widthF = logicalWidthF(state);
    const Uint8 alpha = static_cast<Uint8>(std::clamp(state.frontend.screenshotFreezeNoticeTicks * 4, 24, 188));
    const float panelW = 70.0f;
    const float x = widthF - panelW - 7.0f;
    constexpr float y = 7.0f;
    setColor(renderer, 4, 7, 12, alpha);
    fillRect(renderer, x, y, panelW, 13.0f);
    setColor(renderer, 128, 255, 190, static_cast<Uint8>(std::min<int>(alpha + 42, 240)));
    drawRect(renderer, x, y, panelW, 13.0f);
    setColor(renderer, 220, 255, 236, static_cast<Uint8>(std::min<int>(alpha + 58, 255)));
    debugText(renderer, x + 8.0f, y + 4.0f, "FREEZE");
}

bool storyForwardCueVisible(const AppState& state) {
    return state.frontend.pendingMode == PendingMode::Story
        && state.matchPhase == MatchPhase::Fight
        && !state.story.stageClear
        && !state.story.stageFailed
        && state.story.waveTransitionTicks > 0
        && state.story.waveIndex + 1 < storyWaveCount(state)
        && livingStoryEnemyCount(state) <= 0;
}

void drawStoryForwardCue(SDL_Renderer* renderer, const AppState& state) {
    if (!storyForwardCueVisible(state)) {
        return;
    }

    const float widthF = logicalWidthF(state);
    const float heightF = logicalHeightF(state);
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(state.frame) * 0.18f);
    if (state.storyForwardCueImage.texture
        && state.storyForwardCueImage.width > 0
        && state.storyForwardCueImage.height > 0) {
        const float sourceW = static_cast<float>(state.storyForwardCueImage.width);
        const float sourceH = static_cast<float>(state.storyForwardCueImage.height);
        const float targetW = std::clamp(widthF * 0.16f, 96.0f, 210.0f);
        const float targetH = std::clamp(targetW * (sourceH / sourceW), 24.0f, heightF * 0.16f);
        const float targetX = std::min(widthF - targetW - 18.0f, screenCenterX(state) + 82.0f);
        const float targetY = std::max(52.0f, heightF * 0.50f - targetH * 0.5f);
        const Uint8 alpha = static_cast<Uint8>(205.0f + pulse * 50.0f);
        SDL_SetTextureAlphaMod(state.storyForwardCueImage.texture, alpha);
        SDL_FRect dst{ targetX, targetY, targetW, targetH };
        SDL_RenderTexture(renderer, state.storyForwardCueImage.texture, nullptr, &dst);
        SDL_SetTextureAlphaMod(state.storyForwardCueImage.texture, 255);
        return;
    }

    const float panelW = 114.0f;
    const float panelH = 20.0f;
    const float x = std::min(widthF - panelW - 18.0f, screenCenterX(state) + 72.0f);
    const float y = std::max(58.0f, heightF * 0.52f);
    const Uint8 glow = static_cast<Uint8>(140.0f + pulse * 70.0f);

    setColor(renderer, 5, 8, 14, 188);
    fillRect(renderer, x, y, panelW, panelH);
    setColor(renderer, 81, 210, 198, glow);
    drawRect(renderer, x, y, panelW, panelH);
    fillRect(renderer, x + panelW - 31.0f, y + 5.0f, 18.0f, 3.0f);
    fillRect(renderer, x + panelW - 31.0f, y + 12.0f, 18.0f, 3.0f);
    setColor(renderer, 231, 195, 90, static_cast<Uint8>(180.0f + pulse * 60.0f));
    debugText(renderer, x + 10.0f, y + 6.0f, "CLEAR");
    setColor(renderer, 81, 210, 198, 235);
    debugText(renderer, x + 64.0f, y + 6.0f, ">>> ");
}

void drawStoryShopDoorCue(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    if (!storyShopDoorPromptVisible(state, stage)) {
        return;
    }

    const ArenaProjectedPoint door = projectArenaWorldPoint(
        state,
        stage,
        storyShopDoorX(state, stage),
        0.0f,
        storyShopDoorDepthZ(state));
    const float widthF = logicalWidthF(state);
    const float heightF = logicalHeightF(state);
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(state.frame) * 0.20f);
    const std::string prompt = storyShopDoorPromptText(state);
    const float panelW = std::clamp(18.0f + static_cast<float>(prompt.size()) * 8.0f, 96.0f, 184.0f);
    const float panelH = 18.0f;
    const float x = std::clamp(door.screenX - panelW * 0.5f, 8.0f, widthF - panelW - 8.0f);
    const float y = std::clamp(door.screenY - 46.0f, 52.0f, heightF - 54.0f);
    const Uint8 glow = static_cast<Uint8>(150.0f + pulse * 70.0f);

    setColor(renderer, 5, 8, 14, 200);
    fillRect(renderer, x, y, panelW, panelH);
    setColor(renderer, 81, 210, 198, glow);
    drawRect(renderer, x, y, panelW, panelH);
    setColor(renderer, 231, 195, 90, 236);
    debugText(renderer, x + 8.0f, y + 5.0f, prompt);
}

void drawFightViewFrame(SDL_Renderer* renderer, const AppState& state, bool present) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    const bool hasStageBackground = hasSelectedStageBackground(state);
    const bool hideBackground = anyFighterHasAssertSpecialFlag(state, "nobg");
    const bool hideForeground = anyFighterHasAssertSpecialFlag(state, "nofg");
    const bool hideHud = anyFighterHasAssertSpecialFlag(state, "nobardisplay");
    const bool arenaMode = state.frontend.pendingMode == PendingMode::Arena;
    SDL_Rect defaultViewport;
    SDL_GetRenderViewport(renderer, &defaultViewport);
    const int shakeOffsetY = arenaMode ? 0 : static_cast<int>(std::lround(state.display.envShakeOffsetY));
    int impactShakeX = 0;
    int impactShakeY = 0;
    int maxHitPause = 0;
    for (const auto& fighter : state.fighters) {
        maxHitPause = std::max(maxHitPause, fighter.hitPauseTicks);
    }
    if (!arenaMode && maxHitPause >= 6) {
        const int magnitude = std::clamp(maxHitPause / 6, 1, 2);
        impactShakeX = ((state.frontend.screenFrame / 2) % 2 == 0) ? magnitude : -magnitude;
        impactShakeY = ((state.frontend.screenFrame / 3) % 2 == 0) ? 1 : -1;
    }
    const int viewportOffsetX = impactShakeX;
    const int viewportOffsetY = shakeOffsetY + impactShakeY;
    if (viewportOffsetX != 0 || viewportOffsetY != 0) {
        SDL_Rect shakeViewport{ viewportOffsetX, viewportOffsetY, logicalWidth(state), logicalHeight(state) };
        SDL_SetRenderViewport(renderer, &shakeViewport);
    }

    {
        FramePerfScope scope(state.framePerf, FramePerfSection::StageDraw);
        if (hasStageBackground && !hideBackground) {
            drawStageLayer(renderer, state, 0);
        } else if (!hideBackground) {
            drawFallbackStage(renderer, state, stage, state.cameraY);
        }
        if (!hideBackground) {
            drawPaletteOverlay(renderer, state, state.backgroundPaletteEffect, 180);
        }
    }

    {
        FramePerfScope scope(state.framePerf, FramePerfSection::ActorDraw);
        drawWorldActors(renderer, state, stage);
        drawStoryRewardFeedback(renderer, state, stage);
    }
    if (hasStageBackground && !hideForeground) {
        FramePerfScope scope(state.framePerf, FramePerfSection::StageDraw);
        drawStageLayer(renderer, state, 1);
    }

    if (!hasStageBackground && !hideBackground) {
        setColor(renderer, 74, 100, 128);
        fillRect(renderer, 18, stage.zoffset - state.cameraY - 62.0f, 284, 8);
    }

    if (viewportOffsetX != 0 || viewportOffsetY != 0) {
        SDL_SetRenderViewport(renderer, &defaultViewport);
    }

    if (state.envColor.ticksLeft > 0) {
        setColor(
            renderer,
            static_cast<Uint8>(state.envColor.r),
            static_cast<Uint8>(state.envColor.g),
            static_cast<Uint8>(state.envColor.b),
            220);
        fillRect(renderer, 0, 0, widthF, logicalHeightF(state));
    }

    {
        FramePerfScope scope(state.framePerf, FramePerfSection::HudUiDraw);
        if (!hideHud && state.frontend.pendingMode == PendingMode::Training) {
            drawDebugOverlay(renderer, state, stage);
        }

        if (!hideHud) {
            drawFightHudView(renderer, state);
            drawStoryForwardCue(renderer, state);
            drawStoryShopDoorCue(renderer, state, stage);
        }

        if (state.frontend.pendingMode == PendingMode::Training
            && !state.training.options.menuOpen
            && !state.frontend.fightPauseOpen
            && !hideHud) {
            drawTrainingCommandHud(renderer, state);
        }

        if (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen) {
            drawTrainingOptionsMenu(renderer, state);
        } else if (isMatchMode(state) && state.frontend.singleFightPauseOpen) {
            PauseMenuView view{
                pendingModeTitle(state.frontend.pendingMode),
                state.frontend.selectedSingleFightPauseOption,
            };
            if (state.frontend.pendingMode == PendingMode::Arena) {
                view.optionLabels[3] = "ARENA SETUP";
            }
            drawSingleFightPauseMenu(uiRenderContext(renderer, state), view);
        } else if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundStart) {
            drawRoundStartOverlay(renderer, state);
        } else if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundFinish) {
            drawRoundFinishOverlay(renderer, state);
        } else if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundResult) {
            drawRoundResultOverlay(renderer, state);
        } else if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
            drawMatchResultScreen(renderer, state);
        }

        drawFightFreezeWatchOverlay(renderer, state);
        drawLightFightPauseOverlay(renderer, state);
        drawScreenshotFreezeOverlay(renderer, state);
        drawFpsCounter(renderer, state);
    }
    if (present) {
        FramePerfScope scope(state.framePerf, FramePerfSection::Present);
        presentPresentationFrame(renderer, state);
    }
}

void drawFightView(SDL_Renderer* renderer, const AppState& state) {
    drawFightViewFrame(renderer, state, true);
}

bool isMainProfileSettingOption(int option) {
    return option == kMainSettingP1ProfileOption || option == kMainSettingP2ProfileOption;
}

int mainProfileSettingPlayerIndex(int option) {
    return option == kMainSettingP2ProfileOption ? 1 : 0;
}

void saveProgressionStateQuietly(AppState& state) {
    if (!state.progression.loaded) {
        return;
    }
    try {
        saveDragonProgressionSave(state.progression.savePath, state.progression.save);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon progression save failed: %s", ex.what());
    }
}

void cycleMainProfileSetting(AppState& state, int option, int direction) {
    if (!isMainProfileSettingOption(option) || direction == 0) {
        return;
    }
    if (!state.progression.loaded) {
        loadProgressionState(state);
    }
    if (!state.progression.data.config.enabled) {
        return;
    }
    cycleDragonProgressionPlayerProfile(
        state.progression.save,
        mainProfileSettingPlayerIndex(option),
        direction);
    saveProgressionStateQuietly(state);
}

void createMainProfileForSetting(AppState& state, int option) {
    if (!isMainProfileSettingOption(option)) {
        return;
    }
    if (!state.progression.loaded) {
        loadProgressionState(state);
    }
    if (!state.progression.data.config.enabled) {
        return;
    }
    const int playerIndex = mainProfileSettingPlayerIndex(option);
    const std::string profileId = createNextDragonProgressionProfile(state.progression.save, "Player");
    setDragonProgressionPlayerProfile(state.progression.save, playerIndex, profileId);
    saveProgressionStateQuietly(state);
}

#include "ControlsOptionsFlow.h"

#include "FrontendFlow.h"
