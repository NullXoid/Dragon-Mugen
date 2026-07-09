#pragma once

verification::RuntimePerformanceResult measureVerificationPerformance(
    AppState& state,
    SDL_Renderer* renderer,
    int warmupFrames,
    int measuredFrames,
    bool renderEachFrame,
    bool stressInputs) {
    verification::RuntimePerformanceResult result;
    if (!renderer) {
        return result;
    }
    result.ran = true;
    result.warmupFrames = std::max(0, warmupFrames);
    result.measuredFrames = std::max(0, measuredFrames);

    const auto stressInputForFrame = [](int frame) {
        verification::SymbolicInput input;
        const int phase = frame % 54;
        if (phase < 6) {
            input.down = true;
        } else if (phase < 12) {
            input.down = true;
            input.right = true;
        } else if (phase < 18) {
            input.right = true;
            input.x = true;
        } else if (phase < 24) {
            input.y = true;
        } else if (phase < 32) {
            input.z = true;
        } else if (phase < 40) {
            input.right = true;
        } else if (phase < 46) {
            input.a = true;
        } else {
            input.b = true;
        }
        return input;
    };

    double pauseTotalMs = 0.0;
    const auto runFrame = [&](int frame, bool measured) {
        state.framePerf.beginFrame(1.0 / 60.0);
        const verification::SymbolicInput symbolic = stressInputs ? stressInputForFrame(frame) : verification::SymbolicInput{};
        const FighterInputState p1 = verificationProbeInput(symbolic);
        const FighterInputState p2;
        FightInputOverride inputOverride;
        inputOverride.p1 = &p1;
        inputOverride.p2 = &p2;
        const FightInputOverride* previous = gFightInputOverride;
        gFightInputOverride = &inputOverride;
        {
            FramePerfScope scope(state.framePerf, FramePerfSection::FixedUpdate);
            fixedUpdate(renderer, state);
        }
        gFightInputOverride = previous;

        if (renderEachFrame) {
            beginPresentationFrame(renderer, state);
            drawFightViewFrame(renderer, state, false);
            presentPresentationFrame(renderer, state);
        }

        collectFramePerformanceCounters(state);
        const bool anyHitPause = std::any_of(state.fighters.begin(), state.fighters.end(), [](const FighterState& fighter) {
            return fighter.hitPauseTicks > 0;
        });
        const bool paused = state.frontend.fightPauseOpen
            || state.frontend.screenshotFreeze
            || (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen)
            || (isMatchMode(state) && state.frontend.singleFightPauseOpen);
        const bool superpause = state.globalPauseIsSuper && state.globalPauseTicks > 0;
        state.framePerf.endFrame(paused, anyHitPause, superpause);
        if (measured && (paused || anyHitPause || superpause)) {
            pauseTotalMs += state.framePerf.latestFrame().totalMs;
            ++result.pauseFrames;
        }
    };

    for (int i = 0; i < result.warmupFrames; ++i) {
        runFrame(i, false);
    }
    state.framePerf.resetHistory();
    for (int i = 0; i < result.measuredFrames; ++i) {
        runFrame(i, true);
    }

    const FramePerfSummary summary = state.framePerf.summary(true);
    result.fpsEquivalent = summary.fpsEquivalent;
    result.avgFrameMs = summary.avgFrameMs;
    result.p95FrameMs = summary.p95FrameMs;
    result.worstFrameMs = summary.worstFrameMs;
    result.gameplayFrames = summary.gameplayFrameCount;
    result.pauseFrameAvgMs = result.pauseFrames > 0 ? pauseTotalMs / static_cast<double>(result.pauseFrames) : 0.0;
    result.counters = summary.latestCounters;
    result.dominantSection = std::string(framePerfSectionLabel(summary.dominantSection));
    return result;
}
