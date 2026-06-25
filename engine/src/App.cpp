#include "dragon/App.h"
#include "dragon/Compatibility.h"
#include "dragon/DragonProgression.h"
#include "dragon/FightData.h"
#include "dragon/MugenData.h"
#include "dragon/MugenText.h"
#include "dragon/Sff.h"
#include "dragon/Snd.h"
#include "ArenaConfig.h"
#include "ArenaSetupOverlay.h"
#include "AppTypes.h"
#include "ControlsOptionsMenu.h"
#include "ControlsStore.h"
#include "FightDisplayState.h"
#include "FightDebugLog.h"
#include "FightHudOverlay.h"
#include "FightMessageState.h"
#include "FightPresentationView.h"
#include "FightResultOverlay.h"
#include "FramePerformance.h"
#include "FrontendMenu.h"
#include "FrontendState.h"
#include "Input.h"
#include "CharacterSelectOverlay.h"
#include "LoadingProgressState.h"
#include "MainMenuOverlay.h"
#include "OptionsMenuOverlay.h"
#include "PauseMenuOverlay.h"
#include "ProgressionState.h"
#include "SelectionState.h"
#include "ShopCatalog.h"
#include "ShopDemoCollision.h"
#include "StageSelectOverlay.h"
#include "StoryStageSelectOverlay.h"
#include "TrainingState.h"
#include "TrainingCommandInputRenderer.h"
#include "TrainingCommandView.h"
#include "TrainingCommandOverlay.h"
#include "TrainingDebugView.h"
#include "TrainingDebugOverlay.h"
#include "TrainingOptionsBehavior.h"
#include "TrainingOptionsOverlay.h"
#include "UiRenderContext.h"
#include "UiRenderPrimitives.h"
#include "UiSpriteView.h"
#include "VsScreenOverlay.h"
#include "VerificationScenario.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <exception>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <initializer_list>
#include <numbers>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dragon {
namespace {

#include "AppRuntimeTypesCore.h"

#include "AppRuntimeTypesControllers.h"

#include "AppRuntimeTypesCommandsMedia.h"
#include "AppRuntimeTypesFighters.h"

#include "AppUiProjectionAssembly.h"

#include "AppParsingAssembly.h"

#include "AppHitDefinitionLoadingAssembly.h"

#include "AppInputMenuAssembly.h"
#include "AppActorRuntimeHelpers.h"

#include "AppHitStateRuntime.h"

#include "AppFighterStateRuntime.h"

#include "AppStateControllerRuntimeAssembly.h"
#include "AppMovementCameraRuntime.h"
#include "AppHitCollisionRuntime.h"

#include "AppProjectileHelperRuntime.h"

#include "AppCommandRecoveryRuntime.h"
#include "StoryRewardFeedbackRuntime.h"
#include "AppRoundProgressionRuntime.h"

#include "AppFightUpdateAssembly.h"

#include "AppScreenFlowAssembly.h"

#include "AppTrainingCommandDisplayAssembly.h"

#include "AppTrainingMoveListAssembly.h"

#include "ShopDemoRuntime.h"

#include "AppFightPresentationAssembly.h"

#include "AppMainLoopAssembly.h"

} // namespace

int runVerificationScenario(
    const std::filesystem::path& gameRoot,
    std::string_view scenarioName,
    std::ostream& out) {
    return runVerificationScenarioInternal(gameRoot, scenarioName, out);
}

int runApp(const std::filesystem::path& gameRoot) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Dragon MUGEN", kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderLogicalPresentation(renderer, kLogicalWidth, kLogicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    AppState state;
    state.gameRoot = gameRoot;
    state.arenaConfig = loadArenaConfig(gameRoot);
    setArenaDefaultsFromConfig(state);
    loadProgressionState(state);
    loadControlsState(state);
    initAudio(state);
    state.fightRoundSettings = loadFightRoundSettings(gameRoot);
    state.selection.characters = loadCharacters(gameRoot);
    state.selection.stages = loadStages(gameRoot);
    selectPreferredStage(state);
    loadVisualAssets(renderer, state);
    openExistingGamepads(state);

    using clock = std::chrono::steady_clock;
    constexpr double fixedStep = 1.0 / 60.0;
    constexpr double targetFrameSeconds = 1.0 / 60.0;
    auto previous = clock::now();

    while (state.running) {
        const auto now = clock::now();
        const std::chrono::duration<double> elapsed = now - previous;
        previous = now;
        const double realElapsedSeconds = std::max(0.0, elapsed.count());
        state.framePerf.beginFrame(realElapsedSeconds);
        const double simulationElapsedSeconds = std::min(realElapsedSeconds, fixedStep * 5.0);
        state.accumulator = std::min(state.accumulator + simulationElapsedSeconds, fixedStep * 5.0);

        {
            FramePerfScope scope(state.framePerf, FramePerfSection::EventPump);
            pumpEvents(renderer, state);
        }
        int fixedStepsThisFrame = 0;
        while (state.accumulator >= fixedStep && fixedStepsThisFrame < 5) {
            {
                FramePerfScope scope(state.framePerf, FramePerfSection::FixedUpdate);
                fixedUpdate(state);
            }
            state.accumulator -= fixedStep;
            ++fixedStepsThisFrame;
            state.framePerf.addFixedStep();
        }
        if (fixedStepsThisFrame >= 5 && state.accumulator >= fixedStep) {
            state.accumulator = 0.0;
            state.framePerf.addDroppedAccumulatorFrame();
        }

        clearPhysicalFrame(renderer);
        applyLogicalPresentation(renderer, state);

        if (state.frontend.screen == Screen::ModeSelect) {
            drawModeSelect(renderer, state);
        } else if (state.frontend.screen == Screen::MainSettings) {
            drawMainSettings(renderer, state);
        } else if (state.frontend.screen == Screen::CharacterSelect) {
            drawCharacterSelect(renderer, state);
        } else if (state.frontend.screen == Screen::ArenaSetup) {
            drawArenaSetup(renderer, state);
        } else if (state.frontend.screen == Screen::StageSelect) {
            drawStageSelect(renderer, state);
        } else if (state.frontend.screen == Screen::VersusScreen) {
            drawVersusScreen(renderer, state);
            prepareVersusSessionAfterPresent(renderer, state);
        } else if (state.frontend.screen == Screen::FightView) {
            drawFightView(renderer, state);
        } else if (state.frontend.screen == Screen::ShopDemo) {
            drawShopDemo(renderer, state);
        }

        const auto frameEnd = clock::now();
        collectFramePerformanceCounters(state);
        const bool anyHitPause = std::any_of(state.fighters.begin(), state.fighters.end(), [](const FighterState& fighter) {
            return fighter.hitPauseTicks > 0;
        });
        const bool globalPause = state.frontend.fightPauseOpen
            || state.frontend.screenshotFreeze
            || (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen)
            || (isMatchMode(state) && state.frontend.singleFightPauseOpen);
        state.framePerf.endFrame(globalPause, anyHitPause, state.globalPauseIsSuper && state.globalPauseTicks > 0);
        appendFramePerformanceLog(state.gameRoot, state.framePerf, state.frame);
        const std::chrono::duration<double> frameDuration = frameEnd - now;
        const double remainingSeconds = targetFrameSeconds - frameDuration.count();
        if (state.mainSettings.fpsCapEnabled && remainingSeconds > 0.001) {
            SDL_Delay(static_cast<Uint32>(remainingSeconds * 1000.0));
        }
    }

    saveControlsStateQuietly(state);
    closeAllGamepads(state);
    destroyVisualAssets(state);
    destroyAudioAssets(state);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace dragon
