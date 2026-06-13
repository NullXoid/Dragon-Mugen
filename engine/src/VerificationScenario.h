#pragma once

#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace dragon::verification {

enum class ScenarioMode {
    Training,
    SinglePlayer,
    Versus,
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
    int targetIndex = -1;
    int targetTicks = 0;
    int targetHitId = -1;
    int hitCount = 0;
    int hitPauseTicks = 0;
    int hitStunTicks = 0;
    float hitDownVelocityX = 0.0f;
    float hitDownVelocityY = 0.0f;
    float displayOffsetX = 0.0f;
    float displayOffsetY = 0.0f;
    float screenX = 0.0f;
    float screenY = 0.0f;
    float viewDepth = 0.0f;
    int facing = 1;
    char stateType = 'S';
    char moveType = 'I';
    char physics = 'S';
    bool ctrl = false;
    bool onGround = true;
    bool moveContact = false;
    bool moveHit = false;
    bool moveGuarded = false;
};

struct RuntimeSnapshot {
    int frame = 0;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float arenaCameraYawDeg = 0.0f;
    float arenaCameraTargetYawDeg = 0.0f;
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
    int p1RuntimeStates = 0;
    int p2RuntimeStates = 0;
    int p1RuntimeHitDefs = 0;
    int p2RuntimeHitDefs = 0;
    int p1RuntimeCommandEntries = 0;
    int p2RuntimeCommandEntries = 0;
    int roundWinner = 0;
    int arenaRuntimeCount = 0;
    bool arenaZAxisEnabled = false;
    bool arenaCameraRotationSelected = false;
    bool arenaCameraRotationActive = false;
    bool p1P2BoxesOverlap = false;
    bool p1ActiveHitDef = false;
    bool p1HitFlagAllowsP2 = false;
    bool p2HittableByP1 = false;
    int p1AnimElem = 0;
    int p2AnimElem = 0;
    int p1Clsn1Count = 0;
    int p2Clsn2Count = 0;
    float p1P2BodyDistX = 0.0f;
    std::string arenaDrawOrder;
    std::string lastHitText;
    std::string p1Commands;
    std::string p2Commands;
    std::string selectedTrainingMoveLabel;
    FighterSnapshot p1;
    FighterSnapshot p2;
};

struct TrainingMoveInfo {
    std::string label;
    std::string input;
    int targetState = -1;
    char requiredStateType = 0;
    int requiredPower = 0;
    std::vector<std::string> commandNames;
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
    virtual void setFighterPosition(int fighterIndex, float x, float y) = 0;
    virtual void setFighterDepth(int fighterIndex, float depthZ) = 0;
    virtual void setFighterLife(int fighterIndex, int life) = 0;
    virtual void setFighterPower(int fighterIndex, int power) = 0;
    virtual void setFighterVar(int fighterIndex, int varIndex, int value) = 0;
    virtual int fighterVar(int fighterIndex, int varIndex) const = 0;
    virtual void setFighterMoveContact(int fighterIndex, bool hit, bool guarded) = 0;
    virtual void setFighterControl(int fighterIndex, bool enabled) = 0;
    virtual void setArenaZAxisEnabled(bool enabled) = 0;
    virtual void setArenaCameraRotationEnabled(bool enabled) = 0;
    virtual void setFighterHitPause(int fighterIndex, int ticks) = 0;
    virtual void forceFighterState(int fighterIndex, int stateNo) = 0;
    virtual void forceFighterLiedown(int fighterIndex, int hitStunTicks) = 0;
    virtual void spawnHelper(int ownerIndex, int helperId, int stateNo, int pauseMoveTime = 0, int superMoveTime = 0) = 0;
    virtual std::vector<TrainingMoveInfo> trainingMoves() const = 0;
    virtual bool selectTrainingMoveIndex(int index) = 0;
    virtual bool selectTrainingMove(std::string_view label) = 0;
    virtual void startTrainingCommandDemo() = 0;
    virtual RuntimeSnapshot snapshot() const = 0;
    virtual std::string rootText() const = 0;
    virtual std::string stageName() const = 0;
    virtual std::string p1Name() const = 0;
};

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out);

} // namespace dragon::verification
