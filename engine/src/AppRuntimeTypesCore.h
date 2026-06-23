#pragma once

// Internal App.cpp implementation shard.
// Core runtime enums and low-level effect/hit data used by App.cpp internals.

enum class GuardStance {
    None,
    Stand,
    Crouch,
};

enum class CompareOp {
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
};

enum class CommandConditionSubject {
    StateNo,
    Time,
    Power,
    RoundState,
    AiLevel,
};

enum class MugenVariableBank {
    Var,
    FVar,
    SysVar,
    SysFVar,
};

struct MugenVariableRef {
    MugenVariableBank bank = MugenVariableBank::Var;
    int index = 0;
};

struct TextureSprite {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    int axisX = 0;
    int axisY = 0;
};

struct CollisionBox {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
};

struct EnvShakeSpec {
    bool enabled = false;
    int time = 0;
    int frequency = 60;
    float amplitude = 0.0f;
    int phase = 0;
    std::string timeExpression;
    std::string frequencyExpression;
    std::string amplitudeExpression;
    std::string phaseExpression;
};

struct PaletteEffectSpec {
    bool enabled = false;
    int time = 0;
    int addR = 0;
    int addG = 0;
    int addB = 0;
    int mulR = 256;
    int mulG = 256;
    int mulB = 256;
    int sinAddR = 0;
    int sinAddG = 0;
    int sinAddB = 0;
    int sinPeriod = 0;
    int color = 256;
    bool invertAll = false;
    std::string timeExpression;
    std::array<std::string, 3> addExpressions;
    std::array<std::string, 3> mulExpressions;
    std::array<std::string, 4> sinAddExpressions;
    std::string colorExpression;
    std::string invertAllExpression;
};

struct ActivePaletteEffect {
    PaletteEffectSpec spec;
    int ticksLeft = 0;
    int elapsedTicks = 0;
};

enum class ActorBlendMode {
    Normal,
    Add,
    Add1,
    AddAlpha,
};

struct ActiveTransEffect {
    bool active = false;
    ActorBlendMode mode = ActorBlendMode::Normal;
    int alphaSource = 256;
    int alphaDest = 0;
};

struct AfterImageSnapshot {
    int action = 0;
    int actionClipOwnerIndex = -1;
    int animTick = 0;
    float x = 0.0f;
    float y = 0.0f;
    float depthZ = 0.0f;
    int facing = 1;
    int ageTicks = 0;
};

struct ActiveAfterImageEffect {
    bool configured = false;
    bool active = false;
    int ticksLeft = 0;
    int length = 4;
    int timeGap = 1;
    int frameGap = 1;
    int captureCountdown = 0;
    ActorBlendMode blendMode = ActorBlendMode::Normal;
    std::array<int, 3> palBright{ 30, 30, 30 };
    std::array<int, 3> palContrast{ 120, 120, 220 };
    std::array<int, 3> palPostBright{ 0, 0, 0 };
    std::array<int, 3> palAdd{ 10, 10, 25 };
    std::array<float, 3> palMul{ 0.65f, 0.65f, 0.75f };
    std::vector<AfterImageSnapshot> trail;
};

struct EnvColorEffect {
    int ticksLeft = 0;
    int r = 255;
    int g = 255;
    int b = 255;
};

struct HitDefinition {
    int id = 0;
    int targetId = 0;
    int stateNo = 0;
    int triggerTime = -1;
    int triggerAnimElem = -1;
    bool hasP2DistX = false;
    CompareOp p2DistXOp = CompareOp::Equal;
    float p2DistX = 0.0f;
    bool hasP2BodyDistX = false;
    CompareOp p2BodyDistXOp = CompareOp::Equal;
    float p2BodyDistX = 0.0f;
    std::string attr = "Unknown";
    std::string animtype = "Light";
    std::string hitflag = "MAF";
    std::string guardflag = "MA";
    int damage = 0;
    int guardDamage = 0;
    std::string damageExpression;
    std::string guardDamageExpression;
    int guardDistance = -1;
    int pausetimeP1 = 0;
    int pausetimeP2 = 0;
    std::string pausetimeP1Expression;
    std::string pausetimeP2Expression;
    int sparkNo = 2;
    int guardSparkNo = 40;
    float sparkX = 0.0f;
    float sparkY = 0.0f;
    std::string sparkNoExpression;
    std::string guardSparkNoExpression;
    std::string sparkXExpression;
    std::string sparkYExpression;
    int hitSoundGroup = -1;
    int hitSoundIndex = -1;
    int guardSoundGroup = -1;
    int guardSoundIndex = -1;
    bool hitSoundForceCommon = false;
    bool guardSoundForceCommon = false;
    std::string hitSoundGroupExpression;
    std::string hitSoundIndexExpression;
    std::string guardSoundGroupExpression;
    std::string guardSoundIndexExpression;
    std::string groundType = "High";
    int groundSlideTime = 0;
    int groundHitTime = 0;
    float groundVelocityX = 0.0f;
    float groundVelocityY = 0.0f;
    std::string groundSlideTimeExpression;
    std::string groundHitTimeExpression;
    std::string groundVelocityXExpression;
    std::string groundVelocityYExpression;
    bool hasAirVelocity = false;
    float airVelocityX = 0.0f;
    float airVelocityY = 0.0f;
    int airHitTime = 0;
    std::string airVelocityXExpression;
    std::string airVelocityYExpression;
    std::string airHitTimeExpression;
    bool hasSnap = false;
    float snapX = 0.0f;
    float snapY = 0.0f;
    std::string snapXExpression;
    std::string snapYExpression;
    bool fall = false;
    bool airFall = false;
    std::string fallExpression;
    std::string airFallExpression;
    std::string fallAnimtype;
    bool fallRecover = true;
    int fallRecoverTime = 4;
    int fallDamage = 0;
    std::string fallRecoverExpression;
    std::string fallRecoverTimeExpression;
    std::string fallDamageExpression;
    bool downRecover = true;
    int downRecoverTime = -1;
    std::string downRecoverExpression;
    std::string downRecoverTimeExpression;
    bool hasDownVelocity = false;
    float downVelocityX = 0.0f;
    float downVelocityY = 0.0f;
    int downHitTime = 20;
    bool downBounce = false;
    std::string downVelocityXExpression;
    std::string downVelocityYExpression;
    std::string downHitTimeExpression;
    std::string downBounceExpression;
    float fallXVelocity = 0.0f;
    bool hasFallXVelocity = false;
    std::string fallXVelocityExpression;
    float fallYVelocity = -4.5f;
    bool hasFallYVelocity = false;
    std::string fallYVelocityExpression;
    float yAccel = 0.0f;
    bool hasYAccel = false;
    std::string yAccelExpression;
    float guardVelocityX = 0.0f;
    float guardVelocityY = 0.0f;
    bool hasGuardVelocity = false;
    std::string guardVelocityXExpression;
    std::string guardVelocityYExpression;
    int p1StateNo = -1;
    std::string p1StateNoExpression;
    bool hasP1Facing = false;
    int p1Facing = 0;
    std::string p1FacingExpression;
    int p2StateNo = -1;
    std::string p2StateNoExpression;
    bool p2GetP1State = false;
    std::string p2GetP1StateExpression;
    bool hasP2Facing = false;
    int p2Facing = 0;
    std::string p2FacingExpression;
    EnvShakeSpec envShake;
    EnvShakeSpec fallEnvShake;
    PaletteEffectSpec palFx;
};
