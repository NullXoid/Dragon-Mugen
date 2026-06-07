#include "dragon/App.h"
#include "dragon/FightData.h"
#include "dragon/MugenData.h"
#include "dragon/MugenText.h"
#include "dragon/Sff.h"
#include "dragon/Snd.h"
#include "ArenaConfig.h"
#include "ArenaSetupOverlay.h"
#include "AppTypes.h"
#include "FightDisplayState.h"
#include "FightHudOverlay.h"
#include "FightMessageState.h"
#include "FightPresentationView.h"
#include "FightResultOverlay.h"
#include "FrontendMenu.h"
#include "FrontendState.h"
#include "Input.h"
#include "CharacterSelectOverlay.h"
#include "MainMenuOverlay.h"
#include "OptionsMenuOverlay.h"
#include "PauseMenuOverlay.h"
#include "SelectionState.h"
#include "StageSelectOverlay.h"
#include "TrainingState.h"
#include "TrainingCommandView.h"
#include "TrainingCommandOverlay.h"
#include "TrainingDebugView.h"
#include "TrainingDebugOverlay.h"
#include "TrainingOptionsBehavior.h"
#include "TrainingOptionsOverlay.h"
#include "UiRenderContext.h"
#include "UiRenderPrimitives.h"
#include "UiSpriteView.h"
#include "VsScreenOverlay.h"
#include "VerificationScenario.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dragon {
namespace {

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
    int animTick = 0;
    float x = 0.0f;
    float y = 0.0f;
    int facing = 1;
    int ageTicks = 0;
};

struct ActiveAfterImageEffect {
    bool active = false;
    int ticksLeft = 0;
    int length = 4;
    int timeGap = 1;
    int frameGap = 1;
    int captureCountdown = 0;
    bool additive = true;
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
    int p2StateNo = -1;
    bool hasP2Facing = false;
    int p2Facing = 0;
    EnvShakeSpec envShake;
    EnvShakeSpec fallEnvShake;
    PaletteEffectSpec palFx;
};

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
    bool requiresMoveContact = false;
    std::vector<AnimElemTimeCondition> animElemTimeConditions;
};

struct StateHelperController {
    int id = 0;
    StateControllerTrigger trigger;
    int helperId = 0;
    int stateNo = 0;
    float x = 0.0f;
    float y = 0.0f;
    std::string postype = "p1";
    int sprPriority = 0;
    int facing = 0;
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
    int length = 4;
    int timeGap = 1;
    int frameGap = 1;
    bool additive = true;
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
    bool hasCtrl = false;
    bool ctrl = false;
};

struct StateDefinition {
    int stateNo = 0;
    char stateType = 'S';
    char moveType = 'I';
    char physics = 'S';
    int anim = 0;
    bool hasAnim = false;
    bool ctrl = true;
    bool hasVelset = false;
    float velsetX = 0.0f;
    float velsetY = 0.0f;
    bool hasAnimEndChangeState = false;
    int animEndChangeState = 0;
    std::string animEndChangeStateExpression;
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

struct CommandStateEntry {
    std::string label;
    std::vector<std::string> requiredCommands;
    std::vector<std::string> forbiddenCommands;
    std::vector<std::vector<std::string>> commandOptionGroups;
    int targetState = 0;
    std::string targetStateExpression;
    char requiredStateType = 0;
    std::vector<char> forbiddenStateTypes;
    struct IntCondition {
        CommandConditionSubject subject = CommandConditionSubject::StateNo;
        CompareOp op = CompareOp::Equal;
        int value = 0;
    };
    struct IntRangeCondition {
        CommandConditionSubject subject = CommandConditionSubject::StateNo;
        CompareOp op = CompareOp::Equal;
        int minValue = 0;
        int maxValue = 0;
    };
    std::vector<IntCondition> intConditions;
    std::vector<IntRangeCondition> intRangeConditions;
    std::vector<MugenExpressionCondition> expressionConditions;
    std::vector<std::string> booleanExpressions;
    bool requiresCtrl = false;
    bool requiresMoveContact = false;
};

struct FightInputOverride {
    const FighterInputState* p1 = nullptr;
    const FighterInputState* p2 = nullptr;
};

const FightInputOverride* gFightInputOverride = nullptr;

struct CommandAtom {
    std::string symbol;
    bool hold = false;
    bool broadDirection = false;
    bool release = false;
};

struct CommandStep {
    std::vector<CommandAtom> atoms;
};

struct CommandDefinition {
    std::string name;
    std::vector<CommandStep> steps;
    int maxTime = 15;
    int bufferTime = 1;
};

struct AnimationFrame {
    TextureSprite sprite;
    int offsetX = 0;
    int offsetY = 0;
    int duration = 1;
    bool flipX = false;
    bool flipY = false;
    bool additive = false;
    std::vector<CollisionBox> clsn1;
    std::vector<CollisionBox> clsn2;
};

struct AnimationClip {
    int action = 0;
    int loopStartTick = 0;
    int loopTicks = 1;
    bool hasLoopStart = false;
    std::vector<AnimationFrame> frames;
};

struct StageBackgroundElement {
    TextureSprite sprite;
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 1.0f;
    float deltaY = 1.0f;
    bool tileX = false;
    bool tileY = false;
    int layerNo = 0;
};

struct SystemScreenAssets {
    TextureSprite titleLogo;
    TextureSprite titleTop;
    TextureSprite titleFloor;
    TextureSprite titleShade;
    TextureSprite selectBackdrop;
    TextureSprite selectShadow;
    TextureSprite selectTitleA;
    TextureSprite selectTitleB;
    TextureSprite selectTitleC;
    TextureSprite selectCell;
    TextureSprite selectP1Cursor;
    TextureSprite selectP1Done;
};

struct RuntimeEffect {
    int id = -1;
    int ownerIndex = -1;
    int action = 0;
    int animTick = 0;
    int ageTicks = 0;
    float x = 0.0f;
    float y = 0.0f;
    float bindOffsetX = 0.0f;
    float bindOffsetY = 0.0f;
    int bindTicks = 0;
    int removeTime = -2;
    bool fromFightFx = true;
    int sprPriority = 0;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct DecodedSoundSample {
    int group = 0;
    int index = 0;
    std::vector<float> audio;
};

struct ActiveSoundVoice {
    const DecodedSoundSample* sample = nullptr;
    int group = -1;
    int index = -1;
    int channel = -1;
    int frameOffset = 0;
    int startedFrame = 0;
    float gain = 1.0f;
    bool loop = false;
};

struct AudioState {
    SDL_AudioSpec playbackSpec{ SDL_AUDIO_F32, 2, 44100 };
    SDL_AudioStream* stream = nullptr;
    bool subsystemInitialized = false;
    std::vector<DecodedSoundSample> characterSamples;
    std::vector<DecodedSoundSample> systemSamples;
    std::vector<DecodedSoundSample> commonSamples;
    std::vector<DecodedSoundSample> fightSamples;
    std::vector<ActiveSoundVoice> activeVoices;
    std::vector<float> mixBuffer;
    int menuCursorMoveSoundGroup = 100;
    int menuCursorMoveSoundIndex = 0;
    int menuCursorDoneSoundGroup = 100;
    int menuCursorDoneSoundIndex = 1;
    int menuCancelSoundGroup = 100;
    int menuCancelSoundIndex = 2;
};

struct ParsedSoundValue {
    int group = -1;
    int index = -1;
    bool forceCommon = false;
    std::string groupExpression;
    std::string indexExpression;
};

struct FighterState {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    int facing = 1;
    int stateNo = 0;
    int stateTime = 0;
    int action = 0;
    int animTick = 0;
    char stateType = 'S';
    char moveType = 'I';
    char physics = 'S';
    bool ctrl = true;
    int hitPauseTicks = 0;
    int hitStunTicks = 0;
    int hitSlideTicks = 0;
    float hitVelocityX = 0.0f;
    float hitVelocityY = 0.0f;
    int getHitAnimType = 0;
    int getHitGroundType = 0;
    int getHitAirType = 0;
    int getHitSlideTime = 0;
    int getHitHitTime = 0;
    int getHitCtrlTime = 0;
    int getHitHitCount = 0;
    int hitRecoverAnim = 5005;
    bool hitFall = false;
    bool hitFallTrip = false;
    bool hitCrouch = false;
    bool hitAirborne = false;
    bool hitFallRecover = true;
    int hitFallRecoverTime = 0;
    int hitFallDamage = 0;
    float hitFallYAccel = 0.0f;
    int hitFallAirAction = 5050;
    float hitFallBounceXVelocity = 0.0f;
    float hitFallBounceYVelocity = -4.5f;
    EnvShakeSpec hitFallEnvShake;
    bool hitFallEnvShakePlayed = false;
    ActivePaletteEffect paletteEffect;
    ActiveTransEffect transEffect;
    ActiveAfterImageEffect afterImage;
    float drawAngle = 0.0f;
    bool angleDrawActive = false;
    float displayOffsetX = 0.0f;
    float displayOffsetY = 0.0f;
    std::string debugClipboard;
    int victoryQuote = -1;
    int paletteNo = 1;
    std::vector<PaletteRemap> paletteRemaps;
    int life = 1000;
    int power = 0;
    int hitCount = 0;
    float defenceMultiplier = 1.0f;
    float attackMultiplier = 1.0f;
    int attackDistanceOverride = -1;
    int projectileHitId = -1;
    int projectileHitTicks = 0;
    int projectileContactId = -1;
    int projectileContactTicks = 0;
    int projectileGuardedId = -1;
    int projectileGuardedTicks = 0;
    std::vector<int> appliedHitDefIds;
    std::vector<int> firedStateSoundControllerIds;
    std::vector<int> firedStateCtrlControllerIds;
    std::vector<int> firedStatePosAddControllerIds;
    std::vector<int> firedStateChangeAnimControllerIds;
    std::vector<int> firedStateRuntimeControllerIds;
    std::vector<CommandInputFrame> inputHistory;
    bool moveContact = false;
    bool moveHit = false;
    bool moveGuarded = false;
    bool guarding = false;
    bool crouchGuard = false;
    bool onGround = true;
    bool helper = false;
    bool destroyRequested = false;
    int ownerIndex = -1;
    int helperId = 0;
    int jumpBaseAction = 0;
    bool jumpPeakActionApplied = false;
    bool screenBound = true;
    bool screenBoundMoveCameraX = false;
    bool screenBoundMoveCameraY = false;
    bool playerPush = true;
    bool posFreezeX = false;
    bool posFreezeY = false;
    bool customHitState = false;
    int prevStateNo = 0;
    int sprPriority = 0;
    int rootBindTicks = 0;
    float rootBindOffsetX = 0.0f;
    float rootBindOffsetY = 0.0f;
    int rootBindFacing = 0;
    std::vector<std::string> assertSpecialFlags;
    std::array<int, 60> vars{};
    std::array<int, 60> sysVars{};
    std::array<float, 40> fvars{};
    std::array<float, 40> sysFvars{};
    float edgeWidthFront = -1.0f;
    float edgeWidthBack = -1.0f;
    float playerWidthFront = -1.0f;
    float playerWidthBack = -1.0f;
    int notHitByTicks = 0;
    std::string notHitByValue;
    int hitByTicks = 0;
    std::string hitByValue;
    int targetIndex = -1;
    int targetHitId = -1;
    int targetTicks = 0;
    int boundByIndex = -1;
    int bindTicks = 0;
};

struct AppState {
    std::filesystem::path gameRoot;
    LoadedContentSummary content;
    FrontendState frontend;
    SelectionState selection;
    TrainingState training;
    FightMessageState messages;
    FightDisplayState display;
    bool running = true;
    MainSettings mainSettings;
    FightRoundSettings fightRoundSettings;
    MatchPhase matchPhase = MatchPhase::Fight;
    RoundEndReason roundEndReason = RoundEndReason::None;
    int matchTimerTicks = 99 * 60;
    int matchPhaseTicks = 0;
    int roundWinner = 0;
    std::array<int, 2> roundWins{ 0, 0 };
    int currentRound = 1;
    bool matchComplete = false;
    bool roundScoreApplied = false;
    bool roundPoseApplied = false;
    bool fightSessionPrepared = false;
    bool fightSessionLoadFailed = false;
    int globalPauseTicks = 0;
    int globalPauseOwnerIndex = -1;
    int globalPauseOwnerMoveTicks = 0;
    bool globalPauseIsSuper = false;
    ActivePaletteEffect backgroundPaletteEffect;
    EnvColorEffect envColor;
    double accumulator = 0.0;
    int frame = 0;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    std::vector<FighterState> fighters = std::vector<FighterState>(2);
    std::vector<FighterState> helpers;
    std::vector<RuntimeProjectile> projectiles;
    ArenaConfig arenaConfig;
    CharacterConstants characterConstants;
    std::vector<HitDefinition> hitDefs;
    std::vector<StateDefinition> stateDefs;
    std::vector<CommandStateEntry> commandEntries;
    std::vector<CommandDefinition> commandDefinitions;
    std::vector<AnimationClip> characterClips;
    std::vector<AnimationClip> opponentCharacterClips;
    std::vector<std::vector<AnimationClip>> arenaFighterClips;
    std::vector<AnimationClip> fightFxClips;
    std::vector<RuntimeEffect> runtimeEffects;
    std::vector<std::string> victoryQuotes;
    TextureSprite characterLargePortrait;
    std::vector<TextureSprite> characterIconSprites;
    std::vector<TextureSprite> characterFaceSprites;
    SystemScreenAssets systemScreens;
    std::vector<StageBackgroundElement> stageBackground;
    AudioState audio;
    std::vector<GamepadDevice> gamepads;
};

int logicalWidth(const AppState& state) {
    return std::clamp(state.mainSettings.canvasWidth, kClassicLogicalWidth, kExtraWideLogicalWidth);
}

float logicalWidthF(const AppState& state) {
    return static_cast<float>(logicalWidth(state));
}

float screenCenterX(const AppState& state) {
    return logicalWidthF(state) * 0.5f;
}

float motifOriginX(const AppState& state) {
    return (logicalWidthF(state) - static_cast<float>(kClassicLogicalWidth)) * 0.5f;
}

float uiScale(const AppState& state) {
    return uiScaleFromPercent(state.mainSettings.uiScalePercent);
}

UiRenderContext uiRenderContext(SDL_Renderer* renderer, const AppState& state) {
    return UiRenderContext{
        renderer,
        logicalWidth(state),
        kLogicalHeight,
        uiScale(state),
    };
}

void clearComboCounters(AppState& state);
void startEnvShake(AppState& state, const EnvShakeSpec& shake);
void startPaletteEffect(ActivePaletteEffect& active, const PaletteEffectSpec& effect);
void startEnvColor(AppState& state, const StateEnvColorController& envColor);

std::string unquote(std::string value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

bool startsWithNoCase(std::string_view value, std::string_view prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); ++i) {
        const char a = static_cast<char>(SDL_tolower(static_cast<unsigned char>(value[i])));
        const char b = static_cast<char>(SDL_tolower(static_cast<unsigned char>(prefix[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    return lhs.size() == rhs.size() && startsWithNoCase(lhs, rhs);
}

std::string lowercaseCopy(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        out.push_back(static_cast<char>(SDL_tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

std::string uppercaseCopy(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        out.push_back(static_cast<char>(SDL_toupper(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool isIdentifierChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

int countSectionsWithPrefix(const MugenDocument& doc, std::string_view prefix) {
    int count = 0;
    for (const auto& section : doc.sections) {
        if (startsWithNoCase(section.name, prefix)) {
            ++count;
        }
    }
    return count;
}

std::vector<std::string> splitCsv(const std::string& line);
int parseIntValue(const std::string& value, int fallback);
size_t findNoCase(std::string_view value, std::string_view needle, size_t start);

std::filesystem::path resolveContentPath(const std::filesystem::path& base, std::string value) {
    value = unquote(trim(value));
    if (value.empty()) {
        return {};
    }
    std::filesystem::path path(value);
    if (path.is_absolute()) {
        return path;
    }
    return base / path;
}

bool isMatchMode(const AppState& state) {
    return isMatchMode(state.frontend.pendingMode);
}

OpponentType activeOpponentType(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Training && state.training.options.p2Controlled) {
        return OpponentType::LocalP2;
    }
    return state.selection.sessionSlots.opponentType;
}

bool usesLocalP2Controls(const AppState& state) {
    return activeOpponentType(state) == OpponentType::LocalP2;
}

#include "ArenaModeState.h"

std::string opponentDisplayName(const AppState& state) {
    if (isArenaMode(state)) {
        return std::to_string(arenaCpuCount(state)) + " CPU";
    }
    if (const CharacterSlot* character = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
        return character->displayName;
    }
    switch (activeOpponentType(state)) {
    case OpponentType::Dummy:
        return "Dummy";
    case OpponentType::Cpu:
        return "CPU";
    case OpponentType::LocalP2:
    default:
        return "Player 2";
    }
}

void drawSpriteTopLeft(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    float scale = 1.0f,
    SDL_FlipMode flip = SDL_FLIP_NONE) {
    if (!sprite.texture) {
        return;
    }
    SDL_FRect dst{
        x,
        y,
        static_cast<float>(sprite.width) * scale,
        static_cast<float>(sprite.height) * scale,
    };
    SDL_RenderTextureRotated(renderer, sprite.texture, nullptr, &dst, 0.0, nullptr, flip);
}

void drawSpriteAtAxis(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    float scale = 1.0f,
    SDL_FlipMode flip = SDL_FLIP_NONE) {
    drawSpriteTopLeft(
        renderer,
        sprite,
        x - static_cast<float>(sprite.axisX) * scale,
        y - static_cast<float>(sprite.axisY) * scale,
        scale,
        flip);
}

void drawTiledSprite(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    int repeatX,
    int repeatY) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }
    for (int ty = 0; ty < repeatY; ++ty) {
        for (int tx = 0; tx < repeatX; ++tx) {
            drawSpriteTopLeft(
                renderer,
                sprite,
                x + static_cast<float>(tx * sprite.width),
                y + static_cast<float>(ty * sprite.height));
        }
    }
}

void drawTiledSpriteCoverX(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    int logicalWidth,
    int repeatY) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }
    while (x > 0.0f) {
        x -= static_cast<float>(sprite.width);
    }
    const int repeatX = (logicalWidth / sprite.width) + 3;
    drawTiledSprite(renderer, sprite, x, y, repeatX, repeatY);
}

void drawParallaxFloorSprite(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    float topWidth,
    float bottomWidth,
    int logicalWidth,
    int frame) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }

    const float sourceWidth = static_cast<float>(sprite.width);
    const float sourceHeight = static_cast<float>(sprite.height);
    const float scroll = std::fmod(static_cast<float>(frame), sourceWidth);
    for (int row = 0; row < sprite.height; ++row) {
        const float t = sourceHeight <= 1.0f ? 0.0f : static_cast<float>(row) / (sourceHeight - 1.0f);
        const float rowWidth = topWidth + (bottomWidth - topWidth) * t;
        const float rowScroll = std::fmod(scroll * (rowWidth / sourceWidth), rowWidth);
        float drawX = x - rowScroll;
        while (drawX > 0.0f) {
            drawX -= rowWidth;
        }

        SDL_FRect src{ 0.0f, static_cast<float>(row), sourceWidth, 1.0f };
        for (float x = drawX; x < static_cast<float>(logicalWidth); x += rowWidth) {
            SDL_FRect dst{ x, y + static_cast<float>(row), rowWidth, 1.0f };
            SDL_RenderTexture(renderer, sprite.texture, &src, &dst);
        }
    }
}

void drawSpriteTopLeftWithBlend(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    SDL_BlendMode blendMode,
    Uint8 alpha) {
    if (!sprite.texture) {
        return;
    }

    SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
    Uint8 previousAlpha = 255;
    SDL_GetTextureBlendMode(sprite.texture, &previousBlend);
    SDL_GetTextureAlphaMod(sprite.texture, &previousAlpha);
    SDL_SetTextureBlendMode(sprite.texture, blendMode);
    SDL_SetTextureAlphaMod(sprite.texture, alpha);
    drawSpriteTopLeft(renderer, sprite, x, y);
    SDL_SetTextureBlendMode(sprite.texture, previousBlend);
    SDL_SetTextureAlphaMod(sprite.texture, previousAlpha);
}

const TextureSprite* spriteAt(const std::vector<TextureSprite>& sprites, int index) {
    if (index < 0 || index >= static_cast<int>(sprites.size())) {
        return nullptr;
    }
    const auto& sprite = sprites[static_cast<size_t>(index)];
    return sprite.texture ? &sprite : nullptr;
}

UiSpriteView uiSpriteView(const TextureSprite* sprite) {
    if (!sprite || !sprite->texture) {
        return {};
    }
    return UiSpriteView{
        sprite->texture,
        sprite->width,
        sprite->height,
        sprite->axisX,
        sprite->axisY,
    };
}

SDL_Texture* createTexture(SDL_Renderer* renderer, const DecodedSprite& sprite) {
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        sprite.width,
        sprite.height);
    if (!texture) {
        return nullptr;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, nullptr, sprite.rgba.data(), sprite.width * 4);
    return texture;
}

TextureSprite makeTextureSprite(SDL_Renderer* renderer, const DecodedSprite& sprite) {
    TextureSprite out;
    out.texture = createTexture(renderer, sprite);
    out.width = sprite.width;
    out.height = sprite.height;
    out.axisX = sprite.axisX;
    out.axisY = sprite.axisY;
    return out;
}

struct AirElement {
    int group = 0;
    int image = 0;
    int offsetX = 0;
    int offsetY = 0;
    int duration = 1;
    bool flipX = false;
    bool flipY = false;
    bool additive = false;
    std::vector<CollisionBox> clsn1;
    std::vector<CollisionBox> clsn2;
};

struct AirActionData {
    std::vector<AirElement> elements;
    size_t loopStart = 0;
    bool hasLoopStart = false;
};

std::optional<std::pair<float, float>> parsePair(const std::string& value) {
    const auto comma = value.find(',');
    if (comma == std::string::npos) {
        return std::nullopt;
    }
    try {
        return std::pair<float, float>{
            std::stof(trim(std::string_view(value).substr(0, comma))),
            std::stof(trim(std::string_view(value).substr(comma + 1))),
        };
    } catch (...) {
        return std::nullopt;
    }
}

float parseFloatValue(const std::string& value, float fallback = 0.0f) {
    try {
        return std::stof(trim(value));
    } catch (...) {
        return fallback;
    }
}

std::optional<float> parsePlainFloatValue(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    size_t consumed = 0;
    try {
        const float parsed = std::stof(trimmed, &consumed);
        if (consumed == trimmed.size()) {
            return parsed;
        }
    } catch (...) {
    }
    return std::nullopt;
}

std::vector<std::string> splitCsv(const std::string& line);

int parseIntValue(const std::string& value, int fallback = 0) {
    return static_cast<int>(parseFloatValue(value, static_cast<float>(fallback)));
}

std::optional<int> parsePlainIntValue(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    size_t consumed = 0;
    try {
        const int parsed = std::stoi(trimmed, &consumed, 10);
        if (consumed == trimmed.size()) {
            return parsed;
        }
    } catch (...) {
    }
    return std::nullopt;
}

std::pair<float, float> parseFloatPairValue(const std::string& value, float fallbackX = 0.0f, float fallbackY = 0.0f) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackX, fallbackY };
    }
    const float x = parseFloatValue(parts[0], fallbackX);
    const float y = parts.size() >= 2 ? parseFloatValue(parts[1], fallbackY) : fallbackY;
    return { x, y };
}

std::pair<int, int> parseIntPairValue(const std::string& value, int fallbackX = 0, int fallbackY = 0) {
    const auto pair = parseFloatPairValue(value, static_cast<float>(fallbackX), static_cast<float>(fallbackY));
    return { static_cast<int>(pair.first), static_cast<int>(pair.second) };
}

std::pair<std::string, std::string> parseExpressionPairValue(
    const std::string& value,
    std::string fallbackX = {},
    std::string fallbackY = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { std::move(fallbackX), std::move(fallbackY) };
    }
    std::string x = trim(parts[0]);
    std::string y = parts.size() >= 2 ? trim(parts[1]) : fallbackY;
    if (x.empty()) {
        x = std::move(fallbackX);
    }
    return { std::move(x), std::move(y) };
}

std::optional<ParsedSoundValue> parseSoundValue(const std::string& value) {
    const auto parts = splitCsv(value);
    if (parts.size() < 2) {
        return std::nullopt;
    }

    std::string groupPart = trim(parts[0]);
    ParsedSoundValue sound;
    if (!groupPart.empty() && (groupPart.front() == 'F' || groupPart.front() == 'f')) {
        sound.forceCommon = true;
        groupPart = trim(std::string_view(groupPart).substr(1));
    } else if (!groupPart.empty() && (groupPart.front() == 'S' || groupPart.front() == 's')) {
        groupPart = trim(std::string_view(groupPart).substr(1));
    }
    sound.groupExpression = groupPart;
    sound.indexExpression = trim(parts[1]);

    const auto group = parsePlainIntValue(groupPart);
    const auto index = parsePlainIntValue(parts[1]);
    if (!group || !index) {
        if (!sound.groupExpression.empty() && !sound.indexExpression.empty()) {
            return sound;
        }
        return std::nullopt;
    }

    sound.group = *group;
    sound.index = *index;
    return sound;
}

float mugenVolumeToGain(const std::string& value) {
    const int volume = parseIntValue(value, 0);
    return std::clamp(std::pow(10.0f, static_cast<float>(volume) / 100.0f), 0.0f, 4.0f);
}

std::optional<float> lookupCharacterConstant(const CharacterConstants& constants, std::string_view name) {
    const std::string key = lowercaseCopy(trim(name));
    if (key == "velocity.walk.fwd.x") {
        return constants.velocityWalkFwdX;
    }
    if (key == "velocity.walk.back.x") {
        return constants.velocityWalkBackX;
    }
    if (key == "velocity.run.fwd.x") {
        return constants.velocityRunFwdX;
    }
    if (key == "velocity.run.fwd.y") {
        return constants.velocityRunFwdY;
    }
    if (key == "velocity.run.back.x") {
        return constants.velocityRunBackX;
    }
    if (key == "velocity.run.back.y") {
        return constants.velocityRunBackY;
    }
    if (key == "movement.yaccel") {
        return constants.movementYAccel;
    }
    if (key == "data.power") {
        return static_cast<float>(constants.maxPower);
    }
    if (key == "size.ground.back") {
        return constants.size.groundBack;
    }
    if (key == "size.ground.front") {
        return constants.size.groundFront;
    }
    if (key == "size.air.back") {
        return constants.size.airBack;
    }
    if (key == "size.air.front") {
        return constants.size.airFront;
    }
    if (key == "size.height") {
        return constants.size.height;
    }
    return std::nullopt;
}

std::optional<float> parseControllerFloatValue(const std::string& value, const CharacterConstants& constants) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    if (const auto plain = parsePlainFloatValue(trimmed)) {
        return plain;
    }

    const std::string lowered = lowercaseCopy(trimmed);
    if (startsWithNoCase(lowered, "const(") && trimmed.back() == ')') {
        const std::string key = trim(std::string_view(trimmed).substr(6, trimmed.size() - 7));
        return lookupCharacterConstant(constants, key);
    }
    return std::nullopt;
}

std::optional<std::pair<float, float>> parseControllerFloatPairValue(
    const std::string& value,
    const CharacterConstants& constants,
    float fallbackY = 0.0f) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return std::nullopt;
    }

    const auto x = parseControllerFloatValue(parts[0], constants);
    if (!x) {
        return std::nullopt;
    }
    const auto y = parts.size() >= 2 ? parseControllerFloatValue(parts[1], constants) : std::optional<float>{ fallbackY };
    if (!y) {
        return std::nullopt;
    }
    return std::pair<float, float>{ *x, *y };
}

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= line.size()) {
        const auto comma = line.find(',', start);
        const auto end = comma == std::string::npos ? line.size() : comma;
        parts.push_back(trim(std::string_view(line).substr(start, end - start)));
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return parts;
}

bool hasFlagNoCase(std::string_view value, char flag) {
    const char wanted = static_cast<char>(SDL_tolower(static_cast<unsigned char>(flag)));
    for (const char ch : value) {
        const char current = static_cast<char>(SDL_tolower(static_cast<unsigned char>(ch)));
        if (current == wanted) {
            return true;
        }
    }
    return false;
}

enum class CollisionKind {
    None,
    Clsn1,
    Clsn2,
};

struct CollisionDirective {
    CollisionKind kind = CollisionKind::None;
    bool isDefault = false;
};

std::optional<CollisionDirective> parseCollisionDirective(const std::string& line) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
        return std::nullopt;
    }

    const auto name = trim(std::string_view(line).substr(0, colon));
    if (startsWithNoCase(name, "Clsn1Default")) {
        return CollisionDirective{ CollisionKind::Clsn1, true };
    }
    if (startsWithNoCase(name, "Clsn2Default")) {
        return CollisionDirective{ CollisionKind::Clsn2, true };
    }
    if (startsWithNoCase(name, "Clsn1")) {
        return CollisionDirective{ CollisionKind::Clsn1, false };
    }
    if (startsWithNoCase(name, "Clsn2")) {
        return CollisionDirective{ CollisionKind::Clsn2, false };
    }
    return std::nullopt;
}

std::optional<CollisionBox> parseCollisionBox(const std::string& line) {
    if (!startsWithNoCase(line, "Clsn1[") && !startsWithNoCase(line, "Clsn2[")) {
        return std::nullopt;
    }

    const auto equals = line.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }

    const auto parts = splitCsv(trim(std::string_view(line).substr(equals + 1)));
    if (parts.size() < 4) {
        return std::nullopt;
    }

    try {
        const float x1 = std::stof(parts[0]);
        const float y1 = std::stof(parts[1]);
        const float x2 = std::stof(parts[2]);
        const float y2 = std::stof(parts[3]);
        return CollisionBox{
            std::min(x1, x2),
            std::min(y1, y2),
            std::max(x1, x2),
            std::max(y1, y2),
        };
    } catch (...) {
        return std::nullopt;
    }
}

AirActionData loadAirActionData(const MugenDocument& doc, int actionNo) {
    const auto wanted = std::string("Begin Action ") + std::to_string(actionNo);
    const MugenSection* action = nullptr;
    for (const auto& section : doc.sections) {
        if (startsWithNoCase(section.name, wanted)) {
            action = &section;
            break;
        }
    }
    if (!action) {
        return {};
    }

    AirActionData data;
    std::vector<CollisionBox> defaultClsn1;
    std::vector<CollisionBox> defaultClsn2;
    std::optional<std::vector<CollisionBox>> pendingClsn1;
    std::optional<std::vector<CollisionBox>> pendingClsn2;
    CollisionDirective activeCollision;

    for (const auto& line : action->body) {
        if (startsWithNoCase(line, "Loopstart")) {
            data.loopStart = data.elements.size();
            data.hasLoopStart = true;
            continue;
        }

        if (const auto directive = parseCollisionDirective(line)) {
            activeCollision = *directive;
            if (activeCollision.kind == CollisionKind::Clsn1) {
                if (activeCollision.isDefault) {
                    defaultClsn1.clear();
                } else {
                    pendingClsn1 = std::vector<CollisionBox>{};
                }
            } else if (activeCollision.kind == CollisionKind::Clsn2) {
                if (activeCollision.isDefault) {
                    defaultClsn2.clear();
                } else {
                    pendingClsn2 = std::vector<CollisionBox>{};
                }
            }
            continue;
        }

        if (const auto box = parseCollisionBox(line)) {
            if (activeCollision.kind == CollisionKind::Clsn1) {
                if (activeCollision.isDefault) {
                    defaultClsn1.push_back(*box);
                } else {
                    if (!pendingClsn1) {
                        pendingClsn1 = std::vector<CollisionBox>{};
                    }
                    pendingClsn1->push_back(*box);
                }
            } else if (activeCollision.kind == CollisionKind::Clsn2) {
                if (activeCollision.isDefault) {
                    defaultClsn2.push_back(*box);
                } else {
                    if (!pendingClsn2) {
                        pendingClsn2 = std::vector<CollisionBox>{};
                    }
                    pendingClsn2->push_back(*box);
                }
            }
            continue;
        }

        if (line.find('=') != std::string::npos || startsWithNoCase(line, "Clsn")) {
            continue;
        }

        const auto parts = splitCsv(line);
        if (parts.size() < 5) {
            continue;
        }

        try {
            AirElement element;
            element.group = std::stoi(parts[0]);
            element.image = std::stoi(parts[1]);
            element.offsetX = std::stoi(parts[2]);
            element.offsetY = std::stoi(parts[3]);
            element.duration = std::max(1, std::stoi(parts[4]));
            if (parts.size() >= 6) {
                element.flipX = hasFlagNoCase(parts[5], 'H');
                element.flipY = hasFlagNoCase(parts[5], 'V');
            }
            if (parts.size() >= 7) {
                element.additive = hasFlagNoCase(parts[6], 'A');
            }
            element.clsn1 = pendingClsn1 ? *pendingClsn1 : defaultClsn1;
            element.clsn2 = pendingClsn2 ? *pendingClsn2 : defaultClsn2;
            pendingClsn1.reset();
            pendingClsn2.reset();
            data.elements.push_back(element);
        } catch (...) {
            continue;
        }
    }
    data.loopStart = std::min(data.loopStart, data.elements.size());
    return data;
}

AnimationClip loadSffClip(
    SDL_Renderer* renderer,
    const SffArchive& sff,
    const MugenDocument& air,
    int actionNo,
    DecodeOptions options) {
    AnimationClip clip;
    clip.action = actionNo;
    clip.loopTicks = 0;

    const auto action = loadAirActionData(air, actionNo);
    clip.hasLoopStart = action.hasLoopStart;
    int tick = 0;
    for (size_t i = 0; i < action.elements.size(); ++i) {
        const auto& element = action.elements[i];
        if (i == action.loopStart) {
            clip.loopStartTick = tick;
        }
        if (element.group < 0 || element.image < 0) {
            tick += element.duration;
            continue;
        }
        const auto* sprite = findSprite(sff, element.group, element.image);
        if (!sprite) {
            tick += element.duration;
            continue;
        }
        DecodeOptions frameOptions = options;
        if (frameOptions.fallbackPalette && sprite->sharedPalette) {
            frameOptions.preferFallbackPalette = true;
            frameOptions.reverseFallbackPalette = true;
        } else {
            frameOptions.reverseFallbackPalette = false;
        }
        const auto decoded = decodeSffSprite(sff, *sprite, frameOptions);
        if (!decoded) {
            tick += element.duration;
            continue;
        }
        AnimationFrame frame;
        frame.sprite = makeTextureSprite(renderer, *decoded);
        frame.offsetX = element.offsetX;
        frame.offsetY = element.offsetY;
        frame.duration = element.duration;
        frame.flipX = element.flipX;
        frame.flipY = element.flipY;
        frame.additive = element.additive;
        frame.clsn1 = element.clsn1;
        frame.clsn2 = element.clsn2;
        clip.loopTicks += frame.duration;
        clip.frames.push_back(frame);
        tick += element.duration;
    }
    clip.loopTicks = std::max(1, clip.loopTicks);
    if (action.loopStart >= action.elements.size()) {
        clip.loopStartTick = 0;
    }
    return clip;
}

AnimationClip loadCharacterClip(
    SDL_Renderer* renderer,
    const SffArchive& sff,
    const Palette& palette,
    const MugenDocument& air,
    int actionNo) {
    DecodeOptions options;
    options.fallbackPalette = &palette;
    options.preferFallbackPalette = false;
    options.reverseFallbackPalette = true;
    options.transparentColorZero = true;
    return loadSffClip(renderer, sff, air, actionNo, options);
}

std::vector<int> collectAirActionNumbers(const MugenDocument& air);

std::vector<AnimationClip> loadCharacterClips(SDL_Renderer* renderer, const CharacterFiles& files) {
    const auto palette = loadActPalette(files.palette);
    const auto sff = loadSffArchive(files.sprite);
    const auto air = parseMugenTextFile(files.anim);
    const auto actions = collectAirActionNumbers(air);

    std::vector<AnimationClip> clips;
    clips.reserve(actions.size());
    for (const int action : actions) {
        auto clip = loadCharacterClip(renderer, sff, palette, air, action);
        if (!clip.frames.empty()) {
            clips.push_back(std::move(clip));
        }
    }
    return clips;
}

std::optional<int> parseAirActionNumber(std::string_view sectionName) {
    constexpr std::string_view prefix = "Begin Action ";
    if (!startsWithNoCase(sectionName, prefix)) {
        return std::nullopt;
    }
    try {
        return std::stoi(trim(sectionName.substr(prefix.size())));
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<int> collectAirActionNumbers(const MugenDocument& air) {
    std::vector<int> actions;
    for (const auto& section : air.sections) {
        if (const auto action = parseAirActionNumber(section.name)) {
            if (std::find(actions.begin(), actions.end(), *action) == actions.end()) {
                actions.push_back(*action);
            }
        }
    }
    std::sort(actions.begin(), actions.end());
    return actions;
}

std::vector<AnimationClip> loadFightFxClips(SDL_Renderer* renderer, const std::filesystem::path& gameRoot) {
    const auto dataRoot = gameRoot / "data";
    const auto sff = loadSffArchive(dataRoot / "fightfx.sff");
    const auto air = parseMugenTextFile(dataRoot / "fightfx.air");

    std::vector<AnimationClip> clips;
    const auto actions = collectAirActionNumbers(air);
    clips.reserve(actions.size());
    DecodeOptions options;
    options.transparentColorZero = true;
    for (const int action : actions) {
        auto clip = loadSffClip(renderer, sff, air, action, options);
        if (!clip.frames.empty()) {
            clips.push_back(std::move(clip));
        }
    }
    return clips;
}

#include "StateControllerParsing.h"

bool parseP2BodyDistXCondition(const std::string& clause, HitDefinition& hitDef) {
    const std::string condition = stripOuterParens(clause);
    size_t subjectEnd = 0;
    const auto subject = parseStateTriggerSubject(condition, subjectEnd);
    if (!subject || *subject != StateTriggerSubject::P2BodyDistX) {
        return false;
    }

    const std::string tail = trim(std::string_view(condition).substr(subjectEnd));
    const auto compare = findCompareOp(tail);
    if (!compare) {
        return false;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    const auto value = parsePlainFloatValue(trim(std::string_view(tail).substr(pos + opLength)));
    if (!value) {
        return false;
    }

    hitDef.hasP2BodyDistX = true;
    hitDef.p2BodyDistXOp = op;
    hitDef.p2BodyDistX = *value;
    return true;
}

std::vector<HitDefinition> loadHitDefinitions(const CharacterFiles& files) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<HitDefinition> hitDefs;
    int nextHitDefId = 1;
    int defaultSparkNo = 2;
    int defaultGuardSparkNo = 40;
    for (const auto& cns : documents) {
        if (const auto* data = findSection(cns, "Data")) {
            if (const auto* sparkNo = findProperty(*data, "sparkno")) {
                defaultSparkNo = parseIntValue(sparkNo->value, defaultSparkNo);
            }
            if (const auto* guardSparkNo = findProperty(*data, "guard.sparkno")) {
                defaultGuardSparkNo = parseIntValue(guardSparkNo->value, defaultGuardSparkNo);
            }
        }
    }

    for (const auto& cns : documents) {
        int currentState = -1;
        for (const auto& section : cns.sections) {
            if (const auto stateNo = parseStateNumber(section.name, "Statedef ")) {
                currentState = *stateNo;
                continue;
            }
            if (currentState < 0 || !startsWithNoCase(section.name, "State ")) {
                continue;
            }

            const auto* type = findProperty(section, "type");
            if (!type || !startsWithNoCase(trim(type->value), "HitDef")) {
                continue;
            }

            HitDefinition hitDef;
            hitDef.id = nextHitDefId++;
            hitDef.stateNo = currentState;
            hitDef.sparkNo = defaultSparkNo;
            hitDef.guardSparkNo = defaultGuardSparkNo;
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "trigger")) {
                    continue;
                }
                const auto trigger = trim(property.value);
                for (const auto& clause : splitAndClauses(trigger)) {
                    if (startsWithNoCase(clause, "Time")) {
                        if (const auto equals = clause.find('='); equals != std::string::npos) {
                            hitDef.triggerTime = parseIntValue(trim(std::string_view(clause).substr(equals + 1)), hitDef.triggerTime);
                        }
                    } else if (startsWithNoCase(clause, "AnimElem") && !startsWithNoCase(clause, "AnimElemTime")) {
                        if (const auto equals = clause.find('='); equals != std::string::npos) {
                            hitDef.triggerAnimElem = parseIntValue(trim(std::string_view(clause).substr(equals + 1)), hitDef.triggerAnimElem);
                        }
                    } else {
                        parseP2BodyDistXCondition(clause, hitDef);
                    }
                }
            }
            if (const auto* attr = findProperty(section, "attr")) {
                hitDef.attr = trim(attr->value);
            }
            if (const auto* id = findProperty(section, "id")) {
                hitDef.targetId = std::max(0, parseIntValue(id->value, hitDef.targetId));
            }
            if (const auto* animtype = findProperty(section, "animtype")) {
                hitDef.animtype = trim(animtype->value);
            }
            if (const auto* hitflag = findProperty(section, "hitflag")) {
                hitDef.hitflag = trim(hitflag->value);
            }
            if (const auto* guardflag = findProperty(section, "guardflag")) {
                hitDef.guardflag = trim(guardflag->value);
            }
            if (const auto* damage = findProperty(section, "damage")) {
                const auto values = parseIntPairValue(damage->value);
                hitDef.damage = values.first;
                hitDef.guardDamage = values.second;
                const auto expressions = parseExpressionPairValue(damage->value);
                hitDef.damageExpression = expressions.first;
                hitDef.guardDamageExpression = expressions.second;
            }
            if (const auto* guardDamage = findProperty(section, "guard.damage")) {
                hitDef.guardDamage = parseIntValue(guardDamage->value, hitDef.guardDamage);
                hitDef.guardDamageExpression = trim(guardDamage->value);
            }
            if (const auto* guardDistance = findProperty(section, "guard.dist")) {
                hitDef.guardDistance = parseIntValue(guardDistance->value, hitDef.guardDistance);
            }
            if (const auto* pausetime = findProperty(section, "pausetime")) {
                const auto values = parseIntPairValue(pausetime->value);
                hitDef.pausetimeP1 = values.first;
                hitDef.pausetimeP2 = values.second;
                const auto expressions = parseExpressionPairValue(pausetime->value);
                hitDef.pausetimeP1Expression = expressions.first;
                hitDef.pausetimeP2Expression = expressions.second;
            }
            if (const auto* sparkNo = findProperty(section, "sparkno")) {
                hitDef.sparkNo = parseIntValue(sparkNo->value, hitDef.sparkNo);
                hitDef.sparkNoExpression = trim(sparkNo->value);
            }
            if (const auto* guardSparkNo = findProperty(section, "guard.sparkno")) {
                hitDef.guardSparkNo = parseIntValue(guardSparkNo->value, hitDef.guardSparkNo);
                hitDef.guardSparkNoExpression = trim(guardSparkNo->value);
            }
            if (const auto* sparkxy = findProperty(section, "sparkxy")) {
                const auto values = parseFloatPairValue(sparkxy->value);
                hitDef.sparkX = values.first;
                hitDef.sparkY = values.second;
                const auto expressions = parseExpressionPairValue(sparkxy->value);
                hitDef.sparkXExpression = expressions.first;
                hitDef.sparkYExpression = expressions.second;
            }
            if (const auto* hitSound = findProperty(section, "hitsound")) {
                if (const auto values = parseSoundValue(hitSound->value)) {
                    hitDef.hitSoundGroup = values->group;
                    hitDef.hitSoundIndex = values->index;
                    hitDef.hitSoundForceCommon = values->forceCommon;
                    hitDef.hitSoundGroupExpression = values->groupExpression;
                    hitDef.hitSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* guardSound = findProperty(section, "guardsound")) {
                if (const auto values = parseSoundValue(guardSound->value)) {
                    hitDef.guardSoundGroup = values->group;
                    hitDef.guardSoundIndex = values->index;
                    hitDef.guardSoundForceCommon = values->forceCommon;
                    hitDef.guardSoundGroupExpression = values->groupExpression;
                    hitDef.guardSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* groundType = findProperty(section, "ground.type")) {
                hitDef.groundType = trim(groundType->value);
            }
            if (const auto* groundSlideTime = findProperty(section, "ground.slidetime")) {
                hitDef.groundSlideTime = parseIntValue(groundSlideTime->value, hitDef.groundSlideTime);
                hitDef.groundSlideTimeExpression = trim(groundSlideTime->value);
            }
            if (const auto* groundHitTime = findProperty(section, "ground.hittime")) {
                hitDef.groundHitTime = parseIntValue(groundHitTime->value, hitDef.groundHitTime);
                hitDef.groundHitTimeExpression = trim(groundHitTime->value);
            }
            if (const auto* groundVelocity = findProperty(section, "ground.velocity")) {
                const auto values = parseFloatPairValue(groundVelocity->value);
                hitDef.groundVelocityX = values.first;
                hitDef.groundVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(groundVelocity->value);
                hitDef.groundVelocityXExpression = expressions.first;
                hitDef.groundVelocityYExpression = expressions.second;
            }
            if (const auto* airVelocity = findProperty(section, "air.velocity")) {
                const auto values = parseFloatPairValue(airVelocity->value);
                hitDef.hasAirVelocity = true;
                hitDef.airVelocityX = values.first;
                hitDef.airVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(airVelocity->value);
                hitDef.airVelocityXExpression = expressions.first;
                hitDef.airVelocityYExpression = expressions.second;
            }
            if (const auto* airHitTime = findProperty(section, "air.hittime")) {
                hitDef.airHitTime = parseIntValue(airHitTime->value, hitDef.airHitTime);
                hitDef.airHitTimeExpression = trim(airHitTime->value);
            }
            if (const auto* fall = findProperty(section, "fall")) {
                hitDef.fall = parseIntValue(fall->value, 0) != 0;
                hitDef.fallExpression = trim(fall->value);
            }
            if (const auto* airFall = findProperty(section, "air.fall")) {
                hitDef.airFall = parseIntValue(airFall->value, 0) != 0;
                hitDef.airFallExpression = trim(airFall->value);
            }
            if (const auto* fallAnimtype = findProperty(section, "fall.animtype")) {
                hitDef.fallAnimtype = trim(fallAnimtype->value);
            }
            if (const auto* fallRecover = findProperty(section, "fall.recover")) {
                hitDef.fallRecover = parseIntValue(fallRecover->value, 1) != 0;
                hitDef.fallRecoverExpression = trim(fallRecover->value);
            }
            if (const auto* fallRecoverTime = findProperty(section, "fall.recovertime")) {
                hitDef.fallRecoverTime = parseIntValue(fallRecoverTime->value, hitDef.fallRecoverTime);
                hitDef.fallRecoverTimeExpression = trim(fallRecoverTime->value);
            }
            if (const auto* fallDamage = findProperty(section, "fall.damage")) {
                hitDef.fallDamage = parseIntValue(fallDamage->value, 0);
                hitDef.fallDamageExpression = trim(fallDamage->value);
            }
            if (const auto* fallXVelocity = findProperty(section, "fall.xvelocity")) {
                hitDef.hasFallXVelocity = true;
                hitDef.fallXVelocity = parseFloatValue(fallXVelocity->value, hitDef.fallXVelocity);
                hitDef.fallXVelocityExpression = trim(fallXVelocity->value);
            }
            if (const auto* fallYVelocity = findProperty(section, "fall.yvelocity")) {
                hitDef.hasFallYVelocity = true;
                hitDef.fallYVelocity = parseFloatValue(fallYVelocity->value, hitDef.fallYVelocity);
                hitDef.fallYVelocityExpression = trim(fallYVelocity->value);
            }
            if (const auto* yAccel = findProperty(section, "yaccel")) {
                hitDef.hasYAccel = true;
                hitDef.yAccel = parseFloatValue(yAccel->value, 0.0f);
                hitDef.yAccelExpression = trim(yAccel->value);
            }
            if (const auto* guardVelocity = findProperty(section, "guard.velocity")) {
                const auto values = parseFloatPairValue(guardVelocity->value);
                hitDef.hasGuardVelocity = true;
                hitDef.guardVelocityX = values.first;
                hitDef.guardVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(guardVelocity->value);
                hitDef.guardVelocityXExpression = expressions.first;
                hitDef.guardVelocityYExpression = expressions.second;
            } else {
                hitDef.guardVelocityX = hitDef.groundVelocityX;
                hitDef.guardVelocityY = hitDef.groundVelocityY;
            }
            if (const auto* p2StateNo = findProperty(section, "p2stateno")) {
                hitDef.p2StateNo = parseIntValue(p2StateNo->value, -1);
            }
            if (const auto* p2Facing = findProperty(section, "p2facing")) {
                hitDef.hasP2Facing = true;
                hitDef.p2Facing = parseIntValue(p2Facing->value, 0);
            }
            if (const auto* envShakeTime = findProperty(section, "envshake.time")) {
                hitDef.envShake.time = std::max(0, parseIntValue(envShakeTime->value, hitDef.envShake.time));
                hitDef.envShake.timeExpression = trim(envShakeTime->value);
                hitDef.envShake.enabled = hitDef.envShake.time > 0;
            }
            if (const auto* envShakeFrequency = findProperty(section, "envshake.freq")) {
                hitDef.envShake.frequency = std::max(1, parseIntValue(envShakeFrequency->value, hitDef.envShake.frequency));
                hitDef.envShake.frequencyExpression = trim(envShakeFrequency->value);
            }
            if (const auto* envShakeAmplitude = findProperty(section, "envshake.ampl")) {
                hitDef.envShake.amplitude = parseFloatValue(envShakeAmplitude->value, hitDef.envShake.amplitude);
                hitDef.envShake.amplitudeExpression = trim(envShakeAmplitude->value);
                hitDef.envShake.enabled = hitDef.envShake.time > 0 && std::abs(hitDef.envShake.amplitude) > 0.001f;
            }
            if (const auto* envShakePhase = findProperty(section, "envshake.phase")) {
                hitDef.envShake.phase = parseIntValue(envShakePhase->value, hitDef.envShake.phase);
                hitDef.envShake.phaseExpression = trim(envShakePhase->value);
            }
            if (const auto* fallEnvShakeTime = findProperty(section, "fall.envshake.time")) {
                hitDef.fallEnvShake.time = std::max(0, parseIntValue(fallEnvShakeTime->value, hitDef.fallEnvShake.time));
                hitDef.fallEnvShake.timeExpression = trim(fallEnvShakeTime->value);
                hitDef.fallEnvShake.enabled = hitDef.fallEnvShake.time > 0;
            }
            if (const auto* fallEnvShakeFrequency = findProperty(section, "fall.envshake.freq")) {
                hitDef.fallEnvShake.frequency = std::max(1, parseIntValue(fallEnvShakeFrequency->value, hitDef.fallEnvShake.frequency));
                hitDef.fallEnvShake.frequencyExpression = trim(fallEnvShakeFrequency->value);
            }
            if (const auto* fallEnvShakeAmplitude = findProperty(section, "fall.envshake.ampl")) {
                hitDef.fallEnvShake.amplitude = parseFloatValue(fallEnvShakeAmplitude->value, hitDef.fallEnvShake.amplitude);
                hitDef.fallEnvShake.amplitudeExpression = trim(fallEnvShakeAmplitude->value);
                hitDef.fallEnvShake.enabled = hitDef.fallEnvShake.time > 0 && std::abs(hitDef.fallEnvShake.amplitude) > 0.001f;
            }
            if (const auto* fallEnvShakePhase = findProperty(section, "fall.envshake.phase")) {
                hitDef.fallEnvShake.phase = parseIntValue(fallEnvShakePhase->value, hitDef.fallEnvShake.phase);
                hitDef.fallEnvShake.phaseExpression = trim(fallEnvShakePhase->value);
            }
            hitDef.palFx = parsePaletteEffectSpec(section, "palfx");
            hitDefs.push_back(std::move(hitDef));
        }
    }

    return hitDefs;
}

bool isSupportedStateType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'S' || value == 'C' || value == 'A' || value == 'L';
}

void addUniqueCommand(std::vector<std::string>& commands, const std::string& command) {
    if (std::find(commands.begin(), commands.end(), command) == commands.end()) {
        commands.push_back(command);
    }
}

size_t findNoCase(std::string_view value, std::string_view needle, size_t start = 0) {
    if (needle.empty() || start >= value.size()) {
        return std::string_view::npos;
    }
    const std::string loweredValue = lowercaseCopy(value);
    const std::string loweredNeedle = lowercaseCopy(needle);
    return loweredValue.find(loweredNeedle, start);
}

#include "CommandParsing.h"
#include "AudioRuntime.h"
#include "RuntimeLoading.h"

GamepadDevice* findGamepadDevice(AppState& state, SDL_JoystickID id) {
    auto it = std::find_if(state.gamepads.begin(), state.gamepads.end(), [id](const GamepadDevice& device) {
        return device.id == id;
    });
    return it == state.gamepads.end() ? nullptr : &*it;
}

const GamepadDevice* findGamepadDevice(const AppState& state, SDL_JoystickID id) {
    auto it = std::find_if(state.gamepads.begin(), state.gamepads.end(), [id](const GamepadDevice& device) {
        return device.id == id;
    });
    return it == state.gamepads.end() ? nullptr : &*it;
}

void openGamepadDevice(AppState& state, SDL_JoystickID id) {
    if (findGamepadDevice(state, id) || !SDL_IsGamepad(id)) {
        return;
    }

    SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
    if (!gamepad) {
        SDL_Log("Failed to open gamepad %u: %s", static_cast<unsigned int>(id), SDL_GetError());
        return;
    }

    const char* name = SDL_GetGamepadName(gamepad);
    GamepadDevice device;
    device.id = id;
    device.handle = gamepad;
    device.name = name && *name ? name : "Gamepad";
    device.type = SDL_GetGamepadType(gamepad);
    state.gamepads.push_back(std::move(device));
    SDL_Log("Gamepad connected: %s (%s)", state.gamepads.back().name.c_str(), gamepadFamilyName(state.gamepads.back().type).c_str());
}

void openExistingGamepads(AppState& state) {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (!ids) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        openGamepadDevice(state, ids[i]);
    }
    SDL_free(ids);
}

void refreshGamepadDevice(AppState& state, SDL_JoystickID id) {
    GamepadDevice* device = findGamepadDevice(state, id);
    if (!device || !device->handle) {
        return;
    }
    const char* name = SDL_GetGamepadName(device->handle);
    device->name = name && *name ? name : "Gamepad";
    device->type = SDL_GetGamepadType(device->handle);
}

void closeGamepadDevice(AppState& state, SDL_JoystickID id) {
    auto it = std::find_if(state.gamepads.begin(), state.gamepads.end(), [id](const GamepadDevice& device) {
        return device.id == id;
    });
    if (it == state.gamepads.end()) {
        return;
    }
    if (it->handle) {
        SDL_CloseGamepad(it->handle);
    }
    SDL_Log("Gamepad disconnected: %s", it->name.c_str());
    state.gamepads.erase(it);
}

void closeAllGamepads(AppState& state) {
    for (auto& device : state.gamepads) {
        if (device.handle) {
            SDL_CloseGamepad(device.handle);
            device.handle = nullptr;
        }
    }
    state.gamepads.clear();
}

const GamepadDevice* assignedGamepad(const AppState& state, int playerIndex) {
    const int assignment = playerIndex == 0
        ? state.mainSettings.p1GamepadAssignment
        : state.mainSettings.p2GamepadAssignment;
    if (assignment < 0 || state.gamepads.empty()) {
        return nullptr;
    }
    if (assignment > 0) {
        const int index = assignment - 1;
        return index >= 0 && index < static_cast<int>(state.gamepads.size())
            ? &state.gamepads[static_cast<size_t>(index)]
            : nullptr;
    }
    return playerIndex >= 0 && playerIndex < static_cast<int>(state.gamepads.size())
        ? &state.gamepads[static_cast<size_t>(playerIndex)]
        : nullptr;
}

std::string gamepadAssignmentText(const AppState& state, int playerIndex) {
    const int assignment = playerIndex == 0
        ? state.mainSettings.p1GamepadAssignment
        : state.mainSettings.p2GamepadAssignment;
    if (assignment < 0) {
        return "OFF";
    }
    const GamepadDevice* device = assignedGamepad(state, playerIndex);
    if (assignment == 0) {
        return device ? "AUTO " + gamepadFamilyName(device->type) : "AUTO NONE";
    }
    if (!device) {
        return "PAD " + std::to_string(assignment) + " MISSING";
    }
    return "PAD " + std::to_string(assignment) + " " + gamepadFamilyName(device->type);
}

GamepadPromptStyle effectiveGamepadPromptStyle(const AppState& state, int playerIndex) {
    if (state.mainSettings.gamepadPromptStyle != GamepadPromptStyle::Auto) {
        return state.mainSettings.gamepadPromptStyle;
    }
    const GamepadDevice* device = assignedGamepad(state, playerIndex);
    return device && isPlaystationGamepad(device->type) ? GamepadPromptStyle::Playstation : GamepadPromptStyle::Xbox;
}

std::string gamepadActionLayoutText(const AppState& state, int playerIndex) {
    return effectiveGamepadPromptStyle(state, playerIndex) == GamepadPromptStyle::Playstation
        ? "Sq/Tri/L1 Cross/Cir/R1"
        : "X/Y/LB A/B/RB";
}

std::string contentLine(const LoadedContentSummary& content) {
    std::ostringstream out;
    out << content.characterName << " / " << content.stageName;
    return out.str();
}

#include "UiRenderHelpers.h"

std::string mainSettingStatus(const AppState& state, int option) {
    switch (option) {
    case 0:
        return matchTimerSettingText(state.mainSettings);
    case 1:
        return canvasSizeSettingText(state.mainSettings);
    case 2:
        return uiScaleSettingText(state.mainSettings);
    case 3:
        return gamepadPromptStyleText(state.mainSettings.gamepadPromptStyle);
    case 4:
        return gamepadAssignmentText(state, 0);
    case 5:
        return gamepadAssignmentText(state, 1);
    default:
        return "";
    }
}

std::string mainSettingsPadSummary(const AppState& state) {
    if (state.gamepads.empty()) {
        return "PADS NONE";
    }
    return "PADS " + std::to_string(state.gamepads.size()) + "  " + compactSettingText(state.gamepads.front().name, 18);
}

std::vector<OptionsMenuRowView> buildOptionsMenuRows(const AppState& state) {
    std::vector<OptionsMenuRowView> rows;
    rows.reserve(kMainSettingsCount);
    for (int i = 0; i < kMainSettingsCount; ++i) {
        const bool adjustable = i < kMainSettingsCount - 1;
        rows.push_back(OptionsMenuRowView{
            std::string(mainSettingLabel(i)),
            adjustable ? compactSettingText(mainSettingStatus(state, i), 16) : "",
            i == state.mainSettings.selectedOption,
            adjustable,
        });
    }
    return rows;
}

void drawModeSelect(SDL_Renderer* renderer, const AppState& state) {
    drawTitleBackground(renderer, state);
    const UiRenderContext ui = uiRenderContext(renderer, state);
    const float centerX = screenCenterX(state);

    drawMainMenuTitleText(ui);
    drawSpriteAtAxis(renderer, state.systemScreens.titleLogo, centerX, 40);
    drawMainMenuOverlay(ui, MainMenuView{
        state.frontend.selectedMode,
        state.frontend.screenFrame,
        state.frontend.exitConfirmOpen });

    SDL_RenderPresent(renderer);
}

void drawMainSettings(SDL_Renderer* renderer, const AppState& state) {
    drawTitleBackground(renderer, state);

    std::vector<OptionsMenuRowView> rows = buildOptionsMenuRows(state);
    drawOptionsMenuOverlay(
        uiRenderContext(renderer, state),
        OptionsMenuView{
            rows,
            mainSettingsPadSummary(state),
        });

    SDL_RenderPresent(renderer);
}

std::string_view opponentSlotLabel(PendingMode mode) {
    return opponentTypeLabel(defaultOpponentTypeForMode(mode));
}

std::string_view opponentSlotLabel(const AppState& state) {
    return opponentTypeLabel(activeOpponentType(state));
}

void drawCharacterSelect(SDL_Renderer* renderer, const AppState& state) {
    drawSelectBackground(renderer, state);

    std::vector<CharacterCellView> cells;
    int selectedCell = 0;
    int p2SelectedCell = 0;
    std::string activePlayerLabel = "P1";
    std::string selectedName;
    std::string opponentName = std::string(opponentSlotLabel(state.frontend.pendingMode));
    std::string preferredStageLabel;
    UiSpriteView selectedPortrait;
    UiSpriteView opponentPortrait;

    if (!state.selection.characters.empty()) {
        const bool vsSelect = state.frontend.pendingMode == PendingMode::SingleFight;
        const int p1Selected = safeCharacterIndex(state.selection, state.selection.selectedCharacter);
        const int p2Selected = safeCharacterIndex(state.selection, state.selection.selectedP2Character);
        const int p1DisplayIndex = p1Selected >= 0 ? p1Selected : 0;
        const int p2DisplayIndex = p2Selected >= 0 ? p2Selected : p1DisplayIndex;
        const int page = p1DisplayIndex / kCharacterSelectPageSize;
        const int firstIndex = page * kCharacterSelectPageSize;
        const int lastIndex = std::min(
            firstIndex + kCharacterSelectPageSize,
            static_cast<int>(state.selection.characters.size()));

        cells.reserve(static_cast<size_t>(lastIndex - firstIndex));
        for (int i = firstIndex; i < lastIndex; ++i) {
            cells.push_back(CharacterCellView{
                uiSpriteView(spriteAt(state.characterIconSprites, i)),
                true,
            });
        }

        selectedCell = p1DisplayIndex - firstIndex;
        p2SelectedCell = p2DisplayIndex - firstIndex;
        const auto& p1Character = state.selection.characters[static_cast<size_t>(p1DisplayIndex)];
        selectedName = compactSettingText(p1Character.displayName, 15);
        preferredStageLabel = compactSettingText(characterPreferredStageName(state.selection, p1DisplayIndex), 22);
        selectedPortrait = uiSpriteView(spriteAt(state.characterFaceSprites, p1DisplayIndex));

        if (state.frontend.pendingMode == PendingMode::Arena) {
            activePlayerLabel = "ARENA";
            opponentName = "RANDOM CPU";
            preferredStageLabel = compactSettingText(selectedStageName(state.selection), 22);
        }

        if (vsSelect) {
            activePlayerLabel = "P1 / P2";
            const auto& p2Character = state.selection.characters[static_cast<size_t>(p2DisplayIndex)];
            opponentName = compactSettingText(p2Character.displayName, 15);
            opponentPortrait = uiSpriteView(spriteAt(state.characterFaceSprites, p2DisplayIndex));
        }
    }

    drawCharacterSelectOverlay(
        uiRenderContext(renderer, state),
        CharacterSelectView{
            cells,
            std::string(pendingModeTitle(state.frontend.pendingMode)),
            activePlayerLabel,
            selectedName,
            opponentName,
            preferredStageLabel,
            selectedPortrait,
            opponentPortrait,
            uiSpriteView(&state.systemScreens.selectCell),
            uiSpriteView(&state.systemScreens.selectP1Cursor),
            selectedCell,
            p2SelectedCell,
            kCharacterSelectColumns,
            state.frame,
            state.frontend.pendingMode == PendingMode::SingleFight,
            state.selection.p1CharacterConfirmed,
            state.selection.p2CharacterConfirmed,
        });
    SDL_RenderPresent(renderer);
}

const AnimationClip* findClip(const AppState& state, int action) {
    for (const auto& clip : state.characterClips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    for (const auto& clip : state.characterClips) {
        if (clip.action == 0) {
            return &clip;
        }
    }
    return state.characterClips.empty() ? nullptr : &state.characterClips.front();
}

const AnimationClip* findClipInSet(const std::vector<AnimationClip>& clips, int action) {
    for (const auto& clip : clips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    for (const auto& clip : clips) {
        if (clip.action == 0) {
            return &clip;
        }
    }
    return clips.empty() ? nullptr : &clips.front();
}

const AnimationClip* findClipForFighter(const AppState& state, size_t fighterIndex, int action) {
    if (state.frontend.pendingMode == PendingMode::Arena
        && fighterIndex < state.arenaFighterClips.size()
        && !state.arenaFighterClips[fighterIndex].empty()) {
        return findClipInSet(state.arenaFighterClips[fighterIndex], action);
    }
    if (fighterIndex == 1 && !state.opponentCharacterClips.empty()) {
        return findClipInSet(state.opponentCharacterClips, action);
    }
    return findClip(state, action);
}

const AnimationClip* findExactClip(const AppState& state, int action) {
    for (const auto& clip : state.characterClips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    return nullptr;
}

int ownedHelperCount(const AppState& state, const FighterState& fighter, std::optional<int> helperId = std::nullopt) {
    int ownerIndex = fighter.helper ? fighter.ownerIndex : -1;
    if (!fighter.helper) {
        const auto* first = state.fighters.data();
        const auto* current = &fighter;
        if (current >= first && current < first + state.fighters.size()) {
            ownerIndex = static_cast<int>(current - first);
        }
    }
    if (ownerIndex < 0) {
        return 0;
    }

    int count = 0;
    for (const auto& helper : state.helpers) {
        if (helper.destroyRequested || helper.ownerIndex != ownerIndex) {
            continue;
        }
        if (helperId && helper.helperId != *helperId) {
            continue;
        }
        ++count;
    }
    return count;
}

int ownedProjectileCount(const AppState& state, const FighterState& fighter, std::optional<int> projectileId = std::nullopt) {
    int ownerIndex = fighter.helper ? fighter.ownerIndex : -1;
    if (!fighter.helper) {
        const auto* first = state.fighters.data();
        const auto* current = &fighter;
        if (current >= first && current < first + state.fighters.size()) {
            ownerIndex = static_cast<int>(current - first);
        }
    }
    if (ownerIndex < 0) {
        return 0;
    }

    int count = 0;
    for (const auto& projectile : state.projectiles) {
        if (projectile.ownerIndex != ownerIndex || projectile.removing) {
            continue;
        }
        if (projectileId && projectile.id != *projectileId) {
            continue;
        }
        ++count;
    }
    return count;
}

const FighterState* ownedHelperById(const AppState& state, const FighterState& fighter, int helperId) {
    int ownerIndex = fighter.helper ? fighter.ownerIndex : -1;
    if (!fighter.helper) {
        const auto* first = state.fighters.data();
        const auto* current = &fighter;
        if (current >= first && current < first + state.fighters.size()) {
            ownerIndex = static_cast<int>(current - first);
        }
    }
    if (ownerIndex < 0) {
        return nullptr;
    }
    for (const auto& helper : state.helpers) {
        if (!helper.destroyRequested && helper.ownerIndex == ownerIndex && helper.helperId == helperId) {
            return &helper;
        }
    }
    return nullptr;
}

const FighterState* storedTargetFighter(const AppState& state, const FighterState& fighter) {
    if (fighter.targetTicks <= 0 || fighter.targetIndex < 0 || fighter.targetIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(fighter.targetIndex)];
}

FighterState* fighterOwner(AppState& state, const FighterState& fighter) {
    if (!fighter.helper || fighter.ownerIndex < 0 || fighter.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(fighter.ownerIndex)];
}

const FighterState* fighterOwner(const AppState& state, const FighterState& fighter) {
    if (!fighter.helper || fighter.ownerIndex < 0 || fighter.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(fighter.ownerIndex)];
}

int firstExistingAction(const AppState& state, std::initializer_list<int> actions) {
    for (const int action : actions) {
        if (findExactClip(state, action)) {
            return action;
        }
    }
    return 0;
}

const AnimationClip* findFightFxClip(const AppState& state, int action) {
    for (const auto& clip : state.fightFxClips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    return nullptr;
}

const StateDefinition* findStateDefinition(const AppState& state, int stateNo) {
    for (const auto& stateDef : state.stateDefs) {
        if (stateDef.stateNo == stateNo) {
            return &stateDef;
        }
    }
    return nullptr;
}

std::array<const StateDefinition*, 3> runtimeControllerStateDefinitions(const AppState& state, const FighterState& fighter) {
    return {
        findStateDefinition(state, -3),
        findStateDefinition(state, -2),
        findStateDefinition(state, fighter.stateNo),
    };
}

template <typename Fn>
void forEachRuntimeControllerStateDefinition(const AppState& state, const FighterState& fighter, Fn&& fn) {
    const auto stateDefs = runtimeControllerStateDefinitions(state, fighter);
    for (auto stateDefIt = stateDefs.begin(); stateDefIt != stateDefs.end(); ++stateDefIt) {
        const StateDefinition* stateDef = *stateDefIt;
        if (!stateDef || std::find(stateDefs.begin(), stateDefIt, stateDef) != stateDefIt) {
            continue;
        }
        if (!fn(*stateDef)) {
            break;
        }
    }
}

int effectiveClipTick(const AnimationClip& clip, int animTick) {
    const int totalTicks = std::max(1, clip.loopTicks);
    const int loopStart = std::clamp(clip.loopStartTick, 0, totalTicks - 1);
    int tick = std::max(0, animTick);
    if (tick >= totalTicks) {
        const int loopLength = std::max(1, totalTicks - loopStart);
        tick = loopStart + ((tick - loopStart) % loopLength);
    }
    return tick;
}

int animElemStartTickForClip(const AnimationClip& clip, int elem) {
    if (clip.frames.empty()) {
        return 0;
    }

    const int targetElem = std::clamp(elem, 1, static_cast<int>(clip.frames.size()));
    int tick = 0;
    for (int i = 0; i < targetElem - 1; ++i) {
        tick += clip.frames[static_cast<size_t>(i)].duration;
    }
    return tick;
}

int animElemTimeForClip(const AnimationClip& clip, int animTick, int elem) {
    if (clip.frames.empty()) {
        return 0;
    }
    return effectiveClipTick(clip, animTick) - animElemStartTickForClip(clip, elem);
}

const AnimationFrame* frameForClip(const AnimationClip& clip, int animTick) {
    if (clip.frames.empty()) {
        return nullptr;
    }

    int tick = effectiveClipTick(clip, animTick);

    for (const auto& frame : clip.frames) {
        if (tick < frame.duration) {
            return &frame;
        }
        tick -= frame.duration;
    }
    return &clip.frames.back();
}

int frameIndexForClip(const AnimationClip& clip, int animTick) {
    if (clip.frames.empty()) {
        return -1;
    }

    int tick = effectiveClipTick(clip, animTick);

    for (size_t i = 0; i < clip.frames.size(); ++i) {
        if (tick < clip.frames[i].duration) {
            return static_cast<int>(i);
        }
        tick -= clip.frames[i].duration;
    }
    return static_cast<int>(clip.frames.size() - 1);
}

std::string fixed1(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

void setFighterAction(FighterState& fighter, int action) {
    if (fighter.action != action) {
        fighter.action = action;
        fighter.animTick = 0;
        fighter.appliedHitDefIds.clear();
    }
}

void setFighterActionElement(const AppState& state, FighterState& fighter, int action, int elem) {
    const AnimationClip* clip = findExactClip(state, action);
    if (!clip) {
        return;
    }
    fighter.action = action;
    fighter.animTick = animElemStartTickForClip(*clip, elem);
}

int chooseMovementAction(const AppState& state, const FighterState& fighter);
int currentAnimElemForFighter(const AppState& state, const FighterState& fighter);
int animElemTimeForFighter(const AppState& state, const FighterState& fighter, int elem);
bool shouldPlayFightSounds(const AppState& state);
bool fighterAnimationEnded(const AppState& state, const FighterState& fighter);
void finishStateIfAnimationEnded(const AppState& state, FighterState& fighter);
void enterCommonLandingState(const AppState& state, FighterState& fighter);

void enterState(const AppState& state, FighterState& fighter, int stateNo) {
    fighter.guarding = false;
    if (stateNo == 0) {
        fighter.prevStateNo = fighter.stateNo;
        fighter.stateNo = stateNo;
        fighter.stateTime = 0;
        fighter.appliedHitDefIds.clear();
        fighter.firedStateSoundControllerIds.clear();
        fighter.firedStateCtrlControllerIds.clear();
        fighter.firedStatePosAddControllerIds.clear();
        fighter.firedStateChangeAnimControllerIds.clear();
        fighter.firedStateRuntimeControllerIds.clear();
        fighter.attackDistanceOverride = -1;
        fighter.drawAngle = 0.0f;
        fighter.angleDrawActive = false;
        fighter.displayOffsetX = 0.0f;
        fighter.displayOffsetY = 0.0f;
        fighter.moveContact = false;
        fighter.moveHit = false;
        fighter.moveGuarded = false;
        fighter.hitCount = 0;
        fighter.customHitState = false;
        fighter.ctrl = true;
        fighter.moveType = 'I';
        fighter.physics = fighter.onGround ? 'S' : 'A';
        fighter.stateType = fighter.onGround ? 'S' : 'A';
        fighter.sprPriority = 0;
        setFighterAction(fighter, chooseMovementAction(state, fighter));
        return;
    }

    const StateDefinition* stateDef = findStateDefinition(state, stateNo);
    if (!stateDef || (stateDef->hasAnim && !findExactClip(state, stateDef->anim))) {
        return;
    }

    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.appliedHitDefIds.clear();
    fighter.firedStateSoundControllerIds.clear();
    fighter.firedStateCtrlControllerIds.clear();
    fighter.firedStatePosAddControllerIds.clear();
    fighter.firedStateChangeAnimControllerIds.clear();
    fighter.firedStateRuntimeControllerIds.clear();
    fighter.attackDistanceOverride = -1;
    fighter.drawAngle = 0.0f;
    fighter.angleDrawActive = false;
    fighter.displayOffsetX = 0.0f;
    fighter.displayOffsetY = 0.0f;
    fighter.moveContact = false;
    fighter.moveHit = false;
    fighter.moveGuarded = false;
    fighter.hitCount = 0;
    fighter.customHitState = false;
    fighter.ctrl = stateDef->ctrl;
    fighter.stateType = stateDef->stateType;
    fighter.moveType = stateDef->moveType;
    fighter.physics = stateDef->physics;
    fighter.onGround = fighter.stateType != 'A';
    fighter.sprPriority = stateDef->sprPriority;
    if (stateDef->hasVelset) {
        fighter.vx = stateDef->velsetX * static_cast<float>(fighter.facing);
        fighter.vy = stateDef->velsetY;
    }
    if (stateDef->hasAnim) {
        setFighterAction(fighter, stateDef->anim);
    }
}

int hitAnimTypeIndex(std::string_view animtype) {
    if (startsWithNoCase(animtype, "Med") || startsWithNoCase(animtype, "Medium")) {
        return 1;
    }
    if (startsWithNoCase(animtype, "Hard") || startsWithNoCase(animtype, "Heavy")) {
        return 2;
    }
    if (startsWithNoCase(animtype, "Back")) {
        return 3;
    }
    if (startsWithNoCase(animtype, "Up")) {
        return 4;
    }
    if (startsWithNoCase(animtype, "DiagUp")) {
        return 5;
    }
    return 0;
}

bool hitGroundTypeIsLow(std::string_view groundType) {
    return startsWithNoCase(trim(groundType), "Low");
}

int hitGroundTypeValue(std::string_view groundType) {
    const std::string value = trim(groundType);
    if (startsWithNoCase(value, "Low")) {
        return 2;
    }
    if (startsWithNoCase(value, "Trip")) {
        return 3;
    }
    return 1;
}

int hitAirTypeValue(std::string_view animtype) {
    const int type = hitAnimTypeIndex(animtype);
    if (type >= 3) {
        return type;
    }
    return 1;
}

bool containsFlagNoCase(std::string_view flags, char flag) {
    const auto wanted = static_cast<char>(std::toupper(static_cast<unsigned char>(flag)));
    return std::any_of(flags.begin(), flags.end(), [wanted](char value) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(value))) == wanted;
    });
}

bool canStandingGuard(const HitDefinition& hitDef) {
    return containsFlagNoCase(hitDef.guardflag, 'M')
        || containsFlagNoCase(hitDef.guardflag, 'H');
}

bool canCrouchingGuard(const HitDefinition& hitDef) {
    return containsFlagNoCase(hitDef.guardflag, 'M')
        || containsFlagNoCase(hitDef.guardflag, 'L');
}

bool hitFlagContainsExpanded(std::string_view flags, char flag) {
    if (flag == 'H' || flag == 'L') {
        return containsFlagNoCase(flags, 'M') || containsFlagNoCase(flags, flag);
    }
    return containsFlagNoCase(flags, flag);
}

bool fighterIsLyingDown(const FighterState& fighter) {
    return fighter.onGround
        && (fighter.stateNo == 5100
            || fighter.stateNo == 5110
            || fighter.stateNo == 5120
            || fighter.stateNo == 5150
            || fighter.stateNo == 5160
            || fighter.stateNo == 5170);
}

bool fighterIsFallingInAir(const FighterState& fighter) {
    return !fighter.onGround && (fighter.hitFall || fighter.stateNo >= 5050);
}

bool hitFlagAllowsDefender(const HitDefinition& hitDef, const FighterState& defender) {
    const std::string flags = trim(hitDef.hitflag).empty()
        ? std::string("MAF")
        : uppercaseCopy(trim(hitDef.hitflag));
    const bool defenderInGetHit = defender.moveType == 'H';

    if (containsFlagNoCase(flags, '+') && !defenderInGetHit) {
        return false;
    }
    if (containsFlagNoCase(flags, '-') && defenderInGetHit) {
        return false;
    }
    if (fighterIsLyingDown(defender)) {
        return hitFlagContainsExpanded(flags, 'D');
    }
    if (!defender.onGround || defender.stateType == 'A') {
        if (fighterIsFallingInAir(defender) && !hitFlagContainsExpanded(flags, 'F')) {
            return false;
        }
        return hitFlagContainsExpanded(flags, 'A');
    }
    if (defender.stateType == 'C' || defender.hitCrouch) {
        return hitFlagContainsExpanded(flags, 'L');
    }
    return hitFlagContainsExpanded(flags, 'H');
}

GuardStance chooseDummyGuardStance(const TrainingOptions& options, const HitDefinition& hitDef, const FighterState& defender) {
    if (options.dummyGuardMode == DummyGuardMode::Off
        || !defender.onGround
        || (defender.moveType == 'H' && !defender.guarding)) {
        return GuardStance::None;
    }

    switch (options.dummyGuardMode) {
    case DummyGuardMode::Stand:
        return canStandingGuard(hitDef) ? GuardStance::Stand : GuardStance::None;
    case DummyGuardMode::Crouch:
        return canCrouchingGuard(hitDef) ? GuardStance::Crouch : GuardStance::None;
    case DummyGuardMode::Auto:
        if (containsFlagNoCase(hitDef.guardflag, 'L') && canCrouchingGuard(hitDef)) {
            return GuardStance::Crouch;
        }
        if (canStandingGuard(hitDef)) {
            return GuardStance::Stand;
        }
        return canCrouchingGuard(hitDef) ? GuardStance::Crouch : GuardStance::None;
    case DummyGuardMode::Off:
    default:
        return GuardStance::None;
    }
}

const FighterInputState* latestFighterInput(const FighterState& fighter) {
    return fighter.inputHistory.empty() ? nullptr : &fighter.inputHistory.back().input;
}

bool fighterInputHoldingBack(const FighterInputState& input, const FighterState& fighter) {
    return fighter.facing >= 0 ? input.left : input.right;
}

GuardStance choosePlayerGuardStance(const HitDefinition& hitDef, const FighterState& defender) {
    if (!defender.onGround || (defender.moveType == 'H' && !defender.guarding) || (!defender.ctrl && !defender.guarding)) {
        return GuardStance::None;
    }
    const FighterInputState* input = latestFighterInput(defender);
    if (!input || !fighterInputHoldingBack(*input, defender)) {
        return GuardStance::None;
    }
    if (input->down) {
        return canCrouchingGuard(hitDef) ? GuardStance::Crouch : GuardStance::None;
    }
    return canStandingGuard(hitDef) ? GuardStance::Stand : GuardStance::None;
}

GuardStance dummyGuardIdleStance(DummyGuardMode mode) {
    switch (mode) {
    case DummyGuardMode::Stand:
        return GuardStance::Stand;
    case DummyGuardMode::Crouch:
        return GuardStance::Crouch;
    case DummyGuardMode::Auto:
        return GuardStance::Stand;
    case DummyGuardMode::Off:
    default:
        return GuardStance::None;
    }
}

int fallbackGroundHitShakeAction(const HitDefinition& hitDef) {
    const int base = hitGroundTypeIsLow(hitDef.groundType) ? 5010 : 5000;
    return base + std::min(hitAnimTypeIndex(hitDef.animtype), 2);
}

int fallbackGroundHitRecoverAction(const HitDefinition& hitDef) {
    const int base = hitGroundTypeIsLow(hitDef.groundType) ? 5015 : 5005;
    return base + std::min(hitAnimTypeIndex(hitDef.animtype), 2);
}

int hitShakeActionForFighter(const AppState& state, const HitDefinition& hitDef, bool crouching) {
    const int variant = std::min(hitAnimTypeIndex(hitDef.animtype), 2);
    const int fallback = fallbackGroundHitShakeAction(hitDef);
    if (crouching) {
        return firstExistingAction(state, { 5020 + variant, fallback, 5010 + variant, 5000 + variant, 0 });
    }
    return firstExistingAction(state, { fallback, 5000 + variant, 5010 + variant, 0 });
}

int hitRecoverActionForFighter(const AppState& state, const HitDefinition& hitDef, bool crouching) {
    const int variant = std::min(hitAnimTypeIndex(hitDef.animtype), 2);
    const int fallback = fallbackGroundHitRecoverAction(hitDef);
    if (crouching) {
        return firstExistingAction(state, { 5025 + variant, fallback, 5015 + variant, 5005 + variant, 0 });
    }
    return firstExistingAction(state, { fallback, 5005 + variant, 5015 + variant, 0 });
}

bool hitGroundTypeIsTrip(std::string_view groundType) {
    return startsWithNoCase(trim(groundType), "Trip");
}

bool hitDefCausesFall(const HitDefinition& hitDef, const FighterState& target) {
    return hitDef.fall || (!target.onGround && hitDef.airFall) || hitGroundTypeIsTrip(hitDef.groundType);
}

void setGetHitVarsFromHitDef(
    FighterState& target,
    const HitDefinition& hitDef,
    bool wasAirborne,
    bool fallHit,
    float hitVelocityX,
    float hitVelocityY) {
    target.getHitAnimType = hitAnimTypeIndex(hitDef.animtype);
    target.getHitGroundType = hitGroundTypeValue(hitDef.groundType);
    target.getHitAirType = hitAirTypeValue(hitDef.animtype);
    target.getHitSlideTime = wasAirborne ? 0 : std::max(0, hitDef.groundSlideTime);
    target.getHitHitTime = wasAirborne && hitDef.airHitTime > 0
        ? hitDef.airHitTime
        : std::max(0, hitDef.groundHitTime);
    target.getHitCtrlTime = target.getHitHitTime;
    target.getHitHitCount = std::max(0, target.getHitHitCount + 1);
    target.hitVelocityX = hitVelocityX;
    target.hitVelocityY = hitVelocityY;
    if (fallHit && hitDef.hasFallYVelocity) {
        target.hitVelocityY = hitDef.fallYVelocity;
    }
}

bool fighterIsCrouchingForHit(const FighterState& fighter) {
    return fighter.stateType == 'C'
        || fighter.crouchGuard
        || fighter.stateNo == 10
        || fighter.stateNo == 11
        || fighter.stateNo == 12
        || fighter.stateNo == 130
        || fighter.stateNo == 131
        || fighter.stateNo == 132;
}

bool fighterIsLyingDownForHit(const FighterState& fighter) {
    return fighter.stateType == 'L'
        || fighter.stateNo == 5080
        || fighter.stateNo == 5110
        || fighter.stateNo == 5120
        || fighter.stateNo == 5150;
}

int fallAirActionForHit(const AppState& state, const HitDefinition& hitDef) {
    const std::string_view animType = hitDef.fallAnimtype.empty() ? std::string_view(hitDef.animtype) : std::string_view(hitDef.fallAnimtype);
    const int candidate = 5050 + hitAnimTypeIndex(animType);
    if (findExactClip(state, candidate)) {
        return candidate;
    }
    return findExactClip(state, 5050) ? 5050 : 0;
}

int fallLandActionForFighter(const AppState& state, const FighterState& target) {
    const int variant = target.hitFallAirAction % 10;
    const int candidate = 5100 + variant;
    if (findExactClip(state, candidate)) {
        return candidate;
    }
    return findExactClip(state, 5100) ? 5100 : 0;
}

int guardStartAction(GuardStance stance) {
    return stance == GuardStance::Crouch ? 130 : 120;
}

int guardIdleAction(GuardStance stance) {
    return stance == GuardStance::Crouch ? 131 : 121;
}

int guardEndAction(GuardStance stance) {
    return stance == GuardStance::Crouch ? 132 : 122;
}

bool isGroundGuardCommonState(int stateNo) {
    return stateNo == 120
        || stateNo == 121
        || stateNo == 122
        || stateNo == 130
        || stateNo == 131
        || stateNo == 132;
}

GuardStance guardStanceFromCommonState(int stateNo) {
    return stateNo >= 130 && stateNo <= 132 ? GuardStance::Crouch : GuardStance::Stand;
}

void enterGroundGuardReadyState(const AppState& state, FighterState& target, GuardStance stance) {
    const bool crouch = stance == GuardStance::Crouch;
    const int startAction = guardStartAction(stance);
    const int idleAction = guardIdleAction(stance);
    const int action = firstExistingAction(state, { startAction, idleAction, crouch ? 130 : 120, 0 });
    target.guarding = false;
    target.crouchGuard = crouch;
    target.stateNo = action == idleAction ? idleAction : startAction;
    target.stateTime = 0;
    target.firedStateSoundControllerIds.clear();
    target.firedStateCtrlControllerIds.clear();
    target.firedStatePosAddControllerIds.clear();
    target.firedStateChangeAnimControllerIds.clear();
    target.firedStateRuntimeControllerIds.clear();
    target.moveContact = false;
    target.moveHit = false;
    target.moveGuarded = false;
    target.stateType = crouch ? 'C' : 'S';
    target.moveType = 'I';
    target.physics = crouch ? 'C' : 'S';
    target.ctrl = true;
    target.onGround = true;
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.hitPauseTicks = 0;
    target.hitStunTicks = 0;
    target.hitSlideTicks = 0;
    target.hitVelocityX = 0.0f;
    target.hitVelocityY = 0.0f;
    target.getHitAnimType = 0;
    target.getHitGroundType = 0;
    target.getHitAirType = 0;
    target.getHitSlideTime = 0;
    target.getHitHitTime = 0;
    target.getHitCtrlTime = 0;
    target.getHitHitCount = 0;
    target.hitRecoverAnim = 5005;
    target.hitCrouch = false;
    target.hitAirborne = false;
    setFighterAction(target, action);
}

void enterGroundGuardEndState(const AppState& state, FighterState& target) {
    const GuardStance stance = guardStanceFromCommonState(target.stateNo);
    const int action = findExactClip(state, guardEndAction(stance)) ? guardEndAction(stance) : 0;
    if (action == 0) {
        enterState(state, target, 0);
        return;
    }

    target.guarding = false;
    target.crouchGuard = stance == GuardStance::Crouch;
    target.stateNo = guardEndAction(stance);
    target.stateTime = 0;
    target.moveType = 'I';
    target.stateType = target.crouchGuard ? 'C' : 'S';
    target.physics = target.crouchGuard ? 'C' : 'S';
    target.ctrl = false;
    target.onGround = true;
    target.vx = 0.0f;
    target.vy = 0.0f;
    setFighterAction(target, action);
}

void updateGroundGuardReadyState(const AppState& state, FighterState& target) {
    if (!isGroundGuardCommonState(target.stateNo)) {
        return;
    }

    const GuardStance stance = guardStanceFromCommonState(target.stateNo);
    const int startAction = guardStartAction(stance);
    const int idleAction = guardIdleAction(stance);
    const int endAction = guardEndAction(stance);

    target.vx = 0.0f;
    target.vy = 0.0f;
    target.onGround = true;
    if (target.stateNo == startAction && fighterAnimationEnded(state, target)) {
        target.stateNo = findExactClip(state, idleAction) ? idleAction : startAction;
        target.stateTime = 0;
        setFighterAction(target, findExactClip(state, idleAction) ? idleAction : startAction);
        return;
    }
    if (target.stateNo == endAction && fighterAnimationEnded(state, target)) {
        enterState(state, target, 0);
    }
}

int fightHitPauseTicks(const AppState& state, int ticks, int minimum) {
    const int resolved = std::max(minimum, ticks);
    return state.frontend.pendingMode == PendingMode::Arena
        ? std::min(resolved, 12)
        : resolved;
}

float arenaFallInitialYVelocityCap(const FighterState& fighter) {
    return fighter.hitFallTrip ? 0.0f : -2.2f;
}

float arenaFallBounceYVelocityCap(const FighterState& fighter) {
    return fighter.hitFallTrip ? 0.0f : -1.0f;
}

void clampArenaHitFallRuntime(const AppState& state, FighterState& fighter) {
    if (!isArenaMode(state) || !fighter.hitFall) {
        return;
    }
    fighter.hitFallRecover = false;
    fighter.hitVelocityY = std::max(fighter.hitVelocityY, arenaFallInitialYVelocityCap(fighter));
    fighter.hitFallBounceYVelocity = std::max(fighter.hitFallBounceYVelocity, arenaFallBounceYVelocityCap(fighter));
    fighter.hitFallYAccel = std::max(fighter.hitFallYAccel, fighter.hitFallTrip ? 0.72f : 0.70f);
}

void clampArenaAppliedFallVelocity(const AppState& state, FighterState& fighter, bool bounceVelocity) {
    if (!isArenaMode(state) || !fighter.hitFall) {
        return;
    }
    const float cap = bounceVelocity
        ? arenaFallBounceYVelocityCap(fighter)
        : arenaFallInitialYVelocityCap(fighter);
    fighter.vy = std::max(fighter.vy, cap);
    if (fighter.hitFallTrip && fighter.y >= 0.0f) {
        fighter.vy = std::max(fighter.vy, 0.0f);
    }
}

void enterGroundGetHitState(const AppState& state, FighterState& target, const HitDefinition& hitDef, int attackerFacing) {
    const bool wasAirborne = !target.onGround || target.stateType == 'A';
    const bool wasCrouching = fighterIsCrouchingForHit(target);
    const bool wasLyingDown = fighterIsLyingDownForHit(target);
    const bool useAirVelocity = wasAirborne && hitDef.hasAirVelocity;
    const float hitVelocityX = useAirVelocity ? hitDef.airVelocityX : hitDef.groundVelocityX;
    const float hitVelocityY = useAirVelocity ? hitDef.airVelocityY : hitDef.groundVelocityY;
    const bool fallHit = hitDefCausesFall(hitDef, target);
    const bool arenaGroundLaunchFall = isArenaMode(state) && !wasAirborne && !fallHit && hitVelocityY < -0.05f;
    const bool resolvedFallHit = fallHit || arenaGroundLaunchFall;
    const bool liedownBounce = wasLyingDown && hitVelocityY < -0.05f;

    target.guarding = false;
    target.stateNo = wasAirborne ? 5030 : (wasLyingDown ? (liedownBounce ? 5090 : 5080) : 5000);
    target.stateTime = 0;
    target.firedStateSoundControllerIds.clear();
    target.firedStateCtrlControllerIds.clear();
    target.firedStatePosAddControllerIds.clear();
    target.firedStateChangeAnimControllerIds.clear();
    target.firedStateRuntimeControllerIds.clear();
    target.moveContact = false;
    target.moveHit = false;
    target.moveGuarded = false;
    target.stateType = wasAirborne || liedownBounce ? 'A' : (wasLyingDown ? 'L' : (wasCrouching ? 'C' : 'S'));
    target.moveType = 'H';
    target.physics = wasAirborne || liedownBounce ? 'A' : 'N';
    target.ctrl = false;
    target.onGround = !(wasAirborne || liedownBounce);
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 1);
    const int hitTime = wasAirborne && hitDef.airHitTime > 0 ? hitDef.airHitTime : hitDef.groundHitTime;
    target.hitStunTicks = std::max(hitTime, target.hitPauseTicks);
    target.hitSlideTicks = wasAirborne ? 0 : std::max(0, hitDef.groundSlideTime);
    setGetHitVarsFromHitDef(
        target,
        hitDef,
        wasAirborne,
        resolvedFallHit,
        -hitVelocityX * static_cast<float>(attackerFacing),
        hitVelocityY);
    target.hitRecoverAnim = hitRecoverActionForFighter(state, hitDef, wasCrouching);
    target.hitFall = resolvedFallHit || wasLyingDown;
    target.hitFallTrip = hitGroundTypeIsTrip(hitDef.groundType);
    target.hitCrouch = wasCrouching;
    target.hitAirborne = wasAirborne || liedownBounce;
    target.hitFallRecover = hitDef.fallRecover;
    target.hitFallRecoverTime = hitDef.fallRecoverTime;
    target.hitFallDamage = hitDef.fallDamage;
    target.hitFallYAccel = hitDef.hasYAccel ? hitDef.yAccel : state.characterConstants.movementYAccel;
    target.hitFallAirAction = fallAirActionForHit(state, hitDef);
    target.hitFallBounceXVelocity = hitDef.hasFallXVelocity
        ? -hitDef.fallXVelocity * static_cast<float>(attackerFacing)
        : target.hitVelocityX;
    target.hitFallBounceYVelocity = hitDef.fallYVelocity;
    clampArenaHitFallRuntime(state, target);
    target.hitFallEnvShake = hitDef.fallEnvShake;
    target.hitFallEnvShakePlayed = false;

    const int action = wasAirborne
        ? firstExistingAction(state, { 5030, fallAirActionForHit(state, hitDef), hitShakeActionForFighter(state, hitDef, false), 0 })
        : (wasLyingDown
            ? firstExistingAction(state, { liedownBounce ? 5090 : 5080, fallLandActionForFighter(state, target), 5110, 0 })
            : hitShakeActionForFighter(state, hitDef, wasCrouching));
    const int tripAction = target.hitFallTrip && findExactClip(state, 5070) ? 5070 : 0;
    const int shakeAction = tripAction != 0 && !wasAirborne ? tripAction : firstExistingAction(state, { action, 5000, 0 });
    setFighterAction(target, shakeAction);
}

bool enterCustomHitState(const AppState& state, FighterState& target, const HitDefinition& hitDef, int attackerFacing) {
    if (hitDef.p2StateNo < 0 || !findStateDefinition(state, hitDef.p2StateNo)) {
        return false;
    }

    const float hitVelocityX = hitDef.hasAirVelocity ? hitDef.airVelocityX : hitDef.groundVelocityX;
    const float hitVelocityY = hitDef.hasAirVelocity ? hitDef.airVelocityY : hitDef.groundVelocityY;
    enterState(state, target, hitDef.p2StateNo);
    target.customHitState = true;
    target.moveType = 'H';
    target.ctrl = false;
    target.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 0);
    target.hitStunTicks = std::max(hitDef.groundHitTime, target.hitPauseTicks);
    target.hitSlideTicks = std::max(0, hitDef.groundSlideTime);
    setGetHitVarsFromHitDef(
        target,
        hitDef,
        !target.onGround || target.stateType == 'A',
        hitDefCausesFall(hitDef, target),
        -hitVelocityX * static_cast<float>(attackerFacing),
        hitVelocityY);
    if (hitDef.hasP2Facing && hitDef.p2Facing != 0) {
        target.facing = hitDef.p2Facing > 0 ? -attackerFacing : attackerFacing;
    }
    return true;
}

void enterGroundGuardHitState(const AppState& state, FighterState& target, const HitDefinition& hitDef, int attackerFacing, GuardStance stance) {
    const bool crouch = stance == GuardStance::Crouch;
    target.guarding = true;
    target.crouchGuard = crouch;
    target.stateNo = crouch ? 152 : 150;
    target.stateTime = 0;
    target.firedStateSoundControllerIds.clear();
    target.firedStateCtrlControllerIds.clear();
    target.firedStatePosAddControllerIds.clear();
    target.firedStateChangeAnimControllerIds.clear();
    target.firedStateRuntimeControllerIds.clear();
    target.moveContact = false;
    target.stateType = crouch ? 'C' : 'S';
    target.moveType = 'H';
    target.physics = 'N';
    target.ctrl = false;
    target.onGround = true;
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 1);
    target.hitStunTicks = std::max(1, hitDef.groundHitTime);
    target.hitSlideTicks = std::max(0, hitDef.groundSlideTime);
    target.hitVelocityX = -hitDef.guardVelocityX * static_cast<float>(attackerFacing);
    target.hitVelocityY = hitDef.guardVelocityY;
    target.getHitAnimType = hitAnimTypeIndex(hitDef.animtype);
    target.getHitGroundType = hitGroundTypeValue(hitDef.groundType);
    target.getHitAirType = hitAirTypeValue(hitDef.animtype);
    target.getHitSlideTime = target.hitSlideTicks;
    target.getHitHitTime = target.hitStunTicks;
    target.getHitCtrlTime = target.hitStunTicks;
    target.getHitHitCount = std::max(0, target.getHitHitCount + 1);
    target.hitRecoverAnim = crouch ? 131 : 130;

    const int action = crouch ? 151 : 150;
    const int guardAction = findExactClip(state, action) ? action : 0;
    setFighterAction(target, guardAction);
}

void clearFighterHitRuntime(FighterState& fighter) {
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 0;
    fighter.hitSlideTicks = 0;
    fighter.hitVelocityX = 0.0f;
    fighter.hitVelocityY = 0.0f;
    fighter.getHitAnimType = 0;
    fighter.getHitGroundType = 0;
    fighter.getHitAirType = 0;
    fighter.getHitSlideTime = 0;
    fighter.getHitHitTime = 0;
    fighter.getHitCtrlTime = 0;
    fighter.getHitHitCount = 0;
    fighter.hitRecoverAnim = 5005;
    fighter.hitFall = false;
    fighter.hitFallTrip = false;
    fighter.hitCrouch = false;
    fighter.hitAirborne = false;
    fighter.hitFallRecover = true;
    fighter.hitFallRecoverTime = 0;
    fighter.hitFallDamage = 0;
    fighter.hitFallYAccel = 0.0f;
    fighter.hitFallAirAction = 5050;
    fighter.hitFallBounceXVelocity = 0.0f;
    fighter.hitFallBounceYVelocity = -4.5f;
    fighter.hitFallEnvShake = {};
    fighter.hitFallEnvShakePlayed = false;
    fighter.notHitByTicks = 0;
    fighter.notHitByValue.clear();
    fighter.hitByTicks = 0;
    fighter.hitByValue.clear();
    fighter.customHitState = false;
    fighter.targetIndex = -1;
    fighter.targetHitId = -1;
    fighter.targetTicks = 0;
    fighter.boundByIndex = -1;
    fighter.bindTicks = 0;
    fighter.appliedHitDefIds.clear();
    fighter.firedStateSoundControllerIds.clear();
    fighter.firedStateCtrlControllerIds.clear();
    fighter.firedStatePosAddControllerIds.clear();
    fighter.firedStateChangeAnimControllerIds.clear();
    fighter.firedStateRuntimeControllerIds.clear();
    fighter.moveContact = false;
    fighter.moveHit = false;
    fighter.moveGuarded = false;
    fighter.guarding = false;
    fighter.crouchGuard = false;
    fighter.onGround = true;
    fighter.jumpBaseAction = 0;
    fighter.jumpPeakActionApplied = false;
}

void applyTrainingPowerMode(AppState& state) {
    if (state.frontend.pendingMode != PendingMode::Training || state.training.options.powerMode != TrainingPowerMode::Max) {
        return;
    }
    const int maxPower = std::max(0, state.characterConstants.maxPower);
    for (auto& fighter : state.fighters) {
        fighter.power = maxPower;
    }
}

float clampFighterOriginToStage(float x, const StageSlot& stage);

#include "FightSessionRuntime.h"

bool isCrouchStateNo(int stateNo) {
    return stateNo == 10 || stateNo == 11 || stateNo == 12;
}

bool fighterAnimationEnded(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findExactClip(state, fighter.action);
    return clip && fighter.animTick >= clip->loopTicks;
}

void enterCrouchState(const AppState& state, FighterState& fighter, int stateNo) {
    fighter.guarding = false;
    fighter.crouchGuard = false;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.appliedHitDefIds.clear();
    fighter.firedStateSoundControllerIds.clear();
    fighter.firedStateCtrlControllerIds.clear();
    fighter.firedStatePosAddControllerIds.clear();
    fighter.firedStateChangeAnimControllerIds.clear();
    fighter.firedStateRuntimeControllerIds.clear();
    fighter.moveContact = false;
    fighter.moveHit = false;
    fighter.moveGuarded = false;
    fighter.stateType = stateNo == 12 ? 'S' : 'C';
    fighter.moveType = 'I';
    fighter.physics = stateNo == 12 ? 'S' : 'C';
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 0;
    fighter.hitSlideTicks = 0;
    setFighterAction(fighter, findExactClip(state, stateNo) ? stateNo : 0);
}

void updatePlayerCrouchInput(const AppState& state, FighterState& fighter, bool holdingDown) {
    if (!fighter.onGround || fighter.moveType == 'H' || fighter.guarding) {
        return;
    }

    if (holdingDown) {
        if (fighter.stateNo == 0 || fighter.stateNo == 12) {
            enterCrouchState(state, fighter, 10);
        } else if (fighter.stateNo == 10 && fighterAnimationEnded(state, fighter)) {
            enterCrouchState(state, fighter, 11);
        }
    } else if (fighter.stateNo == 10 || fighter.stateNo == 11) {
        enterCrouchState(state, fighter, 12);
    } else if (fighter.stateNo == 12 && fighterAnimationEnded(state, fighter)) {
        enterState(state, fighter, 0);
    }
}

void applyParsedChangeState(const AppState& state, FighterState& fighter, int targetState, std::optional<bool> ctrl) {
    if (isCrouchStateNo(targetState)) {
        enterCrouchState(state, fighter, targetState);
    } else {
        enterState(state, fighter, targetState);
    }
    if (ctrl) {
        fighter.ctrl = *ctrl;
    }
}

bool stateControllerTriggerActive(
    const AppState& state,
    const FighterState& fighter,
    const StateControllerTrigger& trigger,
    const FighterState* opponent,
    const StageSlot* stage);
bool simpleControllerTriggerActive(const AppState& state, const FighterState& fighter, int triggerTime, int triggerAnimElem);

bool stateCtrlAlreadyFired(const FighterState& fighter, int ctrlControllerId) {
    return std::find(
        fighter.firedStateCtrlControllerIds.begin(),
        fighter.firedStateCtrlControllerIds.end(),
        ctrlControllerId) != fighter.firedStateCtrlControllerIds.end();
}

void markStateCtrlFired(FighterState& fighter, int ctrlControllerId) {
    if (!stateCtrlAlreadyFired(fighter, ctrlControllerId)) {
        fighter.firedStateCtrlControllerIds.push_back(ctrlControllerId);
    }
}

void updateStateCtrlControllers(AppState& state, FighterState& fighter) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& ctrl : stateDef.ctrlSets) {
            const bool triggerActive = ctrl.trigger.hasTrigger
                ? stateControllerTriggerActive(state, fighter, ctrl.trigger, nullptr, nullptr)
                : simpleControllerTriggerActive(state, fighter, ctrl.triggerTime, ctrl.triggerAnimElem);
            if (!triggerActive || stateCtrlAlreadyFired(fighter, ctrl.id)) {
                continue;
            }
            markStateCtrlFired(fighter, ctrl.id);
            fighter.ctrl = ctrl.value;
        }
        return true;
    });
}

bool statePosAddAlreadyFired(const FighterState& fighter, int posAddControllerId) {
    return std::find(
        fighter.firedStatePosAddControllerIds.begin(),
        fighter.firedStatePosAddControllerIds.end(),
        posAddControllerId) != fighter.firedStatePosAddControllerIds.end();
}

void markStatePosAddFired(FighterState& fighter, int posAddControllerId) {
    if (!statePosAddAlreadyFired(fighter, posAddControllerId)) {
        fighter.firedStatePosAddControllerIds.push_back(posAddControllerId);
    }
}

bool stateChangeAnimAlreadyFired(const FighterState& fighter, int changeAnimControllerId) {
    return std::find(
        fighter.firedStateChangeAnimControllerIds.begin(),
        fighter.firedStateChangeAnimControllerIds.end(),
        changeAnimControllerId) != fighter.firedStateChangeAnimControllerIds.end();
}

void markStateChangeAnimFired(FighterState& fighter, int changeAnimControllerId) {
    if (!stateChangeAnimAlreadyFired(fighter, changeAnimControllerId)) {
        fighter.firedStateChangeAnimControllerIds.push_back(changeAnimControllerId);
    }
}

#include "RuntimeExpressionEvaluation.h"

void setFighterVariableValue(FighterState& fighter, const MugenVariableRef& ref, float value) {
    if (!variableRefInRange(ref)) {
        return;
    }
    switch (ref.bank) {
    case MugenVariableBank::Var:
        fighter.vars[static_cast<size_t>(ref.index)] = static_cast<int>(std::lround(value));
        break;
    case MugenVariableBank::SysVar:
        fighter.sysVars[static_cast<size_t>(ref.index)] = static_cast<int>(std::lround(value));
        break;
    case MugenVariableBank::FVar:
        fighter.fvars[static_cast<size_t>(ref.index)] = value;
        break;
    case MugenVariableBank::SysFVar:
        fighter.sysFvars[static_cast<size_t>(ref.index)] = value;
        break;
    default:
        break;
    }
}

int fighterIndexInState(const AppState& state, const FighterState& fighter) {
    const auto* first = state.fighters.data();
    const auto* current = &fighter;
    if (current < first || current >= first + state.fighters.size()) {
        return -1;
    }
    return static_cast<int>(current - first);
}

#include "StateControllerUtilityRuntime.h"

bool globalPauseActive(const AppState& state) {
    return state.globalPauseTicks > 0;
}

bool fighterCanUpdateDuringGlobalPause(const AppState& state, int fighterIndex) {
    if (!globalPauseActive(state)) {
        return true;
    }
    return fighterIndex == state.globalPauseOwnerIndex && state.globalPauseOwnerMoveTicks > 0;
}

void startGlobalPause(AppState& state, const FighterState& fighter, const StatePauseController& pause) {
    const int ownerIndex = fighterIndexInState(state, fighter);
    state.globalPauseTicks = std::max(state.globalPauseTicks, pause.time);
    state.globalPauseOwnerIndex = ownerIndex;
    state.globalPauseOwnerMoveTicks = std::max(state.globalPauseOwnerMoveTicks, pause.moveTime);
    state.globalPauseIsSuper = pause.superPause;
    if (pause.soundGroup >= 0 && pause.soundIndex >= 0) {
        playSound(state, pause.soundGroup, pause.soundIndex, pause.soundForceCommon);
    }
}

void updateGlobalPauseTimers(AppState& state) {
    if (state.globalPauseTicks > 0) {
        --state.globalPauseTicks;
    }
    if (state.globalPauseOwnerMoveTicks > 0) {
        --state.globalPauseOwnerMoveTicks;
    }
    if (state.globalPauseTicks <= 0) {
        state.globalPauseTicks = 0;
        state.globalPauseOwnerIndex = -1;
        state.globalPauseOwnerMoveTicks = 0;
        state.globalPauseIsSuper = false;
    }
}

#include "StateControllerVariableRuntime.h"

#include "StateControllerPowerRuntime.h"

void updateStateMeterControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        updateStatePowerControllersForDefinition(state, fighter, stateDef, opponent, stage);

        for (const auto& lifeAdd : stateDef.lifeAdds) {
            if (!shouldRunStateRuntimeController(state, fighter, lifeAdd.id, lifeAdd.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, lifeAdd.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            const int minimumLife = lifeAdd.kill ? 0 : 1;
            fighter.life = std::clamp(
                fighter.life + static_cast<int>(std::lround(*value)),
                minimumLife,
                1000);
        }

        for (const auto& hitAdd : stateDef.hitAdds) {
            if (!shouldRunStateRuntimeController(state, fighter, hitAdd.id, hitAdd.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, hitAdd.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.hitCount = std::max(0, fighter.hitCount + static_cast<int>(std::lround(*value)));
        }

        for (const auto& attackDist : stateDef.attackDists) {
            if (!shouldRunStateRuntimeController(state, fighter, attackDist.id, attackDist.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, attackDist.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.attackDistanceOverride = std::max(0, static_cast<int>(std::lround(*value)));
        }

        for (const auto& defenceMulSet : stateDef.defenceMulSets) {
            if (!shouldRunStateRuntimeController(state, fighter, defenceMulSet.id, defenceMulSet.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, defenceMulSet.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.defenceMultiplier = std::max(0.001f, *value);
        }

        for (const auto& attackMulSet : stateDef.attackMulSets) {
            if (!shouldRunStateRuntimeController(state, fighter, attackMulSet.id, attackMulSet.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, attackMulSet.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.attackMultiplier = std::max(0.0f, *value);
        }
        return true;
    });
}

#include "StateControllerPosAddRuntime.h"

void updateStatePosAddControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        updateStatePosAddControllersForDefinition(state, fighter, runtimeStateDef, opponent, stage);
        return true;
    });
}

void updateStateChangeAnimControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& changeAnim : stateDef->changeAnims) {
        if (changeAnim.trigger.hasTrigger) {
            if (!shouldRunStateRuntimeController(state, fighter, changeAnim.id, changeAnim.trigger, opponent, stage)) {
                continue;
            }
        } else {
            if (stateChangeAnimAlreadyFired(fighter, changeAnim.id)
                || !changeAnimTriggerActive(state, fighter, changeAnim)) {
                continue;
            }
            markStateChangeAnimFired(fighter, changeAnim.id);
        }
        setFighterActionElement(state, fighter, changeAnim.value, changeAnim.elem);
    }
        return true;
    });
}

#include "StateControllerVelocityRuntime.h"
#include "StateControllerPosSetRuntime.h"
#include "StateControllerSprPriorityRuntime.h"
#include "StateControllerPosFreezeRuntime.h"
#include "StateControllerTurnRuntime.h"

void updateStateMovementControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    updateStateVariableControllers(state, fighter, opponent, stage);
    updateStateMeterControllers(state, fighter, opponent, stage);
    updateStateVisualControllers(state, fighter, opponent, stage);

    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    updateStateSprPriorityControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    updateStatePosFreezeControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    updateStateTurnControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    for (const auto& pause : stateDef->pauses) {
        if (!shouldRunStateRuntimeController(state, fighter, pause.id, pause.trigger, opponent, stage)) {
            continue;
        }
        startGlobalPause(state, fighter, pause);
    }

    for (const auto& forceFeedback : stateDef->forceFeedbacks) {
        if (!shouldRunStateRuntimeController(state, fighter, forceFeedback.id, forceFeedback.trigger, opponent, stage)) {
            continue;
        }
        runForceFeedbackController(state, fighter, forceFeedback);
    }

    for (const auto& envShake : stateDef->envShakes) {
        if (!shouldRunStateRuntimeController(state, fighter, envShake.id, envShake.trigger, opponent, stage)) {
            continue;
        }
        startEnvShake(state, envShake.shake);
    }

    for (const auto& fallEnvShake : stateDef->fallEnvShakes) {
        if (!shouldRunStateRuntimeController(state, fighter, fallEnvShake.id, fallEnvShake.trigger, opponent, stage)) {
            continue;
        }
        if (!fighter.hitFallEnvShakePlayed) {
            fighter.hitFallEnvShakePlayed = true;
            startEnvShake(state, fighter.hitFallEnvShake);
        }
    }

    for (const auto& paletteEffect : stateDef->paletteEffects) {
        if (!shouldRunStateRuntimeController(state, fighter, paletteEffect.id, paletteEffect.trigger, opponent, stage)) {
            continue;
        }
        if (paletteEffect.background) {
            startPaletteEffect(state.backgroundPaletteEffect, paletteEffect.effect);
        } else {
            startPaletteEffect(fighter.paletteEffect, paletteEffect.effect);
        }
    }

    for (const auto& envColor : stateDef->envColors) {
        if (!shouldRunStateRuntimeController(state, fighter, envColor.id, envColor.trigger, opponent, stage)) {
            continue;
        }
        startEnvColor(state, envColor);
    }

    for (const auto& hitVelSet : stateDef->hitVelSets) {
        if (!shouldRunStateRuntimeController(state, fighter, hitVelSet.id, hitVelSet.trigger, opponent, stage)) {
            continue;
        }
        if (hitVelSet.applyX) {
            fighter.vx = fighter.hitVelocityX;
        }
        if (hitVelSet.applyY) {
            fighter.vy = fighter.hitVelocityY;
            clampArenaAppliedFallVelocity(state, fighter, false);
            if (fighter.vy < 0.0f) {
                fighter.onGround = false;
                fighter.stateType = 'A';
            }
        }
    }

    for (const auto& hitFallDamage : stateDef->hitFallDamages) {
        if (!shouldRunStateRuntimeController(state, fighter, hitFallDamage.id, hitFallDamage.trigger, opponent, stage)) {
            continue;
        }
        if (fighter.hitFall && fighter.hitFallDamage > 0) {
            fighter.life = std::max(0, fighter.life - fighter.hitFallDamage);
            fighter.hitFallDamage = 0;
        }
    }

    for (const auto& hitFallVel : stateDef->hitFallVels) {
        if (!shouldRunStateRuntimeController(state, fighter, hitFallVel.id, hitFallVel.trigger, opponent, stage)) {
            continue;
        }
        if (fighter.hitFall) {
            fighter.vx = fighter.hitFallBounceXVelocity;
            fighter.vy = fighter.hitFallBounceYVelocity;
            clampArenaAppliedFallVelocity(state, fighter, true);
            if (fighter.vy < 0.0f) {
                fighter.onGround = false;
                fighter.stateType = 'A';
            }
        }
    }

    for (const auto& hitFallSet : stateDef->hitFallSets) {
        if (!shouldRunStateRuntimeController(state, fighter, hitFallSet.id, hitFallSet.trigger, opponent, stage)) {
            continue;
        }
        if (hitFallSet.value == 0) {
            fighter.hitFall = false;
        } else if (hitFallSet.value == 1) {
            fighter.hitFall = true;
        }
        if (hitFallSet.hasXVelocity) {
            fighter.hitFallBounceXVelocity = hitFallSet.xVelocity * static_cast<float>(fighter.facing);
        }
        if (hitFallSet.hasYVelocity) {
            fighter.hitFallBounceYVelocity = hitFallSet.yVelocity;
            clampArenaHitFallRuntime(state, fighter);
        }
    }

    for (const auto& protection : stateDef->hitProtections) {
        if (!shouldRunStateRuntimeController(state, fighter, protection.id, protection.trigger, opponent, stage)) {
            continue;
        }
        if (protection.notHitBy) {
            fighter.notHitByTicks = std::max(fighter.notHitByTicks, protection.time);
            fighter.notHitByValue = protection.value;
        } else {
            fighter.hitByTicks = std::max(fighter.hitByTicks, protection.time);
            fighter.hitByValue = protection.value;
        }
    }

    for (const auto& typeSet : stateDef->stateTypeSets) {
        if (!shouldRunStateRuntimeController(state, fighter, typeSet.id, typeSet.trigger, opponent, stage)) {
            continue;
        }
        if (typeSet.hasStateType) {
            fighter.stateType = typeSet.stateType;
            fighter.onGround = fighter.stateType != 'A';
        }
        if (typeSet.hasMoveType) {
            fighter.moveType = typeSet.moveType;
        }
        if (typeSet.hasPhysics) {
            fighter.physics = typeSet.physics;
        }
    }

    updateStateVelocityControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    updateStatePosSetControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    for (const auto& screenBound : stateDef->screenBounds) {
        if (!shouldRunStateRuntimeController(state, fighter, screenBound.id, screenBound.trigger, opponent, stage)) {
            continue;
        }
        fighter.screenBound = screenBound.value;
        fighter.screenBoundMoveCameraX = screenBound.moveCameraX;
        fighter.screenBoundMoveCameraY = screenBound.moveCameraY;
    }

    for (const auto& width : stateDef->widths) {
        if (!shouldRunStateRuntimeController(state, fighter, width.id, width.trigger, opponent, stage)) {
            continue;
        }
        if (width.hasEdge) {
            fighter.edgeWidthFront = width.edgeFront;
            fighter.edgeWidthBack = width.edgeBack;
        }
        if (width.hasPlayer) {
            fighter.playerWidthFront = width.playerFront;
            fighter.playerWidthBack = width.playerBack;
        }
    }

    for (const auto& playerPush : stateDef->playerPushes) {
        if (!shouldRunStateRuntimeController(state, fighter, playerPush.id, playerPush.trigger, opponent, stage)) {
            continue;
        }
        fighter.playerPush = playerPush.value;
    }
        return true;
    });
}

FighterState* opponentForActor(AppState& state, const FighterState& actor) {
    int ownerIndex = actor.helper ? actor.ownerIndex : fighterIndexInState(state, actor);
    if (ownerIndex < 0 || state.fighters.size() < 2) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(ownerIndex == 0 ? 1 : 0)];
}

const FighterState* opponentForActor(const AppState& state, const FighterState& actor) {
    int ownerIndex = actor.helper ? actor.ownerIndex : fighterIndexInState(state, actor);
    if (ownerIndex < 0 || state.fighters.size() < 2) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(ownerIndex == 0 ? 1 : 0)];
}

void spawnStateHelper(AppState& state, const FighterState& owner, const StateHelperController& controller) {
    const int ownerIndex = fighterIndexInState(state, owner);
    if (ownerIndex < 0 || !findStateDefinition(state, controller.stateNo)) {
        return;
    }

    const FighterState* opponent = opponentForActor(state, owner);
    float baseX = owner.x;
    float baseY = owner.y;
    float offsetFacing = static_cast<float>(owner.facing);
    if (controller.postype == "p2" && opponent) {
        baseX = opponent->x;
        baseY = opponent->y;
        offsetFacing = static_cast<float>(opponent->facing);
    }

    FighterState helper;
    helper.helper = true;
    helper.ownerIndex = ownerIndex;
    helper.helperId = controller.helperId;
    helper.x = baseX + controller.x * offsetFacing;
    helper.y = baseY + controller.y;
    helper.facing = controller.facing == 0 ? owner.facing : (controller.facing > 0 ? owner.facing : -owner.facing);
    helper.onGround = helper.y >= 0.0f;
    helper.life = 1000;
    helper.sprPriority = controller.sprPriority;
    enterState(state, helper, controller.stateNo);
    state.helpers.push_back(std::move(helper));
}

int rootOwnerIndexInState(const AppState& state, const FighterState& actor) {
    if (actor.helper) {
        return actor.ownerIndex;
    }
    return fighterIndexInState(state, actor);
}

void spawnStateProjectile(
    AppState& state,
    const FighterState& owner,
    const FighterState* opponent,
    const StageSlot* stage,
    const StateProjectileController& controller) {
    const int ownerIndex = rootOwnerIndexInState(state, owner);
    if (ownerIndex < 0 || ownerIndex >= static_cast<int>(state.fighters.size()) || !findExactClip(state, controller.anim)) {
        return;
    }

    const auto evalFloat = [&](const std::string& expression, float fallback) {
        if (trim(expression).empty()) {
            return fallback;
        }
        if (const auto value = evalMugenExpression(state, owner, expression, opponent, stage)) {
            return *value;
        }
        return fallback;
    };
    const float offsetX = evalFloat(controller.xExpression, controller.x);
    const float offsetY = evalFloat(controller.yExpression, controller.y);
    const std::string postype = lowercaseCopy(controller.postype);
    float baseX = owner.x;
    float baseY = owner.y;
    float offsetFacing = static_cast<float>(owner.facing);
    if (postype == "p2" && opponent) {
        baseX = opponent->x;
        baseY = opponent->y;
        offsetFacing = static_cast<float>(opponent->facing);
    } else if (postype == "front" || postype == "back") {
        const float visibleLeft = state.cameraX - logicalWidthF(state) / 2.0f;
        const float visibleRight = state.cameraX + logicalWidthF(state) / 2.0f;
        const bool useFrontEdge = postype == "front";
        const bool edgeIsRight = useFrontEdge ? owner.facing > 0 : owner.facing < 0;
        baseX = edgeIsRight ? visibleRight : visibleLeft;
        baseY = owner.y;
        offsetFacing = edgeIsRight ? 1.0f : -1.0f;
    } else if (postype == "left") {
        baseX = state.cameraX - logicalWidthF(state) / 2.0f;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    } else if (postype == "right") {
        baseX = state.cameraX + logicalWidthF(state) / 2.0f;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    }

    RuntimeProjectile projectile;
    projectile.id = controller.projectileId;
    projectile.ownerIndex = ownerIndex;
    projectile.action = controller.anim;
    projectile.hitAction = controller.hitAnim;
    projectile.removeAction = controller.removeAnim;
    projectile.cancelAction = controller.cancelAnim;
    projectile.x = baseX + offsetX * offsetFacing;
    projectile.y = baseY + offsetY;
    projectile.vx = evalFloat(controller.vxExpression, controller.vx);
    projectile.vy = evalFloat(controller.vyExpression, controller.vy);
    projectile.facing = owner.facing;
    projectile.hitsRemaining = std::max(1, controller.hits);
    projectile.removeTime = controller.removeTime;
    projectile.missTime = controller.missTime;
    projectile.removeWhenHit = controller.removeWhenHit;
    projectile.priority = controller.priority;
    projectile.cancelPriority = controller.cancelPriority;
    projectile.pauseMoveTime = controller.pauseMoveTime;
    projectile.superMoveTime = controller.superMoveTime;
    projectile.projEdgeBound = controller.projEdgeBound;
    projectile.projStageBound = controller.projStageBound;
    projectile.projHeightBoundLow = controller.projHeightBoundLow;
    projectile.projHeightBoundHigh = controller.projHeightBoundHigh;
    projectile.ax = evalFloat(controller.axExpression, controller.ax);
    projectile.ay = evalFloat(controller.ayExpression, controller.ay);
    projectile.velMulX = evalFloat(controller.velMulXExpression, controller.velMulX);
    projectile.velMulY = evalFloat(controller.velMulYExpression, controller.velMulY);
    projectile.removeVx = evalFloat(controller.removeVxExpression, controller.removeVx);
    projectile.removeVy = evalFloat(controller.removeVyExpression, controller.removeVy);
    projectile.scaleX = evalFloat(controller.scaleXExpression, controller.scaleX);
    projectile.scaleY = evalFloat(controller.scaleYExpression, controller.scaleY);
    projectile.shadowEnabled = controller.shadowEnabled;
    projectile.shadowR = controller.shadowR;
    projectile.shadowG = controller.shadowG;
    projectile.shadowB = controller.shadowB;
    projectile.hitDef = controller.hitDef;
    state.projectiles.push_back(std::move(projectile));
}

void applyRootBinding(AppState& state, FighterState& helper) {
    if (!helper.helper || helper.rootBindTicks <= 0) {
        return;
    }
    FighterState* root = fighterOwner(state, helper);
    if (!root) {
        helper.rootBindTicks = 0;
        return;
    }

    helper.x = root->x + helper.rootBindOffsetX * static_cast<float>(root->facing);
    helper.y = root->y + helper.rootBindOffsetY;
    if (helper.rootBindFacing > 0) {
        helper.facing = root->facing;
    } else if (helper.rootBindFacing < 0) {
        helper.facing = -root->facing;
    }
    --helper.rootBindTicks;
}

void updateStateProjectileControllers(AppState& state, FighterState& fighter, const FighterState* opponent, const StageSlot* stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& projectile : stateDef.projectiles) {
            if (!shouldRunStateRuntimeController(state, fighter, projectile.id, projectile.trigger, opponent, stage)) {
                continue;
            }
            spawnStateProjectile(state, fighter, opponent, stage, projectile);
        }
        return true;
    });
}

void updateStateHelperControllers(AppState& state, FighterState& fighter, const FighterState* opponent, const StageSlot* stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& helper : stateDef.helpers) {
            if (!shouldRunStateRuntimeController(state, fighter, helper.id, helper.trigger, opponent, stage)) {
                continue;
            }
            spawnStateHelper(state, fighter, helper);
        }

    for (const auto& bind : stateDef.bindToParents) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, bind.id, bind.trigger, opponent, stage)) {
            continue;
        }
        if (const FighterState* owner = fighterOwner(state, fighter)) {
            fighter.x = owner->x + bind.x * static_cast<float>(owner->facing);
            fighter.y = owner->y + bind.y;
            fighter.facing = owner->facing;
        }
    }

    for (const auto& bind : stateDef.bindToRoots) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, bind.id, bind.trigger, opponent, stage)) {
            continue;
        }
        fighter.rootBindTicks = std::max(1, bind.time);
        fighter.rootBindOffsetX = bind.x;
        fighter.rootBindOffsetY = bind.y;
        fighter.rootBindFacing = bind.facing;
        applyRootBinding(state, fighter);
    }

    for (const auto& parentVarAdd : stateDef.parentVarAdds) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, parentVarAdd.id, parentVarAdd.trigger, opponent, stage)) {
            continue;
        }
        FighterState* owner = fighterOwner(state, fighter);
        if (!owner) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, parentVarAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        setFighterVariableValue(*owner, parentVarAdd.target, fighterVariableValue(*owner, parentVarAdd.target) + *value);
    }

    for (const auto& destroySelf : stateDef.destroySelfs) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, destroySelf.id, destroySelf.trigger, opponent, stage)) {
            continue;
        }
        fighter.destroyRequested = true;
        return false;
    }
        return true;
    });
}

void spawnStateExplod(
    AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const StageSlot& stage,
    const StateExplodController& explod) {
    const AnimationClip* clip = explod.fromFightFx ? findFightFxClip(state, explod.anim) : findExactClip(state, explod.anim);
    if (!clip) {
        return;
    }

    float baseX = fighter.x;
    float baseY = fighter.y;
    float offsetFacing = static_cast<float>(fighter.facing);
    if (explod.postype == "p2" && opponent) {
        baseX = opponent->x;
        baseY = opponent->y;
        offsetFacing = static_cast<float>(opponent->facing);
    } else if (explod.postype == "front") {
        const float halfWidth = logicalWidthF(state) * 0.5f;
        baseX = fighter.facing >= 0 ? state.cameraX + halfWidth : state.cameraX - halfWidth;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    } else if (explod.postype == "back") {
        const float halfWidth = logicalWidthF(state) * 0.5f;
        baseX = fighter.facing >= 0 ? state.cameraX - halfWidth : state.cameraX + halfWidth;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    }

    RuntimeEffect effect;
    effect.id = explod.explodId;
    effect.ownerIndex = fighterIndexInState(state, fighter);
    effect.action = explod.anim;
    effect.x = std::clamp(baseX + explod.x * offsetFacing, stage.leftbound, stage.rightbound);
    effect.y = baseY + explod.y;
    effect.bindOffsetX = explod.x;
    effect.bindOffsetY = explod.y;
    effect.bindTicks = explod.bindTime;
    effect.removeTime = explod.removeTime;
    effect.fromFightFx = explod.fromFightFx;
    effect.sprPriority = explod.sprPriority;
    effect.scaleX = explod.scaleX;
    effect.scaleY = explod.scaleY;
    state.runtimeEffects.push_back(effect);
}

void spawnGameMakeAnim(
    AppState& state,
    const FighterState& fighter,
    const StageSlot& stage,
    const StateGameMakeAnimController& gameMakeAnim) {
    const auto value = evalMugenExpression(state, fighter, gameMakeAnim.valueExpression, nullptr, &stage);
    if (!value) {
        return;
    }
    const int action = std::max(0, static_cast<int>(std::lround(*value)));
    if (!findFightFxClip(state, action)) {
        return;
    }

    float randomX = 0.0f;
    float randomY = 0.0f;
    if (gameMakeAnim.random > 0) {
        const float spread = static_cast<float>(gameMakeAnim.random) * 0.5f;
        randomX = (static_cast<float>(pseudoMugenRandom(state, fighter, gameMakeAnim.id)) / 999.0f - 0.5f) * static_cast<float>(gameMakeAnim.random);
        randomY = (static_cast<float>(pseudoMugenRandom(state, fighter, gameMakeAnim.id + 7919)) / 999.0f - 0.5f) * static_cast<float>(gameMakeAnim.random);
        randomX = std::clamp(randomX, -spread, spread);
        randomY = std::clamp(randomY, -spread, spread);
    }

    RuntimeEffect effect;
    effect.ownerIndex = rootOwnerIndexInState(state, fighter);
    effect.action = action;
    effect.fromFightFx = true;
    effect.x = std::clamp(
        fighter.x + (gameMakeAnim.x + randomX) * static_cast<float>(fighter.facing),
        stage.leftbound,
        stage.rightbound);
    effect.y = fighter.y + gameMakeAnim.y + randomY;
    effect.removeTime = -2;
    effect.sprPriority = gameMakeAnim.under ? -3 : 3;
    state.runtimeEffects.push_back(effect);
}

void spawnDustCloud(AppState& state, const FighterState& fighter, const StageSlot& stage, float x, float y) {
    constexpr int kSmallFloorDustAction = 120;
    if (!findFightFxClip(state, kSmallFloorDustAction)) {
        return;
    }

    RuntimeEffect effect;
    effect.action = kSmallFloorDustAction;
    effect.fromFightFx = true;
    effect.ownerIndex = rootOwnerIndexInState(state, fighter);
    effect.x = std::clamp(fighter.x + x * static_cast<float>(fighter.facing), stage.leftbound, stage.rightbound);
    effect.y = fighter.y + y;
    effect.removeTime = -2;
    effect.sprPriority = 3;
    state.runtimeEffects.push_back(effect);
}

void updateStateMakeDustControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot& stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& makeDust : stateDef.makeDusts) {
            if (!shouldRunStateRuntimeController(state, fighter, makeDust.id, makeDust.trigger, opponent, &stage)) {
                continue;
            }
            if (fighter.stateTime % std::max(1, makeDust.spacing) != 0) {
                continue;
            }
            spawnDustCloud(state, fighter, stage, makeDust.x, makeDust.y);
            if (makeDust.hasPos2) {
                spawnDustCloud(state, fighter, stage, makeDust.x2, makeDust.y2);
            }
        }
        return true;
    });
}

bool runtimeEffectMatchesOwnerAndId(const RuntimeEffect& effect, int ownerIndex, int explodId) {
    if (effect.ownerIndex != ownerIndex) {
        return false;
    }
    return explodId < 0 || effect.id == explodId;
}

void modifyRuntimeExplods(AppState& state, const FighterState& fighter, const StateModifyExplodController& modifyExplod) {
    const int ownerIndex = fighterIndexInState(state, fighter);
    for (auto& effect : state.runtimeEffects) {
        if (!runtimeEffectMatchesOwnerAndId(effect, ownerIndex, modifyExplod.explodId)) {
            continue;
        }
        if (modifyExplod.hasSprPriority) {
            effect.sprPriority = modifyExplod.sprPriority;
        }
        if (modifyExplod.hasScale) {
            effect.scaleX = modifyExplod.scaleX;
            effect.scaleY = modifyExplod.scaleY;
        }
    }
}

void removeRuntimeExplods(AppState& state, const FighterState& fighter, const StateRemoveExplodController& removeExplod) {
    const int ownerIndex = fighterIndexInState(state, fighter);
    state.runtimeEffects.erase(
        std::remove_if(state.runtimeEffects.begin(), state.runtimeEffects.end(), [ownerIndex, &removeExplod](const RuntimeEffect& effect) {
            return runtimeEffectMatchesOwnerAndId(effect, ownerIndex, removeExplod.explodId);
        }),
        state.runtimeEffects.end());
}

void updateStateExplodControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot& stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& explod : stateDef.explods) {
            if (!shouldRunStateRuntimeController(state, fighter, explod.id, explod.trigger, opponent, &stage)) {
                continue;
            }
            spawnStateExplod(state, fighter, opponent, stage, explod);
        }

    for (const auto& gameMakeAnim : stateDef.gameMakeAnims) {
        if (!shouldRunStateRuntimeController(state, fighter, gameMakeAnim.id, gameMakeAnim.trigger, opponent, &stage)) {
            continue;
        }
        spawnGameMakeAnim(state, fighter, stage, gameMakeAnim);
    }

    for (const auto& modifyExplod : stateDef.modifyExplods) {
        if (!shouldRunStateRuntimeController(state, fighter, modifyExplod.id, modifyExplod.trigger, opponent, &stage)) {
            continue;
        }
        modifyRuntimeExplods(state, fighter, modifyExplod);
    }

    for (const auto& removeExplod : stateDef.removeExplods) {
        if (!shouldRunStateRuntimeController(state, fighter, removeExplod.id, removeExplod.trigger, opponent, &stage)) {
            continue;
        }
        removeRuntimeExplods(state, fighter, removeExplod);
    }
        return true;
    });
}

bool updateStateChangeStateControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    bool changed = false;
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& changeState : stateDef.changeStates) {
            if (!shouldRunStateRuntimeController(state, fighter, changeState.id, changeState.trigger, opponent, stage)) {
                continue;
            }
            std::optional<bool> ctrl;
            if (changeState.hasCtrl) {
                ctrl = changeState.ctrl;
            }
            const auto targetState = evalMugenExpression(
                state,
                fighter,
                changeState.targetStateExpression.empty()
                    ? std::to_string(changeState.targetState)
                    : changeState.targetStateExpression,
                opponent,
                stage);
            if (!targetState) {
                continue;
            }
            applyParsedChangeState(state, fighter, static_cast<int>(std::lround(*targetState)), ctrl);
            changed = true;
            return false;
        }
        return true;
    });
    return changed;
}

void resetFighterOneTickBounds(AppState& state) {
    auto resetActor = [](FighterState& fighter) {
        fighter.transEffect = {};
        fighter.angleDrawActive = false;
        fighter.displayOffsetX = 0.0f;
        fighter.displayOffsetY = 0.0f;
        if (fighter.projectileHitTicks > 0) {
            --fighter.projectileHitTicks;
            if (fighter.projectileHitTicks <= 0) {
                fighter.projectileHitId = -1;
            }
        }
        if (fighter.projectileContactTicks > 0) {
            --fighter.projectileContactTicks;
            if (fighter.projectileContactTicks <= 0) {
                fighter.projectileContactId = -1;
            }
        }
        if (fighter.projectileGuardedTicks > 0) {
            --fighter.projectileGuardedTicks;
            if (fighter.projectileGuardedTicks <= 0) {
                fighter.projectileGuardedId = -1;
            }
        }
        fighter.screenBound = true;
        fighter.screenBoundMoveCameraX = false;
        fighter.screenBoundMoveCameraY = false;
        fighter.playerPush = true;
        fighter.posFreezeX = false;
        fighter.posFreezeY = false;
        fighter.edgeWidthFront = -1.0f;
        fighter.edgeWidthBack = -1.0f;
        fighter.playerWidthFront = -1.0f;
        fighter.playerWidthBack = -1.0f;
        if (fighter.notHitByTicks > 0) {
            --fighter.notHitByTicks;
            if (fighter.notHitByTicks <= 0) {
                fighter.notHitByValue.clear();
            }
        }
        if (fighter.hitByTicks > 0) {
            --fighter.hitByTicks;
            if (fighter.hitByTicks <= 0) {
                fighter.hitByValue.clear();
            }
        }
        if (fighter.targetTicks > 0) {
            --fighter.targetTicks;
            if (fighter.targetTicks <= 0) {
                fighter.targetIndex = -1;
                fighter.targetHitId = -1;
            }
        }
        if (fighter.bindTicks > 0) {
            --fighter.bindTicks;
            if (fighter.bindTicks <= 0) {
                fighter.boundByIndex = -1;
            }
        }
    };

    for (auto& fighter : state.fighters) {
        resetActor(fighter);
    }
    for (auto& helper : state.helpers) {
        resetActor(helper);
    }
}

float fighterBaseFrontWidth(const AppState& state, const FighterState& fighter) {
    return fighter.onGround ? state.characterConstants.size.groundFront : state.characterConstants.size.airFront;
}

float fighterBaseBackWidth(const AppState& state, const FighterState& fighter) {
    return fighter.onGround ? state.characterConstants.size.groundBack : state.characterConstants.size.airBack;
}

float fighterPlayerFrontWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.playerWidthFront >= 0.0f ? fighter.playerWidthFront : fighterBaseFrontWidth(state, fighter));
}

float fighterPlayerBackWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.playerWidthBack >= 0.0f ? fighter.playerWidthBack : fighterBaseBackWidth(state, fighter));
}

float fighterEdgeFrontWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.edgeWidthFront >= 0.0f ? fighter.edgeWidthFront : fighterBaseFrontWidth(state, fighter));
}

float fighterEdgeBackWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.edgeWidthBack >= 0.0f ? fighter.edgeWidthBack : fighterBaseBackWidth(state, fighter));
}

float fighterWidthTowardDirection(float frontWidth, float backWidth, const FighterState& fighter, float direction) {
    const bool directionIsFront = direction * static_cast<float>(fighter.facing) > 0.0f;
    return directionIsFront ? frontWidth : backWidth;
}

float fighterPlayerWidthToward(const AppState& state, const FighterState& fighter, float direction) {
    return fighterWidthTowardDirection(
        fighterPlayerFrontWidth(state, fighter),
        fighterPlayerBackWidth(state, fighter),
        fighter,
        direction);
}

float fighterEdgeWidthToward(const AppState& state, const FighterState& fighter, float direction) {
    return fighterWidthTowardDirection(
        fighterEdgeFrontWidth(state, fighter),
        fighterEdgeBackWidth(state, fighter),
        fighter,
        direction);
}

float clampFighterOriginToStage(float x, const StageSlot& stage) {
    return std::clamp(x, stage.leftbound, stage.rightbound);
}

void applyScreenBounds(AppState& state, const StageSlot& stage) {
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float visibleLeft = state.cameraX - halfWidth;
    const float visibleRight = state.cameraX + halfWidth;

    for (auto& fighter : state.fighters) {
        if (!fighter.screenBound) {
            continue;
        }

        const float widthLeft = fighterEdgeWidthToward(state, fighter, -1.0f);
        const float widthRight = fighterEdgeWidthToward(state, fighter, 1.0f);
        const float minX = std::max(stage.leftbound, visibleLeft + widthLeft - stage.screenleft);
        const float maxX = std::min(stage.rightbound, visibleRight - widthRight + stage.screenright);
        if (minX > maxX) {
            continue;
        }
        fighter.x = std::clamp(fighter.x, minX, maxX);
    }
}

int airMovementBaseAction(const FighterState& fighter) {
    if (fighter.jumpBaseAction >= 41 && fighter.jumpBaseAction <= 43) {
        return fighter.jumpBaseAction;
    }

    const float directionalVelocity = fighter.vx * static_cast<float>(fighter.facing);
    if (directionalVelocity > 0.05f) {
        return 42;
    }
    if (directionalVelocity < -0.05f) {
        return 43;
    }
    return 41;
}

int chooseMovementAction(const AppState& state, const FighterState& fighter) {
    if (!fighter.onGround) {
        const int baseAction = airMovementBaseAction(fighter);
        const int peakAction = baseAction + 3;
        if (fighter.jumpPeakActionApplied && findExactClip(state, peakAction)) {
            return peakAction;
        }
        return baseAction;
    }

    if (std::fabs(fighter.vx) > 0.05f) {
        return fighter.vx * static_cast<float>(fighter.facing) > 0.0f ? 20 : 21;
    }
    return 0;
}

void updateFighterFacing(AppState& state) {
    state.fighters[0].facing = state.fighters[0].x <= state.fighters[1].x ? 1 : -1;
    state.fighters[1].facing = -state.fighters[0].facing;
}

void updateArenaFighterFacing(AppState& state) {
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (state.fighters[i].life <= 0) {
            continue;
        }
        const int target = nearestLivingEnemyIndex(state, static_cast<int>(i));
        if (target >= 0) {
            state.fighters[i].facing = state.fighters[i].x <= state.fighters[static_cast<size_t>(target)].x ? 1 : -1;
        }
    }
}

void updateFighterPhysics(const AppState& state, FighterState& fighter, const StageSlot& stage) {
    if (!fighter.posFreezeX) {
        fighter.x = std::clamp(fighter.x + fighter.vx, stage.leftbound, stage.rightbound);
    }
    if (!fighter.posFreezeY) {
        fighter.y += fighter.vy;
    }
    if (!fighter.posFreezeY && !fighter.onGround && fighter.physics == 'A') {
        fighter.vy += fighter.hitFall && fighter.hitFallYAccel > 0.0f
            ? fighter.hitFallYAccel
            : state.characterConstants.movementYAccel;
    }
    if (fighter.physics != 'N' && fighter.y >= 0.0f) {
        const bool shouldUseCommonLanding =
            !fighter.helper
            && fighter.stateType == 'A'
            && fighter.physics == 'A'
            && fighter.moveType != 'H';
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        fighter.jumpBaseAction = 0;
        fighter.jumpPeakActionApplied = false;
        if (shouldUseCommonLanding) {
            enterCommonLandingState(state, fighter);
            return;
        }
    }
}

void applyPlayerPush(AppState& state, const StageSlot& stage) {
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    if (!p1.onGround
        || !p2.onGround
        || !p1.playerPush
        || !p2.playerPush
        || p1.life <= 0
        || p2.life <= 0) {
        return;
    }

    float delta = p2.x - p1.x;
    const float distance = std::fabs(delta);
    if (distance < 0.001f) {
        delta = p1.facing >= 0 ? 1.0f : -1.0f;
    }
    const float direction = delta >= 0.0f ? 1.0f : -1.0f;
    const float minSeparation =
        fighterPlayerWidthToward(state, p1, direction)
        + fighterPlayerWidthToward(state, p2, -direction);
    if (distance >= minSeparation) {
        return;
    }

    const float overlap = minSeparation - distance;

    if (state.frontend.pendingMode == PendingMode::Training && activeOpponentType(state) == OpponentType::Dummy) {
        p1.x = clampFighterOriginToStage(p1.x - direction * overlap, stage);
        return;
    }

    p1.x = clampFighterOriginToStage(p1.x - direction * (overlap * 0.5f), stage);
    p2.x = clampFighterOriginToStage(p2.x + direction * (overlap * 0.5f), stage);

    const float adjustedDelta = p2.x - p1.x;
    const float adjustedDistance = std::fabs(adjustedDelta);
    if (adjustedDistance >= minSeparation) {
        return;
    }

    const float remaining = minSeparation - adjustedDistance;
    if (p1.x <= stage.leftbound + 0.001f) {
        p2.x = clampFighterOriginToStage(p2.x + direction * remaining, stage);
    } else if (p2.x >= stage.rightbound - 0.001f) {
        p1.x = clampFighterOriginToStage(p1.x - direction * remaining, stage);
    }
}

void applyArenaPlayerPush(AppState& state, const StageSlot& stage) {
    for (size_t lhs = 0; lhs < state.fighters.size(); ++lhs) {
        auto& p1 = state.fighters[lhs];
        if (!p1.onGround || !p1.playerPush || p1.life <= 0) {
            continue;
        }
        for (size_t rhs = lhs + 1; rhs < state.fighters.size(); ++rhs) {
            auto& p2 = state.fighters[rhs];
            if (!p2.onGround || !p2.playerPush || p2.life <= 0) {
                continue;
            }

            float delta = p2.x - p1.x;
            const float distance = std::fabs(delta);
            if (distance < 0.001f) {
                delta = p1.facing >= 0 ? 1.0f : -1.0f;
            }
            const float direction = delta >= 0.0f ? 1.0f : -1.0f;
            const float minSeparation =
                fighterPlayerWidthToward(state, p1, direction)
                + fighterPlayerWidthToward(state, p2, -direction);
            if (distance >= minSeparation) {
                continue;
            }

            const float overlap = minSeparation - distance;
            p1.x = clampFighterOriginToStage(p1.x - direction * (overlap * 0.5f), stage);
            p2.x = clampFighterOriginToStage(p2.x + direction * (overlap * 0.5f), stage);
        }
    }
}

void updateCamera(AppState& state, const StageSlot& stage) {
    const float minFighterX = std::min(state.fighters[0].x, state.fighters[1].x);
    const float maxFighterX = std::max(state.fighters[0].x, state.fighters[1].x);
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float leftEdge = state.cameraX - halfWidth + stage.cameraTension;
    const float rightEdge = state.cameraX + halfWidth - stage.cameraTension;

    float targetX = state.cameraX;
    if (minFighterX < leftEdge) {
        targetX += minFighterX - leftEdge;
    }
    if (maxFighterX > rightEdge) {
        targetX += maxFighterX - rightEdge;
    }
    state.cameraX = std::clamp(targetX, stage.cameraBoundleft, stage.cameraBoundright);

    const float highestY = std::min(state.fighters[0].y, state.fighters[1].y);
    float targetY = stage.cameraStarty;
    if (highestY < -stage.cameraFloortension) {
        targetY = (highestY + stage.cameraFloortension) * stage.cameraVerticalfollow;
    }
    state.cameraY = std::clamp(targetY, stage.cameraBoundhigh, stage.cameraBoundlow);
}

void updateArenaCamera(AppState& state, const StageSlot& stage) {
    bool any = false;
    float minFighterX = 0.0f;
    float maxFighterX = 0.0f;
    float highestY = 0.0f;
    for (const auto& fighter : state.fighters) {
        if (fighter.life <= 0) {
            continue;
        }
        if (!any) {
            minFighterX = fighter.x;
            maxFighterX = fighter.x;
            highestY = fighter.y;
            any = true;
            continue;
        }
        minFighterX = std::min(minFighterX, fighter.x);
        maxFighterX = std::max(maxFighterX, fighter.x);
        highestY = std::min(highestY, fighter.y);
    }
    if (!any) {
        updateCamera(state, stage);
        return;
    }

    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float leftEdge = state.cameraX - halfWidth + stage.cameraTension;
    const float rightEdge = state.cameraX + halfWidth - stage.cameraTension;

    float targetX = state.cameraX;
    if (minFighterX < leftEdge) {
        targetX += minFighterX - leftEdge;
    }
    if (maxFighterX > rightEdge) {
        targetX += maxFighterX - rightEdge;
    }
    state.cameraX = std::clamp(targetX, stage.cameraBoundleft, stage.cameraBoundright);

    state.cameraY = std::clamp(stage.cameraStarty, stage.cameraBoundhigh, stage.cameraBoundlow);
}

void startEnvShake(AppState& state, const EnvShakeSpec& shake) {
    if (!shake.enabled || shake.time <= 0 || std::abs(shake.amplitude) <= 0.001f) {
        return;
    }
    if (shake.time < state.display.envShakeTicks && std::abs(shake.amplitude) <= std::abs(state.display.envShakeAmplitude)) {
        return;
    }
    state.display.envShakeTicks = shake.time;
    state.display.envShakeTotalTicks = shake.time;
    state.display.envShakeFrequency = std::max(1, shake.frequency);
    state.display.envShakeAmplitude = shake.amplitude;
    state.display.envShakePhase = shake.phase;
}

void startPaletteEffect(ActivePaletteEffect& active, const PaletteEffectSpec& effect) {
    if (!effect.enabled || effect.time == 0) {
        return;
    }
    active.spec = effect;
    active.elapsedTicks = 0;
    active.ticksLeft = effect.time < 0 ? 999999 : std::max(1, effect.time);
}

void updatePaletteEffect(ActivePaletteEffect& active) {
    if (active.ticksLeft <= 0) {
        active = {};
        return;
    }
    ++active.elapsedTicks;
    if (active.spec.time >= 0) {
        --active.ticksLeft;
        if (active.ticksLeft <= 0) {
            active = {};
        }
    }
}

void startEnvColor(AppState& state, const StateEnvColorController& envColor) {
    state.envColor.ticksLeft = std::max(1, envColor.time);
    state.envColor.r = std::clamp(envColor.r, 0, 255);
    state.envColor.g = std::clamp(envColor.g, 0, 255);
    state.envColor.b = std::clamp(envColor.b, 0, 255);
}

void updateEnvColor(AppState& state) {
    if (state.envColor.ticksLeft <= 0) {
        state.envColor = {};
        return;
    }
    --state.envColor.ticksLeft;
    if (state.envColor.ticksLeft <= 0) {
        state.envColor = {};
    }
}

void updateEnvShake(AppState& state) {
    if (state.display.envShakeTicks <= 0) {
        state.display.envShakeTicks = 0;
        state.display.envShakeOffsetY = 0.0f;
        return;
    }

    const int elapsed = std::max(0, state.display.envShakeTotalTicks - state.display.envShakeTicks);
    const float progress = state.display.envShakeTotalTicks > 0
        ? static_cast<float>(state.display.envShakeTicks) / static_cast<float>(state.display.envShakeTotalTicks)
        : 0.0f;
    constexpr float tau = 6.28318530718f;
    const float phase = (static_cast<float>(elapsed + state.display.envShakePhase) * static_cast<float>(state.display.envShakeFrequency) / 60.0f) * tau;
    state.display.envShakeOffsetY = std::sin(phase) * state.display.envShakeAmplitude * progress;
    --state.display.envShakeTicks;
}

std::string soundPairText(int group, int index) {
    if (group < 0 || index < 0) {
        return "-";
    }
    return std::to_string(group) + "," + std::to_string(index);
}

void spawnContactSpark(AppState& state, int action, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& target) {
    if (action < 0 || !findFightFxClip(state, action)) {
        return;
    }

    RuntimeEffect effect;
    effect.action = action;
    effect.x = target.x + (hitDef.sparkX * static_cast<float>(attacker.facing));
    effect.y = attacker.y + hitDef.sparkY;
    state.runtimeEffects.push_back(effect);
}

void spawnHitSpark(AppState& state, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& target) {
    spawnContactSpark(state, hitDef.sparkNo, hitDef, attacker, target);
}

void spawnGuardSpark(AppState& state, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& target) {
    spawnContactSpark(state, hitDef.guardSparkNo, hitDef, attacker, target);
}

void updateRuntimeEffects(AppState& state) {
    updateEnvShake(state);
    updateEnvColor(state);
    updatePaletteEffect(state.backgroundPaletteEffect);
    for (auto& fighter : state.fighters) {
        updatePaletteEffect(fighter.paletteEffect);
    }
    for (auto& effect : state.runtimeEffects) {
        ++effect.animTick;
        ++effect.ageTicks;
        if (effect.bindTicks != 0 && effect.ownerIndex >= 0 && effect.ownerIndex < static_cast<int>(state.fighters.size())) {
            const FighterState& owner = state.fighters[static_cast<size_t>(effect.ownerIndex)];
            effect.x = owner.x + effect.bindOffsetX * static_cast<float>(owner.facing);
            effect.y = owner.y + effect.bindOffsetY;
            if (effect.bindTicks > 0) {
                --effect.bindTicks;
            }
        }
    }
    state.runtimeEffects.erase(
        std::remove_if(state.runtimeEffects.begin(), state.runtimeEffects.end(), [&state](const RuntimeEffect& effect) {
            const AnimationClip* clip = effect.fromFightFx ? findFightFxClip(state, effect.action) : findExactClip(state, effect.action);
            if (!clip) {
                return true;
            }
            if (effect.removeTime >= 0) {
                return effect.ageTicks >= effect.removeTime;
            }
            if (effect.removeTime == -1) {
                return false;
            }
            return effect.animTick >= clip->loopTicks;
        }),
        state.runtimeEffects.end());
}

const AnimationFrame* currentFrameForFighter(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findClip(state, fighter.action);
    return clip ? frameForClip(*clip, fighter.animTick) : nullptr;
}

int currentAnimElemForFighter(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findClip(state, fighter.action);
    return clip ? frameIndexForClip(*clip, fighter.animTick) + 1 : 0;
}

int animElemTimeForFighter(const AppState& state, const FighterState& fighter, int elem) {
    const AnimationClip* clip = findClip(state, fighter.action);
    return clip ? animElemTimeForClip(*clip, fighter.animTick, elem) : 0;
}

int hitDefApplicationKey(int hitDefId, int animElem) {
    return hitDefId * 100000 + std::max(0, animElem);
}

bool hitDefAlreadyApplied(const FighterState& fighter, int hitDefId, int animElem) {
    const int key = hitDefApplicationKey(hitDefId, animElem);
    return std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), hitDefId)
        != fighter.appliedHitDefIds.end()
        || std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), key)
        != fighter.appliedHitDefIds.end();
}

void markHitDefApplied(FighterState& fighter, int hitDefId, int animElem) {
    const int key = hitDefApplicationKey(hitDefId, animElem);
    if (std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), key)
        == fighter.appliedHitDefIds.end()) {
        fighter.appliedHitDefIds.push_back(key);
    }
}

float p2BodyDistXValue(const FighterState& attacker, const FighterState& defender) {
    return std::max(0.0f, (defender.x - attacker.x) * static_cast<float>(attacker.facing));
}

char hitDefStateAttr(const HitDefinition& hitDef) {
    const auto parts = splitCsv(hitDef.attr);
    if (parts.empty()) {
        return 'S';
    }
    const std::string statePart = uppercaseCopy(trim(parts.front()));
    for (const char ch : statePart) {
        if (ch == 'S' || ch == 'C' || ch == 'A') {
            return ch;
        }
    }
    return 'S';
}

std::vector<std::string> hitDefAttackAttrs(const HitDefinition& hitDef) {
    const auto parts = splitCsv(hitDef.attr);
    std::vector<std::string> attrs;
    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string attr = uppercaseCopy(trim(parts[i]));
        if (!attr.empty()) {
            attrs.push_back(attr);
        }
    }
    return attrs;
}

bool hitProtectionMatches(const std::string& protection, const HitDefinition& hitDef) {
    const auto parts = splitCsv(protection);
    if (parts.empty()) {
        return false;
    }

    const std::string states = uppercaseCopy(trim(parts.front()));
    const char hitState = hitDefStateAttr(hitDef);
    if (!states.empty() && states.find(hitState) == std::string::npos) {
        return false;
    }

    std::vector<std::string> allowedAttrs;
    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string attr = uppercaseCopy(trim(parts[i]));
        if (!attr.empty()) {
            allowedAttrs.push_back(attr);
        }
    }
    if (allowedAttrs.empty()) {
        return true;
    }

    const auto hitAttrs = hitDefAttackAttrs(hitDef);
    if (hitAttrs.empty()) {
        return false;
    }
    for (const auto& hitAttr : hitAttrs) {
        for (const auto& allowed : allowedAttrs) {
            if (hitAttr == allowed) {
                return true;
            }
        }
    }
    return false;
}

bool defenderCanBeHitBy(const FighterState& defender, const HitDefinition& hitDef) {
    if (defender.notHitByTicks > 0 && hitProtectionMatches(defender.notHitByValue, hitDef)) {
        return false;
    }
    if (defender.hitByTicks > 0 && !hitProtectionMatches(defender.hitByValue, hitDef)) {
        return false;
    }
    return true;
}

const StateHitOverrideController* activeHitOverrideForDefender(
    AppState& state,
    FighterState& defender,
    const FighterState* attacker,
    const HitDefinition& hitDef) {
    const StateDefinition* stateDef = findStateDefinition(state, defender.stateNo);
    if (!stateDef) {
        return nullptr;
    }

    for (const auto& hitOverride : stateDef->hitOverrides) {
        if (!findStateDefinition(state, hitOverride.stateNo)
            || !hitProtectionMatches(hitOverride.attr, hitDef)
            || !shouldRunStateRuntimeController(state, defender, hitOverride.id, hitOverride.trigger, attacker, nullptr)) {
            continue;
        }
        return &hitOverride;
    }
    return nullptr;
}

bool hitDefTriggerIsActive(const HitDefinition& hitDef, const FighterState& attacker, const FighterState& defender, int animElem) {
    bool hasTrigger = false;
    bool active = false;
    if (hitDef.triggerTime >= 0) {
        hasTrigger = true;
        active = active || attacker.stateTime >= hitDef.triggerTime;
    }
    if (hitDef.triggerAnimElem > 0) {
        hasTrigger = true;
        active = active || animElem >= hitDef.triggerAnimElem;
    }
    if (hasTrigger && !active) {
        return false;
    }
    if (hitDef.hasP2BodyDistX
        && !compareFloatValue(p2BodyDistXValue(attacker, defender), hitDef.p2BodyDistXOp, hitDef.p2BodyDistX)) {
        return false;
    }
    return true;
}

int hitDefTriggerScore(const HitDefinition& hitDef) {
    if (hitDef.triggerAnimElem > 0) {
        return 10000 + hitDef.triggerAnimElem;
    }
    if (hitDef.triggerTime >= 0) {
        return hitDef.triggerTime;
    }
    return 0;
}

const HitDefinition* findActiveHitDefinition(const AppState& state, const FighterState& attacker, const FighterState& defender) {
    const int animElem = currentAnimElemForFighter(state, attacker);
    const HitDefinition* best = nullptr;
    int bestScore = -1;
    for (const auto& hitDef : state.hitDefs) {
        if (hitDef.stateNo != attacker.stateNo || hitDefAlreadyApplied(attacker, hitDef.id, animElem)) {
            continue;
        }
        if (!defenderCanBeHitBy(defender, hitDef)) {
            continue;
        }
        if (!hitFlagAllowsDefender(hitDef, defender)) {
            continue;
        }
        if (!hitDefTriggerIsActive(hitDef, attacker, defender, animElem)) {
            continue;
        }
        const int score = hitDefTriggerScore(hitDef);
        if (!best || score >= bestScore) {
            best = &hitDef;
            bestScore = score;
        }
    }
    return best;
}

HitDefinition resolveHitDefinitionExpressions(
    const AppState& state,
    const HitDefinition& source,
    const FighterState& attacker,
    const FighterState& defender,
    const StageSlot* stage) {
    HitDefinition resolved = source;

    const auto evalFloat = [&](const std::string& expression, float fallback) {
        if (trim(expression).empty()) {
            return fallback;
        }
        if (const auto value = evalMugenExpression(state, attacker, expression, &defender, stage)) {
            return *value;
        }
        return fallback;
    };
    const auto evalInt = [&](const std::string& expression, int fallback) {
        return static_cast<int>(evalFloat(expression, static_cast<float>(fallback)));
    };
    const auto evalBool = [&](const std::string& expression, bool fallback) {
        return evalFloat(expression, fallback ? 1.0f : 0.0f) != 0.0f;
    };
    const auto resolveEnvShake = [&](const EnvShakeSpec& sourceShake) {
        EnvShakeSpec shake = sourceShake;
        shake.time = std::max(0, evalInt(sourceShake.timeExpression, sourceShake.time));
        shake.frequency = std::max(1, evalInt(sourceShake.frequencyExpression, sourceShake.frequency));
        shake.amplitude = evalFloat(sourceShake.amplitudeExpression, sourceShake.amplitude);
        shake.phase = evalInt(sourceShake.phaseExpression, sourceShake.phase);
        shake.enabled = shake.time > 0 && std::abs(shake.amplitude) > 0.001f;
        return shake;
    };
    const auto resolvePalette = [&](const PaletteEffectSpec& sourceEffect) {
        PaletteEffectSpec effect = sourceEffect;
        effect.time = evalInt(sourceEffect.timeExpression, sourceEffect.time);
        effect.addR = evalInt(sourceEffect.addExpressions[0], sourceEffect.addR);
        effect.addG = evalInt(sourceEffect.addExpressions[1], sourceEffect.addG);
        effect.addB = evalInt(sourceEffect.addExpressions[2], sourceEffect.addB);
        effect.mulR = evalInt(sourceEffect.mulExpressions[0], sourceEffect.mulR);
        effect.mulG = evalInt(sourceEffect.mulExpressions[1], sourceEffect.mulG);
        effect.mulB = evalInt(sourceEffect.mulExpressions[2], sourceEffect.mulB);
        effect.sinAddR = evalInt(sourceEffect.sinAddExpressions[0], sourceEffect.sinAddR);
        effect.sinAddG = evalInt(sourceEffect.sinAddExpressions[1], sourceEffect.sinAddG);
        effect.sinAddB = evalInt(sourceEffect.sinAddExpressions[2], sourceEffect.sinAddB);
        effect.sinPeriod = evalInt(sourceEffect.sinAddExpressions[3], sourceEffect.sinPeriod);
        effect.color = evalInt(sourceEffect.colorExpression, sourceEffect.color);
        effect.invertAll = evalBool(sourceEffect.invertAllExpression, sourceEffect.invertAll);
        effect.enabled = effect.time != 0
            && (effect.addR != 0 || effect.addG != 0 || effect.addB != 0
                || effect.mulR != 256 || effect.mulG != 256 || effect.mulB != 256
                || effect.sinAddR != 0 || effect.sinAddG != 0 || effect.sinAddB != 0
                || effect.color != 256 || effect.invertAll);
        return effect;
    };

    resolved.damage = evalInt(source.damageExpression, source.damage);
    resolved.guardDamage = evalInt(source.guardDamageExpression, source.guardDamage);
    resolved.pausetimeP1 = evalInt(source.pausetimeP1Expression, source.pausetimeP1);
    resolved.pausetimeP2 = evalInt(source.pausetimeP2Expression, source.pausetimeP2);
    resolved.sparkNo = evalInt(source.sparkNoExpression, source.sparkNo);
    resolved.guardSparkNo = evalInt(source.guardSparkNoExpression, source.guardSparkNo);
    resolved.sparkX = evalFloat(source.sparkXExpression, source.sparkX);
    resolved.sparkY = evalFloat(source.sparkYExpression, source.sparkY);
    resolved.hitSoundGroup = evalInt(source.hitSoundGroupExpression, source.hitSoundGroup);
    resolved.hitSoundIndex = evalInt(source.hitSoundIndexExpression, source.hitSoundIndex);
    resolved.guardSoundGroup = evalInt(source.guardSoundGroupExpression, source.guardSoundGroup);
    resolved.guardSoundIndex = evalInt(source.guardSoundIndexExpression, source.guardSoundIndex);
    resolved.envShake = resolveEnvShake(source.envShake);
    resolved.fallEnvShake = resolveEnvShake(source.fallEnvShake);
    resolved.palFx = resolvePalette(source.palFx);
    resolved.groundSlideTime = evalInt(source.groundSlideTimeExpression, source.groundSlideTime);
    resolved.groundHitTime = evalInt(source.groundHitTimeExpression, source.groundHitTime);
    resolved.groundVelocityX = evalFloat(source.groundVelocityXExpression, source.groundVelocityX);
    resolved.groundVelocityY = evalFloat(source.groundVelocityYExpression, source.groundVelocityY);
    resolved.hasAirVelocity = source.hasAirVelocity;
    if (source.hasAirVelocity) {
        resolved.airVelocityX = evalFloat(source.airVelocityXExpression, source.airVelocityX);
        resolved.airVelocityY = evalFloat(source.airVelocityYExpression, source.airVelocityY);
    }
    resolved.airHitTime = evalInt(source.airHitTimeExpression, source.airHitTime);
    resolved.fall = evalBool(source.fallExpression, source.fall);
    resolved.airFall = evalBool(source.airFallExpression, source.airFall);
    resolved.fallRecover = evalBool(source.fallRecoverExpression, source.fallRecover);
    resolved.fallRecoverTime = evalInt(source.fallRecoverTimeExpression, source.fallRecoverTime);
    resolved.fallDamage = evalInt(source.fallDamageExpression, source.fallDamage);
    resolved.hasFallXVelocity = source.hasFallXVelocity;
    if (source.hasFallXVelocity) {
        resolved.fallXVelocity = evalFloat(source.fallXVelocityExpression, source.fallXVelocity);
    }
    resolved.hasFallYVelocity = source.hasFallYVelocity;
    if (source.hasFallYVelocity) {
        resolved.fallYVelocity = evalFloat(source.fallYVelocityExpression, source.fallYVelocity);
    }
    resolved.hasYAccel = source.hasYAccel;
    if (source.hasYAccel) {
        resolved.yAccel = evalFloat(source.yAccelExpression, source.yAccel);
    }
    if (source.hasGuardVelocity) {
        resolved.guardVelocityX = evalFloat(source.guardVelocityXExpression, source.guardVelocityX);
        resolved.guardVelocityY = evalFloat(source.guardVelocityYExpression, source.guardVelocityY);
    } else {
        resolved.guardVelocityX = resolved.groundVelocityX;
        resolved.guardVelocityY = resolved.groundVelocityY;
    }

    return resolved;
}

bool targetControllerMatchesStoredTarget(const FighterState& fighter, int controllerTargetId) {
    return controllerTargetId < 0 || controllerTargetId == fighter.targetHitId;
}

void clearFighterTargetLink(FighterState& fighter) {
    fighter.targetIndex = -1;
    fighter.targetHitId = -1;
    fighter.targetTicks = 0;
}

void updateStateTargetControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (fighter.targetIndex < 0 || fighter.targetIndex >= static_cast<int>(state.fighters.size())) {
        return;
    }

    auto& target = state.fighters[static_cast<size_t>(fighter.targetIndex)];
    const int binderIndex = fighter.helper ? fighter.ownerIndex : fighterIndexInState(state, fighter);
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& targetDrop : stateDef->targetDrops) {
        if (!shouldRunStateRuntimeController(state, fighter, targetDrop.id, targetDrop.trigger, opponent, stage)) {
            continue;
        }
        if (targetDrop.excludeId < 0 || targetDrop.excludeId != fighter.targetHitId) {
            clearFighterTargetLink(fighter);
            return false;
        }
    }

    for (const auto& targetState : stateDef->targetStates) {
        if (!targetControllerMatchesStoredTarget(fighter, targetState.targetId)) {
            continue;
        }
        if (!findStateDefinition(state, targetState.value)
            || !shouldRunStateRuntimeController(state, fighter, targetState.id, targetState.trigger, opponent, stage)) {
            continue;
        }
        enterState(state, target, targetState.value);
        target.customHitState = true;
        target.moveType = 'H';
        target.ctrl = false;
        target.boundByIndex = binderIndex;
        target.bindTicks = std::max(target.bindTicks, 1);
    }

    for (const auto& targetBind : stateDef->targetBinds) {
        if (!targetControllerMatchesStoredTarget(fighter, targetBind.targetId)) {
            continue;
        }
        if (!shouldRunStateRuntimeController(state, fighter, targetBind.id, targetBind.trigger, opponent, stage)) {
            continue;
        }
        target.x = fighter.x + targetBind.x * static_cast<float>(fighter.facing);
        target.y = fighter.y + targetBind.y;
        target.boundByIndex = binderIndex;
        target.bindTicks = targetBind.time < 0 ? 9999 : std::max(1, targetBind.time);
        target.vx = 0.0f;
        target.vy = 0.0f;
        target.onGround = target.y >= 0.0f;
    }

    for (const auto& targetFacing : stateDef->targetFacings) {
        if (!targetControllerMatchesStoredTarget(fighter, targetFacing.targetId)
            || !shouldRunStateRuntimeController(state, fighter, targetFacing.id, targetFacing.trigger, opponent, stage)) {
            continue;
        }
        target.facing = targetFacing.value >= 0 ? fighter.facing : -fighter.facing;
    }

    for (const auto& targetLifeAdd : stateDef->targetLifeAdds) {
        if (!targetControllerMatchesStoredTarget(fighter, targetLifeAdd.targetId)
            || !shouldRunStateRuntimeController(state, fighter, targetLifeAdd.id, targetLifeAdd.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, targetLifeAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        const int delta = static_cast<int>(std::lround(*value));
        const int minimumLife = targetLifeAdd.kill ? 0 : 1;
        target.life = std::clamp(target.life + delta, minimumLife, 1000);
    }

    for (const auto& targetPowerAdd : stateDef->targetPowerAdds) {
        if (!targetControllerMatchesStoredTarget(fighter, targetPowerAdd.targetId)
            || !shouldRunStateRuntimeController(state, fighter, targetPowerAdd.id, targetPowerAdd.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, targetPowerAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        target.power = std::clamp(
            target.power + static_cast<int>(std::lround(*value)),
            0,
            std::max(0, state.characterConstants.maxPower));
    }

    for (const auto& targetVelocity : stateDef->targetVelocities) {
        if (!targetControllerMatchesStoredTarget(fighter, targetVelocity.targetId)
            || !shouldRunStateRuntimeController(state, fighter, targetVelocity.id, targetVelocity.trigger, opponent, stage)) {
            continue;
        }
        if (targetVelocity.hasX) {
            const auto value = evalMugenExpression(state, fighter, targetVelocity.xExpression, opponent, stage);
            if (value) {
                const float x = *value * static_cast<float>(target.facing);
                if (targetVelocity.add) {
                    target.vx += x;
                } else {
                    target.vx = x;
                }
            }
        }
        if (targetVelocity.hasY) {
            const auto value = evalMugenExpression(state, fighter, targetVelocity.yExpression, opponent, stage);
            if (value) {
                if (targetVelocity.add) {
                    target.vy += *value;
                } else {
                    target.vy = *value;
                }
            }
        }
    }
        return true;
    });
}

CollisionBox collisionBoxToWorld(const CollisionBox& box, const FighterState& fighter, const AnimationFrame& frame) {
    const bool facingLeft = fighter.facing < 0;
    const bool mirrorX = frame.flipX != facingLeft;
    float x1 = box.x1;
    float x2 = box.x2;
    float y1 = box.y1;
    float y2 = box.y2;

    if (mirrorX) {
        x1 = -box.x2;
        x2 = -box.x1;
    }
    if (frame.flipY) {
        y1 = -box.y2;
        y2 = -box.y1;
    }

    return CollisionBox{
        fighter.x + std::min(x1, x2),
        fighter.y + std::min(y1, y2),
        fighter.x + std::max(x1, x2),
        fighter.y + std::max(y1, y2),
    };
}

CollisionBox collisionBoxToWorldScaled(
    const CollisionBox& box,
    const FighterState& fighter,
    const AnimationFrame& frame,
    float scaleX,
    float scaleY) {
    const bool facingLeft = fighter.facing < 0;
    const bool mirrorX = frame.flipX != facingLeft;
    float x1 = box.x1 * scaleX;
    float x2 = box.x2 * scaleX;
    float y1 = box.y1 * scaleY;
    float y2 = box.y2 * scaleY;

    if (mirrorX) {
        x1 = -box.x2 * scaleX;
        x2 = -box.x1 * scaleX;
    }
    if (frame.flipY) {
        y1 = -box.y2 * scaleY;
        y2 = -box.y1 * scaleY;
    }

    return CollisionBox{
        fighter.x + std::min(x1, x2),
        fighter.y + std::min(y1, y2),
        fighter.x + std::max(x1, x2),
        fighter.y + std::max(y1, y2),
    };
}

bool boxesOverlap(const CollisionBox& a, const CollisionBox& b) {
    return a.x1 < b.x2
        && a.x2 > b.x1
        && a.y1 < b.y2
        && a.y2 > b.y1;
}

bool fighterBoxesOverlap(
    const FighterState& attacker,
    const AnimationFrame& attackFrame,
    const FighterState& defender,
    const AnimationFrame& defendFrame) {
    for (const auto& attackBox : attackFrame.clsn1) {
        const CollisionBox attackWorld = collisionBoxToWorld(attackBox, attacker, attackFrame);
        for (const auto& hurtBox : defendFrame.clsn2) {
            const CollisionBox hurtWorld = collisionBoxToWorld(hurtBox, defender, defendFrame);
            if (boxesOverlap(attackWorld, hurtWorld)) {
                return true;
            }
        }
    }
    return false;
}

std::string_view fighterLabel(size_t fighterIndex) {
    return fighterIndex == 0 ? "P1" : "P2";
}

bool useTrainingDummyOptions(const AppState& state, size_t defenderIndex) {
    return defenderIndex == 1 && activeOpponentType(state) == OpponentType::Dummy;
}

bool shouldPlayFightSounds(const AppState& state) {
    return state.frontend.pendingMode != PendingMode::Training || state.training.options.playHitSounds;
}

int effectiveGuardDistance(const AppState& state, const FighterState& attacker, const HitDefinition& hitDef) {
    if (attacker.attackDistanceOverride >= 0) {
        return attacker.attackDistanceOverride;
    }
    if (hitDef.guardDistance >= 0) {
        return hitDef.guardDistance;
    }
    return std::max(0, state.characterConstants.attackDistance);
}

int scaleDamageForDefence(int damage, const FighterState& defender) {
    if (damage <= 0) {
        return 0;
    }
    const float multiplier = std::max(0.001f, defender.defenceMultiplier);
    return std::max(0, static_cast<int>(std::lround(static_cast<float>(damage) / multiplier)));
}

int scaleDamageForAttack(int damage, const FighterState& attacker) {
    if (damage <= 0) {
        return 0;
    }
    return std::max(0, static_cast<int>(std::lround(static_cast<float>(damage) * attacker.attackMultiplier)));
}

int scaleAttackThenDefenceDamage(int damage, const FighterState& attacker, const FighterState& defender) {
    return scaleDamageForDefence(scaleDamageForAttack(damage, attacker), defender);
}

void clearComboCounters(AppState& state) {
    state.display.comboCounters = {};
}

void endActiveComboForDefender(AppState& state, size_t defenderIndex) {
    if (defenderIndex >= state.display.comboCounters.size()) {
        return;
    }
    const size_t attackerIndex = defenderIndex == 0 ? 1 : 0;
    auto& combo = state.display.comboCounters[attackerIndex];
    combo.activeHits = 0;
}

void registerComboHit(AppState& state, size_t attackerIndex) {
    if (attackerIndex >= state.display.comboCounters.size()) {
        return;
    }

    auto& combo = state.display.comboCounters[attackerIndex];
    ++combo.activeHits;
    combo.displayHits = combo.activeHits;
    combo.displayTicks = std::max(1, state.fightRoundSettings.combo.displayTime);
}

void updateComboCounterBreaks(AppState& state) {
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        const auto& defender = state.fighters[i];
        if (defender.moveType != 'H' && !defender.guarding) {
            endActiveComboForDefender(state, i);
        }
    }
}

void updateComboDisplayTimers(AppState& state) {
    for (auto& combo : state.display.comboCounters) {
        if (combo.displayTicks <= 0) {
            combo.activeHits = 0;
            combo.displayHits = 0;
            continue;
        }
        --combo.displayTicks;
        if (combo.displayTicks <= 0) {
            combo.activeHits = 0;
            combo.displayHits = 0;
        }
    }
}

void applyHitBetween(AppState& state, size_t attackerIndex, size_t defenderIndex) {
    if (defenderIndex >= state.fighters.size()) {
        return;
    }

    FighterState* attackerPtr = nullptr;
    if (attackerIndex < state.fighters.size()) {
        if (attackerIndex == defenderIndex) {
            return;
        }
        attackerPtr = &state.fighters[attackerIndex];
    } else {
        const size_t helperIndex = attackerIndex - state.fighters.size();
        if (helperIndex >= state.helpers.size() || state.helpers[helperIndex].destroyRequested) {
            return;
        }
        attackerPtr = &state.helpers[helperIndex];
        if (attackerPtr->ownerIndex == static_cast<int>(defenderIndex)) {
            return;
        }
    }

    auto& attacker = *attackerPtr;
    auto& defender = state.fighters[defenderIndex];
    const size_t comboAttackerIndex = attacker.helper && attacker.ownerIndex >= 0
        ? static_cast<size_t>(attacker.ownerIndex)
        : attackerIndex;

    const AnimationFrame* attackFrame = currentFrameForFighter(state, attacker);
    const AnimationFrame* defendFrame = currentFrameForFighter(state, defender);
    if (!attackFrame || !defendFrame || attackFrame->clsn1.empty() || defendFrame->clsn2.empty()) {
        return;
    }

    if (!fighterBoxesOverlap(attacker, *attackFrame, defender, *defendFrame)) {
        return;
    }

    const HitDefinition* hitDef = findActiveHitDefinition(state, attacker, defender);
    if (!hitDef) {
        return;
    }
    const HitDefinition resolvedHitDef = resolveHitDefinitionExpressions(
        state,
        *hitDef,
        attacker,
        defender,
        selectedStageSlot(state.selection));
    hitDef = &resolvedHitDef;

    const bool trainingDummy = useTrainingDummyOptions(state, defenderIndex);
    GuardStance guardStance = trainingDummy
        ? chooseDummyGuardStance(state.training.options, *hitDef, defender)
        : choosePlayerGuardStance(*hitDef, defender);
    if (guardStance != GuardStance::None
        && p2BodyDistXValue(attacker, defender) > static_cast<float>(effectiveGuardDistance(state, attacker, *hitDef))) {
        guardStance = GuardStance::None;
    }
    const StateHitOverrideController* hitOverride = guardStance == GuardStance::None
        ? activeHitOverrideForDefender(state, defender, &attacker, *hitDef)
        : nullptr;

    markHitDefApplied(attacker, hitDef->id, currentAnimElemForFighter(state, attacker));
    attacker.moveContact = true;
    ++attacker.hitCount;
    startEnvShake(state, hitDef->envShake);
    startPaletteEffect(defender.paletteEffect, hitDef->palFx);
    if (!hitOverride) {
        attacker.targetIndex = static_cast<int>(defenderIndex);
        attacker.targetHitId = hitDef->targetId;
        attacker.targetTicks = std::max(attacker.targetTicks, 90);
    }
    std::ostringstream hitText;

    if (guardStance != GuardStance::None) {
        attacker.moveGuarded = true;
        endActiveComboForDefender(state, defenderIndex);
        attacker.hitPauseTicks = fightHitPauseTicks(state, hitDef->pausetimeP1, 1);
        const int guardDamageDone = state.training.options.guardDamage
            ? scaleAttackThenDefenceDamage(hitDef->guardDamage, attacker, defender)
            : 0;
        if (!state.training.options.dummyInvincible && state.training.options.guardDamage) {
            defender.life = std::max(0, defender.life - guardDamageDone);
        }
        if (state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else {
            enterGroundGuardHitState(state, defender, *hitDef, attacker.facing, guardStance);
        }
        spawnGuardSpark(state, *hitDef, attacker, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef->guardSoundGroup, hitDef->guardSoundIndex, hitDef->guardSoundForceCommon);
        }

        hitText << fighterLabel(comboAttackerIndex) << " guard " << hitDef->stateNo << "#" << hitDef->id
                << " gdmg " << guardDamageDone
                << " flag " << hitDef->guardflag
                << " mode " << (guardStance == GuardStance::Crouch ? "C" : "S")
                << " spark " << hitDef->guardSparkNo
                << " snd " << soundPairText(hitDef->guardSoundGroup, hitDef->guardSoundIndex);
    } else {
        attacker.moveHit = true;
        attacker.hitPauseTicks = fightHitPauseTicks(state, hitDef->pausetimeP1, 1);
        const int damageDone = (!trainingDummy || !state.training.options.dummyInvincible)
            ? scaleAttackThenDefenceDamage(hitDef->damage, attacker, defender)
            : 0;
        if (damageDone > 0) {
            defender.life = std::max(0, defender.life - damageDone);
        }
        if (trainingDummy && state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else if (hitOverride) {
            enterState(state, defender, hitOverride->stateNo);
            defender.customHitState = true;
            defender.moveType = 'H';
            defender.ctrl = false;
            defender.hitPauseTicks = fightHitPauseTicks(state, hitDef->pausetimeP2, 0);
            defender.hitStunTicks = std::max(hitDef->groundHitTime, defender.hitPauseTicks);
            setGetHitVarsFromHitDef(
                defender,
                *hitDef,
                !defender.onGround || defender.stateType == 'A',
                hitDefCausesFall(*hitDef, defender),
                -hitDef->groundVelocityX * static_cast<float>(attacker.facing),
                hitDef->groundVelocityY);
        } else if (enterCustomHitState(state, defender, *hitDef, attacker.facing)) {
            // Custom p2stateno states are driven by the character CNS after entry.
        } else {
            enterGroundGetHitState(state, defender, *hitDef, attacker.facing);
        }
        spawnHitSpark(state, *hitDef, attacker, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef->hitSoundGroup, hitDef->hitSoundIndex, hitDef->hitSoundForceCommon);
        }
        registerComboHit(state, comboAttackerIndex);

        hitText << fighterLabel(comboAttackerIndex) << " hit " << hitDef->stateNo << "#" << hitDef->id
                << " dmg " << damageDone
                << " attr " << hitDef->attr
                << " hit " << hitDef->groundHitTime
                << " spark " << hitDef->sparkNo
                << " snd " << soundPairText(hitDef->hitSoundGroup, hitDef->hitSoundIndex);
    }
    state.messages.lastHitText = hitText.str();
    state.messages.lastHitTextTicks = 150;
    SDL_Log("%s", state.messages.lastHitText.c_str());
}

void applyHitIfNeeded(AppState& state) {
    applyHitBetween(state, 0, 1);
    if (activeOpponentType(state) != OpponentType::Dummy) {
        applyHitBetween(state, 1, 0);
    }
    const size_t helperBase = state.fighters.size();
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        const auto& helper = state.helpers[i];
        if (helper.destroyRequested || helper.ownerIndex < 0 || helper.ownerIndex >= static_cast<int>(state.fighters.size())) {
            continue;
        }
        const size_t defenderIndex = helper.ownerIndex == 0 ? 1 : 0;
        applyHitBetween(state, helperBase + i, defenderIndex);
    }
}

#include "ArenaModeCombat.h"

FighterState projectileAsActor(const RuntimeProjectile& projectile) {
    FighterState actor;
    actor.x = projectile.x;
    actor.y = projectile.y;
    actor.vx = projectile.vx;
    actor.vy = projectile.vy;
    actor.facing = projectile.facing;
    actor.action = projectile.action;
    actor.animTick = projectile.animTick;
    actor.stateNo = projectile.hitDef.stateNo;
    actor.moveType = 'A';
    actor.stateType = 'A';
    actor.physics = 'N';
    actor.ctrl = false;
    actor.onGround = false;
    return actor;
}

bool projectileAnimationEnded(const AppState& state, const RuntimeProjectile& projectile) {
    const AnimationClip* clip = findExactClip(state, projectile.action);
    return !clip || (!clip->hasLoopStart && projectile.animTick >= std::max(1, clip->loopTicks));
}

enum class ProjectileRemovalReason {
    Timeout,
    Hit,
    Cancel,
};

void beginProjectileRemoval(
    const AppState& state,
    RuntimeProjectile& projectile,
    int fallbackTicks = 12,
    ProjectileRemovalReason reason = ProjectileRemovalReason::Timeout) {
    if (projectile.removing) {
        return;
    }
    projectile.removing = true;
    projectile.vx = projectile.removeVx;
    projectile.vy = projectile.removeVy;
    projectile.animTick = 0;
    if (reason == ProjectileRemovalReason::Hit && projectile.hitAction >= 0) {
        projectile.action = projectile.hitAction;
    } else if (reason == ProjectileRemovalReason::Cancel && projectile.cancelAction >= 0) {
        projectile.action = projectile.cancelAction;
    } else if (projectile.removeAction >= 0) {
        projectile.action = projectile.removeAction;
    } else if (projectile.hitAction >= 0) {
        projectile.action = projectile.hitAction;
    }
    const AnimationClip* removeClip = findExactClip(state, projectile.action);
    projectile.removeTime = removeClip && !removeClip->hasLoopStart
        ? -1
        : fallbackTicks;
}

bool projectileCanUpdateDuringGlobalPause(const AppState& state, const RuntimeProjectile& projectile) {
    if (!globalPauseActive(state)) {
        return true;
    }
    if (projectile.ownerIndex != state.globalPauseOwnerIndex) {
        return false;
    }
    const int moveTime = state.globalPauseIsSuper ? projectile.superMoveTime : projectile.pauseMoveTime;
    return moveTime > 0 && state.globalPauseOwnerMoveTicks > 0;
}

bool projectileBoxesOverlap(const AppState& state, const RuntimeProjectile& lhs, const RuntimeProjectile& rhs) {
    FighterState lhsActor = projectileAsActor(lhs);
    FighterState rhsActor = projectileAsActor(rhs);
    const AnimationFrame* lhsFrame = currentFrameForFighter(state, lhsActor);
    const AnimationFrame* rhsFrame = currentFrameForFighter(state, rhsActor);
    if (!lhsFrame || !rhsFrame) {
        return false;
    }
    const auto& lhsBoxes = lhsFrame->clsn1.empty() ? lhsFrame->clsn2 : lhsFrame->clsn1;
    const auto& rhsBoxes = rhsFrame->clsn1.empty() ? rhsFrame->clsn2 : rhsFrame->clsn1;
    if (lhsBoxes.empty() || rhsBoxes.empty()) {
        return false;
    }
    for (const auto& lhsBox : lhsBoxes) {
        const CollisionBox lhsWorld = collisionBoxToWorldScaled(lhsBox, lhsActor, *lhsFrame, lhs.scaleX, lhs.scaleY);
        for (const auto& rhsBox : rhsBoxes) {
            const CollisionBox rhsWorld = collisionBoxToWorldScaled(rhsBox, rhsActor, *rhsFrame, rhs.scaleX, rhs.scaleY);
            if (boxesOverlap(lhsWorld, rhsWorld)) {
                return true;
            }
        }
    }
    return false;
}

void resolveProjectileCollisions(AppState& state) {
    for (size_t i = 0; i < state.projectiles.size(); ++i) {
        auto& lhs = state.projectiles[i];
        if (lhs.removing) {
            continue;
        }
        for (size_t j = i + 1; j < state.projectiles.size(); ++j) {
            auto& rhs = state.projectiles[j];
            if (rhs.removing || lhs.ownerIndex == rhs.ownerIndex || !projectileBoxesOverlap(state, lhs, rhs)) {
                continue;
            }
            if (lhs.priority == rhs.priority) {
                beginProjectileRemoval(state, lhs, 12, ProjectileRemovalReason::Cancel);
                beginProjectileRemoval(state, rhs, 12, ProjectileRemovalReason::Cancel);
            } else if (lhs.priority > rhs.priority) {
                beginProjectileRemoval(state, rhs, 12, ProjectileRemovalReason::Cancel);
                lhs.priority = std::max(0, lhs.priority - 1);
            } else {
                beginProjectileRemoval(state, lhs, 12, ProjectileRemovalReason::Cancel);
                rhs.priority = std::max(0, rhs.priority - 1);
            }
            if (lhs.removing) {
                break;
            }
        }
    }
}

void applyProjectileHit(AppState& state, RuntimeProjectile& projectile, size_t defenderIndex) {
    if (projectile.ownerIndex < 0
        || projectile.ownerIndex >= static_cast<int>(state.fighters.size())
        || defenderIndex >= state.fighters.size()
        || projectile.ownerIndex == static_cast<int>(defenderIndex)
        || projectile.hitCooldownTicks > 0) {
        return;
    }

    auto& owner = state.fighters[static_cast<size_t>(projectile.ownerIndex)];
    auto& defender = state.fighters[defenderIndex];
    const HitDefinition resolvedHitDef = resolveHitDefinitionExpressions(
        state,
        projectile.hitDef,
        owner,
        defender,
        selectedStageSlot(state.selection));
    const HitDefinition& hitDef = resolvedHitDef;
    if (!defenderCanBeHitBy(defender, hitDef)) {
        return;
    }
    if (!hitFlagAllowsDefender(hitDef, defender)) {
        return;
    }

    FighterState projectileActor = projectileAsActor(projectile);
    const AnimationFrame* attackFrame = currentFrameForFighter(state, projectileActor);
    const AnimationFrame* defendFrame = currentFrameForFighter(state, defender);
    if (!attackFrame || !defendFrame || attackFrame->clsn1.empty() || defendFrame->clsn2.empty()) {
        return;
    }
    bool hitBoxesOverlap = false;
    for (const auto& attackBox : attackFrame->clsn1) {
        const CollisionBox attackWorld = collisionBoxToWorldScaled(attackBox, projectileActor, *attackFrame, projectile.scaleX, projectile.scaleY);
        for (const auto& hurtBox : defendFrame->clsn2) {
            const CollisionBox hurtWorld = collisionBoxToWorld(hurtBox, defender, *defendFrame);
            if (boxesOverlap(attackWorld, hurtWorld)) {
                hitBoxesOverlap = true;
                break;
            }
        }
        if (hitBoxesOverlap) {
            break;
        }
    }
    if (!hitBoxesOverlap) {
        return;
    }

    const bool trainingDummy = useTrainingDummyOptions(state, defenderIndex);
    GuardStance guardStance = trainingDummy
        ? chooseDummyGuardStance(state.training.options, hitDef, defender)
        : choosePlayerGuardStance(hitDef, defender);
    if (guardStance != GuardStance::None
        && p2BodyDistXValue(projectileActor, defender) > static_cast<float>(effectiveGuardDistance(state, owner, hitDef))) {
        guardStance = GuardStance::None;
    }
    const StateHitOverrideController* hitOverride = guardStance == GuardStance::None
        ? activeHitOverrideForDefender(state, defender, &owner, hitDef)
        : nullptr;

    owner.moveContact = true;
    ++owner.hitCount;
    owner.projectileContactId = projectile.id;
    owner.projectileContactTicks = std::max(owner.projectileContactTicks, 32);
    startEnvShake(state, hitDef.envShake);
    startPaletteEffect(defender.paletteEffect, hitDef.palFx);

    std::ostringstream hitText;
    if (guardStance != GuardStance::None) {
        owner.moveGuarded = true;
        endActiveComboForDefender(state, defenderIndex);
        owner.projectileGuardedId = projectile.id;
        owner.projectileGuardedTicks = std::max(owner.projectileGuardedTicks, 32);
        owner.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP1, 1);
        const int guardDamageDone = state.training.options.guardDamage
            ? scaleAttackThenDefenceDamage(hitDef.guardDamage, owner, defender)
            : 0;
        if (!state.training.options.dummyInvincible && state.training.options.guardDamage) {
            defender.life = std::max(0, defender.life - guardDamageDone);
        }
        if (state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else {
            enterGroundGuardHitState(state, defender, hitDef, projectile.facing, guardStance);
        }
        spawnGuardSpark(state, hitDef, projectileActor, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef.guardSoundGroup, hitDef.guardSoundIndex, hitDef.guardSoundForceCommon);
        }
        hitText << fighterLabel(static_cast<size_t>(projectile.ownerIndex)) << " proj guard " << hitDef.stateNo << "#" << projectile.id
                << " gdmg " << guardDamageDone
                << " spark " << hitDef.guardSparkNo
                << " snd " << soundPairText(hitDef.guardSoundGroup, hitDef.guardSoundIndex);
    } else {
        owner.moveHit = true;
        owner.projectileHitId = projectile.id;
        owner.projectileHitTicks = std::max(owner.projectileHitTicks, 32);
        owner.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP1, 1);
        const int damageDone = (!trainingDummy || !state.training.options.dummyInvincible)
            ? scaleAttackThenDefenceDamage(hitDef.damage, owner, defender)
            : 0;
        if (damageDone > 0) {
            defender.life = std::max(0, defender.life - damageDone);
        }
        if (!hitOverride) {
            owner.targetIndex = static_cast<int>(defenderIndex);
            owner.targetHitId = hitDef.targetId;
            owner.targetTicks = std::max(owner.targetTicks, 90);
        }
        if (trainingDummy && state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else if (hitOverride) {
            enterState(state, defender, hitOverride->stateNo);
            defender.customHitState = true;
            defender.moveType = 'H';
            defender.ctrl = false;
            defender.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 0);
            defender.hitStunTicks = std::max(hitDef.groundHitTime, defender.hitPauseTicks);
            setGetHitVarsFromHitDef(
                defender,
                hitDef,
                !defender.onGround || defender.stateType == 'A',
                hitDefCausesFall(hitDef, defender),
                -hitDef.groundVelocityX * static_cast<float>(projectile.facing),
                hitDef.groundVelocityY);
        } else if (enterCustomHitState(state, defender, hitDef, projectile.facing)) {
            // Custom projectile p2stateno states are driven by the target CNS after entry.
        } else {
            enterGroundGetHitState(state, defender, hitDef, projectile.facing);
        }
        spawnHitSpark(state, hitDef, projectileActor, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef.hitSoundGroup, hitDef.hitSoundIndex, hitDef.hitSoundForceCommon);
        }
        registerComboHit(state, static_cast<size_t>(projectile.ownerIndex));
        hitText << fighterLabel(static_cast<size_t>(projectile.ownerIndex)) << " proj hit " << hitDef.stateNo << "#" << projectile.id
                << " dmg " << damageDone
                << " spark " << hitDef.sparkNo
                << " snd " << soundPairText(hitDef.hitSoundGroup, hitDef.hitSoundIndex);
    }

    if (projectile.removeWhenHit != 0) {
        --projectile.hitsRemaining;
        if (projectile.hitsRemaining <= 0) {
            beginProjectileRemoval(state, projectile, 12, ProjectileRemovalReason::Hit);
        }
    }
    projectile.hitCooldownTicks = std::max(projectile.hitCooldownTicks, projectile.missTime);
    state.messages.lastHitText = hitText.str();
    state.messages.lastHitTextTicks = 150;
    SDL_Log("%s", state.messages.lastHitText.c_str());
}

void updateRuntimeProjectiles(AppState& state, const StageSlot& stage) {
    for (auto& projectile : state.projectiles) {
        if (!projectileCanUpdateDuringGlobalPause(state, projectile)) {
            continue;
        }
        if (projectile.hitCooldownTicks > 0) {
            --projectile.hitCooldownTicks;
        }
        if (!projectile.removing) {
            projectile.vx = (projectile.vx + projectile.ax) * projectile.velMulX;
            projectile.vy = (projectile.vy + projectile.ay) * projectile.velMulY;
        }
        projectile.x += projectile.vx * static_cast<float>(projectile.facing);
        projectile.y += projectile.vy;
        ++projectile.animTick;
        if (projectile.removeTime > 0) {
            --projectile.removeTime;
        }

        if (!projectile.removing) {
            const float visibleLeft = state.cameraX - logicalWidthF(state) / 2.0f;
            const float visibleRight = state.cameraX + logicalWidthF(state) / 2.0f;
            const float deadLeft = std::max(stage.leftbound - projectile.projStageBound, visibleLeft - projectile.projEdgeBound);
            const float deadRight = std::min(stage.rightbound + projectile.projStageBound, visibleRight + projectile.projEdgeBound);
            if (projectile.removeTime == 0
                || projectile.x < deadLeft
                || projectile.x > deadRight
                || projectile.y < projectile.projHeightBoundLow
                || projectile.y > projectile.projHeightBoundHigh) {
                beginProjectileRemoval(state, projectile);
                continue;
            }
            size_t defenderIndex = projectile.ownerIndex == 0 ? 1 : 0;
            if (state.frontend.pendingMode == PendingMode::Arena) {
                const int arenaDefender = nearestLivingEnemyIndex(state, projectile.ownerIndex);
                if (arenaDefender < 0) {
                    continue;
                }
                defenderIndex = static_cast<size_t>(arenaDefender);
            }
            applyProjectileHit(state, projectile, defenderIndex);
        }
    }
    resolveProjectileCollisions(state);

    state.projectiles.erase(
        std::remove_if(state.projectiles.begin(), state.projectiles.end(), [&state](const RuntimeProjectile& projectile) {
            if (!projectile.removing) {
                return false;
            }
            if (projectile.removeTime == 0) {
                return true;
            }
            return projectileAnimationEnded(state, projectile);
        }),
        state.projectiles.end());
}

void eraseDestroyedHelpers(AppState& state) {
    state.helpers.erase(
        std::remove_if(state.helpers.begin(), state.helpers.end(), [](const FighterState& helper) {
            return helper.destroyRequested;
        }),
        state.helpers.end());
}

void updateHelperActors(AppState& state, const StageSlot& stage) {
    const size_t helperCount = state.helpers.size();
    for (size_t i = 0; i < helperCount && i < state.helpers.size(); ++i) {
        auto& helper = state.helpers[i];
        if (helper.destroyRequested) {
            continue;
        }
        FighterState* opponent = opponentForActor(state, helper);
        updateStateMovementControllers(state, helper, opponent, &stage);
        updateStateHelperControllers(state, helper, opponent, &stage);
        updateStateProjectileControllers(state, helper, opponent, &stage);
        updateStateMakeDustControllers(state, helper, opponent, stage);
        updateStateExplodControllers(state, helper, opponent, stage);
        const bool changedBeforePhysics = updateStateChangeStateControllers(state, helper, opponent, &stage);
        applyRootBinding(state, helper);
        updateFighterPhysics(state, helper, stage);
        applyRootBinding(state, helper);
        if (!changedBeforePhysics && updateStateChangeStateControllers(state, helper, opponent, &stage) && helper.y >= 0.0f && helper.stateType != 'A') {
            helper.y = 0.0f;
            helper.vy = 0.0f;
            helper.onGround = true;
        }
        updateStateTargetControllers(state, helper, opponent, &stage);
        updateStateChangeAnimControllers(state, helper, opponent, &stage);
        updateStatePosAddControllers(state, helper, opponent, &stage);
        updateStateCtrlControllers(state, helper);
        updateStateAudioControllers(state, helper, opponent, &stage);
        if (helper.hitPauseTicks <= 0) {
            ++helper.animTick;
            ++helper.stateTime;
        }
        updateAfterImageEffect(helper);
        finishStateIfAnimationEnded(state, helper);
    }
    eraseDestroyedHelpers(state);
}

#include "CommandRecognition.h"

#include "CommandStateEligibility.h"

bool applyCommandState(AppState& state, FighterState& fighter, const FighterState* opponent, const std::vector<std::string>& commands) {
    for (const auto& entry : state.commandEntries) {
        if (canEnterCommandState(state, fighter, opponent, entry, commands)) {
            const auto targetState = resolveCommandTargetState(state, fighter, opponent, entry, commands);
            if (!targetState) {
                continue;
            }
            enterState(state, fighter, *targetState);
            return true;
        }
    }
    return false;
}

void updateStateZeroFromMovement(const AppState& state, FighterState& fighter) {
    if (fighter.stateNo != 0 || fighter.guarding) {
        return;
    }
    fighter.ctrl = true;
    fighter.stateType = fighter.onGround ? 'S' : 'A';
    fighter.moveType = 'I';
    fighter.physics = fighter.onGround ? 'S' : 'A';
    if (!fighter.onGround && !fighter.jumpPeakActionApplied && fighter.vy > -2.0f) {
        const int peakAction = airMovementBaseAction(fighter) + 3;
        fighter.jumpPeakActionApplied = findExactClip(state, peakAction) != nullptr;
    }
    setFighterAction(fighter, chooseMovementAction(state, fighter));
}

void finishStateIfAnimationEnded(const AppState& state, FighterState& fighter) {
    if (fighter.stateNo == 0
        || fighter.moveType == 'H'
        || isCrouchStateNo(fighter.stateNo)
        || fighter.stateNo == 20 || fighter.stateNo == 130
        || fighter.stateNo == 131) {
        return;
    }

    if (fighterAnimationEnded(state, fighter)) {
        const StateDefinition* stateDef = findStateDefinition(state, fighter.stateNo);
        if (stateDef && stateDef->hasAnimEndChangeState) {
            std::optional<bool> ctrl;
            if (stateDef->hasAnimEndCtrl) {
                ctrl = stateDef->animEndCtrl;
            }
            const auto targetState = evalMugenExpression(
                state,
                fighter,
                stateDef->animEndChangeStateExpression.empty()
                    ? std::to_string(stateDef->animEndChangeState)
                    : stateDef->animEndChangeStateExpression,
                nullptr,
                nullptr);
            if (targetState) {
                applyParsedChangeState(state, fighter, static_cast<int>(std::lround(*targetState)), ctrl);
            }
        } else if (!(fighter.stateType == 'A' && fighter.physics == 'N')) {
            enterState(state, fighter, 0);
        }
    }
}

void clearHitStatusForRecovery(FighterState& fighter) {
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 0;
    fighter.hitSlideTicks = 0;
    fighter.hitVelocityX = 0.0f;
    fighter.hitVelocityY = 0.0f;
    fighter.getHitAnimType = 0;
    fighter.getHitGroundType = 0;
    fighter.getHitAirType = 0;
    fighter.getHitSlideTime = 0;
    fighter.getHitHitTime = 0;
    fighter.getHitCtrlTime = 0;
    fighter.getHitHitCount = 0;
    fighter.hitRecoverAnim = 5005;
    fighter.hitFall = false;
    fighter.hitFallTrip = false;
    fighter.hitCrouch = false;
    fighter.hitAirborne = false;
    fighter.hitFallRecover = true;
    fighter.hitFallRecoverTime = 0;
    fighter.hitFallDamage = 0;
    fighter.hitFallYAccel = 0.0f;
    fighter.hitFallAirAction = 5050;
    fighter.hitFallBounceXVelocity = 0.0f;
    fighter.hitFallBounceYVelocity = -4.5f;
    fighter.customHitState = false;
    fighter.guarding = false;
    fighter.crouchGuard = false;
}

void enterCommonLandingState(const AppState& state, FighterState& fighter) {
    const int action = firstExistingAction(state, { 47, 52, 0 });
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = action == 52 ? 52 : (action == 47 ? 47 : 0);
    fighter.stateTime = 0;
    fighter.stateType = 'S';
    fighter.moveType = 'I';
    fighter.physics = 'S';
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    clearHitStatusForRecovery(fighter);
    setFighterAction(fighter, action);
}

void enterCommonRecoveryLandingState(const AppState& state, FighterState& fighter) {
    const int action = firstExistingAction(state, { 5170, 47, 52, 0 });
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = action == 5170 ? 5170 : (action == 52 ? 52 : (action == 47 ? 47 : 0));
    fighter.stateTime = 0;
    fighter.stateType = 'S';
    fighter.moveType = 'I';
    fighter.physics = 'S';
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    clearHitStatusForRecovery(fighter);
    setFighterAction(fighter, action);
}

void enterDirectCommonRecoveryState(
    const AppState& state,
    FighterState& fighter,
    int stateNo,
    int action,
    char stateType,
    char physics,
    bool ctrl) {
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.stateType = stateType;
    fighter.moveType = 'I';
    fighter.physics = physics;
    fighter.ctrl = ctrl;
    fighter.onGround = stateType != 'A';
    clearHitStatusForRecovery(fighter);
    setFighterAction(fighter, action);
}

void enterAirRecoveryState(const AppState& state, FighterState& fighter, bool nearGround) {
    const int action = nearGround
        ? firstExistingAction(state, { 5200, 5140, 5210, 5040, 47, 0 })
        : firstExistingAction(state, { 5210, 5040, 5140, 5200, 47, 0 });
    if (action == 47 || action == 0) {
        enterCommonLandingState(state, fighter);
        return;
    }
    enterDirectCommonRecoveryState(state, fighter, action, action, 'A', 'A', true);
    fighter.onGround = false;
    fighter.vy = std::min(fighter.vy, nearGround ? -3.5f : -5.5f);
}

void enterGroundImpactState(const AppState& state, FighterState& fighter, int action) {
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = action;
    fighter.stateTime = 0;
    fighter.stateType = 'L';
    fighter.moveType = 'H';
    fighter.physics = 'N';
    fighter.ctrl = false;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vy = 0.0f;
    fighter.vx *= 0.75f;
    setFighterAction(fighter, action);
}

void triggerFallEnvShakeIfNeeded(AppState& state, FighterState& target);

int fallGroundImpactActionForFighter(const AppState& state, const FighterState& fighter) {
    if (fighter.hitFallTrip || fighter.stateNo == 5070 || fighter.stateNo == 5071) {
        return firstExistingAction(state, { 5110, 5170, fallLandActionForFighter(state, fighter), 0 });
    }
    if (fighter.stateNo == 5160 || fighter.stateNo == 5101) {
        return firstExistingAction(state, { 5170, fallLandActionForFighter(state, fighter), 5110, 0 });
    }
    return fallLandActionForFighter(state, fighter);
}

bool enterFallGroundImpactIfAvailable(AppState& state, FighterState& fighter) {
    const int action = fallGroundImpactActionForFighter(state, fighter);
    if (action == 0) {
        enterState(state, fighter, 0);
        return true;
    }
    enterGroundImpactState(state, fighter, action);
    triggerFallEnvShakeIfNeeded(state, fighter);
    if (fighter.hitFallDamage > 0) {
        fighter.life = std::max(0, fighter.life - fighter.hitFallDamage);
        fighter.hitFallDamage = 0;
    }
    fighter.hitStunTicks = std::max(fighter.hitStunTicks, fighter.hitFallRecover ? std::max(12, fighter.hitFallRecoverTime) : 36);
    return true;
}

bool resolveArenaFallGrounding(AppState& state, FighterState& fighter) {
    if (!isArenaMode(state) || !fighter.hitFall || fighter.hitPauseTicks > 0) {
        return false;
    }

    clampArenaHitFallRuntime(state, fighter);
    const bool floorContact = fighter.onGround || fighter.y >= 0.0f;
    if (fighter.hitFallTrip && fighter.stateNo == 5070 && fighter.stateTime >= 3) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    if ((fighter.stateNo == 5071 || fighter.stateNo == 5050 || fighter.stateNo == 5160 || fighter.stateNo == 5101)
        && floorContact) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    if (fighter.hitFallTrip && fighter.stateNo == 5071 && fighter.stateTime >= 8) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    if (fighter.stateNo == 5050 && fighter.stateTime >= 18 && fighter.vy > 0.0f && fighter.y >= -8.0f) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    return false;
}

bool isCommonAirRecoveryState(int stateNo) {
    return stateNo == 5040
        || stateNo == 5140
        || stateNo == 5200
        || stateNo == 5210;
}

void updateCommonAirRecoveryState(const AppState& state, FighterState& fighter) {
    if (!isCommonAirRecoveryState(fighter.stateNo)) {
        return;
    }
    if (fighter.onGround) {
        enterCommonRecoveryLandingState(state, fighter);
    }
}

bool isCommonDizzyAction(int action) {
    return action == 5300 || action == 5301;
}

void updateCommonDizzyState(const AppState& state, FighterState& fighter) {
    if (!isCommonDizzyAction(fighter.action)) {
        return;
    }

    fighter.stateType = 'S';
    fighter.moveType = 'I';
    fighter.physics = 'S';
    fighter.ctrl = false;
    fighter.customHitState = false;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.notHitByTicks = 0;
    fighter.notHitByValue.clear();
    fighter.hitByTicks = 0;
    fighter.hitByValue.clear();

    if (fighter.stateTime >= 200) {
        clearFighterHitRuntime(fighter);
        enterState(state, fighter, 0);
    }
}

void triggerFallEnvShakeIfNeeded(AppState& state, FighterState& target) {
    if (target.hitFallEnvShakePlayed) {
        return;
    }
    target.hitFallEnvShakePlayed = true;
    startEnvShake(state, target.hitFallEnvShake);
}

void updateGroundGetHitState(AppState& state, FighterState& target) {
    if (target.moveType != 'H') {
        return;
    }

    if (resolveArenaFallGrounding(state, target)) {
        return;
    }

    if (target.hitFall
        && target.stateNo == 5070
        && target.stateTime >= 8
        && !(isArenaMode(state) && target.hitFallTrip)) {
        target.stateNo = 5071;
        target.stateTime = 0;
        target.stateType = 'A';
        target.physics = 'A';
        target.onGround = false;
        setFighterAction(target, findExactClip(state, 5071) ? 5071 : target.hitFallAirAction);
    }

    if (target.hitFall
        && target.hitFallRecover
        && (target.stateNo == 5050 || target.stateNo == 5071)
        && !target.onGround
        && target.stateTime >= std::max(20, target.hitFallRecoverTime)) {
        enterAirRecoveryState(state, target, target.vy > 0.0f && target.y > -48.0f);
        return;
    }

    if (target.hitFall && target.stateNo == 5100 && fighterAnimationEnded(state, target)) {
        if (std::abs(target.hitFallBounceYVelocity) < 0.05f || !findExactClip(state, 5160)) {
            const int action = firstExistingAction(state, { 5110, fallLandActionForFighter(state, target), 0 });
            if (action == 0) {
                enterState(state, target, 0);
                return;
            }
            target.stateNo = 5110;
            target.stateTime = 0;
            target.stateType = 'L';
            target.physics = 'N';
            target.onGround = true;
            target.y = 0.0f;
            target.vy = 0.0f;
            setFighterAction(target, action);
            triggerFallEnvShakeIfNeeded(state, target);
            return;
        }

        target.prevStateNo = target.stateNo;
        target.stateNo = 5160;
        target.stateTime = 0;
        target.stateType = 'A';
        target.physics = 'A';
        target.ctrl = false;
        target.onGround = false;
        target.y = -20.0f;
        target.vx = target.hitFallBounceXVelocity;
        target.vy = target.hitFallBounceYVelocity;
        setFighterAction(target, 5160);
        return;
    }

    if (target.hitFall && target.stateNo == 5160 && target.onGround) {
        const int action = firstExistingAction(state, { 5170, fallLandActionForFighter(state, target), 5110, 0 });
        if (action == 0) {
            enterState(state, target, 0);
            return;
        }
        enterGroundImpactState(state, target, action);
        triggerFallEnvShakeIfNeeded(state, target);
        if (target.hitFallDamage > 0) {
            target.life = std::max(0, target.life - target.hitFallDamage);
            target.hitFallDamage = 0;
        }
        target.hitStunTicks = std::max(target.hitStunTicks, target.hitFallRecover ? std::max(12, target.hitFallRecoverTime) : 36);
        return;
    }

    if (target.hitFall && target.stateNo == 5071 && target.onGround) {
        const int action = firstExistingAction(state, { 5170, 5110, fallLandActionForFighter(state, target), 0 });
        if (action == 0) {
            enterState(state, target, 0);
            return;
        }
        enterGroundImpactState(state, target, action);
        triggerFallEnvShakeIfNeeded(state, target);
        if (target.hitFallDamage > 0) {
            target.life = std::max(0, target.life - target.hitFallDamage);
            target.hitFallDamage = 0;
        }
        target.hitStunTicks = std::max(target.hitStunTicks, target.hitFallRecover ? std::max(12, target.hitFallRecoverTime) : 36);
        return;
    }

    if (target.hitFall && target.stateNo == 5050 && target.onGround) {
        const int action = fallLandActionForFighter(state, target);
        if (action == 0) {
            enterState(state, target, 0);
            return;
        }
        enterGroundImpactState(state, target, action);
        triggerFallEnvShakeIfNeeded(state, target);
        if (target.hitFallDamage > 0) {
            target.life = std::max(0, target.life - target.hitFallDamage);
            target.hitFallDamage = 0;
        }
        target.hitStunTicks = std::max(target.hitStunTicks, target.hitFallRecover ? std::max(12, target.hitFallRecoverTime) : 36);
        return;
    }

    if (target.hitFall && (target.stateNo == 5101 || target.stateNo == 5170 || target.stateNo == 5080) && target.stateTime >= 8) {
        target.stateNo = 5110;
        target.stateTime = 0;
        target.stateType = 'L';
        target.physics = 'N';
        setFighterAction(target, findExactClip(state, 5110) ? 5110 : fallLandActionForFighter(state, target));
    }

    if (target.hitFall && target.stateNo == 5120) {
        target.stateType = 'S';
        target.physics = 'S';
        target.onGround = true;
        target.vx = 0.0f;
        target.vy = 0.0f;
        if (!fighterAnimationEnded(state, target)) {
            return;
        }
        clearFighterHitRuntime(target);
        enterState(state, target, 0);
        return;
    }

    if (target.hitPauseTicks > 0) {
        target.vx = 0.0f;
        target.vy = 0.0f;
        --target.hitPauseTicks;
        if (target.hitPauseTicks == 0) {
            clampArenaHitFallRuntime(state, target);
            target.stateNo = target.hitFall
                ? (target.hitFallTrip ? 5070 : 5050)
                : (target.hitAirborne ? 5030 : (target.hitCrouch ? 5025 : 5001));
            target.stateTime = 0;
            if (target.hitAirborne || target.hitFall || target.hitVelocityY < 0.0f) {
                target.stateType = 'A';
                target.physics = 'A';
                target.onGround = false;
            } else {
                target.stateType = target.hitCrouch ? 'C' : 'S';
                target.physics = target.hitCrouch ? 'C' : 'S';
            }
            target.vx = target.hitVelocityX;
            target.vy = target.hitVelocityY;
            const int recoverAction = target.hitFall
                ? (target.hitFallTrip && findExactClip(state, 5070) ? 5070 : target.hitFallAirAction)
                : (target.hitAirborne
                    ? firstExistingAction(state, { 5030, target.hitRecoverAnim, 5005, 0 })
                    : firstExistingAction(state, { target.hitRecoverAnim, 5005, 0 }));
            setFighterAction(target, recoverAction);
        }
        return;
    }

    if (target.hitFall && (target.stateNo == 5050 || target.stateNo == 5071)) {
        return;
    }

    if (target.hitFall && target.stateNo == 5160) {
        return;
    }

    if (target.hitSlideTicks > 0) {
        --target.hitSlideTicks;
    } else {
        target.vx *= 0.6f;
    }

    if (target.hitStunTicks > 0) {
        --target.hitStunTicks;
    }
    if (target.hitAirborne && !target.hitFall) {
        if (target.onGround) {
            enterCommonLandingState(state, target);
            return;
        }
        if (target.hitStunTicks <= 0) {
            const int action = firstExistingAction(state, { 5040, 5210, 5140, 47, 0 });
            if (action == 47 || action == 0) {
                enterCommonLandingState(state, target);
            } else {
                enterDirectCommonRecoveryState(state, target, action, action, 'A', 'A', true);
                target.onGround = false;
            }
            return;
        }
    }
    if (target.hitFall && target.stateNo != 5110) {
        return;
    }
    if (target.hitStunTicks <= 0) {
        target.vx = 0.0f;
        target.vy = 0.0f;
        target.hitVelocityX = 0.0f;
        target.hitVelocityY = 0.0f;
        target.hitSlideTicks = 0;
        if (target.hitFall && findExactClip(state, 5120)) {
            target.stateNo = 5120;
            target.stateTime = 0;
            target.stateType = 'S';
            target.physics = 'S';
            target.ctrl = false;
            target.onGround = true;
            setFighterAction(target, 5120);
            return;
        }
        clearFighterHitRuntime(target);
        enterState(state, target, 0);
    }
}

void updateGroundGuardState(const AppState& state, FighterState& target) {
    if (!target.guarding) {
        return;
    }

    const bool crouch = target.crouchGuard;
    target.stateType = crouch ? 'C' : 'S';
    target.moveType = 'H';
    target.ctrl = false;
    target.onGround = true;

    if (target.hitPauseTicks > 0) {
        target.stateNo = crouch ? 152 : 150;
        target.physics = 'N';
        target.vx = 0.0f;
        target.vy = 0.0f;
        --target.hitPauseTicks;
        if (target.hitPauseTicks == 0) {
            target.stateNo = crouch ? 153 : 151;
            target.stateTime = 0;
            target.physics = crouch ? 'C' : 'S';
            target.vx = target.hitVelocityX;
            target.vy = target.hitVelocityY;
        }
        return;
    }

    target.stateNo = crouch ? 153 : 151;
    target.physics = crouch ? 'C' : 'S';
    if (target.hitSlideTicks > 0) {
        --target.hitSlideTicks;
    } else {
        target.vx = 0.0f;
        target.vy = 0.0f;
    }

    if (target.hitStunTicks > 0) {
        --target.hitStunTicks;
    }
    if (target.hitStunTicks <= 0) {
        clearFighterHitRuntime(target);
        enterGroundGuardReadyState(state, target, crouch ? GuardStance::Crouch : GuardStance::Stand);
    }
}

void updateControlledFighter(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const FighterInputState& input) {
    pushFighterInputFrame(fighter, input, state.frame);

    if (fighter.guarding) {
        updateGroundGuardState(state, fighter);
        return;
    }

    if (fighter.moveType == 'H' && !fighter.customHitState) {
        updateGroundGetHitState(state, fighter);
        return;
    }

    if (fighter.hitPauseTicks > 0) {
        fighter.vx = 0.0f;
        --fighter.hitPauseTicks;
        return;
    }

    if (fighter.moveType == 'H' && fighter.customHitState) {
        return;
    }

    updateStateZeroFromMovement(state, fighter);
    const bool holdingDown = input.down && fighter.onGround;
    updatePlayerCrouchInput(state, fighter, holdingDown);
    const bool holdingHorizontal = input.left != input.right;
    const bool holdingUp = input.up;
    const bool jumpPressedThisFrame = input.up && !previousFighterInputHeldUp(fighter);
    const bool movementLocked = fighterHasAssertSpecialFlag(fighter, "nowalk");
    const bool attackButtonHeld = input.s || input.x || input.y || input.z || input.a || input.b || input.c;
    const int heldWalkAction = ((fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left)) ? 20 : 21;
    const auto startFallbackJump = [&fighter, &input]() {
        const int heldDirection = input.left == input.right ? 0 : (input.left ? -1 : 1);
        fighter.vy = -7.8f;
        fighter.onGround = false;
        fighter.jumpBaseAction =
            heldDirection == 0 ? 41 : (heldDirection * fighter.facing > 0 ? 42 : 43);
        fighter.jumpPeakActionApplied = false;
    };

    if (!attackButtonHeld
        && !movementLocked
        && !holdingDown
        && fighter.ctrl
        && jumpPressedThisFrame
        && fighter.onGround
        && (fighter.stateNo == 0 || fighter.stateNo == 20)) {
        if (fighter.stateNo != 0) {
            enterState(state, fighter, 0);
            fighter.stateTime = 1;
        }
        startFallbackJump();
        return;
    }

    const auto commands = collectFighterCommands(input, fighter, state.commandDefinitions);
    const bool changedStateFromCommand = applyCommandState(state, fighter, opponent, commands);

    if (!changedStateFromCommand && fighter.stateNo == 20 && (!holdingHorizontal || holdingDown || holdingUp || !fighter.ctrl)) {
        enterState(state, fighter, 0);
    } else if (!changedStateFromCommand && fighter.stateNo == 20 && holdingHorizontal) {
        if (findExactClip(state, heldWalkAction)) {
            setFighterAction(fighter, heldWalkAction);
        }
    }

    if (fighter.stateNo == 0) {
        if (fighter.onGround) {
            fighter.vx = 0.0f;
            fighter.jumpBaseAction = 0;
            fighter.jumpPeakActionApplied = false;
            if (!changedStateFromCommand && !movementLocked && !holdingDown && !holdingUp && fighter.ctrl && holdingHorizontal) {
                if (findStateDefinition(state, 20)) {
                    enterState(state, fighter, 20);
                    if (findExactClip(state, heldWalkAction)) {
                        setFighterAction(fighter, heldWalkAction);
                    }
                } else {
                    const bool movingForward = (fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left);
                    const float localVelocity = movingForward
                        ? state.characterConstants.velocityWalkFwdX
                        : state.characterConstants.velocityWalkBackX;
                    fighter.vx = localVelocity * static_cast<float>(fighter.facing);
                }
            }
        }
        if (!changedStateFromCommand && !movementLocked && !holdingDown && fighter.ctrl && jumpPressedThisFrame && fighter.onGround) {
            startFallbackJump();
        }
    }
}

void updateTrainingDummy(AppState& state, FighterState& dummy) {
    if (state.training.options.dummyFrozen) {
        if (dummy.moveType == 'H' || dummy.guarding) {
            clearFighterHitRuntime(dummy);
            enterState(state, dummy, 0);
        }
        dummy.vx = 0.0f;
        dummy.vy = 0.0f;
        dummy.y = 0.0f;
        dummy.onGround = true;
    } else if (dummy.guarding) {
        updateGroundGuardState(state, dummy);
    } else if (dummy.moveType == 'H' && dummy.customHitState) {
        return;
    } else if (dummy.moveType == 'H') {
        updateGroundGetHitState(state, dummy);
    } else {
        dummy.vx = 0.0f;
        dummy.vy = 0.0f;
        dummy.y = 0.0f;
        dummy.onGround = true;
        const GuardStance idleGuard = dummyGuardIdleStance(state.training.options.dummyGuardMode);
        if (idleGuard != GuardStance::None) {
            if (!isGroundGuardCommonState(dummy.stateNo)
                || guardStanceFromCommonState(dummy.stateNo) != idleGuard
                || dummy.stateNo == guardEndAction(idleGuard)) {
                enterGroundGuardReadyState(state, dummy, idleGuard);
            } else {
                updateGroundGuardReadyState(state, dummy);
            }
        } else if (isGroundGuardCommonState(dummy.stateNo)) {
            if (dummy.stateNo == guardEndAction(guardStanceFromCommonState(dummy.stateNo))) {
                updateGroundGuardReadyState(state, dummy);
            } else {
                enterGroundGuardEndState(state, dummy);
            }
        }
    }
}

bool cpuCanChooseInput(const FighterState& cpu) {
    return cpu.ctrl
        && cpu.onGround
        && cpu.stateType != 'A'
        && cpu.moveType != 'H'
        && !cpu.guarding
        && cpu.hitPauseTicks <= 0;
}

void holdBackInput(FighterInputState& input, const FighterState& fighter) {
    if (fighter.facing >= 0) {
        input.left = true;
    } else {
        input.right = true;
    }
}

void holdTowardTargetInput(FighterInputState& input, const FighterState& fighter, const FighterState& target) {
    if (target.x < fighter.x) {
        input.left = true;
    } else if (target.x > fighter.x) {
        input.right = true;
    } else if (fighter.facing >= 0) {
        input.right = true;
    } else {
        input.left = true;
    }
}

FighterInputState buildCpuOpponentInput(const AppState& state, const FighterState& cpu, const FighterState& target) {
    FighterInputState input;
    if (!cpuCanChooseInput(cpu)) {
        return input;
    }

    const float distance = std::fabs(target.x - cpu.x);
    constexpr float guardDistance = 56.0f;
    constexpr float approachDistance = 48.0f;
    if (target.moveType == 'A' && distance <= guardDistance) {
        holdBackInput(input, cpu);
    } else if (distance > approachDistance) {
        holdTowardTargetInput(input, cpu, target);
    } else if (state.frame % 45 < 2) {
        input.x = true;
    }
    return input;
}

void updateCpuOpponent(AppState& state, FighterState& opponent, const FighterState& target) {
    const FighterInputState input = buildCpuOpponentInput(state, opponent, target);
    updateControlledFighter(state, opponent, &target, input);
}

bool trainingCommandDemoActive(const AppState& state);
FighterInputState nextTrainingCommandDemoInput(AppState& state, FighterState& demoFighter);

std::string fighterResultName(const AppState& state, int winner) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        if (winner > 0 && winner <= static_cast<int>(state.fighters.size())) {
            return arenaFighterName(state, static_cast<size_t>(winner - 1));
        }
        return "";
    }

    switch (winner) {
    case 1:
        return selectedCharacterName(state.selection);
    case 2:
        return opponentDisplayName(state);
    default:
        return "";
    }
}

int matchWinsRequired(const AppState& state) {
    return std::max(1, state.fightRoundSettings.matchWins);
}

bool isSingleFightResultPhase(const AppState& state) {
    return state.matchPhase == MatchPhase::RoundFinish
        || state.matchPhase == MatchPhase::RoundResult
        || state.matchPhase == MatchPhase::MatchResult;
}

std::string roundResultText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        if (state.roundWinner > 0 && state.roundWinner <= static_cast<int>(state.fighters.size())) {
            return state.arenaConfig.winTitle + ": " + uppercaseCopy(fighterResultName(state, state.roundWinner));
        }
        return "DRAW GAME";
    }

    if (state.roundWinner == 1 || state.roundWinner == 2) {
        return uppercaseCopy(fighterResultName(state, state.roundWinner)) + " WINS";
    }
    return "DRAW GAME";
}

std::string roundFinishCalloutText(const AppState& state) {
    switch (state.roundEndReason) {
    case RoundEndReason::TimeUp:
        return "TIME OVER";
    case RoundEndReason::DoubleKo:
        return "DOUBLE K.O.";
    case RoundEndReason::Ko:
        return "K.O.";
    default:
        return "ROUND OVER";
    }
}

std::string singleFightScoreText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return "Free-for-all";
    }
    return std::to_string(state.roundWins[0]) + " - " + std::to_string(state.roundWins[1]);
}

int singleFightRoundStartTotalTicks(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    return std::max(1, round.startWaitTime + round.roundDisplayTime + round.ctrlTime);
}

int singleFightRoundDisplayEndTick(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    return round.startWaitTime + round.roundDisplayTime;
}

int singleFightRoundFinishCalloutTicks(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    switch (state.roundEndReason) {
    case RoundEndReason::DoubleKo:
        return std::max(1, round.dkoDisplayTime);
    case RoundEndReason::TimeUp:
        return std::max(1, round.timeOverDisplayTime);
    case RoundEndReason::Ko:
    default:
        return std::max(1, round.koDisplayTime);
    }
}

int singleFightRoundFinishHoldTicks(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    return std::max({ 1, round.overTime, round.overHitTime, round.overWinTime, singleFightRoundFinishCalloutTicks(state) });
}

int singleFightRoundResultHoldTicks(const AppState& state) {
    return std::max(75, state.fightRoundSettings.winTime + 30);
}

bool canEnterState(const AppState& state, int stateNo) {
    const StateDefinition* stateDef = findStateDefinition(state, stateNo);
    return stateDef && (!stateDef->hasAnim || findExactClip(state, stateDef->anim));
}

bool enterStateIfAvailable(const AppState& state, FighterState& fighter, int stateNo) {
    if (!canEnterState(state, stateNo)) {
        return false;
    }
    enterState(state, fighter, stateNo);
    return true;
}

void applySingleFightRoundPoses(AppState& state) {
    if (state.roundPoseApplied
        || state.matchPhaseTicks < state.fightRoundSettings.overWinTime
        || state.matchPhaseTicks < state.fightRoundSettings.overHitTime) {
        return;
    }

    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    p1.ctrl = false;
    p2.ctrl = false;
    p1.vx = 0.0f;
    p2.vx = 0.0f;

    if (state.roundEndReason == RoundEndReason::TimeUp) {
        if (state.roundWinner == 1) {
            enterStateIfAvailable(state, p1, 181);
            enterStateIfAvailable(state, p2, 170);
        } else if (state.roundWinner == 2) {
            enterStateIfAvailable(state, p2, 181);
            enterStateIfAvailable(state, p1, 170);
        } else {
            if (!enterStateIfAvailable(state, p1, 175)) {
                enterStateIfAvailable(state, p1, 170);
            }
            if (!enterStateIfAvailable(state, p2, 175)) {
                enterStateIfAvailable(state, p2, 170);
            }
        }
    } else if (state.roundEndReason == RoundEndReason::Ko) {
        if (state.roundWinner == 1) {
            enterStateIfAvailable(state, p1, 181);
            p2.ctrl = false;
        } else if (state.roundWinner == 2) {
            enterStateIfAvailable(state, p2, 181);
            p1.ctrl = false;
        }
    }

    state.roundPoseApplied = true;
}

std::string roundStartCalloutText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return "ARENA";
    }
    if (state.roundWins[0] == matchWinsRequired(state) - 1
        && state.roundWins[1] == matchWinsRequired(state) - 1) {
        return "FINAL ROUND";
    }
    return "ROUND " + std::to_string(state.currentRound);
}

int matchWinner(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return state.roundWinner;
    }

    const int required = matchWinsRequired(state);
    if (state.roundWins[0] >= required) {
        return 1;
    }
    if (state.roundWins[1] >= required) {
        return 2;
    }
    return 0;
}

std::string matchWinMethodText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return state.roundWinner > 0 ? "Last Fighter Standing" : "No winner recorded";
    }

    if (matchWinner(state) == 0) {
        return "Draw Game";
    }
    switch (state.roundEndReason) {
    case RoundEndReason::Ko:
        return "Won by K.O.";
    case RoundEndReason::TimeUp:
        return "Won by Decision";
    default:
        return "Won by Decision";
    }
}

void finalizeSingleFightRoundAfterGrace(AppState& state) {
    if (state.roundEndReason != RoundEndReason::Ko || state.matchPhaseTicks < state.fightRoundSettings.overHitTime) {
        return;
    }
    if (state.fighters[0].life <= 0 && state.fighters[1].life <= 0) {
        state.roundWinner = 0;
        state.roundEndReason = RoundEndReason::DoubleKo;
        state.messages.lastHitText = roundFinishCalloutText(state);
        state.roundPoseApplied = false;
    }
}

void applySingleFightRoundScore(AppState& state) {
    if (state.roundScoreApplied) {
        return;
    }

    if (state.roundWinner == 1 || state.roundWinner == 2) {
        const size_t winIndex = static_cast<size_t>(state.roundWinner - 1);
        state.roundWins[winIndex] = std::min(matchWinsRequired(state), state.roundWins[winIndex] + 1);
        state.matchComplete = state.roundWins[winIndex] >= matchWinsRequired(state);
    } else {
        state.matchComplete = false;
    }
    state.roundScoreApplied = true;
    state.messages.lastHitText = roundResultText(state);
    state.messages.lastHitTextTicks = singleFightRoundResultHoldTicks(state);
}

void startSingleFightRoundFinish(AppState& state, int winner, RoundEndReason reason) {
    state.matchPhase = MatchPhase::RoundFinish;
    state.matchPhaseTicks = 0;
    state.roundWinner = winner;
    state.roundEndReason = reason;
    state.roundScoreApplied = false;
    state.roundPoseApplied = false;
    state.matchComplete = false;
    state.messages.lastHitText = roundFinishCalloutText(state);
    state.messages.lastHitTextTicks = singleFightRoundFinishHoldTicks(state);
}

void updateSingleFightRules(AppState& state) {
    if (!isMatchMode(state) || state.matchPhase != MatchPhase::Fight) {
        return;
    }

    const bool timerEnabled = state.mainSettings.matchTimerSeconds > 0;
    if (timerEnabled && state.matchTimerTicks > 0 && !anyFighterHasAssertSpecialFlag(state, "timerfreeze")) {
        --state.matchTimerTicks;
    }

    if (anyFighterHasAssertSpecialFlag(state, "roundnotover")
        || anyFighterHasAssertSpecialFlag(state, "intro")) {
        return;
    }

    const int p1Life = state.fighters[0].life;
    const int p2Life = state.fighters[1].life;
    if (p1Life <= 0 && p2Life <= 0) {
        startSingleFightRoundFinish(state, 0, RoundEndReason::DoubleKo);
    } else if (p1Life <= 0) {
        startSingleFightRoundFinish(state, 2, RoundEndReason::Ko);
    } else if (p2Life <= 0) {
        startSingleFightRoundFinish(state, 1, RoundEndReason::Ko);
    } else if (timerEnabled && state.matchTimerTicks <= 0) {
        if (p1Life > p2Life) {
            startSingleFightRoundFinish(state, 1, RoundEndReason::TimeUp);
        } else if (p2Life > p1Life) {
            startSingleFightRoundFinish(state, 2, RoundEndReason::TimeUp);
        } else {
            startSingleFightRoundFinish(state, 0, RoundEndReason::TimeUp);
        }
    }
}

void updateRoundStartWorld(AppState& state, const StageSlot& stage) {
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];

    resetFighterOneTickBounds(state);
    updateStateMovementControllers(state, p1, &p2, &stage);
    updateStateHelperControllers(state, p1, &p2, &stage);
    updateStateProjectileControllers(state, p1, &p2, &stage);
    updateStateMakeDustControllers(state, p1, &p2, stage);
    updateStateExplodControllers(state, p1, &p2, stage);

    updateStateMovementControllers(state, p2, &p1, &stage);
    updateStateHelperControllers(state, p2, &p1, &stage);
    updateStateProjectileControllers(state, p2, &p1, &stage);
    updateStateMakeDustControllers(state, p2, &p1, stage);
    updateStateExplodControllers(state, p2, &p1, stage);

    updateHelperActors(state, stage);
    const bool p1ChangedState = updateStateChangeStateControllers(state, p1, &p2, &stage);
    const bool p2ChangedState = updateStateChangeStateControllers(state, p2, &p1, &stage);
    updateStateTargetControllers(state, p1, &p2, &stage);
    updateStateTargetControllers(state, p2, &p1, &stage);
    updateStateChangeAnimControllers(state, p1, &p2, &stage);
    updateStateChangeAnimControllers(state, p2, &p1, &stage);
    updateStatePosAddControllers(state, p1, &p2, &stage);
    updateStatePosAddControllers(state, p2, &p1, &stage);
    updateStateCtrlControllers(state, p1);
    updateStateCtrlControllers(state, p2);
    updateStateAudioControllers(state, p1, &p2, &stage);
    updateStateAudioControllers(state, p2, &p1, &stage);
    updateFightAssertSpecialControllers(state, stage);

    if (!p1ChangedState && p1.hitPauseTicks <= 0) {
        ++p1.animTick;
        ++p1.stateTime;
        updateAfterImageEffect(p1);
    }
    if (!p2ChangedState && p2.hitPauseTicks <= 0) {
        ++p2.animTick;
        ++p2.stateTime;
        updateAfterImageEffect(p2);
    }
    updateGlobalPauseTimers(state);
    finishStateIfAnimationEnded(state, p1);
    finishStateIfAnimationEnded(state, p2);
}

void updateSingleFightRoundFinishWorld(AppState& state, const StageSlot& stage) {
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];

    resetFighterOneTickBounds(state);
    const bool p1CanUpdate = fighterCanUpdateDuringGlobalPause(state, 0);
    const bool p2CanUpdate = fighterCanUpdateDuringGlobalPause(state, 1);
    if (p1CanUpdate) {
        updateStateMovementControllers(state, p1, &p2, &stage);
        updateStateHelperControllers(state, p1, &p2, &stage);
        updateStateProjectileControllers(state, p1, &p2, &stage);
        updateStateMakeDustControllers(state, p1, &p2, stage);
        updateStateExplodControllers(state, p1, &p2, stage);
    }
    if (p2CanUpdate) {
        updateStateMovementControllers(state, p2, &p1, &stage);
        updateStateHelperControllers(state, p2, &p1, &stage);
        updateStateProjectileControllers(state, p2, &p1, &stage);
        updateStateMakeDustControllers(state, p2, &p1, stage);
        updateStateExplodControllers(state, p2, &p1, stage);
    }
    updateHelperActors(state, stage);
    const bool p1ChangedBeforePhysics = p1CanUpdate && updateStateChangeStateControllers(state, p1, &p2, &stage);
    const bool p2ChangedBeforePhysics = p2CanUpdate && updateStateChangeStateControllers(state, p2, &p1, &stage);
    if (fighterCanUpdateDuringGlobalPause(state, 0)) {
        updateFighterPhysics(state, p1, stage);
    }
    if (fighterCanUpdateDuringGlobalPause(state, 1)) {
        updateFighterPhysics(state, p2, stage);
    }
    updateCommonAirRecoveryState(state, p1);
    updateCommonAirRecoveryState(state, p2);
    updateCommonDizzyState(state, p1);
    updateCommonDizzyState(state, p2);
    if (p1CanUpdate && !p1ChangedBeforePhysics && updateStateChangeStateControllers(state, p1, &p2, &stage) && p1.y >= 0.0f && p1.stateType != 'A') {
        p1.y = 0.0f;
        p1.vy = 0.0f;
        p1.onGround = true;
    }
    if (p2CanUpdate && !p2ChangedBeforePhysics && updateStateChangeStateControllers(state, p2, &p1, &stage) && p2.y >= 0.0f && p2.stateType != 'A') {
        p2.y = 0.0f;
        p2.vy = 0.0f;
        p2.onGround = true;
    }
    applyPlayerPush(state, stage);
    updateFighterFacing(state);
    updateComboCounterBreaks(state);

    if (state.matchPhaseTicks <= state.fightRoundSettings.overHitTime) {
        updateRuntimeProjectiles(state, stage);
        applyHitIfNeeded(state);
    }

    if (p1CanUpdate) {
        updateStateChangeAnimControllers(state, p1, &p2, &stage);
        updateStatePosAddControllers(state, p1, &p2, &stage);
        updateStateCtrlControllers(state, p1);
        updateStateAudioControllers(state, p1, &p2, &stage);
    }
    if (p2CanUpdate) {
        updateStateChangeAnimControllers(state, p2, &p1, &stage);
        updateStatePosAddControllers(state, p2, &p1, &stage);
        updateStateCtrlControllers(state, p2);
        updateStateAudioControllers(state, p2, &p1, &stage);
    }
    updateFightAssertSpecialControllers(state, stage);
    if (state.matchPhaseTicks >= state.fightRoundSettings.overWaitTime) {
        p1.ctrl = false;
        p2.ctrl = false;
    }
    updateCamera(state, stage);
    applyScreenBounds(state, stage);

    if (p1.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 0)) {
        ++p1.animTick;
        ++p1.stateTime;
        updateAfterImageEffect(p1);
    }
    if (p2.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 1)) {
        ++p2.animTick;
        ++p2.stateTime;
        updateAfterImageEffect(p2);
    }
    updateGlobalPauseTimers(state);
    finishStateIfAnimationEnded(state, p1);
    finishStateIfAnimationEnded(state, p2);
}

void updateSingleFightPhaseTimers(AppState& state) {
    if (state.messages.lastHitTextTicks > 0) {
        --state.messages.lastHitTextTicks;
    }
    updateComboDisplayTimers(state);

    switch (state.matchPhase) {
    case MatchPhase::RoundStart:
        ++state.matchPhaseTicks;
        if (state.matchPhaseTicks >= singleFightRoundStartTotalTicks(state)) {
            state.matchPhase = MatchPhase::Fight;
            state.matchPhaseTicks = 0;
        }
        break;
    case MatchPhase::RoundFinish:
        ++state.matchPhaseTicks;
        finalizeSingleFightRoundAfterGrace(state);
        applySingleFightRoundPoses(state);
        if (state.matchPhaseTicks >= singleFightRoundFinishHoldTicks(state)) {
            applySingleFightRoundScore(state);
            state.matchPhase = MatchPhase::RoundResult;
            state.matchPhaseTicks = 0;
        }
        break;
    case MatchPhase::RoundResult:
        ++state.matchPhaseTicks;
        if (state.matchPhaseTicks >= singleFightRoundResultHoldTicks(state)) {
            if (state.matchComplete) {
                state.matchPhase = MatchPhase::MatchResult;
                state.matchPhaseTicks = 0;
                state.frontend.selectedMatchResultOption = 0;
            } else {
                ++state.currentRound;
                resetFightRound(state);
            }
        }
        break;
    case MatchPhase::MatchResult:
        ++state.matchPhaseTicks;
        break;
    case MatchPhase::Fight:
    default:
        break;
    }
}

#include "ArenaModeRuntime.h"

void updateFight(AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        updateArenaFight(state);
        return;
    }

    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    const int p1StateNoAtFrameStart = p1.stateNo;
    const int p2StateNoAtFrameStart = p2.stateNo;

    updateRuntimeEffects(state);
    clearFightAssertSpecialFlags(state);
    updateFightAssertSpecialControllers(state, stage);
    resetFighterOneTickBounds(state);

    if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundStart) {
        updateRoundStartWorld(state, stage);
        updateSingleFightPhaseTimers(state);
        updateCamera(state, stage);
        return;
    }

    if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundFinish) {
        updateSingleFightRoundFinishWorld(state, stage);
        updateSingleFightPhaseTimers(state);
        return;
    }

    if (isMatchMode(state)
        && (state.matchPhase == MatchPhase::RoundResult || state.matchPhase == MatchPhase::MatchResult)) {
        updateSingleFightPhaseTimers(state);
        updateCamera(state, stage);
        return;
    }

    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    updateFighterFacing(state);

    const FighterInputState p1Input = gFightInputOverride && gFightInputOverride->p1
        ? *gFightInputOverride->p1
        : collectFighterInput(keys, p1Controls(), assignedGamepad(state, 0));
    if (fighterCanUpdateDuringGlobalPause(state, 0)) {
        updateControlledFighter(state, p1, &p2, p1Input);
    }
    if (!fighterCanUpdateDuringGlobalPause(state, 1)) {
        p2.vx = 0.0f;
        p2.vy = 0.0f;
    } else if (usesLocalP2Controls(state)) {
        const FighterInputState p2Input = gFightInputOverride && gFightInputOverride->p2
            ? *gFightInputOverride->p2
            : collectFighterInput(keys, p2Controls(), assignedGamepad(state, 1));
        updateControlledFighter(state, p2, &p1, p2Input);
    } else if (activeOpponentType(state) == OpponentType::Dummy && trainingCommandDemoActive(state)) {
        const FighterInputState demoInput = nextTrainingCommandDemoInput(state, p2);
        updateControlledFighter(state, p2, &p1, demoInput);
    } else if (activeOpponentType(state) == OpponentType::Dummy) {
        updateTrainingDummy(state, p2);
    } else {
        updateCpuOpponent(state, p2, p1);
    }

    updateStateZeroFromMovement(state, p1);
    updateStateZeroFromMovement(state, p2);
    const bool p1CanUpdate = fighterCanUpdateDuringGlobalPause(state, 0);
    const bool p2CanUpdate = fighterCanUpdateDuringGlobalPause(state, 1);
    if (p1CanUpdate) {
        updateStateMovementControllers(state, p1, &p2, &stage);
        updateStateHelperControllers(state, p1, &p2, &stage);
        updateStateProjectileControllers(state, p1, &p2, &stage);
        updateStateMakeDustControllers(state, p1, &p2, stage);
        updateStateExplodControllers(state, p1, &p2, stage);
    }
    if (p2CanUpdate) {
        updateStateMovementControllers(state, p2, &p1, &stage);
        updateStateHelperControllers(state, p2, &p1, &stage);
        updateStateProjectileControllers(state, p2, &p1, &stage);
        updateStateMakeDustControllers(state, p2, &p1, stage);
        updateStateExplodControllers(state, p2, &p1, stage);
    }
    updateHelperActors(state, stage);
    const bool p1ChangedBeforePhysics = p1CanUpdate && updateStateChangeStateControllers(state, p1, &p2, &stage);
    const bool p2ChangedBeforePhysics = p2CanUpdate && updateStateChangeStateControllers(state, p2, &p1, &stage);
    if (p1CanUpdate) {
        updateFighterPhysics(state, p1, stage);
    }
    if (p2CanUpdate) {
        updateFighterPhysics(state, p2, stage);
    }
    updateCommonAirRecoveryState(state, p1);
    updateCommonAirRecoveryState(state, p2);
    updateCommonDizzyState(state, p1);
    updateCommonDizzyState(state, p2);
    if (p1CanUpdate && !p1ChangedBeforePhysics && updateStateChangeStateControllers(state, p1, &p2, &stage) && p1.y >= 0.0f && p1.stateType != 'A') {
        p1.y = 0.0f;
        p1.vy = 0.0f;
        p1.onGround = true;
    }
    if (p2CanUpdate && !p2ChangedBeforePhysics && updateStateChangeStateControllers(state, p2, &p1, &stage) && p2.y >= 0.0f && p2.stateType != 'A') {
        p2.y = 0.0f;
        p2.vy = 0.0f;
        p2.onGround = true;
    }
    applyPlayerPush(state, stage);
    updateFighterFacing(state);
    updateStateZeroFromMovement(state, p1);
    updateStateZeroFromMovement(state, p2);
    updateComboCounterBreaks(state);
    updateRuntimeProjectiles(state, stage);
    if (!globalPauseActive(state) || state.globalPauseOwnerMoveTicks > 0) {
        applyHitIfNeeded(state);
    }
    if (p1CanUpdate) {
        updateStateTargetControllers(state, p1, &p2, &stage);
        updateStateChangeAnimControllers(state, p1, &p2, &stage);
        updateStatePosAddControllers(state, p1, &p2, &stage);
        updateStateCtrlControllers(state, p1);
        updateStateAudioControllers(state, p1, &p2, &stage);
    }
    if (p2CanUpdate) {
        updateStateTargetControllers(state, p2, &p1, &stage);
        updateStateChangeAnimControllers(state, p2, &p1, &stage);
        updateStatePosAddControllers(state, p2, &p1, &stage);
        updateStateCtrlControllers(state, p2);
        updateStateAudioControllers(state, p2, &p1, &stage);
    }
    updateFightAssertSpecialControllers(state, stage);
    if (state.frontend.pendingMode == PendingMode::Training
        && activeOpponentType(state) == OpponentType::Dummy
        && (state.training.options.dummyAutoLife || state.training.options.dummyInvincible)) {
        p2.life = 1000;
    }
    updateSingleFightRules(state);
    updateCamera(state, stage);
    applyScreenBounds(state, stage);
    if (state.messages.lastHitTextTicks > 0) {
        --state.messages.lastHitTextTicks;
    }
    updateComboDisplayTimers(state);
    const bool p1EnteredNewStateThisFrame = p1.stateNo != p1StateNoAtFrameStart && p1.stateTime == 0;
    const bool p2EnteredNewStateThisFrame = p2.stateNo != p2StateNoAtFrameStart && p2.stateTime == 0;
    if (!p1EnteredNewStateThisFrame && p1.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 0)) {
        ++p1.animTick;
        ++p1.stateTime;
        updateAfterImageEffect(p1);
    }
    if (!p2EnteredNewStateThisFrame && p2.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 1)) {
        ++p2.animTick;
        ++p2.stateTime;
        updateAfterImageEffect(p2);
    }
    updateGlobalPauseTimers(state);
    finishStateIfAnimationEnded(state, p1);
    finishStateIfAnimationEnded(state, p2);
}

void drawArenaSetup(SDL_Renderer* renderer, const AppState& state) {
    ArenaSetupView view;
    view.title = state.arenaConfig.modeName;
    view.description = state.arenaConfig.description;
    view.fighterName = compactSettingText(selectedCharacterName(state.selection), 18);
    view.cpuCount = arenaCpuCount(state);
    view.modeLabel = "Free-for-all";
    view.stageName = compactSettingText(selectedStageName(state.selection), 20);
    view.selectedOption = state.frontend.selectedArenaSetupOption;
    view.frame = state.frame;
    drawArenaSetupOverlay(uiRenderContext(renderer, state), view);
    SDL_RenderPresent(renderer);
}

void drawStageSelect(SDL_Renderer* renderer, const AppState& state) {
    std::vector<StageSelectRowView> rows;
    rows.reserve(state.selection.stages.size());
    for (int i = 0; i < static_cast<int>(state.selection.stages.size()); ++i) {
        const auto& stage = state.selection.stages[static_cast<std::size_t>(i)];
        rows.push_back(StageSelectRowView{
            stage.displayName,
            i == state.selection.selectedStage,
        });
    }

    StageSelectView view;
    view.rows = rows;
    view.frame = state.frame;
    view.fighterLabel = selectedCharacterName(state.selection);
    view.opponentLabel = compactSettingText(opponentDisplayName(state), 11);
    if (const StageSlot* selected = selectedStageSlot(state.selection)) {
        view.selectedStageName = selected->displayName;
        view.selectedStageId = selected->id;
        view.selectedStageAuthor = selected->author;
    }

    drawStageSelectOverlay(uiRenderContext(renderer, state), view);
    SDL_RenderPresent(renderer);
}

VsScreenLoadStatus vsScreenLoadStatus(const AppState& state) {
    if (state.fightSessionLoadFailed) {
        return VsScreenLoadStatus::Failed;
    }
    if (state.fightSessionPrepared) {
        return VsScreenLoadStatus::Ready;
    }
    return VsScreenLoadStatus::Loading;
}

void drawVersusScreen(SDL_Renderer* renderer, const AppState& state) {
    const TextureSprite* p1Portrait = state.characterLargePortrait.texture
        ? &state.characterLargePortrait
        : spriteAt(state.characterFaceSprites, sessionP1CharacterIndex(state.selection));
    const TextureSprite* opponentPortrait =
        spriteAt(state.characterFaceSprites, state.selection.sessionSlots.opponentCharacter);

    drawVersusScreenOverlay(
        uiRenderContext(renderer, state),
        VsScreenView{
            std::string(pendingModeTitle(state.frontend.pendingMode)),
            compactSettingText(selectedCharacterName(state.selection), 13),
            compactSettingText(opponentDisplayName(state), 10),
            std::string(opponentSlotLabel(state)),
            compactSettingText(selectedStageName(state.selection), 26),
            vsScreenLoadStatus(state),
            uiSpriteView(p1Portrait),
            uiSpriteView(opponentPortrait),
        });
    SDL_RenderPresent(renderer);
}

bool hasSelectedStageBackground(const AppState& state) {
    return !state.stageBackground.empty();
}

#include "WorldRender.h"

#include "TrainingDebugViewAssembly.h"

std::string moveListTokenForCommand(std::string_view command) {
    if (command == "x") {
        return "A";
    }
    if (command == "y") {
        return "S";
    }
    if (command == "z") {
        return "D";
    }
    if (command == "a") {
        return "Z";
    }
    if (command == "b") {
        return "X";
    }
    if (command == "c") {
        return "C";
    }
    if (command == "holddown") {
        return "DOWN";
    }
    if (command == "holdup") {
        return "UP";
    }
    if (command == "holdfwd") {
        return "FWD";
    }
    if (command == "holdback") {
        return "BACK";
    }

    std::string label(command);
    std::replace(label.begin(), label.end(), '_', '+');
    if (label.size() >= 2 && label[label.size() - 2] == '+') {
        const char button = label.back();
        if (button == 'x') {
            label.replace(label.size() - 1, 1, "A");
        } else if (button == 'y') {
            label.replace(label.size() - 1, 1, "S");
        } else if (button == 'z') {
            label.replace(label.size() - 1, 1, "D");
        } else if (button == 'a') {
            label.replace(label.size() - 1, 1, "Z");
        } else if (button == 'b') {
            label.replace(label.size() - 1, 1, "X");
        } else if (button == 'c') {
            label.replace(label.size() - 1, 1, "C");
        }
    }
    return label;
}

bool commandListContains(const std::vector<std::string>& commands, std::string_view command) {
    return std::any_of(commands.begin(), commands.end(), [command](const std::string& current) {
        return current == command;
    });
}

std::string moveListInputText(const CommandStateEntry& entry) {
    std::vector<std::string> parts;
    const auto appendIfRequired = [&parts, &entry](std::string_view command) {
        if (commandListContains(entry.requiredCommands, command)) {
            parts.push_back(moveListTokenForCommand(command));
        }
    };

    for (const auto& optionGroup : entry.commandOptionGroups) {
        std::string groupText;
        for (const auto& option : optionGroup) {
            if (!groupText.empty()) {
                groupText += "/";
            }
            groupText += moveListTokenForCommand(option);
        }
        if (!groupText.empty()) {
            parts.push_back(groupText);
        }
    }

    appendIfRequired("holddown");
    appendIfRequired("holdfwd");
    appendIfRequired("holdback");
    appendIfRequired("holdup");
    appendIfRequired("x");
    appendIfRequired("y");
    appendIfRequired("z");
    appendIfRequired("a");
    appendIfRequired("b");
    appendIfRequired("c");

    for (const auto& command : entry.requiredCommands) {
        if (command == "holddown"
            || command == "holdfwd"
            || command == "holdback"
            || command == "holdup"
            || command == "x"
            || command == "y"
            || command == "z"
            || command == "a"
            || command == "b"
            || command == "c") {
            continue;
        }
        parts.push_back(moveListTokenForCommand(command));
    }

    if (parts.empty()) {
        return "-";
    }

    std::string text = parts.front();
    for (size_t i = 1; i < parts.size(); ++i) {
        text += "+";
        text += parts[i];
    }
    return text;
}

int commandEntryRequiredPower(const CommandStateEntry& entry) {
    int requiredPower = 0;
    for (const auto& condition : entry.intConditions) {
        if (condition.subject != CommandConditionSubject::Power) {
            continue;
        }
        if (condition.op == CompareOp::GreaterEqual) {
            requiredPower = std::max(requiredPower, condition.value);
        } else if (condition.op == CompareOp::Greater) {
            requiredPower = std::max(requiredPower, condition.value + 1);
        }
    }
    return requiredPower;
}

TrainingMoveCategory commandEntryCategory(const CommandStateEntry& entry) {
    const int requiredPower = commandEntryRequiredPower(entry);
    if (requiredPower >= 1000) {
        return TrainingMoveCategory::Supers;
    }
    if (const auto targetState = parsePlainIntValue(entry.targetStateExpression)) {
        if (*targetState >= 3000) {
            return TrainingMoveCategory::Supers;
        }
        if (*targetState >= 200 && *targetState < 700) {
            return TrainingMoveCategory::Normals;
        }
    }
    return TrainingMoveCategory::Specials;
}

bool commandEntryMatchesMoveCategory(const CommandStateEntry& entry, TrainingMoveCategory category) {
    if (category == TrainingMoveCategory::All) {
        return true;
    }
    return commandEntryCategory(entry) == category;
}

std::vector<const CommandStateEntry*> displayableMoveListEntries(const AppState& state) {
    std::vector<const CommandStateEntry*> entries;
    for (const auto& entry : state.commandEntries) {
        if (entry.requiredCommands.empty() && entry.commandOptionGroups.empty()) {
            continue;
        }
        if (!commandEntryMatchesMoveCategory(entry, state.training.options.moveCategory)) {
            continue;
        }
        if (const auto literalTarget = parsePlainIntValue(entry.targetStateExpression)) {
            const StateDefinition* stateDef = findStateDefinition(state, *literalTarget);
            if (!stateDef || (stateDef->hasAnim && !findExactClip(state, stateDef->anim))) {
                continue;
            }
        }
        entries.push_back(&entry);
    }
    return entries;
}

std::string joinTokens(const std::vector<std::string>& tokens, std::string_view separator) {
    if (tokens.empty()) {
        return "-";
    }
    std::string result = tokens.front();
    for (size_t i = 1; i < tokens.size(); ++i) {
        result += separator;
        result += tokens[i];
    }
    return result;
}

std::string inputDirectionToken(const FighterInputState& input, int facing) {
    const bool forward = facing >= 0 ? input.right : input.left;
    const bool back = facing >= 0 ? input.left : input.right;
    if (input.down && forward) {
        return "DF";
    }
    if (input.down && back) {
        return "DB";
    }
    if (input.up && forward) {
        return "UF";
    }
    if (input.up && back) {
        return "UB";
    }
    if (forward) {
        return "F";
    }
    if (back) {
        return "B";
    }
    if (input.down) {
        return "D";
    }
    if (input.up) {
        return "U";
    }
    return "";
}

std::string inputDisplayToken(const FighterInputState& input, int facing) {
    std::vector<std::string> tokens;
    const std::string direction = inputDirectionToken(input, facing);
    if (!direction.empty()) {
        tokens.push_back(direction);
    }
    if (input.x) {
        tokens.push_back("A");
    }
    if (input.y) {
        tokens.push_back("S");
    }
    if (input.z) {
        tokens.push_back("D");
    }
    if (input.a) {
        tokens.push_back("Z");
    }
    if (input.b) {
        tokens.push_back("X");
    }
    if (input.c) {
        tokens.push_back("C");
    }
    return joinTokens(tokens, "+");
}

std::vector<std::string> recentInputDisplayTokens(const FighterState& fighter, int maxTokens) {
    std::vector<std::string> tokens;
    std::string lastToken;
    for (auto it = fighter.inputHistory.rbegin(); it != fighter.inputHistory.rend() && static_cast<int>(tokens.size()) < maxTokens; ++it) {
        std::string token = inputDisplayToken(it->input, fighter.facing);
        if (token == "-" || token == lastToken) {
            continue;
        }
        tokens.push_back(std::move(token));
        lastToken = tokens.back();
    }
    std::reverse(tokens.begin(), tokens.end());
    return tokens;
}

std::string commandEntryTargetLabel(const CommandStateEntry& entry) {
    if (const auto literalTarget = parsePlainIntValue(entry.targetStateExpression)) {
        return std::to_string(*literalTarget);
    }
    return fitDebugText(entry.targetStateExpression.empty() ? std::to_string(entry.targetState) : entry.targetStateExpression, 17);
}

std::string moveListEntryName(const CommandStateEntry& entry) {
    return entry.label.empty() ? "State " + commandEntryTargetLabel(entry) : entry.label;
}

#include "TrainingCommandPracticeAssembly.h"

TrainingCommandHudView trainingCommandHudView(
    const AppState& state,
    std::vector<TrainingCommandRowView>& rows,
    std::vector<TrainingCommandStepView>& steps) {
    rows.clear();
    steps.clear();
    TrainingCommandHudView view;
    view.input.visible = state.training.options.showInputHud;
    view.commandsVisible = state.training.options.showCommandHud;

    if (!view.input.visible && !view.commandsVisible) {
        return view;
    }
    if (state.fighters.empty()) {
        view.input.visible = false;
        view.commandsVisible = false;
        return view;
    }

    const auto& fighter = state.fighters[0];
    const FighterState* opponent = state.fighters.size() > 1 ? &state.fighters[1] : nullptr;
    const std::vector<std::string> commands = fighter.inputHistory.empty()
        ? std::vector<std::string>{}
        : collectFighterCommands(fighter.inputHistory.back().input, fighter, state.commandDefinitions);
    const CommandStateEntry* activeEntry = activeCommandEntry(state, fighter, opponent, commands);

    if (view.input.visible) {
        const std::string current = fighter.inputHistory.empty()
            ? "-"
            : inputDisplayToken(fighter.inputHistory.back().input, fighter.facing);
        view.input.currentInput = fitDebugText(current, 18);
        view.input.recentInputs = fitDebugText(joinTokens(recentInputDisplayTokens(fighter, 8), " "), 27);
    }

    if (view.commandsVisible) {
        const auto entries = displayableMoveListEntries(state);
        const int selected = entries.empty()
            ? -1
            : std::clamp(state.training.options.selectedMoveListEntry, 0, static_cast<int>(entries.size()) - 1);

        constexpr int visibleRows = 7;
        const int maxStart = std::max(0, static_cast<int>(entries.size()) - visibleRows);
        const int firstRow = selected < 0 ? 0 : std::clamp(selected - 2, 0, maxStart);

        rows.reserve(visibleRows);
        for (int row = 0; row < visibleRows; ++row) {
            const int index = firstRow + row;
            if (index >= static_cast<int>(entries.size())) {
                break;
            }
            const auto* entry = entries[static_cast<size_t>(index)];
            if (!entry) {
                continue;
            }
            const CommandDefinition* definition = practiceCommandDefinitionForEntry(state, *entry, commands);
            rows.push_back(TrainingCommandRowView{
                fitDebugText(moveListEntryName(*entry), 16),
                fitDebugText(definition ? commandDefinitionInputLabel(*definition) : moveListInputText(*entry), 10),
                activeEntry == entry,
                index == selected,
            });
        }

        view.categoryLabel = std::string(trainingMoveCategoryStatus(state.training.options.moveCategory));
        view.pageLabel = entries.empty()
            ? "0/0"
            : std::to_string(selected + 1) + "/" + std::to_string(entries.size());
        view.showMeLabel = "H SHOW ME";
        view.demoActive = trainingCommandDemoActive(state);
        if (activeEntry) {
            view.activeCommandLabel = fitDebugText(activeEntry->label, 19);
        }
        if (selected >= 0) {
            const auto& entry = *entries[static_cast<size_t>(selected)];
            const bool complete = activeEntry == &entry;
            const CommandDefinition* definition = practiceCommandDefinitionForEntry(state, entry, commands);
            view.currentMoveName = fitDebugText(moveListEntryName(entry), 20);
            view.currentMoveInput = fitDebugText(definition ? commandDefinitionInputLabel(*definition) : moveListInputText(entry), 25);
            view.completeFlash = complete;

            if (definition) {
                appendDefinitionPracticeSteps(steps, fighter, *definition, complete || commandDefinitionActive(*definition, fighter));
            } else {
                appendEntryPracticeSteps(steps, entry, commands, complete);
            }
        } else {
            view.currentMoveName = "No loaded moves";
            view.currentMoveInput = "-";
        }
    }

    view.commandRows = rows;
    view.practiceSteps = steps;
    return view;
}

void drawTrainingCommandHud(SDL_Renderer* renderer, const AppState& state) {
    std::vector<TrainingCommandRowView> rows;
    std::vector<TrainingCommandStepView> steps;
    const TrainingCommandHudView view = trainingCommandHudView(state, rows, steps);
    dragon::drawTrainingCommandOverlay(uiRenderContext(renderer, state), view);
}

TrainingOptionsMenuView trainingOptionsMenuView(const AppState& state, std::vector<TrainingOptionRowView>& rows) {
    rows.clear();
    rows.reserve(kTrainingOptionCount);
    const auto& options = state.training.options;
    for (int i = 0; i < kTrainingOptionCount; ++i) {
        rows.push_back(TrainingOptionRowView{
            std::string(trainingOptionLabel(i)),
            trainingOptionStatus(options, i),
            i == options.selectedOption,
        });
    }

    TrainingOptionsMenuView view;
    view.rows = rows;
    return view;
}

TrainingMoveListView trainingMoveListView(const AppState& state, std::vector<TrainingMoveRowView>& rows) {
    rows.clear();
    constexpr int visibleRows = 7;

    const auto entries = displayableMoveListEntries(state);
    const int maxScroll = std::max(0, static_cast<int>(entries.size()) - visibleRows);
    const int scroll = std::clamp(state.training.options.moveListScroll, 0, maxScroll);
    const int selected = entries.empty()
        ? -1
        : std::clamp(state.training.options.selectedMoveListEntry, 0, static_cast<int>(entries.size()) - 1);

    rows.reserve(visibleRows);
    for (int row = 0; row < visibleRows; ++row) {
        const int index = scroll + row;
        if (index >= static_cast<int>(entries.size())) {
            break;
        }
        const auto& entry = *entries[static_cast<size_t>(index)];
        rows.push_back(TrainingMoveRowView{
            (index + 1 < 10 ? "0" : "") + std::to_string(index + 1),
            fitDebugText(moveListEntryName(entry), 19),
            index == selected,
        });
    }

    TrainingMoveListView view;
    view.rows = rows;
    view.selectedCharacterLabel = fitDebugText(selectedCharacterName(state.selection), 17);
    view.categoryLabel = std::string(trainingMoveCategoryStatus(state.training.options.moveCategory));
    view.pageLabel = entries.empty()
        ? "0/0"
        : std::to_string(selected + 1) + "/" + std::to_string(entries.size());
    view.empty = entries.empty();

    if (selected >= 0) {
        const auto& entry = *entries[static_cast<size_t>(selected)];
        const std::string inputText = moveListInputText(entry);
        const int requiredPower = commandEntryRequiredPower(entry);
        view.detail = TrainingMoveDetailView{
            fitDebugText(moveListEntryName(entry), 17),
            commandEntryTargetLabel(entry),
            fitDebugText(inputText, 16),
            std::string(trainingMoveCategoryStatus(commandEntryCategory(entry))),
            requiredPower > 0 ? "POWER " + std::to_string(requiredPower) : "LOADED",
            fitDebugText(inputText, 26),
            true,
        };
    }

    return view;
}

void drawTrainingMoveListPage(SDL_Renderer* renderer, const AppState& state) {
    std::vector<TrainingMoveRowView> rows;
    const TrainingMoveListView view = trainingMoveListView(state, rows);
    dragon::drawTrainingMoveListPage(uiRenderContext(renderer, state), view);
}

void drawTrainingOptionsMenu(SDL_Renderer* renderer, const AppState& state) {
    if (state.training.options.moveListOpen) {
        drawTrainingMoveListPage(renderer, state);
        return;
    }

    std::vector<TrainingOptionRowView> rows;
    const TrainingOptionsMenuView view = trainingOptionsMenuView(state, rows);
    dragon::drawTrainingOptionsMenu(uiRenderContext(renderer, state), view);
}

#include "FightPresentationShared.h"

FightComboCounterView fightComboCounterView(const AppState& state, size_t attackerIndex) {
    FightComboCounterView view;
    if (attackerIndex >= state.display.comboCounters.size()) {
        return view;
    }

    const auto& combo = state.display.comboCounters[attackerIndex];
    const auto& settings = state.fightRoundSettings.combo;
    view.displayHits = combo.displayHits;
    view.displayTicks = combo.displayTicks;
    view.displayTime = std::max(1, settings.displayTime);
    view.frame = state.frame;
    view.posX = settings.posX;
    view.posY = settings.posY;
    view.startX = settings.startX;
    view.counterFontPalette = settings.counterFontPalette;
    view.counterShake = settings.counterShake;
    view.text = settings.text;
    view.textFontPalette = settings.textFontPalette;
    view.textOffsetX = settings.textOffsetX;
    view.textOffsetY = settings.textOffsetY;
    return view;
}

FightPowerGaugeView fightPowerGaugeView(const AppState& state, size_t fighterIndex) {
    FightPowerGaugeView view;
    if (fighterIndex >= state.fighters.size()) {
        return view;
    }

    const bool p2 = fighterIndex == 1;
    const auto& settings = state.fightRoundSettings.powerbar;
    view.value = state.fighters[fighterIndex].power;
    view.maxValue = std::max(1, state.characterConstants.maxPower);
    view.anchorX = motifOriginX(state) + (p2 ? settings.p2PosX : settings.p1PosX);
    view.y = p2 ? settings.p2PosY : settings.p1PosY;
    view.rangeStart = p2 ? settings.p2RangeStart : settings.p1RangeStart;
    view.rangeEnd = p2 ? settings.p2RangeEnd : settings.p1RangeEnd;
    return view;
}

FightHudView fightHudView(const AppState& state) {
    FightHudView view;
    view.p1.name = selectedCharacterName(state.selection);
    view.p1.life = state.fighters[0].life;
    view.p1.maxLife = 1000;
    view.p1.power = fightPowerGaugeView(state, 0);

    view.p2.name = compactSettingText(opponentDisplayName(state), 12);
    view.p2.life = state.fighters[1].life;
    view.p2.maxLife = 1000;
    view.p2.power = fightPowerGaugeView(state, 1);
    if (state.frontend.pendingMode == PendingMode::Arena) {
        view.arenaMode = true;
        view.arenaFighterCount = static_cast<int>(std::min<size_t>(view.arenaFighters.size(), state.fighters.size()));
        for (int i = 0; i < view.arenaFighterCount; ++i) {
            auto& fighterView = view.arenaFighters[static_cast<size_t>(i)];
            fighterView.name = compactSettingText((i == 0 ? "P1 " : "P" + std::to_string(i + 1) + " ") + arenaFighterName(state, static_cast<size_t>(i)), 13);
            fighterView.life = state.fighters[static_cast<size_t>(i)].life;
            fighterView.maxLife = 1000;
        }
    }

    view.comboCounters[0] = fightComboCounterView(state, 0);
    view.comboCounters[1] = fightComboCounterView(state, 1);
    view.showMatchTimer = isMatchMode(state);
    view.currentRound = state.currentRound;
    if (state.frontend.pendingMode != PendingMode::Training) {
        if (state.frontend.pendingMode == PendingMode::Arena) {
            view.versusLine =
                "P1 " + compactSettingText(selectedCharacterName(state.selection), 11)
                + " vs " + std::to_string(arenaCpuCount(state)) + " CPU FFA";
        } else {
            view.versusLine =
                "P1 " + compactSettingText(selectedCharacterName(state.selection), 11)
                + " vs " + compactSettingText(opponentDisplayName(state), 9);
        }
    }

    if (view.showMatchTimer) {
        const int winsRequired = matchWinsRequired(state);
        view.timerSeconds = std::max(0, (state.matchTimerTicks + 59) / 60);
        view.timerText = state.mainSettings.matchTimerSeconds <= 0 ? "INF" : std::to_string(view.timerSeconds);
        view.p1.roundPips = FightRoundPipsView{ state.roundWins[0], winsRequired };
        view.p2.roundPips = FightRoundPipsView{ state.roundWins[1], winsRequired, true };
    }

    if (state.frontend.pendingMode == PendingMode::Training
        && state.training.options.showHitLog
        && state.messages.lastHitTextTicks > 0
        && !state.messages.lastHitText.empty()) {
        view.bottomLine = state.messages.lastHitText;
        view.bottomLineHighlighted = true;
    } else if (isMatchMode(state)) {
        if (state.frontend.pendingMode == PendingMode::Arena && state.matchPhase == MatchPhase::Fight) {
            view.bottomLine = "Last fighter standing  Living: " + std::to_string(livingArenaFighterCount(state));
        } else {
            view.bottomLine = singleFightStatusLine(state);
        }
        view.bottomLineHighlighted = isSingleFightResultPhase(state);
    } else if (state.frontend.pendingMode != PendingMode::Training) {
        view.bottomLine = "A/S/D Z/X/C  R reset  F1 boxes  F2 options";
    }
    return view;
}

void drawFightHudView(SDL_Renderer* renderer, const AppState& state) {
    drawFightHud(uiRenderContext(renderer, state), fightHudView(state));
}

std::string_view matchResultLabel(int option) {
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "REMATCH",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

std::string_view arenaMatchResultLabel(int option) {
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "PLAY AGAIN",
        "ARENA SETUP",
        "FIGHTER SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

FightRoundCalloutView roundStartOverlayView(const AppState& state) {
    FightRoundCalloutView view;
    if (state.matchPhaseTicks < state.fightRoundSettings.startWaitTime) {
        return view;
    }

    const bool fightText = state.matchPhaseTicks >= singleFightRoundDisplayEndTick(state);
    view.visible = true;
    view.text = fightText ? "FIGHT" : roundStartCalloutText(state);
    view.r = 230;
    view.g = 220;
    view.b = 172;
    view.frame = state.matchPhaseTicks;
    return view;
}

FightRoundCalloutView roundFinishOverlayView(const AppState& state) {
    FightRoundCalloutView view;
    if (state.matchPhaseTicks < singleFightRoundFinishCalloutTicks(state)) {
        view.visible = true;
        view.text = roundFinishCalloutText(state);
        view.r = 230;
        view.g = 190;
        view.b = 105;
        view.frame = state.matchPhaseTicks;
        return view;
    }
    if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
        view.visible = true;
        view.text = roundResultText(state);
        view.r = 222;
        view.g = 226;
        view.b = 232;
        view.frame = state.matchPhaseTicks - state.fightRoundSettings.winTime;
    }
    return view;
}

FightRoundResultView roundResultOverlayView(const AppState& state) {
    const int winsRequired = matchWinsRequired(state);
    FightRoundResultView view;
    view.visible = true;
    view.resultText = roundResultText(state);
    view.p1RoundPips = FightRoundPipsView{ state.roundWins[0], winsRequired, false, 6.0f };
    view.p2RoundPips = FightRoundPipsView{ state.roundWins[1], winsRequired, true, 6.0f };
    view.footerText = state.matchComplete ? "MATCH COMPLETE" : "NEXT ROUND";
    if (state.frontend.pendingMode == PendingMode::Arena) {
        view.p1RoundPips = FightRoundPipsView{ 0, 0 };
        view.p2RoundPips = FightRoundPipsView{ 0, 0 };
        view.footerText = state.arenaConfig.endTitle;
    }
    view.frame = state.matchPhaseTicks;
    return view;
}

FightMatchResultView matchResultScreenView(const AppState& state) {
    FightMatchResultView view;
    const int winner = matchWinner(state);
    view.modeLabel = isMatchMode(state) ? std::string(pendingModeTitle(state.frontend.pendingMode)) : "";
    view.winnerText = winner == 0 ? "DRAW GAME" : uppercaseCopy(fighterResultName(state, winner));
    if (state.frontend.pendingMode == PendingMode::Arena) {
        view.winnerText = state.arenaConfig.endTitle;
    }
    view.scoreText = singleFightScoreText(state);
    view.methodText = matchWinMethodText(state);
    view.quoteText = winner > 0 && winner <= static_cast<int>(state.fighters.size())
        ? selectedVictoryQuoteText(state, state.fighters[static_cast<size_t>(winner - 1)])
        : std::string{};
    if (state.frontend.pendingMode == PendingMode::Arena) {
        view.quoteText.clear();
    }
    view.stageText = "Stage: " + selectedStageName(state.selection);
    view.menuRowCount = kMatchResultOptionCount;
    view.frame = state.matchPhaseTicks;
    for (int i = 0; i < kMatchResultOptionCount; ++i) {
        auto& row = view.menuRows[static_cast<size_t>(i)];
        row.label = std::string(state.frontend.pendingMode == PendingMode::Arena ? arenaMatchResultLabel(i) : matchResultLabel(i));
        row.selected = i == state.frontend.selectedMatchResultOption;
    }
    return view;
}

void drawRoundStartOverlay(SDL_Renderer* renderer, const AppState& state) {
    drawRoundStartOverlay(uiRenderContext(renderer, state), roundStartOverlayView(state));
}

void drawRoundFinishOverlay(SDL_Renderer* renderer, const AppState& state) {
    drawRoundFinishOverlay(uiRenderContext(renderer, state), roundFinishOverlayView(state));
}

void drawRoundResultOverlay(SDL_Renderer* renderer, const AppState& state) {
    drawRoundResultOverlay(uiRenderContext(renderer, state), roundResultOverlayView(state));
}

void drawMatchResultScreen(SDL_Renderer* renderer, const AppState& state) {
    drawMatchResultScreen(uiRenderContext(renderer, state), matchResultScreenView(state));
}

void drawFightView(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    const bool hasStageBackground = hasSelectedStageBackground(state);
    const bool hideBackground = anyFighterHasAssertSpecialFlag(state, "nobg");
    const bool hideForeground = anyFighterHasAssertSpecialFlag(state, "nofg");
    const bool hideHud = anyFighterHasAssertSpecialFlag(state, "nobardisplay");
    const bool arenaMode = state.frontend.pendingMode == PendingMode::Arena;
    SDL_Rect defaultViewport;
    SDL_GetRenderViewport(renderer, &defaultViewport);
    const int shakeOffsetY = arenaMode ? 0 : static_cast<int>(std::lround(state.display.envShakeOffsetY));
    int impactShakeX = 0;
    int impactShakeY = 0;
    int maxHitPause = 0;
    for (const auto& fighter : state.fighters) {
        maxHitPause = std::max(maxHitPause, fighter.hitPauseTicks);
    }
    if (!arenaMode && maxHitPause >= 6) {
        const int magnitude = std::clamp(maxHitPause / 6, 1, 2);
        impactShakeX = ((state.frontend.screenFrame / 2) % 2 == 0) ? magnitude : -magnitude;
        impactShakeY = ((state.frontend.screenFrame / 3) % 2 == 0) ? 1 : -1;
    }
    const int viewportOffsetX = impactShakeX;
    const int viewportOffsetY = shakeOffsetY + impactShakeY;
    if (viewportOffsetX != 0 || viewportOffsetY != 0) {
        SDL_Rect shakeViewport{ viewportOffsetX, viewportOffsetY, logicalWidth(state), kLogicalHeight };
        SDL_SetRenderViewport(renderer, &shakeViewport);
    }

    if (hasStageBackground && !hideBackground) {
        drawStageLayer(renderer, state, 0);
    } else if (!hideBackground) {
        drawFallbackStage(renderer, state, stage, state.cameraY);
    }
    if (!hideBackground) {
        drawPaletteOverlay(renderer, state, state.backgroundPaletteEffect, 180);
    }

    drawWorldActors(renderer, state, stage);
    if (hasStageBackground && !hideForeground) {
        drawStageLayer(renderer, state, 1);
    }

    if (!hasStageBackground && !hideBackground) {
        setColor(renderer, 74, 100, 128);
        fillRect(renderer, 18, stage.zoffset - state.cameraY - 62.0f, 284, 8);
    }

    if (viewportOffsetX != 0 || viewportOffsetY != 0) {
        SDL_SetRenderViewport(renderer, &defaultViewport);
    }

    if (state.envColor.ticksLeft > 0) {
        setColor(
            renderer,
            static_cast<Uint8>(state.envColor.r),
            static_cast<Uint8>(state.envColor.g),
            static_cast<Uint8>(state.envColor.b),
            220);
        fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    }

    if (!hideHud && state.frontend.pendingMode == PendingMode::Training) {
        drawDebugOverlay(renderer, state, stage);
    }

    if (!hideHud) {
        drawFightHudView(renderer, state);
    }

    if (state.frontend.pendingMode == PendingMode::Training && !state.training.options.menuOpen && !hideHud) {
        drawTrainingCommandHud(renderer, state);
    }

    if (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen) {
        drawTrainingOptionsMenu(renderer, state);
    } else if (isMatchMode(state) && state.frontend.singleFightPauseOpen) {
        PauseMenuView view{
            pendingModeTitle(state.frontend.pendingMode),
            state.frontend.selectedSingleFightPauseOption,
        };
        if (state.frontend.pendingMode == PendingMode::Arena) {
            view.optionLabels[3] = "ARENA SETUP";
        }
        drawSingleFightPauseMenu(uiRenderContext(renderer, state), view);
    } else if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundStart) {
        drawRoundStartOverlay(renderer, state);
    } else if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundFinish) {
        drawRoundFinishOverlay(renderer, state);
    } else if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundResult) {
        drawRoundResultOverlay(renderer, state);
    } else if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
        drawMatchResultScreen(renderer, state);
    }

    SDL_RenderPresent(renderer);
}

#include "FrontendFlow.h"

void pumpEvents(SDL_Renderer* renderer, AppState& state) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            state.running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat) {
                handleKey(renderer, state, event.key.key);
            }
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            openGamepadDevice(state, event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            closeGamepadDevice(state, event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMAPPED:
            refreshGamepadDevice(state, event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            handleGamepadButton(
                renderer,
                state,
                event.gbutton.which,
                static_cast<SDL_GamepadButton>(event.gbutton.button));
            break;
        default:
            break;
        }
    }
}

void fixedUpdate(AppState& state) {
    ++state.frame;
    ++state.frontend.screenFrame;
    if (state.frontend.screen == Screen::VersusScreen && state.fightSessionPrepared && state.frontend.screenFrame > 120) {
        beginFight(state);
    }
    const bool fightPaused =
        (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen)
        || (isMatchMode(state) && state.frontend.singleFightPauseOpen);
    if (state.frontend.screen == Screen::FightView && !fightPaused) {
        updateFight(state);
        applyTrainingPowerMode(state);
    }
    updateAudioMixer(state);
}

void applyLogicalPresentation(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderLogicalPresentation(renderer, logicalWidth(state), kLogicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void clearPhysicalFrame(SDL_Renderer* renderer) {
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_SetRenderViewport(renderer, nullptr);
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);
}

#include "AppVerificationBridge.h"

} // namespace

int runVerificationScenario(
    const std::filesystem::path& gameRoot,
    std::string_view scenarioName,
    std::ostream& out) {
    return runVerificationScenarioInternal(gameRoot, scenarioName, out);
}

int runApp(const std::filesystem::path& gameRoot) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Dragon MUGEN", kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderLogicalPresentation(renderer, kLogicalWidth, kLogicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    AppState state;
    state.gameRoot = gameRoot;
    state.arenaConfig = loadArenaConfig(gameRoot);
    setArenaCpuCount(state, state.arenaConfig.cpuCountDefault);
    initAudio(state);
    state.fightRoundSettings = loadFightRoundSettings(gameRoot);
    state.selection.characters = loadCharacters(gameRoot);
    state.selection.stages = loadStages(gameRoot);
    selectPreferredStage(state);
    loadVisualAssets(renderer, state);
    openExistingGamepads(state);

    using clock = std::chrono::steady_clock;
    constexpr double fixedStep = 1.0 / 60.0;
    auto previous = clock::now();

    while (state.running) {
        const auto now = clock::now();
        const std::chrono::duration<double> elapsed = now - previous;
        previous = now;
        state.accumulator += elapsed.count();

        pumpEvents(renderer, state);
        while (state.accumulator >= fixedStep) {
            fixedUpdate(state);
            state.accumulator -= fixedStep;
        }

        clearPhysicalFrame(renderer);
        applyLogicalPresentation(renderer, state);

        if (state.frontend.screen == Screen::ModeSelect) {
            drawModeSelect(renderer, state);
        } else if (state.frontend.screen == Screen::MainSettings) {
            drawMainSettings(renderer, state);
        } else if (state.frontend.screen == Screen::CharacterSelect) {
            drawCharacterSelect(renderer, state);
        } else if (state.frontend.screen == Screen::ArenaSetup) {
            drawArenaSetup(renderer, state);
        } else if (state.frontend.screen == Screen::StageSelect) {
            drawStageSelect(renderer, state);
        } else if (state.frontend.screen == Screen::VersusScreen) {
            drawVersusScreen(renderer, state);
            prepareVersusSessionAfterPresent(renderer, state);
        } else if (state.frontend.screen == Screen::FightView) {
            drawFightView(renderer, state);
        }

        SDL_Delay(1);
    }

    closeAllGamepads(state);
    destroyVisualAssets(state);
    destroyAudioAssets(state);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace dragon
