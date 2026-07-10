#pragma once

void clearCurrentWave(RuntimeProbe& runtime) {
    const int active = runtime.snapshot().storyActiveEnemies;
    for (int i = 1; i <= active; ++i) {
        runtime.setFighterLife(i, 0);
    }
    runtime.step({}, 70);
}

std::string storyRouteSnapshotDetail(const RuntimeSnapshot& snapshot) {
    return "screen=" + std::to_string(snapshot.screen)
        + " pending=" + std::to_string(snapshot.pendingMode)
        + " phase=" + std::to_string(snapshot.matchPhase)
        + " complete=" + std::to_string(snapshot.matchComplete ? 1 : 0)
        + " winner=" + std::to_string(snapshot.roundWinner)
        + " clear=" + std::to_string(snapshot.storyStageClear ? 1 : 0)
        + " failed=" + std::to_string(snapshot.storyStageFailed ? 1 : 0)
        + " can_continue=" + std::to_string(snapshot.storyCanContinueRoute ? 1 : 0)
        + " result_option=" + std::to_string(snapshot.selectedMatchResultOption)
        + " selected=" + std::to_string(snapshot.storySelectedBoardNode)
        + " active=" + std::to_string(snapshot.storyActiveBoardNode)
        + " next=" + std::to_string(snapshot.storyNextRouteBoardNode)
        + " nodes=" + std::to_string(snapshot.storyBoardNodeCount)
        + " selectable=" + std::to_string(snapshot.storySelectableBoardNodeCount)
        + " wave=" + std::to_string(snapshot.storyWaveIndex + 1)
        + "/" + std::to_string(snapshot.storySelectedBoardWaves)
        + " active_enemies=" + std::to_string(snapshot.storyActiveEnemies)
        + " living=" + std::to_string(snapshot.storyLivingEnemies)
        + " defeated=" + std::to_string(snapshot.storyEnemiesDefeated)
        + "/" + std::to_string(snapshot.storyTotalEnemies);
}

std::string gLastRouteContinueDetail;

bool clearStoryNodeToRouteStop(RuntimeProbe& runtime, int maxClears = 8) {
    runtime.setArenaCpuFrozen(true);
    for (int i = 0; i < maxClears; ++i) {
        clearCurrentWave(runtime);
        for (int frame = 0; frame < 240; ++frame) {
            const auto snap = runtime.snapshot();
            if (snap.storyShopDoorAvailable || snap.matchPhase == static_cast<int>(MatchPhase::MatchResult)) {
                return true;
            }
            if (snap.storyActiveEnemies > 0 && snap.storyLivingEnemies > 0) {
                break;
            }
            runtime.step({}, 1);
        }
    }
    const auto snap = runtime.snapshot();
    return snap.storyShopDoorAvailable || snap.matchPhase == static_cast<int>(MatchPhase::MatchResult);
}

bool continueStoryRouteFromResult(RuntimeProbe& runtime) {
    if (!waitForMatchResult(runtime, 420)) {
        gLastRouteContinueDetail = "wait_for_match_result_failed " + storyRouteSnapshotDetail(runtime.snapshot());
        return false;
    }
    const RuntimeSnapshot before = runtime.snapshot();
    const int expectedNextBoard = before.storyNextRouteBoardNode >= 0
        ? before.storyNextRouteBoardNode
        : before.storyActiveBoardNode + 1;
    gLastRouteContinueDetail = "before_enter{" + storyRouteSnapshotDetail(before) + "}";
    runtime.pressKey("enter");
    const bool preparedImmediately = runtime.preparePendingFight();
    RuntimeSnapshot after = runtime.snapshot();
    for (int frame = 0; frame < 240; ++frame) {
        after = runtime.snapshot();
        const bool reachedExpectedNode = after.storyActiveBoardNode == expectedNextBoard;
        const bool routeFightReady =
            after.screen == static_cast<int>(Screen::FightView)
            && (after.matchPhase == static_cast<int>(MatchPhase::RoundStart)
                || after.matchPhase == static_cast<int>(MatchPhase::Fight));
        if (reachedExpectedNode && routeFightReady) {
            gLastRouteContinueDetail += " after_prepare{" + storyRouteSnapshotDetail(after)
                + "} expected_next=" + std::to_string(expectedNextBoard)
                + " prepared=" + std::to_string(preparedImmediately ? 1 : 0);
            return true;
        }
        if (after.screen == static_cast<int>(Screen::VersusScreen)) {
            runtime.preparePendingFight();
        } else {
            runtime.step({}, 1);
        }
    }
    gLastRouteContinueDetail += " after_enter_route_failed{" + storyRouteSnapshotDetail(after)
        + "} expected_next=" + std::to_string(expectedNextBoard)
        + " prepared=" + std::to_string(preparedImmediately ? 1 : 0);
    return false;
}

bool advanceStoryRouteUntilShopDoor(RuntimeProbe& runtime, int maxSegments = 8) {
    for (int i = 0; i < maxSegments; ++i) {
        if (!clearStoryNodeToRouteStop(runtime)) {
            return false;
        }
        if (runtime.snapshot().storyShopDoorAvailable) {
            return true;
        }
        if (!continueStoryRouteFromResult(runtime)) {
            return false;
        }
    }
    return runtime.snapshot().storyShopDoorAvailable;
}

std::string storyNodeSearchText(const StoryBoardNode& node) {
    return node.id + " " + node.title + " " + node.boardTitle + " " + node.stageRef;
}
