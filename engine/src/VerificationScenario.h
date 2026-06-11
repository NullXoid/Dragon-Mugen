#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

namespace dragon::verification {

enum class ScenarioMode {
    Training,
    SinglePlayer,
    Arena,
};

struct SymbolicInput {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool depthModifier = false;
    bool s = false;
    bool x = false;
    bool y = false;
    bool z = false;
    bool a = false;
    bool b = false;
    bool c = false;
    bool start = false;
    bool back = false;
};

struct FighterSnapshot {
    float x = 0.0f;
    float y = 0.0f;
    float depthZ = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float depthVz = 0.0f;
    int stateNo = 0;
    int action = 0;
    int stateTime = 0;
    int animTick = 0;
    int life = 0;
    int power = 0;
    int hitCount = 0;
    int hitPauseTicks = 0;
    int hitStunTicks = 0;
    float hitDownVelocityX = 0.0f;
    float hitDownVelocityY = 0.0f;
    float displayOffsetX = 0.0f;
    float displayOffsetY = 0.0f;
    char stateType = 'S';
    char moveType = 'I';
    bool ctrl = false;
    bool onGround = true;
    bool moveContact = false;
    bool moveHit = false;
    bool moveGuarded = false;
};

struct RuntimeSnapshot {
    int frame = 0;
    int matchTimerTicks = 0;
    int matchPhase = 0;
    int activeEffects = 0;
    int activeSounds = 0;
    int comboHits = 0;
    int fighterCount = 0;
    int livingFighters = 0;
    int globalPauseTicks = 0;
    int globalPauseOwnerMoveTicks = 0;
    bool globalPauseIsSuper = false;
    int activeHelpers = 0;
    int idleHelpers = 0;
    int firstHelperState = 0;
    int firstHelperAction = 0;
    int firstHelperAnimTick = 0;
    int roundWinner = 0;
    int arenaRuntimeCount = 0;
    std::string arenaDrawOrder;
    std::string lastHitText;
    std::string p1Commands;
    FighterSnapshot p1;
    FighterSnapshot p2;
};

class RuntimeProbe {
public:
    virtual ~RuntimeProbe() = default;

    virtual bool setup(
        std::string_view p1Id,
        std::string_view stageHint,
        ScenarioMode mode,
        std::ostream& out,
        int arenaCpuCount = 1) = 0;
    virtual void step(const SymbolicInput& p1Input, int frames) = 0;
    virtual void positionFighters(float p1X, float p2X) = 0;
    virtual void setFighterDepth(int fighterIndex, float depthZ) = 0;
    virtual void setFighterLife(int fighterIndex, int life) = 0;
    virtual void setFighterPower(int fighterIndex, int power) = 0;
    virtual void setFighterControl(int fighterIndex, bool enabled) = 0;
    virtual void setFighterHitPause(int fighterIndex, int ticks) = 0;
    virtual void forceFighterState(int fighterIndex, int stateNo) = 0;
    virtual void forceFighterLiedown(int fighterIndex, int hitStunTicks) = 0;
    virtual void spawnHelper(int ownerIndex, int helperId, int stateNo, int pauseMoveTime = 0, int superMoveTime = 0) = 0;
    virtual bool selectTrainingMove(std::string_view label) = 0;
    virtual void startTrainingCommandDemo() = 0;
    virtual RuntimeSnapshot snapshot() const = 0;
    virtual std::string rootText() const = 0;
    virtual std::string stageName() const = 0;
    virtual std::string p1Name() const = 0;
};

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out);

} // namespace dragon::verification
