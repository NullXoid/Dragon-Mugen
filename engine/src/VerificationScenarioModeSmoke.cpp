#include "VerificationScenarioCommon.h"

namespace dragon::verification {

int runCpuBaseline(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "cpu-baseline");

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "single_player_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "cpu_checks", "Single Player fight phase was not active");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-110.0f, 110.0f);
    const auto moveBefore = runtime.snapshot();
    runtime.step({}, 90);
    const auto moveAfter = runtime.snapshot();
    const float distanceBefore = horizontalDistance(moveBefore);
    const float distanceAfter = horizontalDistance(moveAfter);
    const bool movedTowardP1 = distanceAfter < distanceBefore - 5.0f && moveAfter.p2.x < moveBefore.p2.x - 1.0f;
    record(out, counts, movedTowardP1 ? Status::Pass : Status::Fail, "cpu_moves_toward_p1",
        "p2_x_before=" + std::to_string(moveBefore.p2.x)
        + " p2_x_after=" + std::to_string(moveAfter.p2.x)
        + " distance_before=" + std::to_string(distanceBefore)
        + " distance_after=" + std::to_string(distanceAfter));

    runtime.positionFighters(-20.0f, 22.0f);
    bool sawCpuAttack = false;
    FighterSnapshot cpuAttackSnap;
    for (int i = 0; i < 135; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        if (snap.p2.moveType == 'A') {
            sawCpuAttack = true;
            cpuAttackSnap = snap.p2;
            break;
        }
    }
    record(out, counts, sawCpuAttack ? Status::Pass : Status::Fail, "cpu_attempts_normal_attack",
        sawCpuAttack
            ? "state=" + std::to_string(cpuAttackSnap.stateNo) + " anim=" + std::to_string(cpuAttackSnap.action)
            : "no CPU attack move observed");

    runtime.positionFighters(-18.0f, 22.0f);
    waitForControllableIdle(runtime, 120);
    runtime.step({}, 3);
    const auto cpuHitBefore = runtime.snapshot();
    bool sawCpuContact = false;
    bool sawCpuHitOrGuard = false;
    for (int i = 0; i < 180; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        sawCpuContact = sawCpuContact || snap.p2.moveContact || snap.p2.moveHit || snap.p2.moveGuarded;
        sawCpuHitOrGuard = sawCpuHitOrGuard || snap.p2.moveHit || snap.p2.moveGuarded || snap.p1.life < cpuHitBefore.p1.life;
    }
    const auto cpuHitAfter = runtime.snapshot();
    const bool cpuDamagedP1 = cpuHitAfter.p1.life < cpuHitBefore.p1.life;
    record(out, counts, (sawCpuContact && sawCpuHitOrGuard && cpuDamagedP1) ? Status::Pass : Status::Fail,
        "cpu_can_damage_p1",
        "contact=" + std::to_string(sawCpuContact ? 1 : 0)
        + " hit_or_guard=" + std::to_string(sawCpuHitOrGuard ? 1 : 0)
        + " p1_life_before=" + std::to_string(cpuHitBefore.p1.life)
        + " p1_life_after=" + std::to_string(cpuHitAfter.p1.life)
        + " last_hit=\"" + cpuHitAfter.lastHitText + "\"");

    runtime.positionFighters(-18.0f, 24.0f);
    waitForControllableIdle(runtime, 120);
    runtime.step({}, 3);
    const auto hitBefore = runtime.snapshot();
    runtime.step(withButton('y'), 2);
    bool sawContact = false;
    bool sawHitOrGuard = false;
    for (int i = 0; i < 60; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        sawContact = sawContact || snap.p1.moveContact || snap.p1.moveHit || snap.p1.moveGuarded;
        sawHitOrGuard = sawHitOrGuard || snap.p1.moveHit || snap.p1.moveGuarded || snap.p2.life < hitBefore.p2.life;
    }
    const auto hitAfter = runtime.snapshot();
    record(out, counts, (sawContact && sawHitOrGuard) ? Status::Pass : Status::Fail, "cpu_can_still_be_hit",
        "contact=" + std::to_string(sawContact ? 1 : 0)
        + " hit_or_guard=" + std::to_string(sawHitOrGuard ? 1 : 0)
        + " p2_life_before=" + std::to_string(hitBefore.p2.life)
        + " p2_life_after=" + std::to_string(hitAfter.p2.life)
        + " last_hit=\"" + hitAfter.lastHitText + "\"");

    const auto timerBefore = runtime.snapshot();
    runtime.step({}, 30);
    const auto timerAfter = runtime.snapshot();
    const bool timerStable = timerAfter.matchPhase == static_cast<int>(MatchPhase::Fight)
        && timerAfter.matchTimerTicks > 0
        && timerAfter.matchTimerTicks <= timerBefore.matchTimerTicks;
    record(out, counts, timerStable ? Status::Pass : Status::Fail, "single_player_timer_stability",
        "phase_before=" + std::to_string(timerBefore.matchPhase)
        + " phase_after=" + std::to_string(timerAfter.matchPhase)
        + " timer_before=" + std::to_string(timerBefore.matchTimerTicks)
        + " timer_after=" + std::to_string(timerAfter.matchTimerTicks));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaSmoke(RuntimeProbe& runtime, std::ostream& out, int cpuCount) {
    Counts counts;
    const std::string scenarioName = "arena-cpu-" + std::to_string(cpuCount);
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Arena, out, cpuCount)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside Arena setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, scenarioName);

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "arena_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "arena_checks", "Arena fight phase was not active");
        summary(out, counts);
        return exitCode(counts);
    }

    const int expectedFighters = 1 + cpuCount;
    const auto start = runtime.snapshot();
    record(out, counts, start.fighterCount == expectedFighters ? Status::Pass : Status::Fail, "arena_fighter_count",
        "expected=" + std::to_string(expectedFighters)
        + " actual=" + std::to_string(start.fighterCount));
    record(out, counts, start.livingFighters == expectedFighters ? Status::Pass : Status::Fail, "arena_initial_living_count",
        "expected=" + std::to_string(expectedFighters)
        + " actual=" + std::to_string(start.livingFighters));
    record(out, counts, start.arenaRuntimeCount == expectedFighters ? Status::Pass : Status::Fail, "arena_runtime_count",
        "expected=" + std::to_string(expectedFighters)
        + " actual=" + std::to_string(start.arenaRuntimeCount));
    record(out, counts, start.matchTimerTicks == 0 ? Status::Pass : Status::Fail, "arena_timer_default_inf",
        "timer_ticks=" + std::to_string(start.matchTimerTicks));
    record(out, counts, !start.arenaDrawOrder.empty() ? Status::Pass : Status::Fail, "arena_draw_order_available",
        "draw_order=" + start.arenaDrawOrder);

    if (cpuCount == 1) {
        waitForControllableIdle(runtime, 120);
        runtime.setArenaCpuFrozen(true);
        runtime.positionFighters(-14.0f, 14.0f);
        runtime.forceFighterState(1, 0);
        runtime.setFighterControl(1, false);
        const auto hitBefore = runtime.snapshot();
        runtime.step(SymbolicInput{ .x = true }, 4);
        bool sawPlayerHit = false;
        RuntimeSnapshot hitAfter = runtime.snapshot();
        for (int i = 0; i < 90; ++i) {
            runtime.step({}, 1);
            hitAfter = runtime.snapshot();
            sawPlayerHit = sawPlayerHit
                || hitAfter.p2.life < hitBefore.p2.life
                || hitAfter.p1.moveHit
                || hitAfter.p1.hitCount > hitBefore.p1.hitCount
                || hitAfter.lastHitText.find("P1 hit ") != std::string::npos;
        }
        record(out, counts, sawPlayerHit ? Status::Pass : Status::Fail, "arena_player_hit_cpu",
            "p2_life_before=" + std::to_string(hitBefore.p2.life)
            + " p2_life_after=" + std::to_string(hitAfter.p2.life)
            + " p1_hit_count_before=" + std::to_string(hitBefore.p1.hitCount)
            + " p1_hit_count_after=" + std::to_string(hitAfter.p1.hitCount)
            + " p2_state=" + std::to_string(hitAfter.p2.stateNo)
            + " last_hit=\"" + hitAfter.lastHitText + "\"");
        runtime.setArenaCpuFrozen(false);
    }

    if (expectedFighters > 2) {
        for (int i = 1; i < expectedFighters - 1; ++i) {
            runtime.setFighterLife(i, 0);
        }
        runtime.step({}, 2);
        const auto partial = runtime.snapshot();
        const bool stillFighting = partial.livingFighters == 2
            && partial.matchPhase == static_cast<int>(MatchPhase::Fight)
            && partial.p2.stateNo >= 5000 && partial.p2.stateNo < 5200;
        record(out, counts, stillFighting ? Status::Pass : Status::Fail, "defeated_fighters_excluded",
            "living=" + std::to_string(partial.livingFighters)
            + " match_phase=" + std::to_string(partial.matchPhase)
            + " p2_state=" + std::to_string(partial.p2.stateNo));
    }

    for (int i = 1; i < expectedFighters; ++i) {
        runtime.setFighterLife(i, 0);
    }
    runtime.step({}, 2);
    const auto resolved = runtime.snapshot();
    const bool winnerPhase = resolved.matchPhase == static_cast<int>(MatchPhase::RoundFinish)
        || resolved.matchPhase == static_cast<int>(MatchPhase::RoundResult)
        || resolved.matchPhase == static_cast<int>(MatchPhase::MatchResult);
    record(out, counts, winnerPhase && resolved.roundWinner == 1 && resolved.livingFighters == 1
            && resolved.p2.stateNo >= 5000 && resolved.p2.stateNo < 5200
            ? Status::Pass
            : Status::Fail,
        "last_fighter_standing_winner",
        "phase=" + std::to_string(resolved.matchPhase)
        + " winner=" + std::to_string(resolved.roundWinner)
        + " living=" + std::to_string(resolved.livingFighters)
        + " p2_state=" + std::to_string(resolved.p2.stateNo)
        + " text=\"" + resolved.lastHitText + "\"");

    bool reachedEndScreen = false;
    for (int i = 0; i < 420; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::MatchResult)) {
            reachedEndScreen = true;
            break;
        }
        runtime.step({}, 1);
    }
    record(out, counts, reachedEndScreen ? Status::Pass : Status::Fail, "arena_end_screen",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runVsP2Runtime(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Versus, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside VS setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "vs-p2-runtime");

    const bool active = waitForActiveFight(runtime, 420);
    record(out, counts, active ? Status::Pass : Status::Fail, "fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase));
    if (!active) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto loaded = runtime.snapshot();
    const bool p2HasSeparateRuntime =
        loaded.p2RuntimeStates > loaded.p1RuntimeStates
        && loaded.p2RuntimeHitDefs > loaded.p1RuntimeHitDefs
        && loaded.p2RuntimeCommandEntries > loaded.p1RuntimeCommandEntries;
    record(out, counts, p2HasSeparateRuntime ? Status::Pass : Status::Fail, "p2_runtime_counts",
        "p1_states=" + std::to_string(loaded.p1RuntimeStates)
        + " p2_states=" + std::to_string(loaded.p2RuntimeStates)
        + " p1_hitdefs=" + std::to_string(loaded.p1RuntimeHitDefs)
        + " p2_hitdefs=" + std::to_string(loaded.p2RuntimeHitDefs)
        + " p1_command_entries=" + std::to_string(loaded.p1RuntimeCommandEntries)
        + " p2_command_entries=" + std::to_string(loaded.p2RuntimeCommandEntries));

    constexpr int kP2OnlyState = 3885;
    runtime.forceFighterState(0, kP2OnlyState);
    const auto p1Forced = runtime.snapshot();
    record(out, counts, p1Forced.p1.stateNo != kP2OnlyState ? Status::Pass : Status::Fail, "p1_rejects_p2_only_state",
        "p1_state=" + std::to_string(p1Forced.p1.stateNo)
        + " requested_state=" + std::to_string(kP2OnlyState));

    runtime.forceFighterState(1, kP2OnlyState);
    const auto p2Forced = runtime.snapshot();
    record(out, counts, p2Forced.p2.stateNo == kP2OnlyState ? Status::Pass : Status::Fail, "p2_enters_selected_runtime_state",
        "p2_state=" + std::to_string(p2Forced.p2.stateNo)
        + " p2_action=" + std::to_string(p2Forced.p2.action)
        + " requested_state=" + std::to_string(kP2OnlyState));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runCompatibilityProfileResolver(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY compatibility-profile-resolver\n";

    record(out, counts,
        resolveCompatibilityProfile("") == CompatibilityProfile::Mugen11 ? Status::Pass : Status::Fail,
        "empty_version_defaults_mugen_11",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("")));
    record(out, counts,
        resolveCompatibilityProfile("04,14,2001") == CompatibilityProfile::Mugen2001 ? Status::Pass : Status::Fail,
        "legacy_version_resolves_mugen_2001",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("04,14,2001")));
    record(out, counts,
        resolveCompatibilityProfile("1.0") == CompatibilityProfile::Mugen10 ? Status::Pass : Status::Fail,
        "version_10_resolves_mugen_10",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("1.0")));
    record(out, counts,
        resolveCompatibilityProfile("1.1") == CompatibilityProfile::Mugen11 ? Status::Pass : Status::Fail,
        "version_11_resolves_mugen_11",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("1.1")));

    const LocalCoord parsedLocalCoord = parseLocalCoord("640, 480");
    record(out, counts,
        parsedLocalCoord.width == 640 && parsedLocalCoord.height == 480 ? Status::Pass : Status::Fail,
        "localcoord_parse",
        "width=" + std::to_string(parsedLocalCoord.width)
            + " height=" + std::to_string(parsedLocalCoord.height));

    const std::filesystem::path gameRoot(runtime.rootText());
    const auto characters = loadCharacters(gameRoot);
    const CharacterSlot* kfm = findCharacterById(characters, "kfm");
    const CharacterSlot* evilRyu = findCharacterById(characters, "EvilRyu");
    const CharacterSlot* evilKen = findCharacterById(characters, "EvilKen");
    record(out, counts, kfm ? Status::Pass : Status::Fail, "kfm_character_loaded",
        kfm ? "profile=" + std::string(compatibilityProfileName(kfm->compatibilityProfile)) : "missing");
    record(out, counts,
        kfm && kfm->compatibilityProfile == CompatibilityProfile::Mugen2001 ? Status::Pass : Status::Fail,
        "kfm_profile_mugen_2001",
        kfm ? "mugenversion=" + kfm->mugenVersion : "missing");
    record(out, counts,
        evilRyu && evilRyu->compatibilityProfile == CompatibilityProfile::Mugen10
            && evilRyu->localCoord.width == 320
            && evilRyu->localCoord.height == 240
            ? Status::Pass : Status::Fail,
        "evilryu_profile_localcoord",
        evilRyu ? "profile=" + std::string(compatibilityProfileName(evilRyu->compatibilityProfile))
                + " localcoord=" + std::to_string(evilRyu->localCoord.width)
                + "," + std::to_string(evilRyu->localCoord.height)
                : "missing");
    record(out, counts,
        evilKen && evilKen->compatibilityProfile == CompatibilityProfile::Mugen10
            && evilKen->localCoord.width == 320
            && evilKen->localCoord.height == 240
            ? Status::Pass : Status::Fail,
        "evilken_profile_localcoord",
        evilKen ? "profile=" + std::string(compatibilityProfileName(evilKen->compatibilityProfile))
                + " localcoord=" + std::to_string(evilKen->localCoord.width)
                + "," + std::to_string(evilKen->localCoord.height)
                : "missing");

    const auto stages = loadStages(gameRoot);
    const StageSlot* openBorStage = findLegacyOpenBorStage(stages);
    record(out, counts, openBorStage ? Status::Pass : Status::Fail, "legacy_openbor_stage_bridge",
        openBorStage ? "stage=" + openBorStage->displayName : "missing");

    if (runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        const RuntimeSnapshot snap = runtime.snapshot();
        record(out, counts, snap.runtimeMode == "Dragon" ? Status::Pass : Status::Fail, "runtime_mode_dragon_default",
            "mode=" + snap.runtimeMode);
        record(out, counts, snap.p1CompatibilityProfile == "Mugen2001" ? Status::Pass : Status::Fail,
            "runtime_p1_profile_visible",
            "profile=" + snap.p1CompatibilityProfile);
        record(out, counts,
            snap.p1LocalCoordWidth == 320 && snap.p1LocalCoordHeight == 240 && snap.p1UsesMugenSemantics
                ? Status::Pass : Status::Fail,
            "runtime_p1_context_visible",
            "localcoord=" + std::to_string(snap.p1LocalCoordWidth)
                + "," + std::to_string(snap.p1LocalCoordHeight)
                + " mugen_semantics=" + std::to_string(snap.p1UsesMugenSemantics ? 1 : 0));
    } else {
        record(out, counts, Status::Blocked, "runtime_setup", "KFM/Mountainside Training setup failed");
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenCornerVisualBounds(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-corner-visual-bounds");
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-1000.0f, -950.0f);
    runtime.setFighterControl(1, false);
    runtime.step({}, 8);
    auto leftCorner = runtime.snapshot();
    float minLeftVisual = std::min(leftCorner.p1.visualScreenLeft, leftCorner.p2.visualScreenLeft);
    float maxLeftVisual = std::max(leftCorner.p1.visualScreenRight, leftCorner.p2.visualScreenRight);
    for (int i = 0; i < 45; ++i) {
        runtime.step(SymbolicInput{ .left = true }, 1);
        const auto snap = runtime.snapshot();
        minLeftVisual = std::min({ minLeftVisual, snap.p1.visualScreenLeft, snap.p2.visualScreenLeft });
        maxLeftVisual = std::max({ maxLeftVisual, snap.p1.visualScreenRight, snap.p2.visualScreenRight });
    }
    leftCorner = runtime.snapshot();
    const bool leftVisible = minLeftVisual >= -0.5f && maxLeftVisual <= static_cast<float>(leftCorner.logicalWidth) + 0.5f;
    record(out, counts, leftVisible ? Status::Pass : Status::Fail, "left_corner_visuals_stay_inside_screen",
        "min_left=" + std::to_string(minLeftVisual)
            + " max_right=" + std::to_string(maxLeftVisual)
            + " logical_width=" + std::to_string(leftCorner.logicalWidth)
            + " p1_x=" + std::to_string(leftCorner.p1.x)
            + " p2_x=" + std::to_string(leftCorner.p2.x));

    runtime.positionFighters(950.0f, 1000.0f);
    runtime.setFighterControl(1, false);
    runtime.step({}, 8);
    auto rightCorner = runtime.snapshot();
    float minRightVisual = std::min(rightCorner.p1.visualScreenLeft, rightCorner.p2.visualScreenLeft);
    float maxRightVisual = std::max(rightCorner.p1.visualScreenRight, rightCorner.p2.visualScreenRight);
    for (int i = 0; i < 45; ++i) {
        runtime.step(SymbolicInput{ .right = true }, 1);
        const auto snap = runtime.snapshot();
        minRightVisual = std::min({ minRightVisual, snap.p1.visualScreenLeft, snap.p2.visualScreenLeft });
        maxRightVisual = std::max({ maxRightVisual, snap.p1.visualScreenRight, snap.p2.visualScreenRight });
    }
    rightCorner = runtime.snapshot();
    const bool rightVisible = minRightVisual >= -0.5f && maxRightVisual <= static_cast<float>(rightCorner.logicalWidth) + 0.5f;
    record(out, counts, rightVisible ? Status::Pass : Status::Fail, "right_corner_visuals_stay_inside_screen",
        "min_left=" + std::to_string(minRightVisual)
            + " max_right=" + std::to_string(maxRightVisual)
            + " logical_width=" + std::to_string(rightCorner.logicalWidth)
            + " p1_x=" + std::to_string(rightCorner.p1.x)
            + " p2_x=" + std::to_string(rightCorner.p2.x));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


} // namespace dragon::verification
