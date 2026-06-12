#pragma once

// Internal App.cpp implementation header.
// This depends on App.cpp-local runtime/render types and helpers; include it only
// from App.cpp after those dependencies are defined.

const char* freezeWatchPhaseName(MatchPhase phase) {
    switch (phase) {
    case MatchPhase::RoundStart:
        return "start";
    case MatchPhase::Fight:
        return "fight";
    case MatchPhase::RoundFinish:
        return "finish";
    case MatchPhase::RoundResult:
        return "result";
    case MatchPhase::MatchResult:
        return "match";
    default:
        return "?";
    }
}

FreezeWatchFighterSample freezeWatchSample(const AppState& state, const FighterState& fighter) {
    return FreezeWatchFighterSample{
        fighter.x,
        fighter.y,
        fighter.vx,
        fighter.vy,
        fighter.depthZ,
        fighter.stateNo,
        fighter.stateTime,
        fighter.action,
        fighter.animTick,
        currentAnimElemForFighter(state, fighter),
        fighter.life,
        fighter.power,
        fighter.hitPauseTicks,
        fighter.hitStunTicks,
        fighter.pauseMoveTime,
        fighter.superMoveTime,
        fighter.ownerIndex,
        fighter.stateType,
        fighter.moveType,
        fighter.physics,
        fighter.ctrl,
        fighter.onGround,
        fighter.helper,
        fighter.destroyRequested,
    };
}

FreezeWatchSnapshot freezeWatchSnapshot(const AppState& state) {
    FreezeWatchSnapshot snapshot;
    snapshot.matchPhase = state.matchPhase;
    snapshot.matchPhaseTicks = state.matchPhaseTicks;
    snapshot.globalPauseTicks = state.globalPauseTicks;
    snapshot.globalPauseOwnerIndex = state.globalPauseOwnerIndex;
    snapshot.globalPauseOwnerMoveTicks = state.globalPauseOwnerMoveTicks;
    snapshot.globalPauseIsSuper = state.globalPauseIsSuper;
    snapshot.activeProjectiles = static_cast<int>(state.projectiles.size());
    snapshot.activeEffects = static_cast<int>(state.runtimeEffects.size());
    snapshot.fighters.reserve(state.fighters.size());
    for (const auto& fighter : state.fighters) {
        snapshot.fighters.push_back(freezeWatchSample(state, fighter));
    }
    snapshot.helpers.reserve(state.helpers.size());
    for (const auto& helper : state.helpers) {
        if (!helper.destroyRequested) {
            snapshot.helpers.push_back(freezeWatchSample(state, helper));
        }
    }
    return snapshot;
}

bool freezeWatchNear(float lhs, float rhs) {
    return std::fabs(lhs - rhs) <= 0.001f;
}

bool freezeWatchActorProgressSame(
    const FreezeWatchFighterSample& lhs,
    const FreezeWatchFighterSample& rhs) {
    return lhs.stateNo == rhs.stateNo
        && lhs.stateTime == rhs.stateTime
        && lhs.action == rhs.action
        && lhs.animTick == rhs.animTick
        && lhs.animElem == rhs.animElem
        && lhs.hitPauseTicks == rhs.hitPauseTicks
        && lhs.hitStunTicks == rhs.hitStunTicks
        && lhs.life == rhs.life
        && lhs.power == rhs.power
        && lhs.ctrl == rhs.ctrl
        && lhs.onGround == rhs.onGround
        && freezeWatchNear(lhs.x, rhs.x)
        && freezeWatchNear(lhs.y, rhs.y)
        && freezeWatchNear(lhs.vx, rhs.vx)
        && freezeWatchNear(lhs.vy, rhs.vy)
        && freezeWatchNear(lhs.depthZ, rhs.depthZ);
}

bool freezeWatchActorListProgressSame(
    const std::vector<FreezeWatchFighterSample>& lhs,
    const std::vector<FreezeWatchFighterSample>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (!freezeWatchActorProgressSame(lhs[i], rhs[i])) {
            return false;
        }
    }
    return true;
}

bool freezeWatchProgressSame(const FreezeWatchSnapshot& lhs, const FreezeWatchSnapshot& rhs) {
    return lhs.matchPhase == rhs.matchPhase
        && lhs.activeProjectiles == rhs.activeProjectiles
        && lhs.activeEffects == rhs.activeEffects
        && freezeWatchActorListProgressSame(lhs.fighters, rhs.fighters)
        && freezeWatchActorListProgressSame(lhs.helpers, rhs.helpers);
}

bool freezeWatchPoseCandidate(const FreezeWatchFighterSample& actor) {
    return actor.life > 0
        && actor.hitPauseTicks <= 0
        && (actor.helper
            || actor.stateNo != 0
            || actor.moveType != 'I'
            || actor.stateType == 'A'
            || !actor.ctrl);
}

bool freezeWatchPoseSame(
    const FreezeWatchFighterSample& lhs,
    const FreezeWatchFighterSample& rhs) {
    return lhs.stateNo == rhs.stateNo
        && lhs.action == rhs.action
        && lhs.animElem == rhs.animElem
        && lhs.stateType == rhs.stateType
        && lhs.moveType == rhs.moveType
        && freezeWatchNear(lhs.x, rhs.x)
        && freezeWatchNear(lhs.y, rhs.y)
        && freezeWatchNear(lhs.depthZ, rhs.depthZ);
}

void updateFreezeWatchPoseList(
    const std::vector<FreezeWatchFighterSample>& current,
    const std::vector<FreezeWatchFighterSample>& previous,
    std::vector<int>& counters,
    std::string_view labelPrefix,
    int& maxFrames,
    std::string& maxLabel) {
    counters.resize(current.size(), 0);
    for (size_t i = 0; i < current.size(); ++i) {
        if (i < previous.size()
            && freezeWatchPoseCandidate(current[i])
            && freezeWatchPoseSame(current[i], previous[i])) {
            ++counters[i];
        } else {
            counters[i] = 0;
        }

        if (counters[i] > maxFrames) {
            maxFrames = counters[i];
            maxLabel = std::string(labelPrefix) + std::to_string(i + 1);
        }
    }
}

void updateFreezeWatchPoseStalls(AppState& state, const FreezeWatchSnapshot& current) {
    state.freezeWatch.poseStalledFrames = 0;
    state.freezeWatch.poseStalledLabel.clear();
    if (!state.freezeWatch.hasSnapshot || state.globalPauseTicks > 0) {
        state.freezeWatch.fighterPoseStallFrames.assign(current.fighters.size(), 0);
        state.freezeWatch.helperPoseStallFrames.assign(current.helpers.size(), 0);
        return;
    }

    updateFreezeWatchPoseList(
        current.fighters,
        state.freezeWatch.previous.fighters,
        state.freezeWatch.fighterPoseStallFrames,
        "P",
        state.freezeWatch.poseStalledFrames,
        state.freezeWatch.poseStalledLabel);
    updateFreezeWatchPoseList(
        current.helpers,
        state.freezeWatch.previous.helpers,
        state.freezeWatch.helperPoseStallFrames,
        "H",
        state.freezeWatch.poseStalledFrames,
        state.freezeWatch.poseStalledLabel);
}

void resetFreezeWatchRuntime(AppState& state) {
    state.freezeWatch.hasSnapshot = false;
    state.freezeWatch.fighterStalledFrames = 0;
    state.freezeWatch.poseStalledFrames = 0;
    state.freezeWatch.lastLogStallFrame = 0;
    state.freezeWatch.lastLogPoseStallFrame = 0;
    state.freezeWatch.poseStalledLabel.clear();
    state.freezeWatch.fighterPoseStallFrames.clear();
    state.freezeWatch.helperPoseStallFrames.clear();
    state.freezeWatch.previous = {};
}

void updateFightFreezeWatch(AppState& state, bool fightPaused) {
    if (state.frontend.screen != Screen::FightView || fightPaused) {
        resetFreezeWatchRuntime(state);
        return;
    }

    const FreezeWatchSnapshot current = freezeWatchSnapshot(state);
    updateFreezeWatchPoseStalls(state, current);
    if (state.freezeWatch.hasSnapshot && freezeWatchProgressSame(state.freezeWatch.previous, current)) {
        ++state.freezeWatch.fighterStalledFrames;
    } else {
        state.freezeWatch.fighterStalledFrames = 0;
    }
    state.freezeWatch.previous = current;
    state.freezeWatch.hasSnapshot = true;

    if (state.freezeWatch.visible
        && state.freezeWatch.fighterStalledFrames >= 90
        && state.freezeWatch.fighterStalledFrames - state.freezeWatch.lastLogStallFrame >= 90) {
        state.freezeWatch.lastLogStallFrame = state.freezeWatch.fighterStalledFrames;
        std::ostringstream line;
        line << "FreezeWatch stall=" << state.freezeWatch.fighterStalledFrames
             << " phase=" << freezeWatchPhaseName(state.matchPhase)
             << " gp=" << state.globalPauseTicks
             << " owner=" << state.globalPauseOwnerIndex
             << " move=" << state.globalPauseOwnerMoveTicks
             << " super=" << (state.globalPauseIsSuper ? 1 : 0);
        for (size_t i = 0; i < state.fighters.size(); ++i) {
            const auto& fighter = state.fighters[i];
            line << " F" << i + 1
                 << " st=" << fighter.stateNo
                 << " t=" << fighter.stateTime
                 << " act=" << fighter.action
                 << " anim=" << fighter.animTick
                 << " hp=" << fighter.hitPauseTicks
                 << " y=" << std::lround(fighter.y);
        }
        SDL_Log("%s", line.str().c_str());
    }

    if (state.freezeWatch.visible
        && state.freezeWatch.poseStalledFrames >= 90
        && state.freezeWatch.poseStalledFrames - state.freezeWatch.lastLogPoseStallFrame >= 90) {
        state.freezeWatch.lastLogPoseStallFrame = state.freezeWatch.poseStalledFrames;
        SDL_Log(
            "FreezeWatch pose-stall actor=%s frames=%d phase=%s",
            state.freezeWatch.poseStalledLabel.c_str(),
            state.freezeWatch.poseStalledFrames,
            freezeWatchPhaseName(state.matchPhase));
    }
}

std::string freezeWatchGlobalPauseText(const AppState& state) {
    if (state.globalPauseTicks <= 0) {
        return "No pause";
    }

    std::ostringstream line;
    line << "Pause " << state.globalPauseTicks << "f";
    if (state.globalPauseOwnerIndex >= 0) {
        line << " P" << state.globalPauseOwnerIndex + 1;
    }
    if (state.globalPauseOwnerMoveTicks > 0) {
        line << " move " << state.globalPauseOwnerMoveTicks;
    }
    if (state.globalPauseIsSuper) {
        line << " super";
    }
    return line.str();
}

std::string freezeWatchStatusText(const AppState& state) {
    if (state.freezeWatch.fighterStalledFrames >= 90) {
        return "STALL";
    }
    if (state.freezeWatch.poseStalledFrames >= 90) {
        return "POSE";
    }
    if (std::max(state.freezeWatch.fighterStalledFrames, state.freezeWatch.poseStalledFrames) >= 45) {
        return "WATCH";
    }
    return "OK";
}

std::string freezeWatchUpdateText(const AppState& state, const FighterState& fighter, int fighterIndex) {
    bool canUpdate = true;
    if (fighter.helper) {
        canUpdate = helperCanUpdateDuringGlobalPause(state, fighter);
    } else {
        canUpdate = fighterCanUpdateDuringGlobalPause(state, fighterIndex);
    }

    if (fighter.hitPauseTicks > 0) {
        return "hit pause";
    }
    if (fighter.posFreezeX || fighter.posFreezeY) {
        return "pos freeze";
    }
    if (!canUpdate) {
        return "global pause";
    }
    return "running";
}

std::string freezeWatchActorLine(
    const AppState& state,
    const FighterState& fighter,
    int fighterIndex,
    std::string_view label) {
    std::ostringstream line;
    line << label
         << " " << freezeWatchUpdateText(state, fighter, fighterIndex)
         << " | State " << fighter.stateNo
         << " | " << fighter.stateTime << "f";
    if (fighter.y != 0.0f) {
        line << " | y " << std::lround(fighter.y);
    }
    if (fighter.helper && fighter.ownerIndex >= 0) {
        line << " | P" << fighter.ownerIndex + 1;
    }
    if (fighter.hitPauseTicks > 0) {
        line << " | hit " << fighter.hitPauseTicks;
    }
    if (fighter.hitStunTicks > 0) {
        line << " | stun " << fighter.hitStunTicks;
    }
    return line.str();
}

void freezeWatchShadowText(
    SDL_Renderer* renderer,
    float x,
    float y,
    const std::string& text,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint8 a = 255) {
    setColor(renderer, 0, 0, 0, 220);
    debugText(renderer, x + 1.0f, y + 1.0f, text);
    setColor(renderer, r, g, b, a);
    debugText(renderer, x, y, text);
}

void drawFightFreezeWatchOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (!state.freezeWatch.visible || state.frontend.screen != Screen::FightView) {
        return;
    }

    const bool stalled = state.freezeWatch.fighterStalledFrames >= 90 || state.freezeWatch.poseStalledFrames >= 90;
    const float badgeW = 60.0f;
    const float badgeH = 12.0f;
    const float badgeX = std::max(6.0f, logicalWidthF(state) - badgeW - 8.0f);
    const float badgeY = 6.0f;

    if (stalled) setColor(renderer, 208, 68, 56, 220);
    else if (std::max(state.freezeWatch.fighterStalledFrames, state.freezeWatch.poseStalledFrames) >= 45) setColor(renderer, 220, 166, 42, 220);
    else setColor(renderer, 54, 156, 92, 220);
    fillRect(renderer, badgeX, badgeY, badgeW, badgeH);
    setColor(renderer, 10, 12, 14, 255);
    debugText(renderer, badgeX + 4.0f, badgeY + 3.0f, "F3 " + freezeWatchStatusText(state));

    if (!stalled) {
        return;
    }

    const float detailX = std::max(6.0f, logicalWidthF(state) - 188.0f);
    float y = badgeY + badgeH + 4.0f;
    freezeWatchShadowText(
        renderer,
        detailX,
        y,
        state.freezeWatch.fighterStalledFrames >= 90
            ? "Runtime unchanged " + std::to_string(state.freezeWatch.fighterStalledFrames) + "f"
            : "Pose stuck " + state.freezeWatch.poseStalledLabel + " " + std::to_string(state.freezeWatch.poseStalledFrames) + "f",
        255,
        156,
        120);
    y += 10.0f;

    freezeWatchShadowText(renderer, detailX, y,
        freezeWatchGlobalPauseText(state)
        + " | H" + std::to_string(state.helpers.size())
        + " P" + std::to_string(state.projectiles.size())
        + " FX" + std::to_string(state.runtimeEffects.size()),
        210, 218, 230);
    y += 10.0f;

    for (size_t i = 0; i < state.fighters.size() && i < 2; ++i) {
        freezeWatchShadowText(
            renderer,
            detailX,
            y,
            freezeWatchActorLine(state, state.fighters[i], static_cast<int>(i), "P" + std::to_string(i + 1)),
            i == 0 ? 170 : 220,
            i == 0 ? 220 : 190,
            i == 0 ? 255 : 150);
        y += 10.0f;
    }

    for (const auto& helper : state.helpers) {
        if (helper.destroyRequested) {
            continue;
        }
        freezeWatchShadowText(renderer, detailX, y, freezeWatchActorLine(state, helper, -1, "Helper"), 160, 232, 180);
        break;
    }
}
