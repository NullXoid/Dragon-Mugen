#pragma once

// Internal App.cpp implementation shard.
// State controller and runtime controller data used by App.cpp internals.

struct RuntimeProjectile {
    int id = 0;
    int ownerIndex = -1;
    int action = 0;
    int hitAction = -1;
    int removeAction = -1;
    int cancelAction = -1;
    int animTick = 0;
    float x = 0.0f;
    float y = 0.0f;
    float depthZ = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    int facing = 1;
    int hitsRemaining = 1;
    int removeTime = -1;
    int missTime = 0;
    int hitCooldownTicks = 0;
    int removeWhenHit = 1;
    int priority = 1;
    int cancelPriority = 1;
    int pauseMoveTime = 0;
    int superMoveTime = 0;
    float projEdgeBound = 40.0f;
    float projStageBound = 40.0f;
    float projHeightBoundLow = -240.0f;
    float projHeightBoundHigh = 40.0f;
    float ax = 0.0f;
    float ay = 0.0f;
    float velMulX = 1.0f;
    float velMulY = 1.0f;
    float removeVx = 0.0f;
    float removeVy = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    bool shadowEnabled = false;
    int shadowR = 0;
    int shadowG = 0;
    int shadowB = 0;
    bool removing = false;
    HitDefinition hitDef;
};

struct AnimElemTimeCondition {
    int elem = 0;
    CompareOp op = CompareOp::Equal;
    int value = 0;
};

enum class StateTriggerSubject {
    Time,
    Anim,
    AnimTime,
    VelX,
    VelY,
    PosX,
    PosY,
    P2BodyDistX,
    P2BodyDistY,
    P2DistX,
    P2DistY,
    FrontEdgeBodyDist,
    BackEdgeBodyDist,
    HitShakeOver,
};

enum class StateVelocityOperation {
    Set,
    Add,
    Mul,
};

enum class StateVariableOperation {
    Set,
    Add,
    Random,
};

struct StateFloatCondition {
    StateTriggerSubject subject = StateTriggerSubject::Time;
    CompareOp op = CompareOp::Equal;
    float value = 0.0f;
};

struct StateRangeCondition {
    StateTriggerSubject subject = StateTriggerSubject::Time;
    float minValue = 0.0f;
    float maxValue = 0.0f;
};

struct StateTypeTriggerCondition {
    char stateType = 0;
    bool negated = false;
};

struct MugenExpressionCondition {
    std::string lhs;
    CompareOp op = CompareOp::Equal;
    std::string rhs;
};

struct StateTriggerGroup {
    std::vector<StateFloatCondition> floatConditions;
    std::vector<StateRangeCondition> rangeConditions;
    std::vector<StateTypeTriggerCondition> stateTypeConditions;
    std::vector<MugenExpressionCondition> expressionConditions;
    std::vector<std::string> booleanExpressions;
    std::vector<AnimElemTimeCondition> animElemTimeConditions;
    std::vector<std::string> requiredCommands;
    std::vector<std::string> forbiddenCommands;
    bool requiresMoveContact = false;
};

struct StateControllerTrigger {
    bool hasTrigger = false;
    int persistent = 1;
    bool ignoreHitPause = false;
    std::vector<std::vector<StateTriggerGroup>> triggerAllExpressions;
    std::vector<StateTriggerGroup> triggerGroups;
};

struct StateSoundController {
    int id = 0;
    StateControllerTrigger trigger;
    int triggerTime = -1;
    int triggerAnimElem = -1;
    int group = -1;
    int index = -1;
    bool forceCommon = false;
    int channel = -1;
    bool lowPriority = false;
    float gain = 1.0f;
    bool loop = false;
};

struct StateStopSoundController {
    int id = 0;
    StateControllerTrigger trigger;
    int triggerTime = -1;
    int triggerAnimElem = -1;
    int channel = -1;
};

enum class StateAudioControllerKind {
    PlaySnd,
    StopSnd,
};

struct StateAudioControllerRef {
    StateAudioControllerKind kind = StateAudioControllerKind::PlaySnd;
    size_t index = 0;
};

struct StateCtrlController {
    int id = 0;
    StateControllerTrigger trigger;
    int triggerTime = -1;
    int triggerAnimElem = -1;
    bool value = false;
};

struct StateVariableController {
    int id = 0;
    StateControllerTrigger trigger;
    MugenVariableRef target;
    StateVariableOperation operation = StateVariableOperation::Set;
    std::string valueExpression = "0";
    std::string rangeMinExpression = "0";
    std::string rangeMaxExpression = "0";
};

struct StatePosAddController {
    int id = 0;
    StateControllerTrigger trigger;
    int triggerTime = -1;
    int triggerAnimElem = -1;
    float x = 0.0f;
    float y = 0.0f;
};

struct StateChangeAnimController {
    int id = 0;
    StateControllerTrigger trigger;
    int triggerTime = -1;
    int triggerAnimElem = -1;
    int value = 0;
    int elem = 1;
    std::string valueExpression;
    std::string elemExpression;
    bool useCustomStateOwnerAnimation = false;
    bool requiresMoveContact = false;
    std::vector<AnimElemTimeCondition> animElemTimeConditions;
};

struct StateHelperController {
    int id = 0;
    StateControllerTrigger trigger;
    int helperId = 0;
    int stateNo = 0;
    std::string stateNoExpression;
    float x = 0.0f;
    float y = 0.0f;
    std::string xExpression;
    std::string yExpression;
    std::string postype = "p1";
    int sprPriority = 0;
    int facing = 0;
    int pauseMoveTime = 0;
    int superMoveTime = 0;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    std::string scaleXExpression;
    std::string scaleYExpression;
};

struct StateDestroySelfController {
    int id = 0;
    StateControllerTrigger trigger;
};

struct StateBindToParentController {
    int id = 0;
    StateControllerTrigger trigger;
    float x = 0.0f;
    float y = 0.0f;
};

struct StateBindToRootController {
    int id = 0;
    StateControllerTrigger trigger;
    float x = 0.0f;
    float y = 0.0f;
    int time = 1;
    int facing = 0;
};

struct StateParentVarAddController {
    int id = 0;
    StateControllerTrigger trigger;
    MugenVariableRef target;
    std::string valueExpression = "0";
};

struct StateVarRangeSetController {
    int id = 0;
    StateControllerTrigger trigger;
    bool floatBank = false;
    std::string valueExpression = "0";
    int first = 0;
    int last = 0;
};

struct StatePowerAddController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "0";
};

struct StateLifeAddController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "0";
    bool kill = true;
};

struct StateHitAddController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "0";
};

struct StateAttackDistController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "0";
};

struct StateDefenceMulSetController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "1";
};

struct StateAttackMulSetController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "1";
};

struct StateHitFallDamageController {
    int id = 0;
    StateControllerTrigger trigger;
};

struct StateHitFallVelController {
    int id = 0;
    StateControllerTrigger trigger;
};

struct StateHitFallSetController {
    int id = 0;
    StateControllerTrigger trigger;
    int value = -1;
    bool hasXVelocity = false;
    bool hasYVelocity = false;
    float xVelocity = 0.0f;
    float yVelocity = 0.0f;
};

enum class StateAngleOperation {
    Set,
    Add,
    Mul,
};

struct StateAngleController {
    int id = 0;
    StateControllerTrigger trigger;
    StateAngleOperation operation = StateAngleOperation::Set;
    std::string valueExpression = "0";
};

struct StateAngleDrawController {
    int id = 0;
    StateControllerTrigger trigger;
};

struct StateOffsetController {
    int id = 0;
    StateControllerTrigger trigger;
    bool hasX = false;
    bool hasY = false;
    std::string xExpression = "0";
    std::string yExpression = "0";
};

struct StateForceFeedbackController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string waveform = "sine";
    int time = 60;
    int amplitude = 128;
    bool self = true;
};

struct StateGameMakeAnimController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "0";
    bool under = false;
    float x = 0.0f;
    float y = 0.0f;
    int random = 0;
};

struct StateClipboardController {
    int id = 0;
    StateControllerTrigger trigger;
    bool append = false;
    std::string text;
    std::vector<std::string> params;
};

struct StateVictoryQuoteController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression = "-1";
};

struct PaletteRemap {
    std::string sourceGroupExpression = "-1";
    std::string sourceItemExpression = "0";
    std::string destGroupExpression = "-1";
    std::string destItemExpression = "0";
};

struct StateRemapPalController {
    int id = 0;
    StateControllerTrigger trigger;
    PaletteRemap remap;
};

struct StateTransController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string trans = "default";
    std::string alphaSourceExpression = "256";
    std::string alphaDestExpression = "0";
};

struct StateAfterImageController {
    int id = 0;
    StateControllerTrigger trigger;
    int time = 1;
    int length = 20;
    int timeGap = 1;
    int frameGap = 4;
    std::string trans = "default";
    std::array<int, 3> palBright{ 30, 30, 30 };
    std::array<int, 3> palContrast{ 120, 120, 220 };
    std::array<int, 3> palPostBright{ 0, 0, 0 };
    std::array<int, 3> palAdd{ 10, 10, 25 };
    std::array<float, 3> palMul{ 0.65f, 0.65f, 0.75f };
};

struct StateAfterImageTimeController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string timeExpression = "0";
};

struct StateProjectileController {
    int id = 0;
    StateControllerTrigger trigger;
    int projectileId = 0;
    int anim = -1;
    int hitAnim = -1;
    int removeAnim = -1;
    int cancelAnim = -1;
    int hits = 1;
    int removeTime = -1;
    int missTime = 0;
    int removeWhenHit = 1;
    int priority = 1;
    int cancelPriority = 1;
    int pauseMoveTime = 0;
    int superMoveTime = 0;
    float projEdgeBound = 40.0f;
    float projStageBound = 40.0f;
    float projHeightBoundLow = -240.0f;
    float projHeightBoundHigh = 40.0f;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float ax = 0.0f;
    float ay = 0.0f;
    float velMulX = 1.0f;
    float velMulY = 1.0f;
    float removeVx = 0.0f;
    float removeVy = 0.0f;
    std::string postype = "p1";
    std::string xExpression;
    std::string yExpression;
    std::string vxExpression;
    std::string vyExpression;
    std::string axExpression;
    std::string ayExpression;
    std::string velMulXExpression;
    std::string velMulYExpression;
    std::string removeVxExpression;
    std::string removeVyExpression;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    std::string scaleXExpression;
    std::string scaleYExpression;
    bool shadowEnabled = false;
    int shadowR = 0;
    int shadowG = 0;
    int shadowB = 0;
    HitDefinition hitDef;
};

struct StateMakeDustController {
    int id = 0;
    StateControllerTrigger trigger;
    float x = 0.0f;
    float y = 0.0f;
    bool hasPos2 = false;
    float x2 = 0.0f;
    float y2 = 0.0f;
    int spacing = 3;
};

struct StateVelocityController {
    int id = 0;
    StateControllerTrigger trigger;
    StateVelocityOperation operation = StateVelocityOperation::Set;
    bool hasX = false;
    bool hasY = false;
    float x = 0.0f;
    float y = 0.0f;
    std::string xExpression;
    std::string yExpression;
};

struct StatePosSetController {
    int id = 0;
    StateControllerTrigger trigger;
    bool hasX = false;
    bool hasY = false;
    float x = 0.0f;
    float y = 0.0f;
};

struct StateTypeSetController {
    int id = 0;
    StateControllerTrigger trigger;
    bool hasStateType = false;
    bool hasMoveType = false;
    bool hasPhysics = false;
    char stateType = 'S';
    char moveType = 'I';
    char physics = 'S';
};

struct StateScreenBoundController {
    int id = 0;
    StateControllerTrigger trigger;
    bool value = true;
    bool moveCameraX = false;
    bool moveCameraY = false;
};

struct StateWidthController {
    int id = 0;
    StateControllerTrigger trigger;
    bool hasEdge = false;
    float edgeFront = 0.0f;
    float edgeBack = 0.0f;
    bool hasPlayer = false;
    float playerFront = 0.0f;
    float playerBack = 0.0f;
};

struct StatePlayerPushController {
    int id = 0;
    StateControllerTrigger trigger;
    bool value = true;
};

struct StateSprPriorityController {
    int id = 0;
    StateControllerTrigger trigger;
    int value = 0;
};

struct StatePosFreezeController {
    int id = 0;
    StateControllerTrigger trigger;
    bool freezeX = false;
    bool freezeY = false;
};

struct StateHitVelSetController {
    int id = 0;
    StateControllerTrigger trigger;
    bool applyX = false;
    bool applyY = false;
};

struct StateHitProtectionController {
    int id = 0;
    StateControllerTrigger trigger;
    bool notHitBy = true;
    std::string value;
    int time = 1;
};

struct StateHitOverrideController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string attr;
    int stateNo = -1;
    int time = 1;
};

struct StateTargetStateController {
    int id = 0;
    StateControllerTrigger trigger;
    int value = 0;
    int targetId = -1;
};

struct StateTargetBindController {
    int id = 0;
    StateControllerTrigger trigger;
    float x = 0.0f;
    float y = 0.0f;
    int time = 1;
    int targetId = -1;
};

struct StateTargetDropController {
    int id = 0;
    StateControllerTrigger trigger;
    int excludeId = -1;
};

struct StateTargetFacingController {
    int id = 0;
    StateControllerTrigger trigger;
    int value = 1;
    int targetId = -1;
};

struct StateTargetLifeAddController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression;
    int targetId = -1;
    bool kill = true;
    bool absolute = false;
};

struct StateTargetPowerAddController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string valueExpression;
    int targetId = -1;
};

struct StateTargetVelocityController {
    int id = 0;
    StateControllerTrigger trigger;
    std::string xExpression = "0";
    std::string yExpression = "0";
    bool hasX = false;
    bool hasY = false;
    bool add = false;
    int targetId = -1;
};

struct StateTurnController {
    int id = 0;
    StateControllerTrigger trigger;
};

struct StatePauseController {
    int id = 0;
    StateControllerTrigger trigger;
    bool superPause = false;
    int time = 30;
    int moveTime = 0;
    int powerAdd = 0;
    int soundGroup = -1;
    int soundIndex = -1;
    bool soundForceCommon = false;
};

struct StateExplodController {
    int id = 0;
    StateControllerTrigger trigger;
    int explodId = -1;
    int anim = 0;
    bool fromFightFx = false;
    float x = 0.0f;
    float y = 0.0f;
    std::string postype = "p1";
    int bindTime = 1;
    int removeTime = -2;
    int sprPriority = 0;
    int pauseMoveTime = 0;
    int superMoveTime = 0;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct StateModifyExplodController {
    int id = 0;
    StateControllerTrigger trigger;
    int explodId = -1;
    bool hasSprPriority = false;
    int sprPriority = 0;
    bool hasScale = false;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct StateRemoveExplodController {
    int id = 0;
    StateControllerTrigger trigger;
    int explodId = -1;
};

struct StateEnvShakeController {
    int id = 0;
    StateControllerTrigger trigger;
    EnvShakeSpec shake;
};

struct StateFallEnvShakeController {
    int id = 0;
    StateControllerTrigger trigger;
};

struct StatePaletteEffectController {
    int id = 0;
    StateControllerTrigger trigger;
    PaletteEffectSpec effect;
    bool background = false;
};

struct StateEnvColorController {
    int id = 0;
    StateControllerTrigger trigger;
    int time = 1;
    int r = 255;
    int g = 255;
    int b = 255;
};

struct StateAssertSpecialController {
    int id = 0;
    StateControllerTrigger trigger;
    std::vector<std::string> flags;
};

struct StateChangeStateController {
    int id = 0;
    StateControllerTrigger trigger;
    int targetState = 0;
    std::string targetStateExpression;
    bool selfState = false;
    bool hasCtrl = false;
    bool ctrl = false;
};

struct StateDefinition {
    int stateNo = 0;
    char stateType = 'S';
    char moveType = 'I';
    char physics = 'N';
    int anim = 0;
    std::string animExpression;
    bool hasAnim = false;
    bool ctrl = true;
    std::string powerAddExpression;
    bool hasVelset = false;
    float velsetX = 0.0f;
    float velsetY = 0.0f;
    bool hasAnimEndChangeState = false;
    int animEndChangeState = 0;
    std::string animEndChangeStateExpression;
    bool animEndSelfState = false;
    bool hasAnimEndCtrl = false;
    bool animEndCtrl = false;
    int sprPriority = 0;
    std::vector<StateSoundController> sounds;
    std::vector<StateStopSoundController> stopSounds;
    std::vector<StateAudioControllerRef> audioControllers;
    std::vector<StateCtrlController> ctrlSets;
    std::vector<StateVariableController> variableControllers;
    std::vector<StatePosAddController> posAdds;
    std::vector<StateChangeAnimController> changeAnims;
    std::vector<StateHelperController> helpers;
    std::vector<StateDestroySelfController> destroySelfs;
    std::vector<StateBindToParentController> bindToParents;
    std::vector<StateBindToRootController> bindToRoots;
    std::vector<StateParentVarAddController> parentVarAdds;
    std::vector<StateVarRangeSetController> varRangeSets;
    std::vector<StatePowerAddController> powerAdds;
    std::vector<StateLifeAddController> lifeAdds;
    std::vector<StateHitAddController> hitAdds;
    std::vector<StateAttackDistController> attackDists;
    std::vector<StateDefenceMulSetController> defenceMulSets;
    std::vector<StateAttackMulSetController> attackMulSets;
    std::vector<StateHitFallDamageController> hitFallDamages;
    std::vector<StateHitFallVelController> hitFallVels;
    std::vector<StateHitFallSetController> hitFallSets;
    std::vector<StateAngleController> angleControllers;
    std::vector<StateAngleDrawController> angleDraws;
    std::vector<StateOffsetController> offsets;
    std::vector<StateForceFeedbackController> forceFeedbacks;
    std::vector<StateGameMakeAnimController> gameMakeAnims;
    std::vector<StateClipboardController> clipboards;
    std::vector<StateVictoryQuoteController> victoryQuotes;
    std::vector<StateRemapPalController> remapPals;
    std::vector<StateTransController> transControllers;
    std::vector<StateAfterImageController> afterImages;
    std::vector<StateAfterImageTimeController> afterImageTimes;
    std::vector<StateProjectileController> projectiles;
    std::vector<StateMakeDustController> makeDusts;
    std::vector<StateVelocityController> velocityControllers;
    std::vector<StatePosSetController> posSets;
    std::vector<StateTypeSetController> stateTypeSets;
    std::vector<StateScreenBoundController> screenBounds;
    std::vector<StateWidthController> widths;
    std::vector<StatePlayerPushController> playerPushes;
    std::vector<StateSprPriorityController> sprPriorities;
    std::vector<StatePosFreezeController> posFreezes;
    std::vector<StateHitVelSetController> hitVelSets;
    std::vector<StateHitProtectionController> hitProtections;
    std::vector<StateHitOverrideController> hitOverrides;
    std::vector<StateTargetStateController> targetStates;
    std::vector<StateTargetBindController> targetBinds;
    std::vector<StateTargetDropController> targetDrops;
    std::vector<StateTargetFacingController> targetFacings;
    std::vector<StateTargetLifeAddController> targetLifeAdds;
    std::vector<StateTargetPowerAddController> targetPowerAdds;
    std::vector<StateTargetVelocityController> targetVelocities;
    std::vector<StateTurnController> turns;
    std::vector<StatePauseController> pauses;
    std::vector<StateExplodController> explods;
    std::vector<StateModifyExplodController> modifyExplods;
    std::vector<StateRemoveExplodController> removeExplods;
    std::vector<StateEnvShakeController> envShakes;
    std::vector<StateFallEnvShakeController> fallEnvShakes;
    std::vector<StatePaletteEffectController> paletteEffects;
    std::vector<StateEnvColorController> envColors;
    std::vector<StateAssertSpecialController> assertSpecials;
    std::vector<StateChangeStateController> changeStates;
};
