#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

namespace dragon::verification {

enum class ScenarioMode {
    Training,
    SinglePlayer,
};

struct SymbolicInput {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
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
    float vx = 0.0f;
    float vy = 0.0f;
    int stateNo = 0;
    int action = 0;
    int stateTime = 0;
    int animTick = 0;
    int life = 0;
    int power = 0;
    int hitCount = 0;
    int hitPauseTicks = 0;
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
    std::string lastHitText;
    FighterSnapshot p1;
    FighterSnapshot p2;
};

class RuntimeProbe {
public:
    virtual ~RuntimeProbe() = default;

    virtual bool setup(std::string_view p1Id, std::string_view stageHint, ScenarioMode mode, std::ostream& out) = 0;
    virtual void step(const SymbolicInput& p1Input, int frames) = 0;
    virtual void positionFighters(float p1X, float p2X) = 0;
    virtual RuntimeSnapshot snapshot() const = 0;
    virtual std::string rootText() const = 0;
    virtual std::string stageName() const = 0;
    virtual std::string p1Name() const = 0;
};

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out);

} // namespace dragon::verification
