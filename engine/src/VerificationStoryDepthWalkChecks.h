#pragma once

#include "VerificationScenario.h"

#include <cmath>
#include <string>

namespace dragon::verification {

struct StoryDepthWalkProbeResult {
    bool idle = false;
    bool forwardPass = false;
    bool reversePass = false;
    int idleState = 0;
    bool idleCtrl = false;
    std::string p1CharacterId;
    std::string p1CharacterName;
    std::string forwardDetail;
    std::string reverseDetail;
};

inline bool waitForStoryControllableIdle(RuntimeProbe& runtime, int maxFrames) {
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

inline StoryDepthWalkProbeResult probeStoryDepthWalkAnimation(RuntimeProbe& runtime) {
    StoryDepthWalkProbeResult result;
    result.idle = waitForStoryControllableIdle(runtime, 240);
    auto idleSnapshot = runtime.snapshot();
    result.idleState = idleSnapshot.p1.stateNo;
    result.idleCtrl = idleSnapshot.p1.ctrl;
    result.p1CharacterId = idleSnapshot.p1CharacterId;
    result.p1CharacterName = idleSnapshot.p1CharacterName;
    if (!result.idle) {
        return result;
    }

    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const auto before = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true }, 36);
    const auto after = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true }, 24);
    const auto afterMore = runtime.snapshot();
    const bool movedDepth = after.p1.depthZ > before.p1.depthZ + 8.0f;
    const bool stayedGrounded = after.p1.onGround && std::fabs(after.p1.y - before.p1.y) <= 0.5f && after.p1.stateType != 'C';
    const bool walkState = after.p1.stateNo == 20;
    const bool walkAction = after.p1.action == 24;
    const bool walkTickAdvanced = after.p1.animTick > before.p1.animTick;
    const bool walkElemAdvanced = afterMore.p1AnimElem != after.p1AnimElem;
    result.forwardPass = movedDepth && stayedGrounded && walkState && walkAction && walkTickAdvanced && walkElemAdvanced;
    result.forwardDetail = "depth_before=" + std::to_string(before.p1.depthZ)
        + " depth_after=" + std::to_string(after.p1.depthZ)
        + " depth_after_more=" + std::to_string(afterMore.p1.depthZ)
        + " y_before=" + std::to_string(before.p1.y)
        + " y_after=" + std::to_string(after.p1.y)
        + " state=" + std::to_string(after.p1.stateNo)
        + " action=" + std::to_string(after.p1.action)
        + " anim_tick_before=" + std::to_string(before.p1.animTick)
        + " anim_tick_after=" + std::to_string(after.p1.animTick)
        + " elem_after=" + std::to_string(after.p1AnimElem)
        + " elem_after_more=" + std::to_string(afterMore.p1AnimElem)
        + " state_type=" + std::string(1, after.p1.stateType);

    runtime.step(SymbolicInput{ .up = true }, 36);
    const auto reverse = runtime.snapshot();
    runtime.step(SymbolicInput{ .up = true }, 24);
    const auto reverseMore = runtime.snapshot();
    const bool reversedDepth = reverse.p1.depthZ < afterMore.p1.depthZ - 8.0f;
    const bool reverseWalkState = reverse.p1.stateNo == 20;
    const bool reverseWalkAction = reverse.p1.action == 25;
    const bool reverseWalkTickAdvanced = reverseMore.p1.animTick > reverse.p1.animTick;
    const bool reverseElemAdvanced = reverseMore.p1AnimElem != reverse.p1AnimElem;
    result.reversePass = reversedDepth && reverseWalkState && reverseWalkAction && reverseWalkTickAdvanced && reverseElemAdvanced;
    result.reverseDetail = "depth_after_down=" + std::to_string(afterMore.p1.depthZ)
        + " depth_after_up=" + std::to_string(reverse.p1.depthZ)
        + " depth_after_up_more=" + std::to_string(reverseMore.p1.depthZ)
        + " anim_tick_up=" + std::to_string(reverse.p1.animTick)
        + " anim_tick_up_more=" + std::to_string(reverseMore.p1.animTick)
        + " elem_up=" + std::to_string(reverse.p1AnimElem)
        + " elem_up_more=" + std::to_string(reverseMore.p1AnimElem)
        + " state=" + std::to_string(reverse.p1.stateNo)
        + " action=" + std::to_string(reverse.p1.action);
    return result;
}

} // namespace dragon::verification
