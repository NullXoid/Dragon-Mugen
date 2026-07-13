#pragma once

#include "VerificationScenario.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace dragon::verification {
namespace {

enum class Status {
    Pass,
    Partial,
    Fail,
    Blocked,
};

struct Counts {
    int pass = 0;
    int partial = 0;
    int fail = 0;
    int blocked = 0;
};

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass:
        return "PASS";
    case Status::Partial:
        return "PARTIAL";
    case Status::Fail:
        return "FAIL";
    case Status::Blocked:
    default:
        return "BLOCKED";
    }
}

void record(std::ostream& out, Counts& counts, Status status, std::string_view name, std::string_view detail) {
    out << statusText(status) << ' ' << name << "\n";
    if (!detail.empty()) {
        out << "  " << detail << "\n";
    }
    if (status == Status::Pass) {
        ++counts.pass;
    } else if (status == Status::Partial) {
        ++counts.partial;
    } else if (status == Status::Fail) {
        ++counts.fail;
    } else {
        ++counts.blocked;
    }
}

int exitCode(const Counts& counts) {
    if (counts.fail > 0) return 1;
    if (counts.blocked > 0) return 2;
    return 0;
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass << " partial=" << counts.partial << " fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

void header(std::ostream& out, RuntimeProbe& runtime, std::string_view scenario) {
    out << "VERIFY " << scenario << "\n" << "root: " << runtime.rootText() << "\n"
        << "stage: " << runtime.stageName() << "\n" << "p1: " << runtime.p1Name() << "\n";
}

bool waitForControllableIdle(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        const auto p1 = runtime.snapshot().p1;
        if (p1.stateNo == 0 && p1.ctrl && p1.onGround && p1.moveType == 'I') {
            return true;
        }
        runtime.step({}, 1);
    }
    const auto p1 = runtime.snapshot().p1;
    return p1.stateNo == 0 && p1.ctrl && p1.onGround && p1.moveType == 'I';
}

[[maybe_unused]] std::string moveFailureText(const TrainingMoveInfo& move, const FighterSnapshot& p2, std::string_view commands) {
    std::ostringstream text;
    text << move.label << " [" << move.input << " -> " << move.targetState << "]"
         << " final_state=" << p2.stateNo
         << " final_action=" << p2.action
         << " final_time=" << p2.stateTime
         << " anim_tick=" << p2.animTick
         << " y=" << p2.y
         << " vy=" << p2.vy
         << " state_type=" << p2.stateType
         << " physics=" << p2.physics
         << " move_type=" << p2.moveType
         << " hitpause=" << p2.hitPauseTicks
         << " ground=" << (p2.onGround ? 1 : 0)
         << " ctrl=" << (p2.ctrl ? 1 : 0)
         << " commands=" << commands;
    return text.str();
}

[[maybe_unused]] std::string joinLimited(const std::vector<std::string>& values, size_t limit = 80) {
    if (values.empty()) {
        return "-";
    }
    std::string text;
    const size_t count = std::min(values.size(), limit);
    for (size_t i = 0; i < count; ++i) {
        if (!text.empty()) {
            text += " | ";
        }
        text += values[i];
    }
    if (values.size() > limit) {
        text += " | +" + std::to_string(values.size() - limit) + " more";
    }
    return text;
}

bool commaListContains(std::string_view text, std::string_view needle) {
    size_t start = 0;
    while (start <= text.size()) {
        const size_t comma = text.find(',', start);
        const size_t end = comma == std::string_view::npos ? text.size() : comma;
        if (text.substr(start, end - start) == needle) {
            return true;
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return false;
}

[[maybe_unused]] bool commandListMatchesExpected(const TrainingMoveInfo& move, std::string_view commands) {
    for (const auto& command : move.commandNames) {
        if (commaListContains(commands, command)) {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] void appendUniqueText(std::vector<std::string>& values, const std::string& value, size_t limit = 12) {
    if (value.empty() || values.size() >= limit) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

[[maybe_unused]] std::string stateTraceText(const FighterSnapshot& fighter) {
    if (fighter.stateNo == 0 || fighter.stateNo == 20) {
        return {};
    }
    return std::to_string(fighter.stateNo) + "/" + std::to_string(fighter.action);
}

bool snapshotMatchesTrainingMoveTarget(const TrainingMoveInfo& move, const FighterSnapshot& fighter) {
    if (move.targetState >= 0) {
        return fighter.stateNo == move.targetState;
    }
    return fighter.stateNo != 0
        && fighter.stateNo != 20
        && fighter.stateNo != 40
        && fighter.stateNo != 41
        && fighter.stateNo != 42
        && fighter.stateNo != 45
        && fighter.stateNo != 50
        && fighter.stateNo != 51
        && fighter.stateNo != 52
        && fighter.moveType != 'I';
}

[[maybe_unused]] bool snapshotIndicatesTrainingMoveTarget(const TrainingMoveInfo& move, const RuntimeSnapshot& snap) {
    if (snapshotMatchesTrainingMoveTarget(move, snap.p2)) {
        return true;
    }
    if (move.targetState < 0) {
        return false;
    }
    const std::string hitNeedle = "P2 hit " + std::to_string(move.targetState) + "#";
    return snap.lastHitText.find(hitNeedle) != std::string::npos;
}

} // namespace

} // namespace dragon::verification
