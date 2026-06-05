#include "dragon/App.h"
#include "dragon/FightData.h"
#include "dragon/MugenData.h"
#include "dragon/MugenText.h"
#include "dragon/Sff.h"
#include "dragon/Snd.h"
#include "Input.h"
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

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;
constexpr int kClassicLogicalWidth = 320;
constexpr int kDefaultLogicalWidth = 426;
constexpr int kExtraWideLogicalWidth = 480;
constexpr int kLogicalHeight = 240;
constexpr int kLogicalWidth = kDefaultLogicalWidth;
constexpr int kTrainingOptionCount = 20;
constexpr int kTrainingOptionRows = 10;
constexpr int kTrainingP2ControlOption = 13;
constexpr int kTrainingCommandHudOption = 14;
constexpr int kTrainingInputHudOption = 15;
constexpr int kTrainingPowerOption = 16;
constexpr int kTrainingMoveTypeOption = 17;
constexpr int kTrainingMoveListOption = 18;
constexpr int kTrainingResetOption = 19;
constexpr int kSingleFightPauseOptionCount = 5;
constexpr int kMatchResultOptionCount = 4;
constexpr int kMainSettingsCount = 7;
constexpr int kVersusPrepareStartFrames = 2;
constexpr int kCharacterSelectColumns = 5;
constexpr int kCharacterSelectRows = 2;
constexpr int kCharacterSelectPageSize = kCharacterSelectColumns * kCharacterSelectRows;

enum class Screen {
    ModeSelect,
    CharacterSelect,
    StageSelect,
    VersusScreen,
    FightView,
    MainSettings,
};

enum class PendingMode {
    Training,
    SinglePlayer,
    SingleFight,
};

enum class OpponentType {
    Dummy,
    Cpu,
    LocalP2,
};

enum class MatchPhase {
    RoundStart,
    Fight,
    RoundFinish,
    RoundResult,
    MatchResult,
};

enum class RoundEndReason {
    None,
    Ko,
    TimeUp,
    DoubleKo,
};

enum class DummyGuardMode {
    Off,
    Stand,
    Crouch,
    Auto,
};

enum class TrainingPowerMode {
    Normal,
    Max,
};

enum class TrainingMoveCategory {
    All,
    Normals,
    Specials,
    Supers,
};

enum class GuardStance {
    None,
    Stand,
    Crouch,
};

enum class GamepadPromptStyle {
    Auto,
    Xbox,
    Playstation,
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

struct TrainingOptions {
    bool menuOpen = false;
    bool moveListOpen = false;
    int selectedOption = 0;
    int selectedMoveListEntry = 0;
    int moveListScroll = 0;
    bool showHitboxes = false;
    bool showOrigins = false;
    bool showFloorLine = false;
    bool showDebugReadout = false;
    bool showHitFlash = true;
    bool showHitSparks = true;
    bool showHitLog = true;
    bool playHitSounds = true;
    bool dummyInvincible = false;
    bool dummyAutoLife = false;
    bool dummyFrozen = false;
    DummyGuardMode dummyGuardMode = DummyGuardMode::Off;
    bool guardDamage = true;
    bool p2Controlled = false;
    bool showCommandHud = true;
    bool showInputHud = true;
    TrainingPowerMode powerMode = TrainingPowerMode::Normal;
    TrainingMoveCategory moveCategory = TrainingMoveCategory::All;
};

struct MainSettings {
    int selectedOption = 0;
    int matchTimerSeconds = 99;
    int canvasWidth = kDefaultLogicalWidth;
    int uiScalePercent = 80;
    GamepadPromptStyle gamepadPromptStyle = GamepadPromptStyle::Auto;
    int p1GamepadAssignment = 0;
    int p2GamepadAssignment = 0;
};

struct LoadedContentSummary {
    std::string characterName = "Unknown";
    std::string characterAuthor = "Unknown";
    std::string stageName = "Unknown";
    int airActions = 0;
    int cnsStates = 0;
    int cmdCommands = 0;
    int stageBackgrounds = 0;
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
    std::vector<DecodedSoundSample> commonSamples;
    std::vector<DecodedSoundSample> fightSamples;
    std::vector<ActiveSoundVoice> activeVoices;
    std::vector<float> mixBuffer;
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

struct ComboCounterState {
    int activeHits = 0;
    int displayHits = 0;
    int displayTicks = 0;
};

struct FightSessionSlots {
    int p1Character = -1;
    int opponentCharacter = -1;
    OpponentType opponentType = OpponentType::Dummy;
};

struct AppState {
    std::filesystem::path gameRoot;
    LoadedContentSummary content;
    std::vector<CharacterSlot> characters;
    std::vector<StageSlot> stages;
    Screen screen = Screen::ModeSelect;
    PendingMode pendingMode = PendingMode::Training;
    int selectedMode = 0;
    int selectedCharacter = 0;
    int loadedP1Character = -1;
    int selectedStage = 0;
    FightSessionSlots sessionSlots;
    bool menuRailOnLeft = true;
    bool running = true;
    TrainingOptions trainingOptions;
    MainSettings mainSettings;
    FightRoundSettings fightRoundSettings;
    bool singleFightPauseOpen = false;
    int selectedSingleFightPauseOption = 0;
    int selectedMatchResultOption = 0;
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
    int envShakeTicks = 0;
    int envShakeTotalTicks = 0;
    int envShakeFrequency = 60;
    float envShakeAmplitude = 0.0f;
    int envShakePhase = 0;
    float envShakeOffsetY = 0.0f;
    ActivePaletteEffect backgroundPaletteEffect;
    EnvColorEffect envColor;
    double accumulator = 0.0;
    int frame = 0;
    int screenFrame = 0;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    std::array<FighterState, 2> fighters;
    std::vector<FighterState> helpers;
    std::vector<RuntimeProjectile> projectiles;
    std::array<ComboCounterState, 2> comboCounters;
    CharacterConstants characterConstants;
    std::vector<HitDefinition> hitDefs;
    std::vector<StateDefinition> stateDefs;
    std::vector<CommandStateEntry> commandEntries;
    std::vector<CommandDefinition> commandDefinitions;
    std::string lastHitText;
    int lastHitTextTicks = 0;
    std::vector<AnimationClip> characterClips;
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

const CharacterSlot* characterSlotAt(const AppState& state, int index) {
    if (state.characters.empty()
        || index < 0
        || index >= static_cast<int>(state.characters.size())) {
        return nullptr;
    }
    return &state.characters[static_cast<size_t>(index)];
}

const CharacterSlot* selectedCharacterSlot(const AppState& state) {
    return characterSlotAt(state, state.selectedCharacter);
}

const CharacterSlot* sessionP1CharacterSlot(const AppState& state) {
    if (const CharacterSlot* character = characterSlotAt(state, state.sessionSlots.p1Character)) {
        return character;
    }
    return selectedCharacterSlot(state);
}

int sessionP1CharacterIndex(const AppState& state) {
    return characterSlotAt(state, state.sessionSlots.p1Character)
        ? state.sessionSlots.p1Character
        : state.selectedCharacter;
}

OpponentType defaultOpponentTypeForMode(PendingMode mode) {
    switch (mode) {
    case PendingMode::Training:
        return OpponentType::Dummy;
    case PendingMode::SinglePlayer:
        return OpponentType::Cpu;
    case PendingMode::SingleFight:
    default:
        return OpponentType::LocalP2;
    }
}

bool isMatchMode(PendingMode mode) {
    return mode == PendingMode::SinglePlayer || mode == PendingMode::SingleFight;
}

bool isMatchMode(const AppState& state) {
    return isMatchMode(state.pendingMode);
}

OpponentType activeOpponentType(const AppState& state) {
    if (state.pendingMode == PendingMode::Training && state.trainingOptions.p2Controlled) {
        return OpponentType::LocalP2;
    }
    return state.sessionSlots.opponentType;
}

bool usesLocalP2Controls(const AppState& state) {
    return activeOpponentType(state) == OpponentType::LocalP2;
}

std::string_view pendingModeTitle(PendingMode mode) {
    switch (mode) {
    case PendingMode::Training:
        return "TRAINING";
    case PendingMode::SinglePlayer:
        return "SINGLE PLAYER";
    case PendingMode::SingleFight:
    default:
        return "VS MODE";
    }
}

std::string_view opponentTypeLabel(OpponentType type) {
    switch (type) {
    case OpponentType::Dummy:
        return "DUMMY";
    case OpponentType::Cpu:
        return "CPU";
    case OpponentType::LocalP2:
    default:
        return "P2";
    }
}

void configureFightSessionSlotsFromSelection(AppState& state) {
    const int characterCount = static_cast<int>(state.characters.size());
    state.sessionSlots.p1Character = characterCount > 0
        ? std::clamp(state.selectedCharacter, 0, characterCount - 1)
        : -1;
    state.sessionSlots.opponentType = defaultOpponentTypeForMode(state.pendingMode);
    state.sessionSlots.opponentCharacter = -1;
}

std::string opponentDisplayName(const AppState& state) {
    if (const CharacterSlot* character = characterSlotAt(state, state.sessionSlots.opponentCharacter)) {
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

void setColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void fillRect(SDL_Renderer* renderer, float x, float y, float w, float h) {
    SDL_FRect rect{ x, y, w, h };
    SDL_RenderFillRect(renderer, &rect);
}

void drawRect(SDL_Renderer* renderer, float x, float y, float w, float h) {
    SDL_FRect rect{ x, y, w, h };
    SDL_RenderRect(renderer, &rect);
}

float uiScale(const AppState& state) {
    return std::clamp(static_cast<float>(state.mainSettings.uiScalePercent) / 100.0f, 0.6f, 1.0f);
}

void fillScaledRect(SDL_Renderer* renderer, float scale, float x, float y, float w, float h) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    fillRect(renderer, x / scale, y / scale, w / scale, h / scale);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

void drawScaledRect(SDL_Renderer* renderer, float scale, float x, float y, float w, float h) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    drawRect(renderer, x / scale, y / scale, w / scale, h / scale);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

void debugText(SDL_Renderer* renderer, float x, float y, const std::string& text) {
    SDL_RenderDebugText(renderer, x, y, text.c_str());
}

void debugText(SDL_Renderer* renderer, float x, float y, const char* text) {
    SDL_RenderDebugText(renderer, x, y, text);
}

void scaledDebugText(SDL_Renderer* renderer, float scale, float x, float y, const std::string& text) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    debugText(renderer, x / scale, y / scale, text);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

struct ScopedUiScale {
    SDL_Renderer* renderer = nullptr;
    SDL_Rect oldViewport{};
    float oldScaleX = 1.0f;
    float oldScaleY = 1.0f;

    ScopedUiScale(SDL_Renderer* renderTarget, const AppState& state, float virtualWidth, float virtualHeight)
        : renderer(renderTarget) {
        const float scale = uiScale(state);
        SDL_GetRenderViewport(renderer, &oldViewport);
        SDL_GetRenderScale(renderer, &oldScaleX, &oldScaleY);

        const float width = virtualWidth * scale;
        const float height = virtualHeight * scale;
        SDL_Rect viewport{
            static_cast<int>(std::lround((logicalWidthF(state) - width) * 0.5f)),
            static_cast<int>(std::lround((static_cast<float>(kLogicalHeight) - height) * 0.5f)),
            static_cast<int>(std::lround(width)),
            static_cast<int>(std::lround(height)),
        };
        SDL_SetRenderViewport(renderer, &viewport);
        SDL_SetRenderScale(renderer, scale, scale);
    }

    ~ScopedUiScale() {
        SDL_SetRenderScale(renderer, oldScaleX, oldScaleY);
        SDL_SetRenderViewport(renderer, &oldViewport);
    }
};

void debugTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text) {
    debugText(renderer, centerX - static_cast<float>(text.size()) * 4.0f, y, text);
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

void drawPanel(SDL_Renderer* renderer, float x, float y, float w, float h) {
    setColor(renderer, 18, 20, 24, 232);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 78, 90, 112);
    drawRect(renderer, x, y, w, h);
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

std::optional<int> parseStateNumber(std::string_view sectionName, std::string_view prefix) {
    if (!startsWithNoCase(sectionName, prefix)) {
        return std::nullopt;
    }

    try {
        return std::stoi(trim(sectionName.substr(prefix.size())));
    } catch (...) {
        return std::nullopt;
    }
}

char parseStateChar(const MugenSection* section, std::string_view key, char fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    const auto value = trim(property->value);
    if (value.empty()) {
        return fallback;
    }
    return static_cast<char>(SDL_toupper(static_cast<unsigned char>(value.front())));
}

std::string stripOuterParens(std::string value);

std::optional<int> parseTimeTrigger(const std::string& trigger) {
    const std::string condition = stripOuterParens(trigger);
    if (!startsWithNoCase(condition, "Time")) {
        return std::nullopt;
    }
    const auto equals = condition.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }
    return parseIntValue(trim(std::string_view(condition).substr(equals + 1)), 0);
}

std::optional<int> parseAnimElemTrigger(const std::string& trigger) {
    const std::string condition = stripOuterParens(trigger);
    if (!startsWithNoCase(condition, "AnimElem") || startsWithNoCase(condition, "AnimElemTime")) {
        return std::nullopt;
    }
    const auto equals = condition.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }
    const auto valuePart = trim(std::string_view(condition).substr(equals + 1));
    const auto values = splitCsv(valuePart);
    if (values.empty()) {
        return std::nullopt;
    }
    return parseIntValue(values.front(), 0);
}

std::optional<std::pair<CompareOp, size_t>> findCompareOp(std::string_view value) {
    const std::array<std::pair<std::string_view, CompareOp>, 6> ops{ {
        { ">=", CompareOp::GreaterEqual },
        { "<=", CompareOp::LessEqual },
        { "!=", CompareOp::NotEqual },
        { "=", CompareOp::Equal },
        { ">", CompareOp::Greater },
        { "<", CompareOp::Less },
    } };

    for (const auto& [token, op] : ops) {
        if (const auto pos = value.find(token); pos != std::string_view::npos) {
            return std::pair<CompareOp, size_t>{ op, pos };
        }
    }
    return std::nullopt;
}

std::string stripOuterParens(std::string value) {
    value = trim(value);
    while (value.size() >= 2 && value.front() == '(' && value.back() == ')') {
        value = trim(std::string_view(value).substr(1, value.size() - 2));
    }
    return value;
}

std::vector<std::string> splitAndClauses(const std::string& trigger) {
    std::vector<std::string> clauses;
    size_t start = 0;
    while (start <= trigger.size()) {
        const auto delimiter = trigger.find("&&", start);
        const auto end = delimiter == std::string::npos ? trigger.size() : delimiter;
        clauses.push_back(stripOuterParens(trim(std::string_view(trigger).substr(start, end - start))));
        if (delimiter == std::string::npos) {
            break;
        }
        start = delimiter + 2;
    }
    return clauses;
}

std::optional<AnimElemTimeCondition> parseAnimElemTimeCondition(const std::string& trigger) {
    const std::string condition = stripOuterParens(trigger);
    if (!startsWithNoCase(condition, "AnimElemTime")) {
        return std::nullopt;
    }

    const auto open = condition.find('(');
    const auto close = condition.find(')', open == std::string::npos ? 0 : open + 1);
    if (open == std::string::npos || close == std::string::npos || close <= open + 1) {
        return std::nullopt;
    }

    const int elem = parseIntValue(trim(std::string_view(condition).substr(open + 1, close - open - 1)), 0);
    const auto tail = trim(std::string_view(condition).substr(close + 1));
    const auto compare = findCompareOp(tail);
    if (!compare) {
        return std::nullopt;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    return AnimElemTimeCondition{
        elem,
        op,
        parseIntValue(trim(std::string_view(tail).substr(pos + opLength)), 0),
    };
}

bool triggerIsMoveContact(const std::string& trigger) {
    const auto trimmed = stripOuterParens(trigger);
    return startsWithNoCase(trimmed, "movecontact") && trimmed.size() == std::string_view("movecontact").size();
}

struct ParsedCommandCondition {
    std::string command;
    bool negated = false;
};

std::vector<ParsedCommandCondition> findCommandConditions(const std::string& value) {
    std::vector<ParsedCommandCondition> commands;
    const std::string condition = stripOuterParens(value);
    size_t cursor = 0;
    while (cursor < condition.size()) {
        const size_t commandPos = findNoCase(condition, "command", cursor);
        if (commandPos == std::string::npos) {
            break;
        }

        const bool leftBoundary = commandPos == 0 || !isIdentifierChar(condition[commandPos - 1]);
        const size_t afterName = commandPos + std::string_view("command").size();
        const bool rightBoundary = afterName >= condition.size() || !isIdentifierChar(condition[afterName]);
        if (!leftBoundary || !rightBoundary) {
            cursor = afterName;
            continue;
        }

        cursor = afterName;
        while (cursor < condition.size() && std::isspace(static_cast<unsigned char>(condition[cursor]))) {
            ++cursor;
        }

        bool negated = false;
        if (cursor + 1 < condition.size() && condition[cursor] == '!' && condition[cursor + 1] == '=') {
            negated = true;
            cursor += 2;
        } else if (cursor < condition.size() && condition[cursor] == '=') {
            ++cursor;
        } else {
            continue;
        }

        while (cursor < condition.size() && std::isspace(static_cast<unsigned char>(condition[cursor]))) {
            ++cursor;
        }
        if (cursor >= condition.size() || condition[cursor] != '"') {
            continue;
        }

        const size_t closingQuote = condition.find('"', cursor + 1);
        if (closingQuote == std::string::npos) {
            break;
        }

        commands.push_back(ParsedCommandCondition{
            std::string(std::string_view(condition).substr(cursor + 1, closingQuote - cursor - 1)),
            negated,
        });
        cursor = closingQuote + 1;
    }
    return commands;
}

bool isSupportedStateType(char value);
void addUniqueCommand(std::vector<std::string>& commands, const std::string& command);

std::vector<std::string> splitTopLevelClauses(const std::string& expression, std::string_view delimiter, bool allowSinglePipe = false) {
    std::vector<std::string> clauses;
    size_t start = 0;
    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;

    for (size_t i = 0; i < expression.size(); ++i) {
        const char ch = expression[i];
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) {
            continue;
        }
        if (ch == '(') {
            ++parenDepth;
        } else if (ch == ')' && parenDepth > 0) {
            --parenDepth;
        } else if (ch == '[') {
            ++bracketDepth;
        } else if (ch == ']' && bracketDepth > 0) {
            --bracketDepth;
        }

        if (parenDepth != 0 || bracketDepth != 0) {
            continue;
        }

        bool matched = false;
        size_t delimiterLength = delimiter.size();
        if (!delimiter.empty() && i + delimiterLength <= expression.size()) {
            matched = std::string_view(expression).substr(i, delimiterLength) == delimiter;
        }
        if (!matched && allowSinglePipe && ch == '|') {
            matched = true;
            delimiterLength = i + 1 < expression.size() && expression[i + 1] == '|' ? 2 : 1;
        }
        if (!matched) {
            continue;
        }

        clauses.push_back(stripOuterParens(trim(std::string_view(expression).substr(start, i - start))));
        start = i + delimiterLength;
        i += delimiterLength - 1;
    }

    clauses.push_back(stripOuterParens(trim(std::string_view(expression).substr(start))));
    return clauses;
}

std::optional<size_t> findTopLevelBinaryOperator(
    std::string_view expression,
    const std::vector<std::string_view>& operators) {
    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;

    for (size_t i = expression.size(); i-- > 0;) {
        const char ch = expression[i];
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) {
            continue;
        }
        if (ch == ')') {
            ++parenDepth;
            continue;
        }
        if (ch == '(' && parenDepth > 0) {
            --parenDepth;
            continue;
        }
        if (ch == ']') {
            ++bracketDepth;
            continue;
        }
        if (ch == '[' && bracketDepth > 0) {
            --bracketDepth;
            continue;
        }
        if (parenDepth != 0 || bracketDepth != 0) {
            continue;
        }

        for (const auto op : operators) {
            if (i + op.size() > expression.size()
                || expression.substr(i, op.size()) != op) {
                continue;
            }
            if ((op == "+" || op == "-") && i == 0) {
                continue;
            }
            if ((op == "+" || op == "-")
                && i > 0
                && (expression[i - 1] == 'e' || expression[i - 1] == 'E')) {
                continue;
            }
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::pair<CompareOp, size_t>> findTopLevelCompareOp(std::string_view expression) {
    const std::array<std::pair<std::string_view, CompareOp>, 6> ops{ {
        { ">=", CompareOp::GreaterEqual },
        { "<=", CompareOp::LessEqual },
        { "!=", CompareOp::NotEqual },
        { "=", CompareOp::Equal },
        { ">", CompareOp::Greater },
        { "<", CompareOp::Less },
    } };

    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;
    for (size_t i = 0; i < expression.size(); ++i) {
        const char ch = expression[i];
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) {
            continue;
        }
        if (ch == '(') {
            ++parenDepth;
            continue;
        }
        if (ch == ')' && parenDepth > 0) {
            --parenDepth;
            continue;
        }
        if (ch == '[') {
            ++bracketDepth;
            continue;
        }
        if (ch == ']' && bracketDepth > 0) {
            --bracketDepth;
            continue;
        }
        if (parenDepth != 0 || bracketDepth != 0) {
            continue;
        }

        for (const auto& [token, op] : ops) {
            if (i + token.size() <= expression.size()
                && expression.substr(i, token.size()) == token) {
                return std::pair<CompareOp, size_t>{ op, i };
            }
        }
    }
    return std::nullopt;
}

std::optional<MugenVariableRef> parseMugenVariableRef(std::string_view value) {
    const std::string trimmed = trim(value);
    const struct {
        std::string_view prefix;
        MugenVariableBank bank;
    } prefixes[] = {
        { "sysfvar(", MugenVariableBank::SysFVar },
        { "sysvar(", MugenVariableBank::SysVar },
        { "fvar(", MugenVariableBank::FVar },
        { "var(", MugenVariableBank::Var },
    };

    for (const auto& candidate : prefixes) {
        if (!startsWithNoCase(trimmed, candidate.prefix)) {
            continue;
        }
        const auto close = trimmed.find(')', candidate.prefix.size());
        if (close == std::string::npos) {
            return std::nullopt;
        }
        const auto index = parsePlainIntValue(trim(std::string_view(trimmed).substr(candidate.prefix.size(), close - candidate.prefix.size())));
        if (!index || *index < 0) {
            return std::nullopt;
        }
        return MugenVariableRef{ candidate.bank, *index };
    }
    return std::nullopt;
}

std::optional<MugenExpressionCondition> parseMugenExpressionCondition(const std::string& clause) {
    const std::string condition = stripOuterParens(clause);
    const auto compare = findTopLevelCompareOp(condition);
    if (!compare) {
        return std::nullopt;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    const std::string lhs = trim(std::string_view(condition).substr(0, pos));
    const std::string rhs = trim(std::string_view(condition).substr(pos + opLength));
    if (lhs.empty() || rhs.empty()) {
        return std::nullopt;
    }
    return MugenExpressionCondition{ lhs, op, rhs };
}

bool expressionLooksSupported(std::string_view value) {
    const std::string lowered = lowercaseCopy(value);
    constexpr std::array<std::string_view, 53> supportedTokens{
        "var(", "fvar(", "sysvar(", "sysfvar(", "random", "ailevel", "teammode", "time", "animtime",
        "stateno", "roundstate", "roundno", "roundsexisted", "power", "powermax", "hitcount", "life", "p2life", "statetype", "movetype",
        "pos", "vel", "p2bodydist", "p2dist", "p2stateno", "p2statetype", "p2movetype", "movecontact", "movehit", "moveguarded",
        "numtarget", "frontedgedist", "backedgedist", "frontedgebodydist", "backedgebodydist", "facing", "alive", "selfanimexist", "numhelper", "numproj", "numprojid",
        "projhit", "projcontact", "projguarded", "projcontacttime", "projguardedtime", "gethitvar", "parent,", "root,",
        "enemy,", "enemynear,", "helper(", "target,",
    };
    return std::any_of(supportedTokens.begin(), supportedTokens.end(), [&lowered](std::string_view token) {
        return lowered.find(token) != std::string::npos;
    });
}

std::optional<StateTriggerSubject> parseStateTriggerSubject(const std::string& clause, size_t& subjectEnd) {
    const std::string lowered = lowercaseCopy(clause);
    if (startsWithNoCase(lowered, "time") && (lowered.size() == 4 || !isIdentifierChar(lowered[4]))) {
        subjectEnd = 4;
        return StateTriggerSubject::Time;
    }
    if (startsWithNoCase(lowered, "animtime") && (lowered.size() == 8 || !isIdentifierChar(lowered[8]))) {
        subjectEnd = 8;
        return StateTriggerSubject::AnimTime;
    }
    if (startsWithNoCase(lowered, "vel")) {
        size_t cursor = 3;
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::VelX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::VelY;
        }
    }
    if (startsWithNoCase(lowered, "pos")) {
        size_t cursor = 3;
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::PosX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::PosY;
        }
    }
    if (startsWithNoCase(lowered, "p2bodydist")) {
        size_t cursor = std::string_view("p2bodydist").size();
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2BodyDistX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2BodyDistY;
        }
    }
    if (startsWithNoCase(lowered, "p2dist")) {
        size_t cursor = std::string_view("p2dist").size();
        while (cursor < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[cursor]))) {
            ++cursor;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'x') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2DistX;
        }
        if (cursor < lowered.size() && lowered[cursor] == 'y') {
            subjectEnd = cursor + 1;
            return StateTriggerSubject::P2DistY;
        }
    }
    if (startsWithNoCase(lowered, "frontedgebodydist") || startsWithNoCase(lowered, "frontedgedist")) {
        subjectEnd = startsWithNoCase(lowered, "frontedgebodydist")
            ? std::string_view("frontedgebodydist").size()
            : std::string_view("frontedgedist").size();
        return StateTriggerSubject::FrontEdgeBodyDist;
    }
    if (startsWithNoCase(lowered, "backedgebodydist") || startsWithNoCase(lowered, "backedgedist")) {
        subjectEnd = startsWithNoCase(lowered, "backedgebodydist")
            ? std::string_view("backedgebodydist").size()
            : std::string_view("backedgedist").size();
        return StateTriggerSubject::BackEdgeBodyDist;
    }
    if (startsWithNoCase(lowered, "hitshakeover")) {
        subjectEnd = std::string_view("hitshakeover").size();
        return StateTriggerSubject::HitShakeOver;
    }
    return std::nullopt;
}

std::optional<StateRangeCondition> parseStateRangeCondition(StateTriggerSubject subject, const std::string& rhs) {
    const std::string value = trim(rhs);
    if (value.size() < 5 || value.front() != '[' || value.back() != ']') {
        return std::nullopt;
    }
    const auto parts = splitCsv(std::string(std::string_view(value).substr(1, value.size() - 2)));
    if (parts.size() != 2) {
        return std::nullopt;
    }
    const auto minValue = parsePlainFloatValue(parts[0]);
    const auto maxValue = parsePlainFloatValue(parts[1]);
    if (!minValue || !maxValue) {
        return std::nullopt;
    }
    return StateRangeCondition{ subject, *minValue, *maxValue };
}

bool parseStateFloatCondition(const std::string& clause, StateTriggerGroup& group) {
    if (equalsNoCase(clause, "!time")) {
        group.floatConditions.push_back(StateFloatCondition{ StateTriggerSubject::Time, CompareOp::Equal, 0.0f });
        return true;
    }
    if (equalsNoCase(clause, "!animtime")) {
        group.floatConditions.push_back(StateFloatCondition{ StateTriggerSubject::AnimTime, CompareOp::Equal, 0.0f });
        return true;
    }

    size_t subjectEnd = 0;
    const auto subject = parseStateTriggerSubject(clause, subjectEnd);
    if (!subject) {
        return false;
    }

    const std::string tail = trim(std::string_view(clause).substr(subjectEnd));
    const auto compare = findCompareOp(tail);
    if (!compare) {
        return false;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    const std::string rhs = trim(std::string_view(tail).substr(pos + opLength));
    if (op == CompareOp::Equal) {
        if (const auto range = parseStateRangeCondition(*subject, rhs)) {
            group.rangeConditions.push_back(*range);
            return true;
        }
    }

    const auto value = parsePlainFloatValue(rhs);
    if (!value) {
        return false;
    }
    group.floatConditions.push_back(StateFloatCondition{ *subject, op, *value });
    return true;
}

bool parseStateTypeTriggerClause(const std::string& clause, StateTriggerGroup& group) {
    const std::string condition = stripOuterParens(clause);
    if (!startsWithNoCase(condition, "statetype")) {
        return false;
    }

    std::string tail = trim(std::string_view(condition).substr(std::string_view("statetype").size()));
    bool negated = false;
    if (startsWithNoCase(tail, "!=")) {
        negated = true;
        tail = trim(std::string_view(tail).substr(2));
    } else if (startsWithNoCase(tail, "=")) {
        tail = trim(std::string_view(tail).substr(1));
    } else {
        return false;
    }

    if (tail.empty()) {
        return false;
    }

    const char stateType = static_cast<char>(SDL_toupper(static_cast<unsigned char>(tail.front())));
    if (!isSupportedStateType(stateType)) {
        return false;
    }
    group.stateTypeConditions.push_back(StateTypeTriggerCondition{ stateType, negated });
    return true;
}

bool parseStateAnimElemTriggerClause(const std::string& clause, StateTriggerGroup& group) {
    const std::string condition = stripOuterParens(clause);
    if (!startsWithNoCase(condition, "AnimElem") || startsWithNoCase(condition, "AnimElemTime")) {
        return false;
    }

    const auto equals = condition.find('=');
    if (equals == std::string::npos) {
        return false;
    }

    const auto values = splitCsv(trim(std::string_view(condition).substr(equals + 1)));
    if (values.empty()) {
        return false;
    }

    const int elem = parseIntValue(values.front(), 0);
    if (elem <= 0) {
        return false;
    }

    CompareOp op = CompareOp::Equal;
    int offset = 0;
    if (values.size() >= 2) {
        const std::string offsetClause = trim(values[1]);
        if (const auto compare = findCompareOp(offsetClause)) {
            const auto [parsedOp, pos] = *compare;
            const size_t opLength = parsedOp == CompareOp::GreaterEqual
                || parsedOp == CompareOp::LessEqual
                || parsedOp == CompareOp::NotEqual
                ? 2
                : 1;
            op = parsedOp;
            offset = parseIntValue(trim(std::string_view(offsetClause).substr(pos + opLength)), 0);
        } else {
            offset = parseIntValue(offsetClause, 0);
        }
    }
    group.animElemTimeConditions.push_back(AnimElemTimeCondition{ elem, op, offset });
    return true;
}

bool parseStateTriggerClause(const std::string& clause, StateTriggerGroup& group) {
    const std::string trimmed = stripOuterParens(clause);
    if (trimmed.empty() || trimmed == "1") {
        return true;
    }
    const auto commands = findCommandConditions(trimmed);
    if (!commands.empty()) {
        for (const auto& command : commands) {
            if (command.negated) {
                addUniqueCommand(group.forbiddenCommands, command.command);
            } else {
                addUniqueCommand(group.requiredCommands, command.command);
            }
        }
        return true;
    }
    if (triggerIsMoveContact(trimmed)) {
        group.requiresMoveContact = true;
        return true;
    }
    if (parseStateFloatCondition(trimmed, group)) {
        return true;
    }
    if (parseStateTypeTriggerClause(trimmed, group)) {
        return true;
    }
    if (parseStateAnimElemTriggerClause(trimmed, group)) {
        return true;
    }
    if (const auto condition = parseAnimElemTimeCondition(trimmed)) {
        group.animElemTimeConditions.push_back(*condition);
        return true;
    }
    if (const auto expression = parseMugenExpressionCondition(trimmed)) {
        if (expressionLooksSupported(expression->lhs) || expressionLooksSupported(expression->rhs)) {
            group.expressionConditions.push_back(*expression);
            return true;
        }
    }
    if (expressionLooksSupported(trimmed)) {
        group.booleanExpressions.push_back(trimmed);
        return true;
    }
    return false;
}

std::optional<std::vector<StateTriggerGroup>> parseStateTriggerExpressionGroups(const std::string& expression) {
    std::vector<StateTriggerGroup> groups;
    for (const auto& orClause : splitTopLevelClauses(expression, "||", true)) {
        StateTriggerGroup group;
        for (const auto& andClause : splitTopLevelClauses(orClause, "&&")) {
            if (!parseStateTriggerClause(andClause, group)) {
                return std::nullopt;
            }
        }
        groups.push_back(std::move(group));
    }
    if (groups.empty()) {
        groups.push_back({});
    }
    return groups;
}

void appendNumberedTriggerExpression(std::vector<std::pair<std::string, std::string>>& expressions, const std::string& key, const std::string& value) {
    const std::string loweredKey = lowercaseCopy(key);
    for (auto& [currentKey, expression] : expressions) {
        if (currentKey == loweredKey) {
            expression += " && ";
            expression += value;
            return;
        }
    }
    expressions.push_back({ loweredKey, value });
}

std::optional<StateControllerTrigger> parseStateControllerTrigger(const MugenSection& section) {
    StateControllerTrigger trigger;
    if (const auto* persistent = findProperty(section, "persistent")) {
        trigger.persistent = std::max(0, parseIntValue(persistent->value, 1));
    }

    std::vector<std::pair<std::string, std::string>> numberedExpressions;
    for (const auto& property : section.properties) {
        if (!startsWithNoCase(property.key, "trigger")) {
            continue;
        }

        trigger.hasTrigger = true;
        if (startsWithNoCase(property.key, "triggerall")) {
            const auto groups = parseStateTriggerExpressionGroups(property.value);
            if (!groups) {
                return std::nullopt;
            }
            trigger.triggerAllExpressions.push_back(*groups);
        } else {
            appendNumberedTriggerExpression(numberedExpressions, property.key, property.value);
        }
    }

    for (const auto& [key, expression] : numberedExpressions) {
        (void)key;
        const auto groups = parseStateTriggerExpressionGroups(expression);
        if (!groups) {
            return std::nullopt;
        }
        trigger.triggerGroups.insert(trigger.triggerGroups.end(), groups->begin(), groups->end());
    }

    if (!trigger.hasTrigger) {
        return std::nullopt;
    }
    return trigger;
}

std::optional<float> parseControllerFloatProperty(
    const MugenSection& section,
    std::string_view key,
    const CharacterConstants& constants) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    return parseControllerFloatValue(property->value, constants);
}

std::optional<std::pair<float, float>> parseControllerFloatPairProperty(
    const MugenSection& section,
    std::string_view key,
    const CharacterConstants& constants,
    float fallbackY = 0.0f) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    return parseControllerFloatPairValue(property->value, constants, fallbackY);
}

float parseExplodPositionComponent(const std::string& value, const CharacterConstants& constants, float fallback) {
    if (const auto parsed = parseControllerFloatValue(value, constants)) {
        return *parsed;
    }

    const std::string lowered = lowercaseCopy(value);
    if (findNoCase(lowered, "screenpos y", 0) != std::string::npos) {
        if (const auto minus = lowered.rfind('-'); minus != std::string::npos) {
            return -parseFloatValue(trim(std::string_view(lowered).substr(minus + 1)), -fallback);
        }
        if (const auto plus = lowered.rfind('+'); plus != std::string::npos) {
            return parseFloatValue(trim(std::string_view(lowered).substr(plus + 1)), fallback);
        }
        return 0.0f;
    }
    return fallback;
}

std::pair<float, float> parseExplodPosition(const std::string& value, const CharacterConstants& constants) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { 0.0f, 0.0f };
    }
    const float x = parseExplodPositionComponent(parts[0], constants, 0.0f);
    const float y = parts.size() >= 2 ? parseExplodPositionComponent(parts[1], constants, 0.0f) : 0.0f;
    return { x, y };
}

EnvShakeSpec parseEnvShakeSpec(const MugenSection& section) {
    EnvShakeSpec shake;
    if (const auto* time = findProperty(section, "time")) {
        shake.time = std::max(0, parseIntValue(time->value, shake.time));
        shake.timeExpression = trim(time->value);
    }
    if (const auto* frequency = findProperty(section, "freq")) {
        shake.frequency = std::max(1, parseIntValue(frequency->value, shake.frequency));
        shake.frequencyExpression = trim(frequency->value);
    }
    if (const auto* amplitude = findProperty(section, "ampl")) {
        shake.amplitude = parseFloatValue(amplitude->value, shake.amplitude);
        shake.amplitudeExpression = trim(amplitude->value);
    }
    if (const auto* phase = findProperty(section, "phase")) {
        shake.phase = parseIntValue(phase->value, shake.phase);
        shake.phaseExpression = trim(phase->value);
    }
    shake.enabled = shake.time > 0 && std::abs(shake.amplitude) > 0.001f;
    return shake;
}

std::array<int, 3> parseIntTripleValue(const std::string& value, int fallbackR, int fallbackG, int fallbackB) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB };
    }
    return {
        parseIntValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseIntValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseIntValue(parts[2], fallbackB) : fallbackB,
    };
}

std::array<std::string, 3> parseExpressionTripleValue(
    const std::string& value,
    std::array<std::string, 3> fallback = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return fallback;
    }
    std::array<std::string, 3> out = fallback;
    for (size_t i = 0; i < std::min<size_t>(parts.size(), out.size()); ++i) {
        out[i] = trim(parts[i]);
    }
    return out;
}

std::array<int, 4> parseIntQuadValue(const std::string& value, int fallbackR, int fallbackG, int fallbackB, int fallbackPeriod) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB, fallbackPeriod };
    }
    return {
        parseIntValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseIntValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseIntValue(parts[2], fallbackB) : fallbackB,
        parts.size() >= 4 ? parseIntValue(parts[3], fallbackPeriod) : fallbackPeriod,
    };
}

std::array<std::string, 4> parseExpressionQuadValue(
    const std::string& value,
    std::array<std::string, 4> fallback = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return fallback;
    }
    std::array<std::string, 4> out = fallback;
    for (size_t i = 0; i < std::min<size_t>(parts.size(), out.size()); ++i) {
        out[i] = trim(parts[i]);
    }
    return out;
}

PaletteEffectSpec parsePaletteEffectSpec(const MugenSection& section, std::string_view prefix = "") {
    PaletteEffectSpec effect;
    const std::string keyPrefix(prefix);
    auto prefixed = [&keyPrefix](std::string_view key) {
        return keyPrefix.empty() ? std::string(key) : keyPrefix + "." + std::string(key);
    };

    if (const auto* time = findProperty(section, prefixed("time"))) {
        effect.time = parseIntValue(time->value, effect.time);
        effect.timeExpression = trim(time->value);
    }
    if (const auto* add = findProperty(section, prefixed("add"))) {
        const auto values = parseIntTripleValue(add->value, 0, 0, 0);
        effect.addR = values[0];
        effect.addG = values[1];
        effect.addB = values[2];
        effect.addExpressions = parseExpressionTripleValue(add->value);
    }
    if (const auto* mul = findProperty(section, prefixed("mul"))) {
        const auto values = parseIntTripleValue(mul->value, 256, 256, 256);
        effect.mulR = values[0];
        effect.mulG = values[1];
        effect.mulB = values[2];
        effect.mulExpressions = parseExpressionTripleValue(mul->value, { "256", "256", "256" });
    }
    if (const auto* sinAdd = findProperty(section, prefixed("sinadd"))) {
        const auto values = parseIntQuadValue(sinAdd->value, 0, 0, 0, 0);
        effect.sinAddR = values[0];
        effect.sinAddG = values[1];
        effect.sinAddB = values[2];
        effect.sinPeriod = values[3];
        effect.sinAddExpressions = parseExpressionQuadValue(sinAdd->value);
    }
    if (const auto* color = findProperty(section, prefixed("color"))) {
        effect.color = parseIntValue(color->value, effect.color);
        effect.colorExpression = trim(color->value);
    }
    if (const auto* invertAll = findProperty(section, prefixed("invertall"))) {
        effect.invertAll = parseIntValue(invertAll->value, 0) != 0;
        effect.invertAllExpression = trim(invertAll->value);
    }

    effect.enabled = effect.time != 0
        && (effect.addR != 0 || effect.addG != 0 || effect.addB != 0
            || effect.mulR != 256 || effect.mulG != 256 || effect.mulB != 256
            || effect.sinAddR != 0 || effect.sinAddG != 0 || effect.sinAddB != 0
            || effect.color != 256 || effect.invertAll);
    return effect;
}

std::optional<char> parseControllerCharProperty(const MugenSection& section, std::string_view key) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    const std::string value = trim(property->value);
    if (value.empty()) {
        return std::nullopt;
    }
    return static_cast<char>(SDL_toupper(static_cast<unsigned char>(value.front())));
}

std::string normalizeAssertSpecialFlag(std::string_view value) {
    std::string normalized = lowercaseCopy(trim(value));
    normalized.erase(
        std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0 || ch == '_' || ch == '-';
        }),
        normalized.end());
    return normalized;
}

std::vector<std::string> parseAssertSpecialFlags(const MugenSection& section) {
    std::vector<std::string> flags;
    for (const auto& property : section.properties) {
        if (!startsWithNoCase(property.key, "flag")) {
            continue;
        }

        const std::string flag = normalizeAssertSpecialFlag(property.value);
        if (!flag.empty()
            && std::find(flags.begin(), flags.end(), flag) == flags.end()) {
            flags.push_back(flag);
        }
    }
    return flags;
}

std::optional<std::pair<MugenVariableRef, std::string>> parseDirectVariableAssignment(const MugenSection& section) {
    for (const auto& property : section.properties) {
        if (const auto target = parseMugenVariableRef(property.key)) {
            return std::pair<MugenVariableRef, std::string>{ *target, trim(property.value) };
        }
    }
    return std::nullopt;
}

std::optional<MugenVariableRef> parseVariableControllerTarget(const MugenSection& section) {
    if (const auto direct = parseDirectVariableAssignment(section)) {
        return direct->first;
    }
    if (const auto* fv = findProperty(section, "fv")) {
        const auto index = parsePlainIntValue(fv->value);
        if (index && *index >= 0) {
            return MugenVariableRef{ MugenVariableBank::FVar, *index };
        }
    }
    if (const auto* v = findProperty(section, "v")) {
        const auto index = parsePlainIntValue(v->value);
        if (index && *index >= 0) {
            return MugenVariableRef{ MugenVariableBank::Var, *index };
        }
    }
    return std::nullopt;
}

bool isSupportedMoveType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'I' || value == 'A' || value == 'H';
}

bool isSupportedPhysicsType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'S' || value == 'C' || value == 'A' || value == 'N';
}

std::vector<MugenDocument> loadCharacterStateDocuments(const CharacterFiles& files) {
    std::vector<MugenDocument> documents;
    documents.reserve(files.stateFiles.size());
    for (const auto& path : files.stateFiles) {
        documents.push_back(parseMugenTextFile(path));
    }
    return documents;
}

std::vector<std::string> loadVictoryQuotes(const CharacterFiles& files) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<std::string> quotes;
    for (const auto& document : documents) {
        for (const auto& section : document.sections) {
            if (!equalsNoCase(section.name, "Quotes")) {
                continue;
            }
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "victory")) {
                    continue;
                }
                const int index = parseIntValue(std::string(std::string_view(property.key).substr(7)), 0);
                if (index <= 0 || index > 99) {
                    continue;
                }
                if (quotes.size() < static_cast<size_t>(index)) {
                    quotes.resize(static_cast<size_t>(index));
                }
                quotes[static_cast<size_t>(index - 1)] = unquote(trim(property.value));
            }
        }
    }
    return quotes;
}

std::vector<StateDefinition> loadStateDefinitions(const CharacterFiles& files, const CharacterConstants& constants) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<StateDefinition> states;
    int nextSoundControllerId = 1;
    int nextCtrlControllerId = 1;
    int nextPosAddControllerId = 1;
    int nextChangeAnimControllerId = 1;
    int nextRuntimeControllerId = 1;

    for (const auto& cns : documents) {
        int currentStateIndex = -1;
        for (const auto& section : cns.sections) {
            const auto stateNo = parseStateNumber(section.name, "Statedef ");
            if (stateNo) {
                StateDefinition state;
                state.stateNo = *stateNo;
                state.stateType = parseStateChar(&section, "type", state.stateType);
                state.moveType = parseStateChar(&section, "movetype", state.moveType);
                state.physics = parseStateChar(&section, "physics", state.physics);
                if (const auto* anim = findProperty(section, "anim")) {
                    state.hasAnim = true;
                    state.anim = parseIntValue(anim->value, state.anim);
                } else {
                    state.anim = state.stateNo;
                }
                if (const auto* ctrl = findProperty(section, "ctrl")) {
                    state.ctrl = parseIntValue(ctrl->value, state.ctrl ? 1 : 0) != 0;
                }
                if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                    state.sprPriority = parseIntValue(sprPriority->value, state.sprPriority);
                }
                if (const auto* velset = findProperty(section, "velset")) {
                    if (const auto values = parseControllerFloatPairValue(velset->value, constants)) {
                        state.hasVelset = true;
                        state.velsetX = values->first;
                        state.velsetY = values->second;
                    }
                }
                states.push_back(std::move(state));
                currentStateIndex = static_cast<int>(states.size()) - 1;
                continue;
            }

            if (currentStateIndex < 0 || !startsWithNoCase(section.name, "State ")) {
                continue;
            }

            const auto* type = findProperty(section, "type");
            if (!type) {
                continue;
            }

            const std::string controllerType = trim(type->value);
            bool hasTriggerProperties = false;
            int triggerTime = -1;
            int triggerAnimElem = -1;
            bool requiresMoveContact = false;
            std::vector<AnimElemTimeCondition> animElemTimeConditions;
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "trigger")) {
                    continue;
                }
                hasTriggerProperties = true;
                const auto trigger = trim(property.value);
                for (const auto& clause : splitAndClauses(trigger)) {
                    if (const auto time = parseTimeTrigger(clause)) {
                        triggerTime = *time;
                    } else if (const auto elem = parseAnimElemTrigger(clause)) {
                        triggerAnimElem = *elem;
                    } else if (const auto condition = parseAnimElemTimeCondition(clause)) {
                        animElemTimeConditions.push_back(*condition);
                    } else if (triggerIsMoveContact(clause)) {
                        requiresMoveContact = true;
                    }
                }
            }
            const auto controllerTrigger = parseStateControllerTrigger(section);

            if (startsWithNoCase(controllerType, "ChangeState") || startsWithNoCase(controllerType, "SelfState")) {
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }

                const auto targetState = parsePlainIntValue(value->value);
                const std::string targetStateExpression = trim(value->value);
                const int parsedTargetState = targetState.value_or(0);

                bool animEndTrigger = false;
                for (const auto& property : section.properties) {
                    if (!startsWithNoCase(property.key, "trigger")) {
                        continue;
                    }
                    const auto trigger = trim(property.value);
                    if (equalsNoCase(stripOuterParens(trigger), "!animtime")) {
                        animEndTrigger = true;
                        continue;
                    }
                    if (!startsWithNoCase(trigger, "AnimTime")) {
                        continue;
                    }
                    if (const auto equals = trigger.find('='); equals != std::string::npos) {
                        animEndTrigger = parseIntValue(trim(std::string_view(trigger).substr(equals + 1)), 1) == 0;
                    }
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                if (animEndTrigger) {
                    state.hasAnimEndChangeState = true;
                    state.animEndChangeState = parsedTargetState;
                    state.animEndChangeStateExpression = targetStateExpression;
                    if (const auto* ctrl = findProperty(section, "ctrl")) {
                        state.hasAnimEndCtrl = true;
                        state.animEndCtrl = parseIntValue(ctrl->value, 0) != 0;
                    }
                    continue;
                }

                if (!controllerTrigger) {
                    continue;
                }

                StateChangeStateController changeState;
                changeState.id = nextRuntimeControllerId++;
                changeState.trigger = *controllerTrigger;
                changeState.targetState = parsedTargetState;
                changeState.targetStateExpression = targetStateExpression;
                if (const auto* ctrl = findProperty(section, "ctrl")) {
                    changeState.hasCtrl = true;
                    changeState.ctrl = parseIntValue(ctrl->value, 0) != 0;
                }
                state.changeStates.push_back(std::move(changeState));
                continue;
            }

        if (startsWithNoCase(controllerType, "PlaySnd")) {
            const auto* value = findProperty(section, "value");
            if (!value
                || (hasTriggerProperties && !controllerTrigger)
                || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }
            const auto sound = parseSoundValue(value->value);
            if (!sound) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateSoundController soundController;
            soundController.id = nextSoundControllerId++;
            if (controllerTrigger) {
                soundController.trigger = *controllerTrigger;
            }
            soundController.triggerTime = triggerTime;
            soundController.triggerAnimElem = triggerAnimElem;
            soundController.group = sound->group;
            soundController.index = sound->index;
            soundController.forceCommon = sound->forceCommon;
            if (const auto* channel = findProperty(section, "channel")) {
                soundController.channel = parseIntValue(channel->value, soundController.channel);
            }
            if (const auto* lowPriority = findProperty(section, "lowpriority")) {
                soundController.lowPriority = parseIntValue(lowPriority->value, 0) != 0;
            }
            if (const auto* volume = findProperty(section, "volume")) {
                soundController.gain = mugenVolumeToGain(volume->value);
            }
            if (const auto* loop = findProperty(section, "loop")) {
                soundController.loop = parseIntValue(loop->value, 0) != 0;
            }
            state.sounds.push_back(std::move(soundController));
            state.audioControllers.push_back(StateAudioControllerRef{
                StateAudioControllerKind::PlaySnd,
                state.sounds.size() - 1,
            });
            continue;
        }

        if (startsWithNoCase(controllerType, "StopSnd")) {
            const auto* channel = findProperty(section, "channel");
            if (!channel
                || (hasTriggerProperties && !controllerTrigger)
                || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }

            const int parsedChannel = parseIntValue(channel->value, -1);
            if (parsedChannel < 0) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateStopSoundController stopSound;
            stopSound.id = nextRuntimeControllerId++;
            if (controllerTrigger) {
                stopSound.trigger = *controllerTrigger;
            }
            stopSound.triggerTime = triggerTime;
            stopSound.triggerAnimElem = triggerAnimElem;
            stopSound.channel = parsedChannel;
            state.stopSounds.push_back(std::move(stopSound));
            state.audioControllers.push_back(StateAudioControllerRef{
                StateAudioControllerKind::StopSnd,
                state.stopSounds.size() - 1,
            });
            continue;
        }

        if (startsWithNoCase(controllerType, "CtrlSet")) {
            const auto* value = findProperty(section, "value");
            if (!value || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateCtrlController ctrlSet;
            ctrlSet.id = nextCtrlControllerId++;
            if (controllerTrigger) {
                ctrlSet.trigger = *controllerTrigger;
            }
            ctrlSet.triggerTime = triggerTime;
            ctrlSet.triggerAnimElem = triggerAnimElem;
            ctrlSet.value = parseIntValue(value->value, 0) != 0;
            state.ctrlSets.push_back(std::move(ctrlSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "VarSet")
            || startsWithNoCase(controllerType, "VarAdd")
            || startsWithNoCase(controllerType, "VarRandom")) {
            if (!controllerTrigger) {
                continue;
            }

            const auto target = parseVariableControllerTarget(section);
            if (!target) {
                continue;
            }

            StateVariableController variable;
            variable.id = nextRuntimeControllerId++;
            variable.trigger = *controllerTrigger;
            variable.target = *target;
            if (startsWithNoCase(controllerType, "VarAdd")) {
                variable.operation = StateVariableOperation::Add;
            } else if (startsWithNoCase(controllerType, "VarRandom")) {
                variable.operation = StateVariableOperation::Random;
            } else {
                variable.operation = StateVariableOperation::Set;
            }

            if (variable.operation == StateVariableOperation::Random) {
                const auto* range = findProperty(section, "range");
                if (!range) {
                    continue;
                }
                const auto parts = splitCsv(range->value);
                if (parts.empty()) {
                    continue;
                }
                variable.rangeMinExpression = trim(parts[0]);
                variable.rangeMaxExpression = parts.size() >= 2 ? trim(parts[1]) : variable.rangeMinExpression;
            } else if (const auto direct = parseDirectVariableAssignment(section)) {
                variable.valueExpression = direct->second;
            } else if (const auto* value = findProperty(section, "value")) {
                variable.valueExpression = trim(value->value);
            } else {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.variableControllers.push_back(std::move(variable));
            continue;
        }

        if (startsWithNoCase(controllerType, "VelSet")
            || startsWithNoCase(controllerType, "VelAdd")
            || startsWithNoCase(controllerType, "VelMul")) {
            if (!controllerTrigger) {
                continue;
            }

            StateVelocityController velocity;
            velocity.id = nextRuntimeControllerId++;
            velocity.trigger = *controllerTrigger;
            if (startsWithNoCase(controllerType, "VelAdd")) {
                velocity.operation = StateVelocityOperation::Add;
            } else if (startsWithNoCase(controllerType, "VelMul")) {
                velocity.operation = StateVelocityOperation::Mul;
            } else {
                velocity.operation = StateVelocityOperation::Set;
            }
            if (const auto x = parseControllerFloatProperty(section, "x", constants)) {
                velocity.hasX = true;
                velocity.x = *x;
            }
            if (const auto y = parseControllerFloatProperty(section, "y", constants)) {
                velocity.hasY = true;
                velocity.y = *y;
            }
            if (!velocity.hasX && !velocity.hasY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.velocityControllers.push_back(std::move(velocity));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePosSetController posSet;
            posSet.id = nextRuntimeControllerId++;
            posSet.trigger = *controllerTrigger;
            if (const auto x = parseControllerFloatProperty(section, "x", constants)) {
                posSet.hasX = true;
                posSet.x = *x;
            }
            if (const auto y = parseControllerFloatProperty(section, "y", constants)) {
                posSet.hasY = true;
                posSet.y = *y;
            }
            if (!posSet.hasX && !posSet.hasY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.posSets.push_back(std::move(posSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "StateTypeSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StateTypeSetController typeSet;
            typeSet.id = nextRuntimeControllerId++;
            typeSet.trigger = *controllerTrigger;
            if (const auto stateType = parseControllerCharProperty(section, "statetype")) {
                if (isSupportedStateType(*stateType)) {
                    typeSet.hasStateType = true;
                    typeSet.stateType = *stateType;
                }
            }
            if (const auto moveType = parseControllerCharProperty(section, "movetype")) {
                if (isSupportedMoveType(*moveType)) {
                    typeSet.hasMoveType = true;
                    typeSet.moveType = *moveType;
                }
            }
            if (const auto physics = parseControllerCharProperty(section, "physics")) {
                if (isSupportedPhysicsType(*physics)) {
                    typeSet.hasPhysics = true;
                    typeSet.physics = *physics;
                }
            }
            if (!typeSet.hasStateType && !typeSet.hasMoveType && !typeSet.hasPhysics) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.stateTypeSets.push_back(std::move(typeSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "ScreenBound")) {
            if (!controllerTrigger) {
                continue;
            }

            StateScreenBoundController screenBound;
            screenBound.id = nextRuntimeControllerId++;
            screenBound.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                screenBound.value = parseIntValue(value->value, screenBound.value ? 1 : 0) != 0;
            }
            if (const auto* moveCamera = findProperty(section, "movecamera")) {
                const auto values = parseIntPairValue(moveCamera->value);
                screenBound.moveCameraX = values.first != 0;
                screenBound.moveCameraY = values.second != 0;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.screenBounds.push_back(std::move(screenBound));
            continue;
        }

        if (startsWithNoCase(controllerType, "Width")) {
            if (!controllerTrigger) {
                continue;
            }

            StateWidthController width;
            width.id = nextRuntimeControllerId++;
            width.trigger = *controllerTrigger;
            const bool hasExplicitEdge = findProperty(section, "edge") != nullptr;
            const bool hasExplicitPlayer = findProperty(section, "player") != nullptr;

            if (const auto edge = parseControllerFloatPairProperty(section, "edge", constants)) {
                width.hasEdge = true;
                width.edgeFront = edge->first;
                width.edgeBack = edge->second;
            }
            if (const auto player = parseControllerFloatPairProperty(section, "player", constants)) {
                width.hasPlayer = true;
                width.playerFront = player->first;
                width.playerBack = player->second;
            }
            if (!hasExplicitEdge && !hasExplicitPlayer) {
                if (const auto value = parseControllerFloatPairProperty(section, "value", constants)) {
                    width.hasEdge = true;
                    width.edgeFront = value->first;
                    width.edgeBack = value->second;
                    width.hasPlayer = true;
                    width.playerFront = value->first;
                    width.playerBack = value->second;
                }
            }
            if (!width.hasEdge && !width.hasPlayer) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.widths.push_back(std::move(width));
            continue;
        }

        if (startsWithNoCase(controllerType, "PlayerPush")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePlayerPushController playerPush;
            playerPush.id = nextRuntimeControllerId++;
            playerPush.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                playerPush.value = parseIntValue(value->value, playerPush.value ? 1 : 0) != 0;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.playerPushes.push_back(std::move(playerPush));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallDamage")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallDamageController hitFallDamage;
            hitFallDamage.id = nextRuntimeControllerId++;
            hitFallDamage.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallDamages.push_back(std::move(hitFallDamage));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallVel")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallVelController hitFallVel;
            hitFallVel.id = nextRuntimeControllerId++;
            hitFallVel.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallVels.push_back(std::move(hitFallVel));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallSetController hitFallSet;
            hitFallSet.id = nextRuntimeControllerId++;
            hitFallSet.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                hitFallSet.value = parseIntValue(value->value, hitFallSet.value);
            }
            if (const auto xvel = parseControllerFloatProperty(section, "xvel", constants)) {
                hitFallSet.hasXVelocity = true;
                hitFallSet.xVelocity = *xvel;
            }
            if (const auto yvel = parseControllerFloatProperty(section, "yvel", constants)) {
                hitFallSet.hasYVelocity = true;
                hitFallSet.yVelocity = *yvel;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallSets.push_back(std::move(hitFallSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "SprPriority")) {
            if (!controllerTrigger) {
                continue;
            }

            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }

            StateSprPriorityController sprPriority;
            sprPriority.id = nextRuntimeControllerId++;
            sprPriority.trigger = *controllerTrigger;
            sprPriority.value = parseIntValue(value->value, 0);

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.sprPriorities.push_back(std::move(sprPriority));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosFreeze")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePosFreezeController posFreeze;
            posFreeze.id = nextRuntimeControllerId++;
            posFreeze.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                posFreeze.freezeX = parseIntValue(x->value, 0) != 0;
            }
            if (const auto* y = findProperty(section, "y")) {
                posFreeze.freezeY = parseIntValue(y->value, 0) != 0;
            }
            if (!posFreeze.freezeX && !posFreeze.freezeY) {
                posFreeze.freezeX = true;
                posFreeze.freezeY = true;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.posFreezes.push_back(std::move(posFreeze));
            continue;
        }

            if (startsWithNoCase(controllerType, "HitVelSet")) {
                if (!controllerTrigger) {
                    continue;
                }

            StateHitVelSetController hitVelSet;
            hitVelSet.id = nextRuntimeControllerId++;
            hitVelSet.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                hitVelSet.applyX = parseIntValue(x->value, 0) != 0;
            }
            if (const auto* y = findProperty(section, "y")) {
                hitVelSet.applyY = parseIntValue(y->value, 0) != 0;
            }
            if (!hitVelSet.applyX && !hitVelSet.applyY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.hitVelSets.push_back(std::move(hitVelSet));
                continue;
            }

            if (startsWithNoCase(controllerType, "NotHitBy") || startsWithNoCase(controllerType, "HitBy")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    value = findProperty(section, "value2");
                }
                if (!value) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateHitProtectionController protection;
                protection.id = nextRuntimeControllerId++;
                protection.trigger = *controllerTrigger;
                protection.notHitBy = startsWithNoCase(controllerType, "NotHitBy");
                protection.value = trim(value->value);
                if (const auto* time = findProperty(section, "time")) {
                    protection.time = std::max(1, parseIntValue(time->value, protection.time));
                }
                state.hitProtections.push_back(std::move(protection));
                continue;
            }

            if (startsWithNoCase(controllerType, "HitOverride")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* attr = findProperty(section, "attr");
                const auto* stateNo = findProperty(section, "stateno");
                if (!attr || !stateNo) {
                    continue;
                }
                const auto parsedState = parsePlainIntValue(stateNo->value);
                if (!parsedState) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateHitOverrideController overrideController;
                overrideController.id = nextRuntimeControllerId++;
                overrideController.trigger = *controllerTrigger;
                overrideController.attr = trim(attr->value);
                overrideController.stateNo = *parsedState;
                if (const auto* time = findProperty(section, "time")) {
                    overrideController.time = parseIntValue(time->value, overrideController.time);
                }
                state.hitOverrides.push_back(std::move(overrideController));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetState")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                const auto parsedValue = parsePlainIntValue(value->value);
                if (!parsedValue) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetStateController targetState;
                targetState.id = nextRuntimeControllerId++;
                targetState.trigger = *controllerTrigger;
                targetState.value = *parsedValue;
                if (const auto* id = findProperty(section, "id")) {
                    targetState.targetId = parseIntValue(id->value, targetState.targetId);
                }
                state.targetStates.push_back(std::move(targetState));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetBind")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetBindController targetBind;
                targetBind.id = nextRuntimeControllerId++;
                targetBind.trigger = *controllerTrigger;
                if (const auto pos = parseControllerFloatPairProperty(section, "pos", constants)) {
                    targetBind.x = pos->first;
                    targetBind.y = pos->second;
                }
                if (const auto* time = findProperty(section, "time")) {
                    targetBind.time = parseIntValue(time->value, targetBind.time);
                }
                if (const auto* id = findProperty(section, "id")) {
                    targetBind.targetId = parseIntValue(id->value, targetBind.targetId);
                }
                state.targetBinds.push_back(std::move(targetBind));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetDrop")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetDropController targetDrop;
                targetDrop.id = nextRuntimeControllerId++;
                targetDrop.trigger = *controllerTrigger;
                if (const auto* excludeId = findProperty(section, "excludeID")) {
                    targetDrop.excludeId = parseIntValue(excludeId->value, targetDrop.excludeId);
                }
                state.targetDrops.push_back(std::move(targetDrop));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetFacing")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetFacingController targetFacing;
                targetFacing.id = nextRuntimeControllerId++;
                targetFacing.trigger = *controllerTrigger;
                targetFacing.value = parseIntValue(value->value, targetFacing.value);
                if (const auto* id = findProperty(section, "id")) {
                    targetFacing.targetId = parseIntValue(id->value, targetFacing.targetId);
                }
                state.targetFacings.push_back(std::move(targetFacing));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetLifeAdd")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetLifeAddController targetLifeAdd;
                targetLifeAdd.id = nextRuntimeControllerId++;
                targetLifeAdd.trigger = *controllerTrigger;
                targetLifeAdd.valueExpression = trim(value->value);
                if (const auto* id = findProperty(section, "id")) {
                    targetLifeAdd.targetId = parseIntValue(id->value, targetLifeAdd.targetId);
                }
                if (const auto* kill = findProperty(section, "kill")) {
                    targetLifeAdd.kill = parseIntValue(kill->value, targetLifeAdd.kill ? 1 : 0) != 0;
                }
                if (const auto* absolute = findProperty(section, "absolute")) {
                    targetLifeAdd.absolute = parseIntValue(absolute->value, targetLifeAdd.absolute ? 1 : 0) != 0;
                }
                state.targetLifeAdds.push_back(std::move(targetLifeAdd));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetPowerAdd")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetPowerAddController targetPowerAdd;
                targetPowerAdd.id = nextRuntimeControllerId++;
                targetPowerAdd.trigger = *controllerTrigger;
                targetPowerAdd.valueExpression = trim(value->value);
                if (const auto* id = findProperty(section, "id")) {
                    targetPowerAdd.targetId = parseIntValue(id->value, targetPowerAdd.targetId);
                }
                state.targetPowerAdds.push_back(std::move(targetPowerAdd));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetVelAdd") || startsWithNoCase(controllerType, "TargetVelSet")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetVelocityController targetVelocity;
                targetVelocity.id = nextRuntimeControllerId++;
                targetVelocity.trigger = *controllerTrigger;
                targetVelocity.add = startsWithNoCase(controllerType, "TargetVelAdd");
                if (const auto* x = findProperty(section, "x")) {
                    targetVelocity.xExpression = trim(x->value);
                    targetVelocity.hasX = true;
                }
                if (const auto* y = findProperty(section, "y")) {
                    targetVelocity.yExpression = trim(y->value);
                    targetVelocity.hasY = true;
                }
                if (const auto* id = findProperty(section, "id")) {
                    targetVelocity.targetId = parseIntValue(id->value, targetVelocity.targetId);
                }
                if (!targetVelocity.hasX && !targetVelocity.hasY) {
                    continue;
                }
                state.targetVelocities.push_back(std::move(targetVelocity));
                continue;
            }

            if (startsWithNoCase(controllerType, "Turn")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTurnController turn;
                turn.id = nextRuntimeControllerId++;
                turn.trigger = *controllerTrigger;
                state.turns.push_back(std::move(turn));
                continue;
            }

            if (equalsNoCase(controllerType, "Pause") || equalsNoCase(controllerType, "SuperPause")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StatePauseController pause;
                pause.id = nextRuntimeControllerId++;
                pause.trigger = *controllerTrigger;
                pause.superPause = equalsNoCase(controllerType, "SuperPause");
                pause.time = pause.superPause ? 30 : 1;
                if (const auto* time = findProperty(section, "time")) {
                    pause.time = std::max(1, parseIntValue(time->value, pause.time));
                }
                if (const auto* moveTime = findProperty(section, "movetime")) {
                    pause.moveTime = std::max(0, parseIntValue(moveTime->value, pause.moveTime));
                }
                if (const auto* sound = findProperty(section, "sound")) {
                    if (const auto parsed = parseSoundValue(sound->value)) {
                        pause.soundGroup = parsed->group;
                        pause.soundIndex = parsed->index;
                        pause.soundForceCommon = parsed->forceCommon;
                    }
                }
                state.pauses.push_back(std::move(pause));
                continue;
            }

            if (startsWithNoCase(controllerType, "Null")) {
                continue;
            }

            if (startsWithNoCase(controllerType, "PalFX") || startsWithNoCase(controllerType, "BGPalFX")) {
                if (!controllerTrigger) {
                    continue;
                }

                StatePaletteEffectController paletteEffect;
                paletteEffect.id = nextRuntimeControllerId++;
                paletteEffect.trigger = *controllerTrigger;
                paletteEffect.background = startsWithNoCase(controllerType, "BGPalFX");
                paletteEffect.effect = parsePaletteEffectSpec(section);
                if (!paletteEffect.effect.enabled) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.paletteEffects.push_back(std::move(paletteEffect));
                continue;
            }

            if (startsWithNoCase(controllerType, "EnvColor")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateEnvColorController envColor;
                envColor.id = nextRuntimeControllerId++;
                envColor.trigger = *controllerTrigger;
                if (const auto* value = findProperty(section, "value")) {
                    const auto values = parseIntTripleValue(value->value, 255, 255, 255);
                    envColor.r = values[0];
                    envColor.g = values[1];
                    envColor.b = values[2];
                }
                if (const auto* time = findProperty(section, "time")) {
                    envColor.time = std::max(0, parseIntValue(time->value, envColor.time));
                }
                if (envColor.time <= 0) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.envColors.push_back(std::move(envColor));
                continue;
            }

            if (startsWithNoCase(controllerType, "EnvShake")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateEnvShakeController envShake;
                envShake.id = nextRuntimeControllerId++;
                envShake.trigger = *controllerTrigger;
                envShake.shake = parseEnvShakeSpec(section);
                if (!envShake.shake.enabled) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.envShakes.push_back(std::move(envShake));
                continue;
            }

            if (startsWithNoCase(controllerType, "FallEnvShake")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateFallEnvShakeController fallEnvShake;
                fallEnvShake.id = nextRuntimeControllerId++;
                fallEnvShake.trigger = *controllerTrigger;

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.fallEnvShakes.push_back(std::move(fallEnvShake));
                continue;
            }

            if (startsWithNoCase(controllerType, "ModifyExplod")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateModifyExplodController modifyExplod;
                modifyExplod.id = nextRuntimeControllerId++;
                modifyExplod.trigger = *controllerTrigger;
                if (const auto* explodId = findProperty(section, "id")) {
                    modifyExplod.explodId = parseIntValue(explodId->value, -1);
                }
                if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                    modifyExplod.hasSprPriority = true;
                    modifyExplod.sprPriority = parseIntValue(sprPriority->value, 0);
                }
                if (const auto* scale = findProperty(section, "scale")) {
                    const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                    modifyExplod.hasScale = true;
                    modifyExplod.scaleX = values.first;
                    modifyExplod.scaleY = values.second;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.modifyExplods.push_back(std::move(modifyExplod));
                continue;
            }

            if (startsWithNoCase(controllerType, "RemoveExplod")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateRemoveExplodController removeExplod;
                removeExplod.id = nextRuntimeControllerId++;
                removeExplod.trigger = *controllerTrigger;
                if (const auto* explodId = findProperty(section, "id")) {
                    removeExplod.explodId = parseIntValue(explodId->value, -1);
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.removeExplods.push_back(std::move(removeExplod));
                continue;
            }

            if (startsWithNoCase(controllerType, "Explod")) {
                if (!controllerTrigger) {
                    continue;
                }

            const auto* anim = findProperty(section, "anim");
            if (!anim) {
                continue;
            }

            std::string animValue = trim(anim->value);
            StateExplodController explod;
            explod.id = nextRuntimeControllerId++;
            explod.trigger = *controllerTrigger;
            if (const auto* explodId = findProperty(section, "id")) {
                explod.explodId = parseIntValue(explodId->value, -1);
            }
            if (!animValue.empty() && (animValue.front() == 'F' || animValue.front() == 'f')) {
                explod.fromFightFx = true;
                animValue = trim(std::string_view(animValue).substr(1));
            }
            explod.anim = parseIntValue(animValue, -1);
            if (explod.anim < 0) {
                continue;
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                explod.x = values.first;
                explod.y = values.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                explod.postype = lowercaseCopy(trim(postype->value));
            }
            if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                explod.sprPriority = parseIntValue(sprPriority->value, 0);
            }
            if (const auto* bindTime = findProperty(section, "bindtime")) {
                explod.bindTime = parseIntValue(bindTime->value, explod.bindTime);
            }
            if (const auto* removeTime = findProperty(section, "removetime")) {
                explod.removeTime = parseIntValue(removeTime->value, explod.removeTime);
            }
            if (const auto* scale = findProperty(section, "scale")) {
                const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                explod.scaleX = values.first;
                explod.scaleY = values.second;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.explods.push_back(std::move(explod));
            continue;
        }

        if (startsWithNoCase(controllerType, "AssertSpecial")) {
            if (!controllerTrigger) {
                continue;
            }

            StateAssertSpecialController assertSpecial;
            assertSpecial.id = nextRuntimeControllerId++;
            assertSpecial.trigger = *controllerTrigger;
            assertSpecial.flags = parseAssertSpecialFlags(section);
            if (assertSpecial.flags.empty()) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.assertSpecials.push_back(std::move(assertSpecial));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosAdd")) {
            if (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StatePosAddController posAdd;
            posAdd.id = controllerTrigger ? nextRuntimeControllerId++ : nextPosAddControllerId++;
            if (controllerTrigger) {
                posAdd.trigger = *controllerTrigger;
            }
            posAdd.triggerTime = triggerTime;
            posAdd.triggerAnimElem = triggerAnimElem;
            posAdd.x = findProperty(section, "x") ? parseFloatValue(findProperty(section, "x")->value, 0.0f) : 0.0f;
            posAdd.y = findProperty(section, "y") ? parseFloatValue(findProperty(section, "y")->value, 0.0f) : 0.0f;
            state.posAdds.push_back(std::move(posAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "ChangeAnim")) {
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateChangeAnimController changeAnim;
            changeAnim.id = controllerTrigger ? nextRuntimeControllerId++ : nextChangeAnimControllerId++;
            if (controllerTrigger) {
                changeAnim.trigger = *controllerTrigger;
            }
            changeAnim.triggerTime = triggerTime;
            changeAnim.triggerAnimElem = triggerAnimElem;
            changeAnim.value = parseIntValue(value->value, state.anim);
            changeAnim.elem = findProperty(section, "elem") ? parseIntValue(findProperty(section, "elem")->value, 1) : 1;
            changeAnim.requiresMoveContact = requiresMoveContact;
            changeAnim.animElemTimeConditions = std::move(animElemTimeConditions);
            state.changeAnims.push_back(std::move(changeAnim));
        }

        if (startsWithNoCase(controllerType, "Helper")) {
            const auto* stateNo = findProperty(section, "stateno");
            if (!controllerTrigger || !stateNo) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateHelperController helper;
            helper.id = nextRuntimeControllerId++;
            helper.trigger = *controllerTrigger;
            helper.stateNo = parseIntValue(stateNo->value, 0);
            helper.helperId = findProperty(section, "id") ? parseIntValue(findProperty(section, "id")->value, helper.stateNo) : helper.stateNo;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                helper.x = values.first;
                helper.y = values.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                helper.postype = lowercaseCopy(trim(postype->value));
            }
            if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                helper.sprPriority = parseIntValue(sprPriority->value, helper.sprPriority);
            }
            if (const auto* facing = findProperty(section, "facing")) {
                helper.facing = parseIntValue(facing->value, helper.facing);
            }
            state.helpers.push_back(std::move(helper));
            continue;
        }

        if (startsWithNoCase(controllerType, "DestroySelf")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateDestroySelfController destroySelf;
            destroySelf.id = nextRuntimeControllerId++;
            destroySelf.trigger = *controllerTrigger;
            state.destroySelfs.push_back(std::move(destroySelf));
            continue;
        }

        if (startsWithNoCase(controllerType, "BindToParent")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateBindToParentController bind;
            bind.id = nextRuntimeControllerId++;
            bind.trigger = *controllerTrigger;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                bind.x = values.first;
                bind.y = values.second;
            }
            state.bindToParents.push_back(std::move(bind));
            continue;
        }

        if (startsWithNoCase(controllerType, "BindToRoot")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateBindToRootController bind;
            bind.id = nextRuntimeControllerId++;
            bind.trigger = *controllerTrigger;
            if (const auto* time = findProperty(section, "time")) {
                bind.time = std::max(1, parseIntValue(time->value, bind.time));
            }
            if (const auto* facing = findProperty(section, "facing")) {
                bind.facing = std::clamp(parseIntValue(facing->value, bind.facing), -1, 1);
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                bind.x = values.first;
                bind.y = values.second;
            }
            state.bindToRoots.push_back(std::move(bind));
            continue;
        }

        if (startsWithNoCase(controllerType, "ParentVarAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto assignment = parseDirectVariableAssignment(section);
            if (!assignment) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateParentVarAddController parentVarAdd;
            parentVarAdd.id = nextRuntimeControllerId++;
            parentVarAdd.trigger = *controllerTrigger;
            parentVarAdd.target = assignment->first;
            parentVarAdd.valueExpression = assignment->second;
            state.parentVarAdds.push_back(std::move(parentVarAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "VarRangeSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            const auto* fvalue = findProperty(section, "fvalue");
            if (!value && !fvalue) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateVarRangeSetController rangeSet;
            rangeSet.id = nextRuntimeControllerId++;
            rangeSet.trigger = *controllerTrigger;
            rangeSet.floatBank = fvalue != nullptr;
            rangeSet.valueExpression = trim(rangeSet.floatBank ? fvalue->value : value->value);
            rangeSet.first = findProperty(section, "first") ? parseIntValue(findProperty(section, "first")->value, 0) : 0;
            rangeSet.last = findProperty(section, "last")
                ? parseIntValue(findProperty(section, "last")->value, rangeSet.floatBank ? 39 : 59)
                : (rangeSet.floatBank ? 39 : 59);
            state.varRangeSets.push_back(std::move(rangeSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "PowerAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StatePowerAddController powerAdd;
            powerAdd.id = nextRuntimeControllerId++;
            powerAdd.trigger = *controllerTrigger;
            powerAdd.valueExpression = trim(value->value);
            state.powerAdds.push_back(std::move(powerAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "LifeAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateLifeAddController lifeAdd;
            lifeAdd.id = nextRuntimeControllerId++;
            lifeAdd.trigger = *controllerTrigger;
            lifeAdd.valueExpression = trim(value->value);
            if (const auto* kill = findProperty(section, "kill")) {
                lifeAdd.kill = parseIntValue(kill->value, 1) != 0;
            }
            state.lifeAdds.push_back(std::move(lifeAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateHitAddController hitAdd;
            hitAdd.id = nextRuntimeControllerId++;
            hitAdd.trigger = *controllerTrigger;
            hitAdd.valueExpression = trim(value->value);
            state.hitAdds.push_back(std::move(hitAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "AttackDist")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAttackDistController attackDist;
            attackDist.id = nextRuntimeControllerId++;
            attackDist.trigger = *controllerTrigger;
            attackDist.valueExpression = trim(value->value);
            state.attackDists.push_back(std::move(attackDist));
            continue;
        }

        if (startsWithNoCase(controllerType, "DefenceMulSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateDefenceMulSetController defenceMulSet;
            defenceMulSet.id = nextRuntimeControllerId++;
            defenceMulSet.trigger = *controllerTrigger;
            defenceMulSet.valueExpression = trim(value->value);
            state.defenceMulSets.push_back(std::move(defenceMulSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "AttackMulSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAttackMulSetController attackMulSet;
            attackMulSet.id = nextRuntimeControllerId++;
            attackMulSet.trigger = *controllerTrigger;
            attackMulSet.valueExpression = trim(value->value);
            state.attackMulSets.push_back(std::move(attackMulSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "AngleSet")
            || startsWithNoCase(controllerType, "AngleAdd")
            || startsWithNoCase(controllerType, "AngleMul")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            StateAngleController angle;
            angle.id = nextRuntimeControllerId++;
            angle.trigger = *controllerTrigger;
            angle.valueExpression = trim(value->value);
            if (startsWithNoCase(controllerType, "AngleAdd")) {
                angle.operation = StateAngleOperation::Add;
            } else if (startsWithNoCase(controllerType, "AngleMul")) {
                angle.operation = StateAngleOperation::Mul;
            } else {
                angle.operation = StateAngleOperation::Set;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.angleControllers.push_back(std::move(angle));
            continue;
        }

        if (startsWithNoCase(controllerType, "AngleDraw")) {
            if (!controllerTrigger) {
                continue;
            }
            StateAngleDrawController angleDraw;
            angleDraw.id = nextRuntimeControllerId++;
            angleDraw.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.angleDraws.push_back(std::move(angleDraw));
            continue;
        }

        if (startsWithNoCase(controllerType, "Offset")) {
            if (!controllerTrigger) {
                continue;
            }
            StateOffsetController offset;
            offset.id = nextRuntimeControllerId++;
            offset.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                offset.hasX = true;
                offset.xExpression = trim(x->value);
            }
            if (const auto* y = findProperty(section, "y")) {
                offset.hasY = true;
                offset.yExpression = trim(y->value);
            }
            if (!offset.hasX && !offset.hasY) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.offsets.push_back(std::move(offset));
            continue;
        }

        if (startsWithNoCase(controllerType, "ForceFeedback")) {
            if (!controllerTrigger) {
                continue;
            }
            StateForceFeedbackController forceFeedback;
            forceFeedback.id = nextRuntimeControllerId++;
            forceFeedback.trigger = *controllerTrigger;
            if (const auto* waveform = findProperty(section, "waveform")) {
                forceFeedback.waveform = lowercaseCopy(trim(waveform->value));
            }
            if (const auto* time = findProperty(section, "time")) {
                forceFeedback.time = std::max(0, parseIntValue(time->value, forceFeedback.time));
            }
            if (const auto* ampl = findProperty(section, "ampl")) {
                const auto values = splitCsv(ampl->value);
                if (!values.empty()) {
                    forceFeedback.amplitude = std::clamp(parseIntValue(values.front(), forceFeedback.amplitude), 0, 255);
                }
            }
            if (const auto* self = findProperty(section, "self")) {
                forceFeedback.self = parseIntValue(self->value, forceFeedback.self ? 1 : 0) != 0;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.forceFeedbacks.push_back(std::move(forceFeedback));
            continue;
        }

        if (startsWithNoCase(controllerType, "GameMakeAnim")) {
            if (!controllerTrigger) {
                continue;
            }
            StateGameMakeAnimController gameMakeAnim;
            gameMakeAnim.id = nextRuntimeControllerId++;
            gameMakeAnim.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                gameMakeAnim.valueExpression = trim(value->value);
            }
            if (const auto* under = findProperty(section, "under")) {
                gameMakeAnim.under = parseIntValue(under->value, 0) != 0;
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseFloatPairValue(pos->value);
                gameMakeAnim.x = values.first;
                gameMakeAnim.y = values.second;
            }
            if (const auto* random = findProperty(section, "random")) {
                gameMakeAnim.random = std::max(0, parseIntValue(random->value, 0));
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.gameMakeAnims.push_back(std::move(gameMakeAnim));
            continue;
        }

        if (startsWithNoCase(controllerType, "DisplayToClipboard")
            || startsWithNoCase(controllerType, "AppendToClipboard")) {
            if (!controllerTrigger) {
                continue;
            }
            StateClipboardController clipboard;
            clipboard.id = nextRuntimeControllerId++;
            clipboard.trigger = *controllerTrigger;
            clipboard.append = startsWithNoCase(controllerType, "AppendToClipboard");
            if (const auto* text = findProperty(section, "text")) {
                clipboard.text = unquote(trim(text->value));
            }
            if (const auto* params = findProperty(section, "params")) {
                clipboard.params = splitCsv(params->value);
                for (auto& param : clipboard.params) {
                    param = trim(param);
                }
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.clipboards.push_back(std::move(clipboard));
            continue;
        }

        if (startsWithNoCase(controllerType, "VictoryQuote")) {
            if (!controllerTrigger) {
                continue;
            }
            StateVictoryQuoteController victoryQuote;
            victoryQuote.id = nextRuntimeControllerId++;
            victoryQuote.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                victoryQuote.valueExpression = trim(value->value);
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.victoryQuotes.push_back(std::move(victoryQuote));
            continue;
        }

        if (startsWithNoCase(controllerType, "RemapPal")) {
            if (!controllerTrigger) {
                continue;
            }
            StateRemapPalController remapPal;
            remapPal.id = nextRuntimeControllerId++;
            remapPal.trigger = *controllerTrigger;
            if (const auto* source = findProperty(section, "source")) {
                const auto values = splitCsv(source->value);
                if (!values.empty()) {
                    remapPal.remap.sourceGroupExpression = trim(values[0]);
                }
                if (values.size() >= 2) {
                    remapPal.remap.sourceItemExpression = trim(values[1]);
                }
            }
            if (const auto* dest = findProperty(section, "dest")) {
                const auto values = splitCsv(dest->value);
                if (!values.empty()) {
                    remapPal.remap.destGroupExpression = trim(values[0]);
                }
                if (values.size() >= 2) {
                    remapPal.remap.destItemExpression = trim(values[1]);
                }
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.remapPals.push_back(std::move(remapPal));
            continue;
        }

        if (startsWithNoCase(controllerType, "Trans")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateTransController trans;
            trans.id = nextRuntimeControllerId++;
            trans.trigger = *controllerTrigger;
            if (const auto* transValue = findProperty(section, "trans")) {
                trans.trans = lowercaseCopy(trim(transValue->value));
            }
            if (const auto* alpha = findProperty(section, "alpha")) {
                const auto values = splitCsv(alpha->value);
                if (!values.empty()) {
                    trans.alphaSourceExpression = values[0];
                }
                if (values.size() >= 2) {
                    trans.alphaDestExpression = values[1];
                }
            }
            state.transControllers.push_back(std::move(trans));
            continue;
        }

        if (startsWithNoCase(controllerType, "AfterImageTime")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* time = findProperty(section, "time");
            if (!time) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAfterImageTimeController afterImageTime;
            afterImageTime.id = nextRuntimeControllerId++;
            afterImageTime.trigger = *controllerTrigger;
            afterImageTime.timeExpression = trim(time->value);
            state.afterImageTimes.push_back(std::move(afterImageTime));
            continue;
        }

        if (startsWithNoCase(controllerType, "AfterImage")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAfterImageController afterImage;
            afterImage.id = nextRuntimeControllerId++;
            afterImage.trigger = *controllerTrigger;
            if (const auto* time = findProperty(section, "time")) {
                afterImage.time = parseIntValue(time->value, afterImage.time);
            }
            if (const auto* length = findProperty(section, "length")) {
                afterImage.length = std::max(1, parseIntValue(length->value, afterImage.length));
            }
            if (const auto* timeGap = findProperty(section, "timegap")) {
                afterImage.timeGap = std::max(1, parseIntValue(timeGap->value, afterImage.timeGap));
            }
            if (const auto* frameGap = findProperty(section, "framegap")) {
                afterImage.frameGap = std::max(1, parseIntValue(frameGap->value, afterImage.frameGap));
            }
            if (const auto* trans = findProperty(section, "trans")) {
                const std::string mode = lowercaseCopy(trim(trans->value));
                afterImage.additive = mode.find("add") != std::string::npos;
            }
            state.afterImages.push_back(std::move(afterImage));
            continue;
        }

        if (startsWithNoCase(controllerType, "Projectile")) {
            if (!controllerTrigger) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateProjectileController projectile;
            projectile.id = nextRuntimeControllerId++;
            projectile.trigger = *controllerTrigger;
            projectile.projectileId = findProperty(section, "projid") ? parseIntValue(findProperty(section, "projid")->value, 0) : projectile.id;
            projectile.anim = findProperty(section, "projanim") ? parseIntValue(findProperty(section, "projanim")->value, -1) : -1;
            projectile.hitAnim = findProperty(section, "projhitanim") ? parseIntValue(findProperty(section, "projhitanim")->value, projectile.anim) : projectile.anim;
            projectile.removeAnim = findProperty(section, "projremanim") ? parseIntValue(findProperty(section, "projremanim")->value, projectile.hitAnim) : projectile.hitAnim;
            projectile.cancelAnim = findProperty(section, "projcancelanim") ? parseIntValue(findProperty(section, "projcancelanim")->value, projectile.removeAnim) : projectile.removeAnim;
            projectile.hits = findProperty(section, "projhits") ? std::max(1, parseIntValue(findProperty(section, "projhits")->value, 1)) : 1;
            projectile.missTime = findProperty(section, "projmisstime") ? std::max(0, parseIntValue(findProperty(section, "projmisstime")->value, 0)) : 0;
            projectile.removeTime = findProperty(section, "projremovetime") ? parseIntValue(findProperty(section, "projremovetime")->value, -1) : -1;
            projectile.removeWhenHit = findProperty(section, "projremove") ? parseIntValue(findProperty(section, "projremove")->value, projectile.removeWhenHit) : projectile.removeWhenHit;
            projectile.pauseMoveTime = findProperty(section, "pausemovetime") ? std::max(0, parseIntValue(findProperty(section, "pausemovetime")->value, 0)) : 0;
            projectile.superMoveTime = findProperty(section, "supermovetime") ? std::max(0, parseIntValue(findProperty(section, "supermovetime")->value, projectile.pauseMoveTime)) : projectile.pauseMoveTime;
            if (const auto* priority = findProperty(section, "projpriority")) {
                const auto values = parseIntPairValue(priority->value, projectile.priority, projectile.cancelPriority);
                projectile.priority = values.first;
                projectile.cancelPriority = values.second;
            } else if (const auto* priority = findProperty(section, "priority")) {
                const auto values = parseIntPairValue(priority->value, projectile.priority, projectile.cancelPriority);
                projectile.priority = values.first;
                projectile.cancelPriority = values.second;
            }
            if (const auto* edgeBound = findProperty(section, "projedgebound")) {
                projectile.projEdgeBound = parseFloatValue(edgeBound->value, projectile.projEdgeBound);
            }
            if (const auto* screenBound = findProperty(section, "projscreenbound")) {
                projectile.projEdgeBound = parseFloatValue(screenBound->value, projectile.projEdgeBound);
            }
            if (const auto* stageBound = findProperty(section, "projstagebound")) {
                projectile.projStageBound = parseFloatValue(stageBound->value, projectile.projStageBound);
            }
            if (const auto* heightBound = findProperty(section, "projheightbound")) {
                const auto values = parseFloatPairValue(heightBound->value, projectile.projHeightBoundLow, projectile.projHeightBoundHigh);
                projectile.projHeightBoundLow = values.first;
                projectile.projHeightBoundHigh = values.second;
            }
            if (const auto* scale = findProperty(section, "projscale")) {
                const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                projectile.scaleX = values.first;
                projectile.scaleY = values.second;
                const auto expressions = parseExpressionPairValue(scale->value, "1", "1");
                projectile.scaleXExpression = expressions.first;
                projectile.scaleYExpression = expressions.second;
            }
            if (const auto* shadow = findProperty(section, "projshadow")) {
                const auto parts = splitCsv(shadow->value);
                if (!parts.empty() && parseIntValue(parts[0], 0) == -1) {
                    projectile.shadowEnabled = true;
                    projectile.shadowR = 96;
                    projectile.shadowG = 96;
                    projectile.shadowB = 96;
                } else {
                    const auto values = parseIntTripleValue(shadow->value, 0, 0, 0);
                    projectile.shadowEnabled = values[0] != 0 || values[1] != 0 || values[2] != 0;
                    projectile.shadowR = values[0];
                    projectile.shadowG = values[1];
                    projectile.shadowB = values[2];
                }
            }
            if (const auto* velocity = findProperty(section, "velocity")) {
                const auto values = parseFloatPairValue(velocity->value);
                projectile.vx = values.first;
                projectile.vy = values.second;
                const auto expressions = parseExpressionPairValue(velocity->value);
                projectile.vxExpression = expressions.first;
                projectile.vyExpression = expressions.second;
            }
            if (const auto* removeVelocity = findProperty(section, "remvelocity")) {
                const auto values = parseFloatPairValue(removeVelocity->value);
                projectile.removeVx = values.first;
                projectile.removeVy = values.second;
                const auto expressions = parseExpressionPairValue(removeVelocity->value);
                projectile.removeVxExpression = expressions.first;
                projectile.removeVyExpression = expressions.second;
            }
            if (const auto* accel = findProperty(section, "accel")) {
                const auto values = parseFloatPairValue(accel->value);
                projectile.ax = values.first;
                projectile.ay = values.second;
                const auto expressions = parseExpressionPairValue(accel->value);
                projectile.axExpression = expressions.first;
                projectile.ayExpression = expressions.second;
            }
            if (const auto* velMul = findProperty(section, "velmul")) {
                const auto values = parseFloatPairValue(velMul->value, 1.0f, 1.0f);
                projectile.velMulX = values.first;
                projectile.velMulY = values.second;
                const auto expressions = parseExpressionPairValue(velMul->value, "1", "1");
                projectile.velMulXExpression = expressions.first;
                projectile.velMulYExpression = expressions.second;
            }
            if (const auto* offset = findProperty(section, "offset")) {
                const auto values = parseFloatPairValue(offset->value);
                projectile.x = values.first;
                projectile.y = values.second;
                const auto expressions = parseExpressionPairValue(offset->value);
                projectile.xExpression = expressions.first;
                projectile.yExpression = expressions.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                projectile.postype = lowercaseCopy(trim(postype->value));
            }

            projectile.hitDef.id = projectile.id;
            projectile.hitDef.targetId = projectile.projectileId;
            projectile.hitDef.stateNo = state.stateNo;
            if (const auto* attr = findProperty(section, "attr")) {
                projectile.hitDef.attr = trim(attr->value);
            }
            if (const auto* damage = findProperty(section, "damage")) {
                const auto values = parseIntPairValue(damage->value);
                projectile.hitDef.damage = values.first;
                projectile.hitDef.guardDamage = values.second;
                const auto expressions = parseExpressionPairValue(damage->value);
                projectile.hitDef.damageExpression = expressions.first;
                projectile.hitDef.guardDamageExpression = expressions.second;
            }
            if (const auto* animtype = findProperty(section, "animtype")) {
                projectile.hitDef.animtype = trim(animtype->value);
            }
            if (const auto* hitflag = findProperty(section, "hitflag")) {
                projectile.hitDef.hitflag = trim(hitflag->value);
            }
            if (const auto* guardflag = findProperty(section, "guardflag")) {
                projectile.hitDef.guardflag = trim(guardflag->value);
            }
            if (const auto* guardDistance = findProperty(section, "guard.dist")) {
                projectile.hitDef.guardDistance = parseIntValue(guardDistance->value, projectile.hitDef.guardDistance);
            }
            if (const auto* pausetime = findProperty(section, "pausetime")) {
                const auto values = parseIntPairValue(pausetime->value);
                projectile.hitDef.pausetimeP1 = values.first;
                projectile.hitDef.pausetimeP2 = values.second;
                const auto expressions = parseExpressionPairValue(pausetime->value);
                projectile.hitDef.pausetimeP1Expression = expressions.first;
                projectile.hitDef.pausetimeP2Expression = expressions.second;
            }
            if (const auto* sparkNo = findProperty(section, "sparkno")) {
                projectile.hitDef.sparkNo = parseIntValue(sparkNo->value, projectile.hitDef.sparkNo);
                projectile.hitDef.sparkNoExpression = trim(sparkNo->value);
            }
            if (const auto* guardSparkNo = findProperty(section, "guard.sparkno")) {
                projectile.hitDef.guardSparkNo = parseIntValue(guardSparkNo->value, projectile.hitDef.guardSparkNo);
                projectile.hitDef.guardSparkNoExpression = trim(guardSparkNo->value);
            }
            if (const auto* sparkxy = findProperty(section, "sparkxy")) {
                const auto values = parseFloatPairValue(sparkxy->value);
                projectile.hitDef.sparkX = values.first;
                projectile.hitDef.sparkY = values.second;
                const auto expressions = parseExpressionPairValue(sparkxy->value);
                projectile.hitDef.sparkXExpression = expressions.first;
                projectile.hitDef.sparkYExpression = expressions.second;
            }
            if (const auto* hitSound = findProperty(section, "hitsound")) {
                if (const auto values = parseSoundValue(hitSound->value)) {
                    projectile.hitDef.hitSoundGroup = values->group;
                    projectile.hitDef.hitSoundIndex = values->index;
                    projectile.hitDef.hitSoundForceCommon = values->forceCommon;
                    projectile.hitDef.hitSoundGroupExpression = values->groupExpression;
                    projectile.hitDef.hitSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* guardSound = findProperty(section, "guardsound")) {
                if (const auto values = parseSoundValue(guardSound->value)) {
                    projectile.hitDef.guardSoundGroup = values->group;
                    projectile.hitDef.guardSoundIndex = values->index;
                    projectile.hitDef.guardSoundForceCommon = values->forceCommon;
                    projectile.hitDef.guardSoundGroupExpression = values->groupExpression;
                    projectile.hitDef.guardSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* groundType = findProperty(section, "ground.type")) {
                projectile.hitDef.groundType = trim(groundType->value);
            }
            if (const auto* groundSlideTime = findProperty(section, "ground.slidetime")) {
                projectile.hitDef.groundSlideTime = parseIntValue(groundSlideTime->value, projectile.hitDef.groundSlideTime);
                projectile.hitDef.groundSlideTimeExpression = trim(groundSlideTime->value);
            }
            if (const auto* groundHitTime = findProperty(section, "ground.hittime")) {
                projectile.hitDef.groundHitTime = parseIntValue(groundHitTime->value, projectile.hitDef.groundHitTime);
                projectile.hitDef.groundHitTimeExpression = trim(groundHitTime->value);
            }
            if (const auto* groundVelocity = findProperty(section, "ground.velocity")) {
                const auto values = parseFloatPairValue(groundVelocity->value);
                projectile.hitDef.groundVelocityX = values.first;
                projectile.hitDef.groundVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(groundVelocity->value);
                projectile.hitDef.groundVelocityXExpression = expressions.first;
                projectile.hitDef.groundVelocityYExpression = expressions.second;
            }
            if (const auto* airVelocity = findProperty(section, "air.velocity")) {
                const auto values = parseFloatPairValue(airVelocity->value);
                projectile.hitDef.hasAirVelocity = true;
                projectile.hitDef.airVelocityX = values.first;
                projectile.hitDef.airVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(airVelocity->value);
                projectile.hitDef.airVelocityXExpression = expressions.first;
                projectile.hitDef.airVelocityYExpression = expressions.second;
            }
            if (const auto* airHitTime = findProperty(section, "air.hittime")) {
                projectile.hitDef.airHitTime = parseIntValue(airHitTime->value, projectile.hitDef.airHitTime);
                projectile.hitDef.airHitTimeExpression = trim(airHitTime->value);
            }
            if (const auto* fall = findProperty(section, "fall")) {
                projectile.hitDef.fall = parseIntValue(fall->value, 0) != 0;
                projectile.hitDef.fallExpression = trim(fall->value);
            }
            if (const auto* airFall = findProperty(section, "air.fall")) {
                projectile.hitDef.airFall = parseIntValue(airFall->value, 0) != 0;
                projectile.hitDef.airFallExpression = trim(airFall->value);
            }
            if (const auto* fallRecover = findProperty(section, "fall.recover")) {
                projectile.hitDef.fallRecover = parseIntValue(fallRecover->value, 1) != 0;
                projectile.hitDef.fallRecoverExpression = trim(fallRecover->value);
            }
            if (const auto* fallRecoverTime = findProperty(section, "fall.recovertime")) {
                projectile.hitDef.fallRecoverTime = parseIntValue(fallRecoverTime->value, projectile.hitDef.fallRecoverTime);
                projectile.hitDef.fallRecoverTimeExpression = trim(fallRecoverTime->value);
            }
            if (const auto* fallDamage = findProperty(section, "fall.damage")) {
                projectile.hitDef.fallDamage = parseIntValue(fallDamage->value, 0);
                projectile.hitDef.fallDamageExpression = trim(fallDamage->value);
            }
            if (const auto* fallXVelocity = findProperty(section, "fall.xvelocity")) {
                projectile.hitDef.hasFallXVelocity = true;
                projectile.hitDef.fallXVelocity = parseFloatValue(fallXVelocity->value, projectile.hitDef.fallXVelocity);
                projectile.hitDef.fallXVelocityExpression = trim(fallXVelocity->value);
            }
            if (const auto* fallYVelocity = findProperty(section, "fall.yvelocity")) {
                projectile.hitDef.hasFallYVelocity = true;
                projectile.hitDef.fallYVelocity = parseFloatValue(fallYVelocity->value, projectile.hitDef.fallYVelocity);
                projectile.hitDef.fallYVelocityExpression = trim(fallYVelocity->value);
            }
            if (const auto* yAccel = findProperty(section, "yaccel")) {
                projectile.hitDef.hasYAccel = true;
                projectile.hitDef.yAccel = parseFloatValue(yAccel->value, 0.0f);
                projectile.hitDef.yAccelExpression = trim(yAccel->value);
            }
            if (const auto* guardVelocity = findProperty(section, "guard.velocity")) {
                const auto values = parseFloatPairValue(guardVelocity->value);
                projectile.hitDef.hasGuardVelocity = true;
                projectile.hitDef.guardVelocityX = values.first;
                projectile.hitDef.guardVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(guardVelocity->value);
                projectile.hitDef.guardVelocityXExpression = expressions.first;
                projectile.hitDef.guardVelocityYExpression = expressions.second;
            } else {
                projectile.hitDef.guardVelocityX = projectile.hitDef.groundVelocityX;
                projectile.hitDef.guardVelocityY = projectile.hitDef.groundVelocityY;
            }
            if (const auto* envShakeTime = findProperty(section, "envshake.time")) {
                projectile.hitDef.envShake.time = std::max(0, parseIntValue(envShakeTime->value, projectile.hitDef.envShake.time));
                projectile.hitDef.envShake.timeExpression = trim(envShakeTime->value);
                projectile.hitDef.envShake.enabled = projectile.hitDef.envShake.time > 0;
            }
            if (const auto* envShakeFrequency = findProperty(section, "envshake.freq")) {
                projectile.hitDef.envShake.frequency = std::max(1, parseIntValue(envShakeFrequency->value, projectile.hitDef.envShake.frequency));
                projectile.hitDef.envShake.frequencyExpression = trim(envShakeFrequency->value);
            }
            if (const auto* envShakeAmplitude = findProperty(section, "envshake.ampl")) {
                projectile.hitDef.envShake.amplitude = parseFloatValue(envShakeAmplitude->value, projectile.hitDef.envShake.amplitude);
                projectile.hitDef.envShake.amplitudeExpression = trim(envShakeAmplitude->value);
                projectile.hitDef.envShake.enabled = projectile.hitDef.envShake.time > 0 && std::abs(projectile.hitDef.envShake.amplitude) > 0.001f;
            }
            if (const auto* envShakePhase = findProperty(section, "envshake.phase")) {
                projectile.hitDef.envShake.phase = parseIntValue(envShakePhase->value, projectile.hitDef.envShake.phase);
                projectile.hitDef.envShake.phaseExpression = trim(envShakePhase->value);
            }
            if (const auto* fallEnvShakeTime = findProperty(section, "fall.envshake.time")) {
                projectile.hitDef.fallEnvShake.time = std::max(0, parseIntValue(fallEnvShakeTime->value, projectile.hitDef.fallEnvShake.time));
                projectile.hitDef.fallEnvShake.timeExpression = trim(fallEnvShakeTime->value);
                projectile.hitDef.fallEnvShake.enabled = projectile.hitDef.fallEnvShake.time > 0;
            }
            if (const auto* fallEnvShakeFrequency = findProperty(section, "fall.envshake.freq")) {
                projectile.hitDef.fallEnvShake.frequency = std::max(1, parseIntValue(fallEnvShakeFrequency->value, projectile.hitDef.fallEnvShake.frequency));
                projectile.hitDef.fallEnvShake.frequencyExpression = trim(fallEnvShakeFrequency->value);
            }
            if (const auto* fallEnvShakeAmplitude = findProperty(section, "fall.envshake.ampl")) {
                projectile.hitDef.fallEnvShake.amplitude = parseFloatValue(fallEnvShakeAmplitude->value, projectile.hitDef.fallEnvShake.amplitude);
                projectile.hitDef.fallEnvShake.amplitudeExpression = trim(fallEnvShakeAmplitude->value);
                projectile.hitDef.fallEnvShake.enabled = projectile.hitDef.fallEnvShake.time > 0 && std::abs(projectile.hitDef.fallEnvShake.amplitude) > 0.001f;
            }
            if (const auto* fallEnvShakePhase = findProperty(section, "fall.envshake.phase")) {
                projectile.hitDef.fallEnvShake.phase = parseIntValue(fallEnvShakePhase->value, projectile.hitDef.fallEnvShake.phase);
                projectile.hitDef.fallEnvShake.phaseExpression = trim(fallEnvShakePhase->value);
            }
            projectile.hitDef.palFx = parsePaletteEffectSpec(section);
            if (projectile.anim >= 0) {
                state.projectiles.push_back(std::move(projectile));
            }
            continue;
        }

        if (startsWithNoCase(controllerType, "MakeDust")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateMakeDustController makeDust;
            makeDust.id = nextRuntimeControllerId++;
            makeDust.trigger = *controllerTrigger;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                makeDust.x = values.first;
                makeDust.y = values.second;
            }
            if (const auto* pos2 = findProperty(section, "pos2")) {
                const auto values = parseExplodPosition(pos2->value, constants);
                makeDust.hasPos2 = true;
                makeDust.x2 = values.first;
                makeDust.y2 = values.second;
            }
            if (const auto* spacing = findProperty(section, "spacing")) {
                makeDust.spacing = std::max(1, parseIntValue(spacing->value, makeDust.spacing));
            }
            state.makeDusts.push_back(std::move(makeDust));
            continue;
        }
        }
    }

    return states;
}

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

void addUniqueStateType(std::vector<char>& stateTypes, char stateType) {
    stateType = static_cast<char>(SDL_toupper(static_cast<unsigned char>(stateType)));
    if (std::find(stateTypes.begin(), stateTypes.end(), stateType) == stateTypes.end()) {
        stateTypes.push_back(stateType);
    }
}

struct StateTypeCondition {
    char stateType = 0;
    bool negated = false;
};

size_t findNoCase(std::string_view value, std::string_view needle, size_t start = 0) {
    if (needle.empty() || start >= value.size()) {
        return std::string_view::npos;
    }
    const std::string loweredValue = lowercaseCopy(value);
    const std::string loweredNeedle = lowercaseCopy(needle);
    return loweredValue.find(loweredNeedle, start);
}

bool expressionHasOr(const std::string& value) {
    return value.find("||") != std::string::npos;
}

std::optional<StateTypeCondition> parseStateTypeCondition(const std::string& value) {
    const auto condition = stripOuterParens(value);
    if (!startsWithNoCase(condition, "statetype")) {
        return std::nullopt;
    }

    std::string tail = trim(std::string_view(condition).substr(std::string_view("statetype").size()));
    bool negated = false;
    if (startsWithNoCase(tail, "!=")) {
        negated = true;
        tail = trim(std::string_view(tail).substr(2));
    } else if (startsWithNoCase(tail, "=")) {
        tail = trim(std::string_view(tail).substr(1));
    } else {
        return std::nullopt;
    }

    if (tail.empty()) {
        return std::nullopt;
    }

    const char stateType = static_cast<char>(SDL_toupper(static_cast<unsigned char>(tail.front())));
    if (!isSupportedStateType(stateType)) {
        return std::nullopt;
    }
    return StateTypeCondition{ stateType, negated };
}

std::optional<CommandConditionSubject> commandConditionSubjectFromName(std::string_view name) {
    if (equalsNoCase(name, "stateno")) {
        return CommandConditionSubject::StateNo;
    }
    if (equalsNoCase(name, "time")) {
        return CommandConditionSubject::Time;
    }
    if (equalsNoCase(name, "power")) {
        return CommandConditionSubject::Power;
    }
    if (equalsNoCase(name, "roundstate")) {
        return CommandConditionSubject::RoundState;
    }
    if (equalsNoCase(name, "ailevel")) {
        return CommandConditionSubject::AiLevel;
    }
    return std::nullopt;
}

std::optional<CommandStateEntry::IntCondition> parseCommandIntCondition(const std::string& value) {
    const std::string condition = stripOuterParens(value);
    if (equalsNoCase(condition, "!AILevel")) {
        return CommandStateEntry::IntCondition{ CommandConditionSubject::AiLevel, CompareOp::Equal, 0 };
    }

    constexpr std::array<std::string_view, 5> subjectNames{ "stateno", "time", "power", "roundstate", "ailevel" };
    for (const std::string_view subjectName : subjectNames) {
        if (!startsWithNoCase(condition, subjectName)) {
            continue;
        }
        const size_t nameLength = subjectName.size();
        if (condition.size() > nameLength && isIdentifierChar(condition[nameLength])) {
            continue;
        }

        const auto subject = commandConditionSubjectFromName(subjectName);
        if (!subject) {
            continue;
        }

        const std::string tail = trim(std::string_view(condition).substr(nameLength));
        const auto compare = findCompareOp(tail);
        if (!compare) {
            return std::nullopt;
        }

        const auto [op, pos] = *compare;
        const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
        const auto rhs = parsePlainIntValue(trim(std::string_view(tail).substr(pos + opLength)));
        if (!rhs) {
            return std::nullopt;
        }
        return CommandStateEntry::IntCondition{ *subject, op, *rhs };
    }

    return std::nullopt;
}

std::optional<CommandStateEntry::IntRangeCondition> parseCommandIntRangeCondition(const std::string& value) {
    const std::string condition = stripOuterParens(value);
    constexpr std::array<std::string_view, 5> subjectNames{ "stateno", "time", "power", "roundstate", "ailevel" };
    for (const std::string_view subjectName : subjectNames) {
        if (!startsWithNoCase(condition, subjectName)) {
            continue;
        }
        const size_t nameLength = subjectName.size();
        if (condition.size() > nameLength && isIdentifierChar(condition[nameLength])) {
            continue;
        }

        const auto subject = commandConditionSubjectFromName(subjectName);
        if (!subject) {
            continue;
        }

        const std::string tail = trim(std::string_view(condition).substr(nameLength));
        const auto compare = findCompareOp(tail);
        if (!compare) {
            return std::nullopt;
        }

        const auto [op, pos] = *compare;
        if (op != CompareOp::Equal && op != CompareOp::NotEqual) {
            return std::nullopt;
        }
        const size_t opLength = op == CompareOp::NotEqual ? 2 : 1;
        const std::string rhs = trim(std::string_view(tail).substr(pos + opLength));
        if (rhs.size() < 5 || rhs.front() != '[' || rhs.back() != ']') {
            return std::nullopt;
        }
        const auto parts = splitCsv(std::string(std::string_view(rhs).substr(1, rhs.size() - 2)));
        if (parts.empty()) {
            return std::nullopt;
        }
        const auto minValue = parsePlainIntValue(parts[0]);
        const auto maxValue = parts.size() >= 2 ? parsePlainIntValue(parts[1]) : minValue;
        if (!minValue || !maxValue) {
            return std::nullopt;
        }
        return CommandStateEntry::IntRangeCondition{
            *subject,
            op,
            std::min(*minValue, *maxValue),
            std::max(*minValue, *maxValue),
        };
    }

    return std::nullopt;
}

bool triggerRequiresCtrl(const std::string& value) {
    const auto trigger = stripOuterParens(value);
    if (!startsWithNoCase(trigger, "ctrl")) {
        return false;
    }
    if (trigger.size() == std::string_view("ctrl").size()) {
        return true;
    }
    const char next = trigger[std::string_view("ctrl").size()];
    return std::isspace(static_cast<unsigned char>(next)) || next == '=' || next == '|' || next == '&';
}

void applyCommandTriggerClause(CommandStateEntry& entry, const std::string& clause) {
    if (clause.empty()) {
        return;
    }

    const bool hasOr = expressionHasOr(clause);
    const auto commands = findCommandConditions(clause);
    if (hasOr && !commands.empty()) {
        std::vector<std::string> options;
        for (const auto& command : commands) {
            if (command.negated) {
                options.clear();
                break;
            }
            addUniqueCommand(options, command.command);
        }
        if (!options.empty()) {
            entry.commandOptionGroups.push_back(std::move(options));
            return;
        }
    }

    for (const auto& command : commands) {
        addUniqueCommand(command.negated ? entry.forbiddenCommands : entry.requiredCommands, command.command);
    }

    if (hasOr) {
        return;
    }

    if (const auto stateType = parseStateTypeCondition(clause)) {
        if (stateType->negated) {
            addUniqueStateType(entry.forbiddenStateTypes, stateType->stateType);
        } else if (entry.requiredStateType == 0) {
            entry.requiredStateType = stateType->stateType;
        }
    }
    if (const auto condition = parseCommandIntCondition(clause)) {
        entry.intConditions.push_back(*condition);
    }
    if (const auto rangeCondition = parseCommandIntRangeCondition(clause)) {
        entry.intRangeConditions.push_back(*rangeCondition);
        return;
    }
    if (const auto expression = parseMugenExpressionCondition(clause)) {
        if (expressionLooksSupported(expression->lhs) || expressionLooksSupported(expression->rhs)) {
            entry.expressionConditions.push_back(*expression);
        }
    } else if (expressionLooksSupported(clause)) {
        entry.booleanExpressions.push_back(stripOuterParens(clause));
    }
    if (triggerRequiresCtrl(clause)) {
        entry.requiresCtrl = true;
    }
    if (triggerIsMoveContact(clause)) {
        entry.requiresMoveContact = true;
    }
}

void applyCommandTriggerExpression(CommandStateEntry& entry, const std::string& value) {
    for (const auto& clause : splitAndClauses(value)) {
        applyCommandTriggerClause(entry, clause);
    }
}

std::vector<CommandStateEntry> loadCommandStateEntries(const CharacterFiles& files) {
    if (files.cmd.empty() || !std::filesystem::exists(files.cmd)) {
        return {};
    }

    const auto cmd = parseMugenTextFile(files.cmd);
    std::vector<CommandStateEntry> entries;
    bool inStateMinusOne = false;

    for (const auto& section : cmd.sections) {
        if (const auto stateNo = parseStateNumber(section.name, "Statedef ")) {
            inStateMinusOne = *stateNo == -1;
            continue;
        }
        if (!inStateMinusOne || !startsWithNoCase(section.name, "State -1")) {
            continue;
        }

        const auto* type = findProperty(section, "type");
        const auto* value = findProperty(section, "value");
        if (!type || !value || !startsWithNoCase(trim(type->value), "ChangeState")) {
            continue;
        }

        const auto targetState = parsePlainIntValue(value->value);
        const std::string targetStateExpression = trim(value->value);

        CommandStateEntry entry;
        entry.targetState = targetState.value_or(0);
        entry.targetStateExpression = targetStateExpression;
        if (const auto comma = section.name.find(','); comma != std::string::npos) {
            entry.label = trim(std::string_view(section.name).substr(comma + 1));
        }

        for (const auto& property : section.properties) {
            const bool isTriggerAll = startsWithNoCase(property.key, "triggerall");
            const bool isPrimaryTrigger = startsWithNoCase(property.key, "trigger1");
            if (!isTriggerAll && !isPrimaryTrigger) {
                continue;
            }

            applyCommandTriggerExpression(entry, property.value);
        }

        if (!entry.requiredCommands.empty() || !entry.commandOptionGroups.empty()) {
            entries.push_back(std::move(entry));
        }
    }

    return entries;
}

std::string normalizeCommandAtomSymbol(std::string token) {
    token = trim(token);
    while (!token.empty() && token.front() == '>') {
        token = trim(std::string_view(token).substr(1));
    }
    return token;
}

CommandAtom parseCommandAtom(std::string token) {
    CommandAtom atom;
    token = normalizeCommandAtomSymbol(token);

    while (!token.empty()) {
        if (token.front() == '/') {
            atom.hold = true;
            token = trim(std::string_view(token).substr(1));
            continue;
        }
        if (token.front() == '~') {
            atom.release = true;
            token = trim(std::string_view(token).substr(1));
            while (!token.empty() && std::isdigit(static_cast<unsigned char>(token.front()))) {
                token.erase(token.begin());
            }
            continue;
        }
        if (token.front() == '$') {
            atom.broadDirection = true;
            token = trim(std::string_view(token).substr(1));
            continue;
        }
        break;
    }

    if (token.size() == 1) {
        const char ch = token.front();
        if (ch == 'a' || ch == 'b' || ch == 'c' || ch == 'x' || ch == 'y' || ch == 'z' || ch == 's') {
            atom.symbol = lowercaseCopy(token);
        } else {
            atom.symbol = uppercaseCopy(token);
        }
    } else {
        atom.symbol = uppercaseCopy(token);
    }
    return atom;
}

CommandStep parseCommandStep(const std::string& token) {
    CommandStep step;
    size_t start = 0;
    while (start <= token.size()) {
        const auto delimiter = token.find('+', start);
        const auto end = delimiter == std::string::npos ? token.size() : delimiter;
        CommandAtom atom = parseCommandAtom(std::string(std::string_view(token).substr(start, end - start)));
        if (!atom.symbol.empty()) {
            step.atoms.push_back(std::move(atom));
        }
        if (delimiter == std::string::npos) {
            break;
        }
        start = delimiter + 1;
    }
    return step;
}

CommandDefinition parseCommandDefinitionSection(const MugenSection& section, int defaultTime, int defaultBufferTime) {
    CommandDefinition definition;
    definition.maxTime = std::max(1, defaultTime);
    definition.bufferTime = std::max(1, defaultBufferTime);
    if (const auto* name = findProperty(section, "name")) {
        definition.name = unquote(name->value);
    }
    if (const auto* time = findProperty(section, "time")) {
        definition.maxTime = std::max(1, parseIntValue(time->value, definition.maxTime));
    }
    const auto* command = findProperty(section, "command");
    if (!command) {
        return definition;
    }

    for (const auto& token : splitCsv(command->value)) {
        CommandStep step = parseCommandStep(token);
        if (!step.atoms.empty()) {
            definition.steps.push_back(std::move(step));
        }
    }
    return definition;
}

std::vector<CommandDefinition> loadCommandDefinitions(const CharacterFiles& files) {
    if (files.cmd.empty() || !std::filesystem::exists(files.cmd)) {
        return {};
    }

    std::vector<CommandDefinition> definitions;
    const auto cmd = parseMugenTextFile(files.cmd);
    int defaultCommandTime = 15;
    int defaultCommandBufferTime = 1;
    for (const auto& section : cmd.sections) {
        if (!equalsNoCase(section.name, "Defaults")) {
            continue;
        }
        if (const auto* time = findProperty(section, "command.time")) {
            defaultCommandTime = std::max(1, parseIntValue(time->value, defaultCommandTime));
        }
        if (const auto* bufferTime = findProperty(section, "command.buffer.time")) {
            defaultCommandBufferTime = std::max(1, parseIntValue(bufferTime->value, defaultCommandBufferTime));
        }
    }

    for (const auto& section : cmd.sections) {
        if (!equalsNoCase(section.name, "Command")) {
            continue;
        }

        CommandDefinition definition = parseCommandDefinitionSection(section, defaultCommandTime, defaultCommandBufferTime);
        if (!definition.name.empty() && !definition.steps.empty()) {
            definitions.push_back(std::move(definition));
        }
    }
    return definitions;
}

TextureSprite loadCharacterSprite(SDL_Renderer* renderer, const CharacterFiles& files, int group, int image) {
    if (files.sprite.empty() || !std::filesystem::exists(files.sprite)) {
        return {};
    }

    const auto sff = loadSffArchive(files.sprite);
    const auto* sprite = findSprite(sff, group, image);
    if (!sprite) {
        return {};
    }

    DecodeOptions options;
    options.transparentColorZero = true;
    std::optional<Palette> palette;
    if (!files.palette.empty() && std::filesystem::exists(files.palette)) {
        palette = loadActPalette(files.palette);
        options.fallbackPalette = &*palette;
        options.reverseFallbackPalette = true;
    }
    const auto decoded = decodeSffSprite(sff, *sprite, options);
    if (!decoded) {
        return {};
    }
    return makeTextureSprite(renderer, *decoded);
}

TextureSprite loadSystemSpriteFromArchive(
    SDL_Renderer* renderer,
    const SffArchive& sff,
    int group,
    int image,
    bool transparentColorZero) {
    const auto* sprite = findSprite(sff, group, image);
    if (!sprite) {
        return {};
    }
    DecodeOptions options;
    options.transparentColorZero = transparentColorZero;
    const auto decoded = decodeSffSprite(sff, *sprite, options);
    return decoded ? makeTextureSprite(renderer, *decoded) : TextureSprite{};
}

SystemScreenAssets loadSystemScreenAssets(SDL_Renderer* renderer, const std::filesystem::path& gameRoot) {
    SystemScreenAssets assets;
    const auto systemSff = gameRoot / "data" / "system.sff";
    if (!std::filesystem::exists(systemSff)) {
        return assets;
    }

    try {
        const auto sff = loadSffArchive(systemSff);
        assets.titleLogo = loadSystemSpriteFromArchive(renderer, sff, 0, 0, true);
        assets.titleTop = loadSystemSpriteFromArchive(renderer, sff, 5, 0, false);
        assets.titleFloor = loadSystemSpriteFromArchive(renderer, sff, 5, 1, false);
        assets.titleShade = loadSystemSpriteFromArchive(renderer, sff, 5, 2, true);
        assets.selectBackdrop = loadSystemSpriteFromArchive(renderer, sff, 100, 0, false);
        assets.selectShadow = loadSystemSpriteFromArchive(renderer, sff, 100, 1, true);
        assets.selectTitleA = loadSystemSpriteFromArchive(renderer, sff, 102, 0, true);
        assets.selectTitleB = loadSystemSpriteFromArchive(renderer, sff, 102, 1, true);
        assets.selectTitleC = loadSystemSpriteFromArchive(renderer, sff, 102, 2, true);
        assets.selectCell = loadSystemSpriteFromArchive(renderer, sff, 150, 0, true);
        assets.selectP1Cursor = loadSystemSpriteFromArchive(renderer, sff, 160, 0, true);
        assets.selectP1Done = loadSystemSpriteFromArchive(renderer, sff, 161, 0, true);
    } catch (const std::exception& ex) {
        SDL_Log("system.sff load failed: %s", ex.what());
    }
    return assets;
}

TextureSprite loadCharacterIconSprite(SDL_Renderer* renderer, const std::filesystem::path& gameRoot, const CharacterSlot& character) {
    try {
        const CharacterFiles files = resolveCharacterFiles(gameRoot, character);
        return loadCharacterSprite(renderer, files, 9000, 0);
    } catch (const std::exception& ex) {
        SDL_Log("Character icon load failed %s: %s", character.displayName.c_str(), ex.what());
        return {};
    }
}

TextureSprite loadCharacterFaceSprite(SDL_Renderer* renderer, const std::filesystem::path& gameRoot, const CharacterSlot& character) {
    try {
        const CharacterFiles files = resolveCharacterFiles(gameRoot, character);
        return loadCharacterSprite(renderer, files, 9000, 1);
    } catch (const std::exception& ex) {
        SDL_Log("Character face load failed %s: %s", character.displayName.c_str(), ex.what());
        return {};
    }
}

std::vector<StageBackgroundElement> loadStageBackground(SDL_Renderer* renderer, const StageSlot& stage) {
    auto sffPath = stage.defPath;
    sffPath.replace_extension(".sff");
    if (!std::filesystem::exists(sffPath)) {
        return {};
    }

    const auto sff = loadSffArchive(sffPath);
    const auto doc = parseMugenTextFile(stage.defPath);
    std::vector<StageBackgroundElement> elements;
    for (const auto& section : doc.sections) {
        if (!(section.name == "BG" || startsWithNoCase(section.name, "BG "))) {
            continue;
        }
        const auto* spriteNo = findProperty(section, "spriteno");
        if (!spriteNo) {
            continue;
        }
        const auto pair = parsePair(spriteNo->value);
        if (!pair) {
            continue;
        }
        const auto* sprite = findSprite(sff, static_cast<int>(pair->first), static_cast<int>(pair->second));
        if (!sprite) {
            continue;
        }

        DecodeOptions options;
        options.transparentColorZero = true;
        if (const auto* mask = findProperty(section, "mask")) {
            options.transparentColorZero = trim(mask->value) != "0";
        }
        const auto decoded = decodeSffSprite(sff, *sprite, options);
        if (!decoded) {
            continue;
        }

        StageBackgroundElement element;
        element.sprite = makeTextureSprite(renderer, *decoded);
        if (const auto* start = findProperty(section, "start")) {
            if (const auto startPair = parsePair(start->value)) {
                element.x = startPair->first;
                element.y = startPair->second;
            }
        }
        if (const auto* delta = findProperty(section, "delta")) {
            if (const auto deltaPair = parsePair(delta->value)) {
                element.deltaX = deltaPair->first;
                element.deltaY = deltaPair->second;
            }
        }
        if (const auto* tile = findProperty(section, "tile")) {
            if (const auto tilePair = parsePair(tile->value)) {
                element.tileX = tilePair->first != 0;
                element.tileY = tilePair->second != 0;
            }
        }
        if (const auto* layerNo = findProperty(section, "layerno")) {
            try {
                element.layerNo = std::stoi(layerNo->value);
            } catch (...) {
                element.layerNo = 0;
            }
        }
        elements.push_back(element);
    }
    return elements;
}

void destroyTextureSprite(TextureSprite& sprite);
void destroyAnimationClips(std::vector<AnimationClip>& clips);
std::vector<DecodedSoundSample> loadDecodedSoundSamples(const std::filesystem::path& path, const SDL_AudioSpec& playbackSpec);

LoadedContentSummary buildContentSummary(
    const CharacterSlot& character,
    const CharacterFiles& files,
    const StageSlot* stage) {
    LoadedContentSummary summary;
    summary.characterName = character.displayName;
    summary.characterAuthor = character.author;
    summary.stageName = stage ? stage->displayName : "Unknown";

    try {
        if (!files.anim.empty() && std::filesystem::exists(files.anim)) {
            summary.airActions = countSectionsWithPrefix(parseMugenTextFile(files.anim), "Begin Action");
        }
        for (const auto& stateFile : files.stateFiles) {
            summary.cnsStates += countSectionsWithPrefix(parseMugenTextFile(stateFile), "Statedef");
        }
        if (!files.cmd.empty() && std::filesystem::exists(files.cmd)) {
            summary.cmdCommands = countSectionsWithPrefix(parseMugenTextFile(files.cmd), "Command");
        }
        if (stage && std::filesystem::exists(stage->defPath)) {
            const auto stageDoc = parseMugenTextFile(stage->defPath);
            for (const auto& section : stageDoc.sections) {
                if (section.name == "BG" || startsWithNoCase(section.name, "BG ")) {
                    ++summary.stageBackgrounds;
                }
            }
        }
    } catch (const std::exception& ex) {
        SDL_Log("Content summary failed: %s", ex.what());
    }

    return summary;
}

const StageSlot* currentStageSlot(const AppState& state) {
    if (state.stages.empty()
        || state.selectedStage < 0
        || state.selectedStage >= static_cast<int>(state.stages.size())) {
        return nullptr;
    }
    return &state.stages[static_cast<size_t>(state.selectedStage)];
}

int findStageIndexByDefPath(const AppState& state, const std::filesystem::path& def) {
    if (def.empty()) {
        return -1;
    }
    const auto wanted = def.lexically_normal().generic_string();
    for (int i = 0; i < static_cast<int>(state.stages.size()); ++i) {
        const auto& stage = state.stages[static_cast<size_t>(i)];
        if (equalsNoCase(stage.defPath.lexically_normal().generic_string(), wanted)) {
            return i;
        }
    }
    return -1;
}

void selectPreferredStage(AppState& state) {
    const CharacterSlot* character = sessionP1CharacterSlot(state);
    if (!character) {
        return;
    }
    const int stageIndex = findStageIndexByDefPath(state, character->preferredStagePath);
    if (stageIndex >= 0) {
        state.selectedStage = stageIndex;
    }
}

bool loadSelectedCharacterRuntime(SDL_Renderer* renderer, AppState& state) {
    const int p1Index = sessionP1CharacterIndex(state);
    const CharacterSlot* character = characterSlotAt(state, p1Index);
    if (!character) {
        return false;
    }

    std::vector<AnimationClip> clips;
    TextureSprite largePortrait;
    std::vector<StateDefinition> stateDefs;
    std::vector<HitDefinition> hitDefs;
    std::vector<CommandStateEntry> commandEntries;
    std::vector<CommandDefinition> commandDefinitions;
    std::vector<DecodedSoundSample> characterSamples;
    std::vector<std::string> victoryQuotes;
    CharacterConstants constants;

    try {
        const CharacterFiles files = resolveCharacterFiles(state.gameRoot, *character);
        constants = loadCharacterConstants(files);
        stateDefs = loadStateDefinitions(files, constants);
        hitDefs = loadHitDefinitions(files);
        commandDefinitions = loadCommandDefinitions(files);
        commandEntries = loadCommandStateEntries(files);
        victoryQuotes = loadVictoryQuotes(files);
        if (state.audio.stream) {
            characterSamples = loadDecodedSoundSamples(files.sound, state.audio.playbackSpec);
        }
        clips = loadCharacterClips(renderer, files);
        largePortrait = loadCharacterSprite(renderer, files, 9000, 1);

        destroyAnimationClips(state.characterClips);
        destroyTextureSprite(state.characterLargePortrait);
        state.runtimeEffects.clear();
        state.characterClips = std::move(clips);
        state.characterLargePortrait = largePortrait;
        largePortrait = {};
        state.stateDefs = std::move(stateDefs);
        state.hitDefs = std::move(hitDefs);
        state.commandEntries = std::move(commandEntries);
        state.commandDefinitions = std::move(commandDefinitions);
        state.victoryQuotes = std::move(victoryQuotes);
        state.characterConstants = constants;
        state.audio.activeVoices.clear();
        if (state.audio.stream) {
            SDL_ClearAudioStream(state.audio.stream);
        }
        state.audio.characterSamples = std::move(characterSamples);
        state.content = buildContentSummary(*character, files, currentStageSlot(state));
        state.loadedP1Character = p1Index;

        SDL_Log(
            "Character loaded: %s actions=%zu states=%zu hitdefs=%zu command-defs=%zu command-states=%zu sounds=%zu",
            character->displayName.c_str(),
            state.characterClips.size(),
            state.stateDefs.size(),
            state.hitDefs.size(),
            state.commandDefinitions.size(),
            state.commandEntries.size(),
            state.audio.characterSamples.size());
        return true;
    } catch (const std::exception& ex) {
        destroyAnimationClips(clips);
        destroyTextureSprite(largePortrait);
        SDL_Log("Selected character load failed %s: %s", character->displayName.c_str(), ex.what());
        return false;
    }
}

void unloadCharacterRuntime(AppState& state) {
    destroyAnimationClips(state.characterClips);
    destroyTextureSprite(state.characterLargePortrait);
    for (auto& element : state.stageBackground) {
        destroyTextureSprite(element.sprite);
    }
    state.stageBackground.clear();
    state.stateDefs.clear();
    state.hitDefs.clear();
    state.commandEntries.clear();
    state.commandDefinitions.clear();
    state.victoryQuotes.clear();
    state.characterConstants = CharacterConstants{};
    state.audio.activeVoices.clear();
    if (state.audio.stream) {
        SDL_ClearAudioStream(state.audio.stream);
    }
    state.audio.characterSamples.clear();
    state.runtimeEffects.clear();
    state.content = LoadedContentSummary{};
    state.loadedP1Character = -1;
    state.fightSessionPrepared = false;
    state.fightSessionLoadFailed = false;
}

void destroySystemScreenAssets(SystemScreenAssets& assets) {
    destroyTextureSprite(assets.titleLogo);
    destroyTextureSprite(assets.titleTop);
    destroyTextureSprite(assets.titleFloor);
    destroyTextureSprite(assets.titleShade);
    destroyTextureSprite(assets.selectBackdrop);
    destroyTextureSprite(assets.selectShadow);
    destroyTextureSprite(assets.selectTitleA);
    destroyTextureSprite(assets.selectTitleB);
    destroyTextureSprite(assets.selectTitleC);
    destroyTextureSprite(assets.selectCell);
    destroyTextureSprite(assets.selectP1Cursor);
    destroyTextureSprite(assets.selectP1Done);
}

void destroyStageBackground(std::vector<StageBackgroundElement>& background) {
    for (auto& element : background) {
        destroyTextureSprite(element.sprite);
    }
    background.clear();
}

void loadVisualAssets(SDL_Renderer* renderer, AppState& state) {
    try {
        destroySystemScreenAssets(state.systemScreens);
        state.systemScreens = loadSystemScreenAssets(renderer, state.gameRoot);
        state.fightFxClips = loadFightFxClips(renderer, state.gameRoot);
        for (auto& sprite : state.characterIconSprites) {
            destroyTextureSprite(sprite);
        }
        for (auto& sprite : state.characterFaceSprites) {
            destroyTextureSprite(sprite);
        }
        state.characterIconSprites.clear();
        state.characterFaceSprites.clear();
        state.characterIconSprites.reserve(state.characters.size());
        state.characterFaceSprites.reserve(state.characters.size());
        for (const auto& character : state.characters) {
            state.characterIconSprites.push_back(loadCharacterIconSprite(renderer, state.gameRoot, character));
            state.characterFaceSprites.push_back(loadCharacterFaceSprite(renderer, state.gameRoot, character));
        }
    } catch (const std::exception& ex) {
        SDL_Log("Visual asset load failed: %s", ex.what());
    }
}

void destroyTextureSprite(TextureSprite& sprite) {
    if (sprite.texture) {
        SDL_DestroyTexture(sprite.texture);
        sprite.texture = nullptr;
    }
}

void destroyAnimationClips(std::vector<AnimationClip>& clips) {
    for (auto& clip : clips) {
        for (auto& frame : clip.frames) {
            destroyTextureSprite(frame.sprite);
        }
    }
    clips.clear();
}

void destroyVisualAssets(AppState& state) {
    destroyAnimationClips(state.characterClips);
    destroyAnimationClips(state.fightFxClips);
    state.runtimeEffects.clear();
    destroySystemScreenAssets(state.systemScreens);
    destroyTextureSprite(state.characterLargePortrait);
    for (auto& sprite : state.characterIconSprites) {
        destroyTextureSprite(sprite);
    }
    state.characterIconSprites.clear();
    for (auto& sprite : state.characterFaceSprites) {
        destroyTextureSprite(sprite);
    }
    state.characterFaceSprites.clear();
    destroyStageBackground(state.stageBackground);
}

std::vector<DecodedSoundSample> loadDecodedSoundSamples(const std::filesystem::path& path, const SDL_AudioSpec& playbackSpec) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    const auto archive = loadSndArchive(path);
    std::vector<DecodedSoundSample> decodedSamples;
    decodedSamples.reserve(archive.samples.size());
    for (const auto& sample : archive.samples) {
        const auto* sampleData = archive.bytes.data() + sample.dataOffset;
        SDL_IOStream* io = SDL_IOFromConstMem(sampleData, sample.dataLength);
        if (!io) {
            SDL_Log("SND sample IO failed %s %d,%d: %s", path.string().c_str(), sample.group, sample.index, SDL_GetError());
            continue;
        }

        SDL_AudioSpec sourceSpec{};
        Uint8* wavData = nullptr;
        Uint32 wavLength = 0;
        if (!SDL_LoadWAV_IO(io, true, &sourceSpec, &wavData, &wavLength)) {
            SDL_Log("SND WAV decode failed %s %d,%d: %s", path.string().c_str(), sample.group, sample.index, SDL_GetError());
            continue;
        }

        Uint8* convertedData = nullptr;
        int convertedLength = 0;
        const bool converted = SDL_ConvertAudioSamples(
            &sourceSpec,
            wavData,
            static_cast<int>(wavLength),
            &playbackSpec,
            &convertedData,
            &convertedLength);
        SDL_free(wavData);
        if (!converted || !convertedData || convertedLength <= 0) {
            SDL_Log("SND sample convert failed %s %d,%d: %s", path.string().c_str(), sample.group, sample.index, SDL_GetError());
            continue;
        }

        DecodedSoundSample out;
        out.group = sample.group;
        out.index = sample.index;
        const int floatCount = convertedLength / static_cast<int>(sizeof(float));
        const auto* convertedFloats = reinterpret_cast<const float*>(convertedData);
        out.audio.assign(convertedFloats, convertedFloats + floatCount);
        SDL_free(convertedData);
        decodedSamples.push_back(std::move(out));
    }
    return decodedSamples;
}

bool initAudio(AppState& state) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("SDL audio init failed: %s", SDL_GetError());
        return false;
    }
    state.audio.subsystemInitialized = true;

    state.audio.stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &state.audio.playbackSpec,
        nullptr,
        nullptr);
    if (!state.audio.stream) {
        SDL_Log("SDL audio stream failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(state.audio.stream)) {
        SDL_Log("SDL audio resume failed: %s", SDL_GetError());
        return false;
    }

    try {
        state.audio.commonSamples = loadDecodedSoundSamples(state.gameRoot / "data" / "common.snd", state.audio.playbackSpec);
        state.audio.fightSamples = loadDecodedSoundSamples(state.gameRoot / "data" / "fight.snd", state.audio.playbackSpec);
        SDL_Log(
            "SND loaded: common %zu fight %zu",
            state.audio.commonSamples.size(),
            state.audio.fightSamples.size());
    } catch (const std::exception& ex) {
        SDL_Log("SND load failed: %s", ex.what());
    }
    return true;
}

void destroyAudioAssets(AppState& state) {
    if (state.audio.stream) {
        SDL_ClearAudioStream(state.audio.stream);
        SDL_DestroyAudioStream(state.audio.stream);
        state.audio.stream = nullptr;
    }
    state.audio.activeVoices.clear();
    state.audio.mixBuffer.clear();
    state.audio.characterSamples.clear();
    state.audio.commonSamples.clear();
    state.audio.fightSamples.clear();
    if (state.audio.subsystemInitialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        state.audio.subsystemInitialized = false;
    }
}

const DecodedSoundSample* findDecodedSoundSample(const std::vector<DecodedSoundSample>& samples, int group, int index) {
    for (const auto& sample : samples) {
        if (sample.group == group && sample.index == index) {
            return &sample;
        }
    }
    return nullptr;
}

const DecodedSoundSample* findPlaybackSound(const AppState& state, int group, int index, bool forceCommon = false) {
    if (forceCommon) {
        if (const auto* sample = findDecodedSoundSample(state.audio.commonSamples, group, index)) {
            return sample;
        }
        return findDecodedSoundSample(state.audio.fightSamples, group, index);
    }
    if (const auto* sample = findDecodedSoundSample(state.audio.characterSamples, group, index)) {
        return sample;
    }
    if (const auto* sample = findDecodedSoundSample(state.audio.commonSamples, group, index)) {
        return sample;
    }
    return findDecodedSoundSample(state.audio.fightSamples, group, index);
}

int soundSampleFrames(const DecodedSoundSample& sample) {
    return static_cast<int>(sample.audio.size() / 2);
}

void stopSoundChannel(AudioState& audio, int channel) {
    if (channel < 0) {
        return;
    }
    audio.activeVoices.erase(
        std::remove_if(audio.activeVoices.begin(), audio.activeVoices.end(), [channel](const ActiveSoundVoice& voice) {
            return voice.channel == channel;
        }),
        audio.activeVoices.end());
}

bool soundChannelActive(const AudioState& audio, int channel) {
    return channel >= 0 && std::any_of(audio.activeVoices.begin(), audio.activeVoices.end(), [channel](const ActiveSoundVoice& voice) {
        return voice.channel == channel;
    });
}

bool sameSoundTriggeredRecently(const AppState& state, int group, int index, int channel) {
    constexpr int kDuplicateSoundWindowFrames = 2;
    return std::any_of(state.audio.activeVoices.begin(), state.audio.activeVoices.end(), [&state, group, index, channel](const ActiveSoundVoice& voice) {
        return voice.group == group
            && voice.index == index
            && voice.channel == channel
            && state.frame - voice.startedFrame <= kDuplicateSoundWindowFrames;
    });
}

void capActiveSoundVoices(AudioState& audio) {
    constexpr size_t kMaxActiveSfxVoices = 32;
    while (audio.activeVoices.size() >= kMaxActiveSfxVoices) {
        auto oldest = std::max_element(audio.activeVoices.begin(), audio.activeVoices.end(), [](const ActiveSoundVoice& lhs, const ActiveSoundVoice& rhs) {
            return lhs.frameOffset < rhs.frameOffset;
        });
        if (oldest == audio.activeVoices.end()) {
            break;
        }
        audio.activeVoices.erase(oldest);
    }
}

void playSound(
    AppState& state,
    int group,
    int index,
    bool forceCommon = false,
    int channel = -1,
    bool lowPriority = false,
    float gain = 1.0f,
    bool loop = false) {
    if (!state.audio.stream || group < 0 || index < 0) {
        return;
    }

    const DecodedSoundSample* sample = findPlaybackSound(state, group, index, forceCommon);
    if (!sample || sample->audio.empty()) {
        SDL_Log("SND sample not found: %d,%d", group, index);
        return;
    }

    if (sameSoundTriggeredRecently(state, group, index, channel)) {
        return;
    }

    if (channel >= 0) {
        if (lowPriority && soundChannelActive(state.audio, channel)) {
            return;
        }
        stopSoundChannel(state.audio, channel);
    }

    capActiveSoundVoices(state.audio);
    state.audio.activeVoices.push_back(ActiveSoundVoice{
        sample,
        group,
        index,
        channel,
        0,
        state.frame,
        gain,
        loop,
    });
}

void mixActiveSoundVoices(AppState& state, int framesToWrite) {
    if (!state.audio.stream || framesToWrite <= 0) {
        return;
    }

    constexpr int kChannels = 2;
    state.audio.mixBuffer.assign(static_cast<size_t>(framesToWrite * kChannels), 0.0f);

    for (auto voiceIt = state.audio.activeVoices.begin(); voiceIt != state.audio.activeVoices.end();) {
        ActiveSoundVoice& voice = *voiceIt;
        if (!voice.sample || voice.sample->audio.empty()) {
            voiceIt = state.audio.activeVoices.erase(voiceIt);
            continue;
        }

        const int sampleFrames = soundSampleFrames(*voice.sample);
        if (sampleFrames <= 0) {
            voiceIt = state.audio.activeVoices.erase(voiceIt);
            continue;
        }

        bool finished = false;
        for (int frame = 0; frame < framesToWrite; ++frame) {
            if (voice.frameOffset >= sampleFrames) {
                if (voice.loop) {
                    voice.frameOffset = 0;
                } else {
                    finished = true;
                    break;
                }
            }

            const size_t source = static_cast<size_t>(voice.frameOffset * kChannels);
            const size_t dest = static_cast<size_t>(frame * kChannels);
            state.audio.mixBuffer[dest] += voice.sample->audio[source] * voice.gain;
            state.audio.mixBuffer[dest + 1] += voice.sample->audio[source + 1] * voice.gain;
            ++voice.frameOffset;
        }

        if (finished) {
            voiceIt = state.audio.activeVoices.erase(voiceIt);
        } else {
            ++voiceIt;
        }
    }

    for (float& sample : state.audio.mixBuffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    const int byteCount = static_cast<int>(state.audio.mixBuffer.size() * sizeof(float));
    if (!SDL_PutAudioStreamData(state.audio.stream, state.audio.mixBuffer.data(), byteCount)) {
        SDL_Log("SND mixer queue failed: %s", SDL_GetError());
    }
}

void updateAudioMixer(AppState& state) {
    if (!state.audio.stream || state.audio.activeVoices.empty()) {
        return;
    }

    constexpr int kChannels = 2;
    constexpr int kBytesPerFrame = static_cast<int>(sizeof(float)) * kChannels;
    constexpr int kTargetQueuedFrames = 2048;
    constexpr int kMixChunkFrames = 512;

    int queuedBytes = SDL_GetAudioStreamQueued(state.audio.stream);
    if (queuedBytes < 0) {
        queuedBytes = 0;
    }
    int queuedFrames = queuedBytes / kBytesPerFrame;

    while (queuedFrames < kTargetQueuedFrames && !state.audio.activeVoices.empty()) {
        const int framesToWrite = std::min(kMixChunkFrames, kTargetQueuedFrames - queuedFrames);
        mixActiveSoundVoices(state, framesToWrite);
        queuedFrames += framesToWrite;
    }
}

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

bool isPlaystationGamepad(SDL_GamepadType type) {
    return type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4 || type == SDL_GAMEPAD_TYPE_PS5;
}

bool isXboxGamepad(SDL_GamepadType type) {
    return type == SDL_GAMEPAD_TYPE_XBOX360 || type == SDL_GAMEPAD_TYPE_XBOXONE;
}

std::string gamepadFamilyName(SDL_GamepadType type) {
    if (isPlaystationGamepad(type)) {
        return "PlayStation";
    }
    if (isXboxGamepad(type)) {
        return "Xbox";
    }
    return "Standard";
}

std::string compactSettingText(const std::string& value, size_t maxChars) {
    if (value.size() <= maxChars) {
        return value;
    }
    if (maxChars <= 1) {
        return value.substr(0, maxChars);
    }
    return value.substr(0, maxChars - 1) + "~";
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

void cycleGamepadAssignment(MainSettings& settings, int playerIndex, int deviceCount, int direction) {
    std::vector<int> values;
    values.push_back(0);
    values.push_back(-1);
    for (int i = 1; i <= deviceCount; ++i) {
        values.push_back(i);
    }

    int& assignment = playerIndex == 0 ? settings.p1GamepadAssignment : settings.p2GamepadAssignment;
    auto current = std::find(values.begin(), values.end(), assignment);
    int index = current == values.end() ? 0 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    assignment = values[static_cast<size_t>(index)];
}

std::string gamepadPromptStyleText(GamepadPromptStyle style) {
    switch (style) {
    case GamepadPromptStyle::Xbox:
        return "XBOX";
    case GamepadPromptStyle::Playstation:
        return "PLAYSTATION";
    case GamepadPromptStyle::Auto:
    default:
        return "AUTO";
    }
}

void cycleGamepadPromptStyle(MainSettings& settings, int direction) {
    static constexpr std::array<GamepadPromptStyle, 3> values{
        GamepadPromptStyle::Auto,
        GamepadPromptStyle::Xbox,
        GamepadPromptStyle::Playstation,
    };
    auto current = std::find(values.begin(), values.end(), settings.gamepadPromptStyle);
    int index = current == values.end() ? 0 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    settings.gamepadPromptStyle = values[static_cast<size_t>(index)];
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

int matchTimerTicksFromSettings(const AppState& state) {
    return std::max(1, state.mainSettings.matchTimerSeconds) * 60;
}

std::string matchTimerSettingText(const MainSettings& settings) {
    if (settings.matchTimerSeconds <= 0) {
        return "OFF";
    }
    return std::to_string(settings.matchTimerSeconds);
}

void cycleMatchTimerSetting(MainSettings& settings, int direction) {
    static constexpr std::array<int, 6> values{ 30, 60, 90, 99, 120, 180 };
    auto current = std::find(values.begin(), values.end(), settings.matchTimerSeconds);
    int index = current == values.end() ? 3 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    settings.matchTimerSeconds = values[static_cast<size_t>(index)];
}

std::string canvasSizeSettingText(const MainSettings& settings) {
    switch (settings.canvasWidth) {
    case kClassicLogicalWidth:
        return "320x240 CLASSIC";
    case kExtraWideLogicalWidth:
        return "480x240 EXTRA";
    case kDefaultLogicalWidth:
    default:
        return "426x240 WIDE";
    }
}

void cycleCanvasSizeSetting(MainSettings& settings, int direction) {
    static constexpr std::array<int, 3> values{
        kClassicLogicalWidth,
        kDefaultLogicalWidth,
        kExtraWideLogicalWidth,
    };
    auto current = std::find(values.begin(), values.end(), settings.canvasWidth);
    int index = current == values.end() ? 1 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    settings.canvasWidth = values[static_cast<size_t>(index)];
}

std::string uiScaleSettingText(const MainSettings& settings) {
    return std::to_string(settings.uiScalePercent) + "%";
}

void cycleUiScaleSetting(MainSettings& settings, int direction) {
    static constexpr std::array<int, 5> values{ 60, 70, 80, 90, 100 };
    auto current = std::find(values.begin(), values.end(), settings.uiScalePercent);
    int index = current == values.end() ? 2 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    settings.uiScalePercent = values[static_cast<size_t>(index)];
}

void drawTitleBackground(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 0, 0, 0);
    SDL_RenderClear(renderer);

    const int width = logicalWidth(state);
    const float widthF = static_cast<float>(width);
    const float centerX = screenCenterX(state);
    const float motifX = motifOriginX(state);
    const auto& system = state.systemScreens;
    if (!system.titleTop.texture) {
        setColor(renderer, 14, 16, 20);
        fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
        return;
    }

    const float topScroll = static_cast<float>(state.frame % std::max(1, system.titleTop.width));
    const float topX = centerX - static_cast<float>(system.titleTop.axisX) - topScroll;
    drawTiledSpriteCoverX(renderer, system.titleTop, topX, 10, width, 2);

    if (system.titleFloor.texture) {
        drawParallaxFloorSprite(renderer, system.titleFloor, motifX, 145, 400.0f, 1200.0f, width, state.frame);
    }
    if (system.titleShade.texture) {
        drawSpriteTopLeftWithBlend(renderer, system.titleShade, motifX - 160.0f, 145.0f, SDL_BLENDMODE_MUL, 180);
    }

    setColor(renderer, 0, 0, 0);
    fillRect(renderer, 0, 0, widthF, 12);
    fillRect(renderer, 0, 220, widthF, 20);
}

void drawSelectBackground(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 0, 0, 0);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const auto& system = state.systemScreens;
    if (system.selectBackdrop.texture) {
        const float backdropOffset = -static_cast<float>((state.frame / 3) % std::max(1, system.selectBackdrop.width));
        drawTiledSprite(renderer, system.selectBackdrop, backdropOffset, 0, 3, 2);
    } else {
        setColor(renderer, 18, 22, 30);
        fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    }

    if (system.selectShadow.texture) {
        drawSpriteTopLeft(renderer, system.selectShadow, 0, 128);
    }

    if (system.selectTitleA.texture) {
        const float stripOffset = -static_cast<float>(state.frame % std::max(1, system.selectTitleA.width));
        drawTiledSprite(renderer, system.selectTitleA, stripOffset, 0, 4, 1);
    }
    if (system.selectTitleB.texture) {
        const float stripOffset = -static_cast<float>((state.frame / 2) % std::max(1, system.selectTitleB.width));
        drawTiledSprite(renderer, system.selectTitleB, stripOffset, 14, 4, 1);
    }
    if (system.selectTitleC.texture) {
        const float stripOffset = -static_cast<float>((state.frame / 2) % std::max(1, system.selectTitleC.width));
        drawTiledSprite(renderer, system.selectTitleC, stripOffset, 224, 2, 1);
    }

    setColor(renderer, 0, 0, 0, 150);
    fillRect(renderer, 0, 160, widthF, 60);
}

#include "MainMenuOverlay.h"

bool moveCharacterSelectCursor(AppState& state, int deltaColumn, int deltaRow) {
    const int characterCount = static_cast<int>(state.characters.size());
    if (characterCount <= 0) {
        return false;
    }

    const int current = std::clamp(state.selectedCharacter, 0, characterCount - 1);
    const int pageFirst = (current / kCharacterSelectPageSize) * kCharacterSelectPageSize;
    const int local = current - pageFirst;
    const int column = local % kCharacterSelectColumns;
    const int row = local / kCharacterSelectColumns;
    const int targetColumn = column + deltaColumn;
    const int targetRow = row + deltaRow;
    if (targetColumn < 0
        || targetColumn >= kCharacterSelectColumns
        || targetRow < 0
        || targetRow >= kCharacterSelectRows) {
        return false;
    }

    const int target = pageFirst + targetRow * kCharacterSelectColumns + targetColumn;
    if (target < 0 || target >= characterCount) {
        return false;
    }

    state.selectedCharacter = target;
    return true;
}

std::string_view opponentSlotLabel(PendingMode mode) {
    return opponentTypeLabel(defaultOpponentTypeForMode(mode));
}

std::string_view opponentSlotLabel(const AppState& state) {
    return opponentTypeLabel(activeOpponentType(state));
}

void drawFixedOpponentSlot(
    SDL_Renderer* renderer,
    float x,
    float y,
    float width,
    float height,
    std::string_view label) {
    setColor(renderer, 12, 14, 18);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, 54, 62, 76);
    drawRect(renderer, x, y, width, height);

    setColor(renderer, 74, 82, 98);
    fillRect(renderer, x + width * 0.38f, y + height * 0.18f, width * 0.24f, height * 0.22f);
    fillRect(renderer, x + width * 0.28f, y + height * 0.44f, width * 0.44f, height * 0.38f);
    setColor(renderer, 34, 38, 46);
    fillRect(renderer, x + width * 0.34f, y + height * 0.50f, width * 0.32f, height * 0.30f);

    setColor(renderer, 150, 160, 176);
    debugTextCentered(renderer, x + width * 0.5f, y + height * 0.86f, std::string(label));
}

std::string characterPreferredStageName(const AppState& state, int characterIndex) {
    if (characterIndex < 0 || characterIndex >= static_cast<int>(state.characters.size())) {
        return "stage select";
    }
    const auto& character = state.characters[static_cast<size_t>(characterIndex)];
    const int preferredStage = findStageIndexByDefPath(state, character.preferredStagePath);
    if (preferredStage >= 0 && preferredStage < static_cast<int>(state.stages.size())) {
        return state.stages[static_cast<size_t>(preferredStage)].displayName;
    }
    return "stage select";
}

#include "OptionsMenuOverlay.h"

void drawCharacterSelect(SDL_Renderer* renderer, const AppState& state) {
    drawSelectBackground(renderer, state);

    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    setColor(renderer, 235, 240, 248);
    debugTextCentered(renderer, centerX, 8, std::string(pendingModeTitle(state.pendingMode)));
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, 20, "SELECT YOUR FIGHTER");

    if (state.characters.empty()) {
        setColor(renderer, 235, 110, 100);
        debugTextCentered(renderer, centerX, 96, "NO CHARACTERS IN SELECT.DEF");
        setColor(renderer, 156, 166, 180);
        debugTextCentered(renderer, centerX, 226, "ESC mode select");
        SDL_RenderPresent(renderer);
        return;
    }

    const int selected = std::clamp(state.selectedCharacter, 0, static_cast<int>(state.characters.size()) - 1);
    const auto& character = state.characters[static_cast<size_t>(selected)];
    const TextureSprite* face = spriteAt(state.characterFaceSprites, selected);
    if (face) {
        const float portraitScale = std::min({ 1.0f, 120.0f / static_cast<float>(face->width), 140.0f / static_cast<float>(face->height) });
        drawSpriteTopLeft(renderer, *face, 18, 30, portraitScale);
    } else {
        setColor(renderer, 34, 38, 46, 210);
        fillRect(renderer, 18, 30, 120, 140);
        setColor(renderer, 94, 108, 130);
        drawRect(renderer, 18, 30, 120, 140);
    }
    const float opponentPortraitX = widthF - 138.0f;
    drawFixedOpponentSlot(renderer, opponentPortraitX, 30, 120, 140, opponentSlotLabel(state.pendingMode));

    const std::string p1Name = compactSettingText(character.displayName, 15);
    const std::string p2Name(opponentSlotLabel(state.pendingMode));
    setColor(renderer, 235, 240, 248);
    debugText(renderer, 10, 154, p1Name);
    debugText(renderer, widthF - 10.0f - static_cast<float>(p2Name.size() * 8), 154, p2Name);

    static constexpr float kCellSize = 27.0f;
    static constexpr float kCellSpacing = 2.0f;
    static constexpr float kGridY = 170.0f;
    constexpr float kGridWidth = static_cast<float>(kCharacterSelectColumns) * kCellSize
        + static_cast<float>(kCharacterSelectColumns - 1) * kCellSpacing;
    const float gridX = centerX - kGridWidth * 0.5f;

    const int page = selected / kCharacterSelectPageSize;
    const int firstIndex = page * kCharacterSelectPageSize;
    const int lastIndex = std::min(firstIndex + kCharacterSelectPageSize, static_cast<int>(state.characters.size()));

    for (int i = firstIndex; i < lastIndex; ++i) {
        const int local = i - firstIndex;
        const int column = local % kCharacterSelectColumns;
        const int row = local / kCharacterSelectColumns;
        const float cellX = gridX + static_cast<float>(column) * (kCellSize + kCellSpacing);
        const float cellY = kGridY + static_cast<float>(row) * (kCellSize + kCellSpacing);

        if (state.systemScreens.selectCell.texture) {
            drawSpriteTopLeft(renderer, state.systemScreens.selectCell, cellX, cellY);
        } else {
            setColor(renderer, 22, 26, 32, 230);
            fillRect(renderer, cellX, cellY, kCellSize, kCellSize);
            setColor(renderer, 92, 110, 136);
            drawRect(renderer, cellX, cellY, kCellSize, kCellSize);
        }

        if (const TextureSprite* icon = spriteAt(state.characterIconSprites, i)) {
            const float scale = std::min({ 1.0f, 25.0f / static_cast<float>(icon->width), 25.0f / static_cast<float>(icon->height) });
            const float iconX = cellX + (kCellSize - static_cast<float>(icon->width) * scale) * 0.5f;
            const float iconY = cellY + (kCellSize - static_cast<float>(icon->height) * scale) * 0.5f;
            drawSpriteTopLeft(renderer, *icon, iconX, iconY, scale);
        }
    }

    const int cursorLocal = selected - firstIndex;
    const float cursorX = gridX + static_cast<float>(cursorLocal % kCharacterSelectColumns) * (kCellSize + kCellSpacing) - 1.0f;
    const float cursorY = kGridY + static_cast<float>(cursorLocal / kCharacterSelectColumns) * (kCellSize + kCellSpacing) - 1.0f;
    if (state.systemScreens.selectP1Cursor.texture) {
        drawSpriteTopLeft(renderer, state.systemScreens.selectP1Cursor, cursorX, cursorY);
    } else {
        const int pulse = 180 + ((state.frame / 6) % 40);
        setColor(renderer, 240, static_cast<Uint8>(pulse), 70);
        drawRect(renderer, cursorX, cursorY, 29, 29);
    }

    setColor(renderer, 238, 210, 94);
    debugTextCentered(renderer, centerX, 208, "P1");
    setColor(renderer, 210, 218, 230);
    debugTextCentered(renderer, centerX, 220, "STAGE: " + compactSettingText(characterPreferredStageName(state, selected), 22));

    setColor(renderer, 156, 166, 180);
    debugTextCentered(renderer, centerX, 232, "ARROWS choose  ENTER stage  ESC back");
    SDL_RenderPresent(renderer);
}

std::string selectedCharacterName(const AppState& state) {
    if (const CharacterSlot* character = sessionP1CharacterSlot(state)) {
        return character->displayName;
    }
    return "Unknown";
}

std::string selectedStageName(const AppState& state) {
    if (state.stages.empty()) {
        return "Unknown";
    }
    return state.stages[static_cast<size_t>(state.selectedStage)].displayName;
}

const StageSlot* selectedStageSlot(const AppState& state) {
    if (state.stages.empty()
        || state.selectedStage < 0
        || state.selectedStage >= static_cast<int>(state.stages.size())) {
        return nullptr;
    }
    return &state.stages[static_cast<size_t>(state.selectedStage)];
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

std::string_view dummyGuardModeStatus(DummyGuardMode mode) {
    switch (mode) {
    case DummyGuardMode::Off:
        return "OFF";
    case DummyGuardMode::Stand:
        return "STAND";
    case DummyGuardMode::Crouch:
        return "CROUCH";
    case DummyGuardMode::Auto:
        return "AUTO";
    default:
        return "OFF";
    }
}

void cycleDummyGuardMode(TrainingOptions& settings) {
    switch (settings.dummyGuardMode) {
    case DummyGuardMode::Off:
        settings.dummyGuardMode = DummyGuardMode::Stand;
        break;
    case DummyGuardMode::Stand:
        settings.dummyGuardMode = DummyGuardMode::Crouch;
        break;
    case DummyGuardMode::Crouch:
        settings.dummyGuardMode = DummyGuardMode::Auto;
        break;
    case DummyGuardMode::Auto:
        settings.dummyGuardMode = DummyGuardMode::Off;
        break;
    }
}

std::string_view trainingPowerModeStatus(TrainingPowerMode mode) {
    switch (mode) {
    case TrainingPowerMode::Normal:
        return "NORMAL";
    case TrainingPowerMode::Max:
        return "MAX";
    default:
        return "NORMAL";
    }
}

void cycleTrainingPowerMode(TrainingOptions& settings) {
    settings.powerMode = settings.powerMode == TrainingPowerMode::Normal
        ? TrainingPowerMode::Max
        : TrainingPowerMode::Normal;
}

std::string_view trainingMoveCategoryStatus(TrainingMoveCategory category) {
    switch (category) {
    case TrainingMoveCategory::All:
        return "ALL";
    case TrainingMoveCategory::Normals:
        return "NORMAL";
    case TrainingMoveCategory::Specials:
        return "SPECIAL";
    case TrainingMoveCategory::Supers:
        return "SUPER";
    default:
        return "ALL";
    }
}

void cycleTrainingMoveCategory(TrainingOptions& settings) {
    switch (settings.moveCategory) {
    case TrainingMoveCategory::All:
        settings.moveCategory = TrainingMoveCategory::Normals;
        break;
    case TrainingMoveCategory::Normals:
        settings.moveCategory = TrainingMoveCategory::Specials;
        break;
    case TrainingMoveCategory::Specials:
        settings.moveCategory = TrainingMoveCategory::Supers;
        break;
    case TrainingMoveCategory::Supers:
        settings.moveCategory = TrainingMoveCategory::All;
        break;
    }
    settings.selectedMoveListEntry = 0;
    settings.moveListScroll = 0;
}

std::string_view trainingOptionLabel(int option) {
    static constexpr std::array<std::string_view, kTrainingOptionCount> labels{
        "HITBOXES",
        "ORIGINS",
        "FLOOR LINE",
        "READOUT",
        "HIT FLASH",
        "HIT SPARKS",
        "HIT LOG",
        "HIT SOUND",
        "DUMMY INV",
        "AUTO LIFE",
        "DUMMY FREEZE",
        "DUMMY GUARD",
        "GUARD DMG",
        "P2 CONTROL",
        "COMMAND HUD",
        "INPUT HUD",
        "POWER",
        "MOVE TYPE",
        "MOVE LIST",
        "RESET POS",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kTrainingOptionCount - 1))];
}

std::string trainingOptionStatus(const TrainingOptions& settings, int option) {
    switch (option) {
    case 0:
        return settings.showHitboxes ? "ON" : "OFF";
    case 1:
        return settings.showOrigins ? "ON" : "OFF";
    case 2:
        return settings.showFloorLine ? "ON" : "OFF";
    case 3:
        return settings.showDebugReadout ? "ON" : "OFF";
    case 4:
        return settings.showHitFlash ? "ON" : "OFF";
    case 5:
        return settings.showHitSparks ? "ON" : "OFF";
    case 6:
        return settings.showHitLog ? "ON" : "OFF";
    case 7:
        return settings.playHitSounds ? "ON" : "OFF";
    case 8:
        return settings.dummyInvincible ? "ON" : "OFF";
    case 9:
        return settings.dummyAutoLife ? "ON" : "OFF";
    case 10:
        return settings.dummyFrozen ? "ON" : "OFF";
    case 11:
        return std::string(dummyGuardModeStatus(settings.dummyGuardMode));
    case 12:
        return settings.guardDamage ? "ON" : "OFF";
    case kTrainingP2ControlOption:
        return settings.p2Controlled ? "ON" : "OFF";
    case kTrainingCommandHudOption:
        return settings.showCommandHud ? "ON" : "OFF";
    case kTrainingInputHudOption:
        return settings.showInputHud ? "ON" : "OFF";
    case kTrainingPowerOption:
        return std::string(trainingPowerModeStatus(settings.powerMode));
    case kTrainingMoveTypeOption:
        return std::string(trainingMoveCategoryStatus(settings.moveCategory));
    case kTrainingMoveListOption:
        return "OPEN";
    case kTrainingResetOption:
        return "RUN";
    default:
        return "";
    }
}

void toggleTrainingOption(TrainingOptions& settings, int option) {
    switch (option) {
    case 0:
        settings.showHitboxes = !settings.showHitboxes;
        break;
    case 1:
        settings.showOrigins = !settings.showOrigins;
        break;
    case 2:
        settings.showFloorLine = !settings.showFloorLine;
        break;
    case 3:
        settings.showDebugReadout = !settings.showDebugReadout;
        break;
    case 4:
        settings.showHitFlash = !settings.showHitFlash;
        break;
    case 5:
        settings.showHitSparks = !settings.showHitSparks;
        break;
    case 6:
        settings.showHitLog = !settings.showHitLog;
        break;
    case 7:
        settings.playHitSounds = !settings.playHitSounds;
        break;
    case 8:
        settings.dummyInvincible = !settings.dummyInvincible;
        break;
    case 9:
        settings.dummyAutoLife = !settings.dummyAutoLife;
        break;
    case 10:
        settings.dummyFrozen = !settings.dummyFrozen;
        break;
    case 11:
        cycleDummyGuardMode(settings);
        break;
    case 12:
        settings.guardDamage = !settings.guardDamage;
        break;
    case kTrainingP2ControlOption:
        settings.p2Controlled = !settings.p2Controlled;
        break;
    case kTrainingCommandHudOption:
        settings.showCommandHud = !settings.showCommandHud;
        break;
    case kTrainingInputHudOption:
        settings.showInputHud = !settings.showInputHud;
        break;
    case kTrainingPowerOption:
        cycleTrainingPowerMode(settings);
        break;
    case kTrainingMoveTypeOption:
        cycleTrainingMoveCategory(settings);
        break;
    case kTrainingMoveListOption:
        settings.moveListOpen = true;
        settings.selectedMoveListEntry = 0;
        settings.moveListScroll = 0;
        break;
    default:
        break;
    }
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

void enterRoundInitialState(const AppState& state, FighterState& fighter) {
    enterState(state, fighter, findStateDefinition(state, 5900) ? 5900 : 0);
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

void enterGroundGetHitState(const AppState& state, FighterState& target, const HitDefinition& hitDef, int attackerFacing) {
    const bool wasAirborne = !target.onGround || target.stateType == 'A';
    const bool wasCrouching = fighterIsCrouchingForHit(target);
    const bool wasLyingDown = fighterIsLyingDownForHit(target);
    const bool useAirVelocity = wasAirborne && hitDef.hasAirVelocity;
    const float hitVelocityX = useAirVelocity ? hitDef.airVelocityX : hitDef.groundVelocityX;
    const float hitVelocityY = useAirVelocity ? hitDef.airVelocityY : hitDef.groundVelocityY;
    const bool fallHit = hitDefCausesFall(hitDef, target);
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
    target.hitPauseTicks = std::max(1, hitDef.pausetimeP2);
    const int hitTime = wasAirborne && hitDef.airHitTime > 0 ? hitDef.airHitTime : hitDef.groundHitTime;
    target.hitStunTicks = std::max(hitTime, target.hitPauseTicks);
    target.hitSlideTicks = wasAirborne ? 0 : std::max(0, hitDef.groundSlideTime);
    setGetHitVarsFromHitDef(
        target,
        hitDef,
        wasAirborne,
        fallHit,
        -hitVelocityX * static_cast<float>(attackerFacing),
        hitVelocityY);
    target.hitRecoverAnim = hitRecoverActionForFighter(state, hitDef, wasCrouching);
    target.hitFall = fallHit || wasLyingDown;
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
    target.hitPauseTicks = std::max(0, hitDef.pausetimeP2);
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
    target.hitPauseTicks = std::max(1, hitDef.pausetimeP2);
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

void clearFighterVariables(FighterState& fighter) {
    fighter.vars.fill(0);
    fighter.sysVars.fill(0);
    fighter.fvars.fill(0.0f);
    fighter.sysFvars.fill(0.0f);
}

void clearGlobalPause(AppState& state);
void clearEnvShake(AppState& state);
void clearPaletteRuntime(AppState& state);

void applyTrainingPowerMode(AppState& state) {
    if (state.pendingMode != PendingMode::Training || state.trainingOptions.powerMode != TrainingPowerMode::Max) {
        return;
    }
    const int maxPower = std::max(0, state.characterConstants.maxPower);
    for (auto& fighter : state.fighters) {
        fighter.power = maxPower;
    }
}

void resetTrainingPositions(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state) ? *selectedStageSlot(state) : fallbackStage;

    state.fighters[0].x = stage.p1startx;
    state.fighters[0].y = stage.p1starty;
    state.fighters[1].x = stage.p2startx;
    state.fighters[1].y = stage.p2starty;
    state.fighters[0].inputHistory.clear();
    state.fighters[1].inputHistory.clear();
    clearFighterHitRuntime(state.fighters[0]);
    clearFighterHitRuntime(state.fighters[1]);
    clearFighterVariables(state.fighters[0]);
    clearFighterVariables(state.fighters[1]);
    state.fighters[0].power = 0;
    state.fighters[1].power = 0;
    applyTrainingPowerMode(state);
    state.fighters[0].hitCount = 0;
    state.fighters[1].hitCount = 0;
    state.fighters[0].defenceMultiplier = 1.0f;
    state.fighters[1].defenceMultiplier = 1.0f;
    state.fighters[0].attackMultiplier = 1.0f;
    state.fighters[1].attackMultiplier = 1.0f;
    state.fighters[0].drawAngle = 0.0f;
    state.fighters[1].drawAngle = 0.0f;
    state.fighters[0].angleDrawActive = false;
    state.fighters[1].angleDrawActive = false;
    state.fighters[0].displayOffsetX = 0.0f;
    state.fighters[1].displayOffsetX = 0.0f;
    state.fighters[0].displayOffsetY = 0.0f;
    state.fighters[1].displayOffsetY = 0.0f;
    state.fighters[0].debugClipboard.clear();
    state.fighters[1].debugClipboard.clear();
    state.fighters[0].victoryQuote = -1;
    state.fighters[1].victoryQuote = -1;
    state.fighters[0].paletteRemaps.clear();
    state.fighters[1].paletteRemaps.clear();
    state.fighters[0].paletteNo = 1;
    state.fighters[1].paletteNo = 1;
    state.fighters[0].attackDistanceOverride = -1;
    state.fighters[1].attackDistanceOverride = -1;
    state.fighters[0].facing = state.fighters[0].x <= state.fighters[1].x ? 1 : -1;
    state.fighters[1].facing = -state.fighters[0].facing;
    state.cameraX = stage.cameraStartx;
    state.cameraY = stage.cameraStarty;
    state.runtimeEffects.clear();
    state.helpers.clear();
    state.projectiles.clear();
    clearGlobalPause(state);
    clearEnvShake(state);
    clearPaletteRuntime(state);
    clearComboCounters(state);
    enterRoundInitialState(state, state.fighters[0]);
    enterRoundInitialState(state, state.fighters[1]);
    state.lastHitText = "Training positions reset";
    state.lastHitTextTicks = 90;
}

void resetFightRound(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state) ? *selectedStageSlot(state) : fallbackStage;

    state.fighters[0] = {};
    state.fighters[1] = {};
    state.helpers.clear();
    state.projectiles.clear();
    state.fighters[0].x = stage.p1startx;
    state.fighters[0].y = stage.p1starty;
    state.fighters[1].x = stage.p2startx;
    state.fighters[1].y = stage.p2starty;
    state.fighters[0].facing = state.fighters[0].x <= state.fighters[1].x ? 1 : -1;
    state.fighters[1].facing = -state.fighters[0].facing;
    state.fighters[0].onGround = true;
    state.fighters[1].onGround = true;
    state.fighters[0].life = 1000;
    state.fighters[1].life = 1000;
    state.fighters[0].hitPauseTicks = 0;
    state.fighters[1].hitPauseTicks = 0;
    state.fighters[0].hitStunTicks = 0;
    state.fighters[1].hitStunTicks = 0;
    state.fighters[0].hitSlideTicks = 0;
    state.fighters[1].hitSlideTicks = 0;
    state.fighters[0].hitVelocityX = 0.0f;
    state.fighters[1].hitVelocityX = 0.0f;
    state.fighters[0].hitVelocityY = 0.0f;
    state.fighters[1].hitVelocityY = 0.0f;
    state.fighters[0].getHitAnimType = 0;
    state.fighters[1].getHitAnimType = 0;
    state.fighters[0].getHitGroundType = 0;
    state.fighters[1].getHitGroundType = 0;
    state.fighters[0].getHitAirType = 0;
    state.fighters[1].getHitAirType = 0;
    state.fighters[0].getHitSlideTime = 0;
    state.fighters[1].getHitSlideTime = 0;
    state.fighters[0].getHitHitTime = 0;
    state.fighters[1].getHitHitTime = 0;
    state.fighters[0].getHitCtrlTime = 0;
    state.fighters[1].getHitCtrlTime = 0;
    state.fighters[0].getHitHitCount = 0;
    state.fighters[1].getHitHitCount = 0;
    state.fighters[0].hitRecoverAnim = 5005;
    state.fighters[1].hitRecoverAnim = 5005;
    state.fighters[0].appliedHitDefIds.clear();
    state.fighters[1].appliedHitDefIds.clear();
    state.fighters[0].firedStateSoundControllerIds.clear();
    state.fighters[1].firedStateSoundControllerIds.clear();
    state.fighters[0].firedStateCtrlControllerIds.clear();
    state.fighters[1].firedStateCtrlControllerIds.clear();
    state.fighters[0].firedStatePosAddControllerIds.clear();
    state.fighters[1].firedStatePosAddControllerIds.clear();
    state.fighters[0].firedStateChangeAnimControllerIds.clear();
    state.fighters[1].firedStateChangeAnimControllerIds.clear();
    state.fighters[0].firedStateRuntimeControllerIds.clear();
    state.fighters[1].firedStateRuntimeControllerIds.clear();
    state.fighters[0].moveContact = false;
    state.fighters[1].moveContact = false;
    state.fighters[0].moveHit = false;
    state.fighters[1].moveHit = false;
    state.fighters[0].moveGuarded = false;
    state.fighters[1].moveGuarded = false;
    state.lastHitText.clear();
    state.lastHitTextTicks = 0;
    clearComboCounters(state);
    state.runtimeEffects.clear();
    clearGlobalPause(state);
    clearEnvShake(state);
    clearPaletteRuntime(state);
    state.audio.activeVoices.clear();
    if (state.audio.stream) {
        SDL_ClearAudioStream(state.audio.stream);
    }
    state.trainingOptions.menuOpen = false;
    state.trainingOptions.moveListOpen = false;
    state.singleFightPauseOpen = false;
    state.selectedSingleFightPauseOption = 0;
    state.selectedMatchResultOption = 0;
    state.matchPhase = isMatchMode(state) ? MatchPhase::RoundStart : MatchPhase::Fight;
    state.roundEndReason = RoundEndReason::None;
    state.matchTimerTicks = matchTimerTicksFromSettings(state);
    state.matchPhaseTicks = 0;
    state.roundWinner = 0;
    state.roundScoreApplied = false;
    state.roundPoseApplied = false;
    state.cameraX = stage.cameraStartx;
    state.cameraY = stage.cameraStarty;
    enterRoundInitialState(state, state.fighters[0]);
    enterRoundInitialState(state, state.fighters[1]);
}

void resetFightState(AppState& state) {
    state.roundWins = { 0, 0 };
    state.currentRound = 1;
    state.matchComplete = false;
    state.roundPoseApplied = false;
    resetFightRound(state);
}

bool prepareFightSession(SDL_Renderer* renderer, AppState& state) {
    state.fightSessionPrepared = false;
    state.fightSessionLoadFailed = false;

    if (const StageSlot* stage = currentStageSlot(state)) {
        state.content.stageName = stage->displayName;
    }

    if (!loadSelectedCharacterRuntime(renderer, state)) {
        state.fightSessionLoadFailed = true;
        return false;
    }

    std::vector<StageBackgroundElement> selectedBackground;
    if (const StageSlot* stage = currentStageSlot(state)) {
        try {
            selectedBackground = loadStageBackground(renderer, *stage);
        } catch (const std::exception& ex) {
            SDL_Log("Selected stage background load failed %s: %s", stage->displayName.c_str(), ex.what());
        }
    }
    destroyStageBackground(state.stageBackground);
    state.stageBackground = std::move(selectedBackground);

    resetFightState(state);
    state.screenFrame = 0;
    state.fightSessionPrepared = true;
    return true;
}

void beginFight(AppState& state) {
    if (!state.fightSessionPrepared) {
        return;
    }
    state.screen = Screen::FightView;
    state.screenFrame = 0;
}

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

int stateControllerFireKey(int controllerId, const FighterState& fighter, int animElem) {
    return controllerId * 100000 + std::max(0, fighter.stateTime) * 100 + std::max(0, animElem);
}

bool stateSoundFireKeyAlreadyFired(const FighterState& fighter, int fireKey) {
    return std::find(
        fighter.firedStateSoundControllerIds.begin(),
        fighter.firedStateSoundControllerIds.end(),
        fireKey) != fighter.firedStateSoundControllerIds.end();
}

bool stateSoundControllerIdAlreadyFired(const FighterState& fighter, int soundControllerId) {
    return std::find(
        fighter.firedStateSoundControllerIds.begin(),
        fighter.firedStateSoundControllerIds.end(),
        soundControllerId) != fighter.firedStateSoundControllerIds.end();
}

void markStateSoundFireKeyFired(FighterState& fighter, int fireKey) {
    if (!stateSoundFireKeyAlreadyFired(fighter, fireKey)) {
        fighter.firedStateSoundControllerIds.push_back(fireKey);
    }
}

void markStateSoundControllerIdFired(FighterState& fighter, int soundControllerId) {
    if (!stateSoundControllerIdAlreadyFired(fighter, soundControllerId)) {
        fighter.firedStateSoundControllerIds.push_back(soundControllerId);
    }
}

bool stateSoundTriggerActive(AppState& state, FighterState& fighter, const StateSoundController& sound) {
    return sound.trigger.hasTrigger
        ? stateControllerTriggerActive(state, fighter, sound.trigger, nullptr, nullptr)
        : simpleControllerTriggerActive(state, fighter, sound.triggerTime, sound.triggerAnimElem);
}

void executePlaySoundController(AppState& state, FighterState& fighter, const StateSoundController& sound) {
    const int animElem = currentAnimElemForFighter(state, fighter);
    if (!stateSoundTriggerActive(state, fighter, sound)) {
        return;
    }
    if (sound.trigger.hasTrigger) {
        if (sound.trigger.persistent == 0) {
            if (stateSoundControllerIdAlreadyFired(fighter, sound.id)) {
                return;
            }
            markStateSoundControllerIdFired(fighter, sound.id);
        } else if (sound.trigger.persistent > 1 && fighter.stateTime % sound.trigger.persistent != 0) {
            return;
        }
    }

    const int fireKey = stateControllerFireKey(sound.id, fighter, animElem);
    if (stateSoundFireKeyAlreadyFired(fighter, fireKey)) {
        return;
    }
    markStateSoundFireKeyFired(fighter, fireKey);
    if (shouldPlayFightSounds(state)) {
        playSound(
            state,
            sound.group,
            sound.index,
            sound.forceCommon,
            sound.channel,
            sound.lowPriority,
            sound.gain,
            sound.loop);
    }
}

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

bool compareIntValue(int lhs, CompareOp op, int rhs) {
    switch (op) {
    case CompareOp::Equal:
        return lhs == rhs;
    case CompareOp::NotEqual:
        return lhs != rhs;
    case CompareOp::Greater:
        return lhs > rhs;
    case CompareOp::GreaterEqual:
        return lhs >= rhs;
    case CompareOp::Less:
        return lhs < rhs;
    case CompareOp::LessEqual:
        return lhs <= rhs;
    default:
        return false;
    }
}

bool compareFloatValue(float lhs, CompareOp op, float rhs) {
    switch (op) {
    case CompareOp::Equal:
        return std::fabs(lhs - rhs) < 0.0001f;
    case CompareOp::NotEqual:
        return std::fabs(lhs - rhs) >= 0.0001f;
    case CompareOp::Greater:
        return lhs > rhs;
    case CompareOp::GreaterEqual:
        return lhs >= rhs;
    case CompareOp::Less:
        return lhs < rhs;
    case CompareOp::LessEqual:
        return lhs <= rhs;
    default:
        return false;
    }
}

float animationTimeValue(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findExactClip(state, fighter.action);
    if (!clip) {
        return 0.0f;
    }
    return static_cast<float>(std::min(0, fighter.animTick - clip->loopTicks));
}

std::vector<std::string> collectFighterCommands(
    const FighterInputState& input,
    const FighterState& fighter,
    const std::vector<CommandDefinition>& definitions);
bool commandActive(const std::vector<std::string>& commands, std::string_view command);

std::vector<std::string> collectCurrentFighterCommands(const AppState& state, const FighterState& fighter) {
    if (fighter.inputHistory.empty()) {
        return {};
    }
    return collectFighterCommands(fighter.inputHistory.back().input, fighter, state.commandDefinitions);
}

float p2AxisDistXValue(const FighterState& fighter, const FighterState* opponent);
float p2AxisDistYValue(const FighterState& fighter, const FighterState* opponent);
float p2BodyDistXExpressionValue(const FighterState& fighter, const FighterState* opponent);
float p2BodyDistYExpressionValue(const FighterState& fighter, const FighterState* opponent);
float edgeDistExpressionValue(const AppState& state, const FighterState& fighter, const StageSlot* stage, bool front);

float stateTriggerSubjectValue(
    const AppState& state,
    const FighterState& fighter,
    StateTriggerSubject subject,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    switch (subject) {
    case StateTriggerSubject::Time:
        return static_cast<float>(fighter.stateTime);
    case StateTriggerSubject::AnimTime:
        return animationTimeValue(state, fighter);
    case StateTriggerSubject::VelX:
        return fighter.vx;
    case StateTriggerSubject::VelY:
        return fighter.vy;
    case StateTriggerSubject::PosX:
        return fighter.x;
    case StateTriggerSubject::PosY:
        return fighter.y;
    case StateTriggerSubject::P2BodyDistX:
        return p2BodyDistXExpressionValue(fighter, opponent);
    case StateTriggerSubject::P2BodyDistY:
        return p2BodyDistYExpressionValue(fighter, opponent);
    case StateTriggerSubject::P2DistX:
        return p2AxisDistXValue(fighter, opponent);
    case StateTriggerSubject::P2DistY:
        return p2AxisDistYValue(fighter, opponent);
    case StateTriggerSubject::FrontEdgeBodyDist:
        return edgeDistExpressionValue(state, fighter, stage, true);
    case StateTriggerSubject::BackEdgeBodyDist:
        return edgeDistExpressionValue(state, fighter, stage, false);
    case StateTriggerSubject::HitShakeOver:
        return fighter.hitPauseTicks <= 0 ? 1.0f : 0.0f;
    default:
        return 0.0f;
    }
}

bool variableRefInRange(const MugenVariableRef& ref) {
    switch (ref.bank) {
    case MugenVariableBank::Var:
    case MugenVariableBank::SysVar:
        return ref.index >= 0 && ref.index < 60;
    case MugenVariableBank::FVar:
    case MugenVariableBank::SysFVar:
        return ref.index >= 0 && ref.index < 40;
    default:
        return false;
    }
}

float fighterVariableValue(const FighterState& fighter, const MugenVariableRef& ref) {
    if (!variableRefInRange(ref)) {
        return 0.0f;
    }
    switch (ref.bank) {
    case MugenVariableBank::Var:
        return static_cast<float>(fighter.vars[static_cast<size_t>(ref.index)]);
    case MugenVariableBank::SysVar:
        return static_cast<float>(fighter.sysVars[static_cast<size_t>(ref.index)]);
    case MugenVariableBank::FVar:
        return fighter.fvars[static_cast<size_t>(ref.index)];
    case MugenVariableBank::SysFVar:
        return fighter.sysFvars[static_cast<size_t>(ref.index)];
    default:
        return 0.0f;
    }
}

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

int pseudoMugenRandom(const AppState& state, const FighterState& fighter, int salt = 0) {
    uint32_t value = static_cast<uint32_t>(state.frame * 1103515245u)
        ^ static_cast<uint32_t>((fighter.stateNo + 37) * 2654435761u)
        ^ static_cast<uint32_t>((fighter.stateTime + 101) * 2246822519u)
        ^ static_cast<uint32_t>((fighter.action + 17) * 3266489917u)
        ^ static_cast<uint32_t>(salt * 374761393u);
    value ^= value >> 16;
    value *= 2246822519u;
    value ^= value >> 13;
    return static_cast<int>(value % 1000u);
}

float mugenStateTypeValue(char stateType) {
    switch (static_cast<char>(SDL_toupper(static_cast<unsigned char>(stateType)))) {
    case 'C':
        return 1.0f;
    case 'A':
        return 2.0f;
    case 'L':
        return 3.0f;
    case 'S':
    default:
        return 0.0f;
    }
}

float mugenMoveTypeValue(char moveType) {
    switch (static_cast<char>(SDL_toupper(static_cast<unsigned char>(moveType)))) {
    case 'A':
        return 2.0f;
    case 'H':
        return 2.0f;
    case 'I':
    default:
        return 0.0f;
    }
}

float p2AxisDistXValue(const FighterState& fighter, const FighterState* opponent) {
    return opponent ? (opponent->x - fighter.x) * static_cast<float>(fighter.facing) : 0.0f;
}

float p2AxisDistYValue(const FighterState& fighter, const FighterState* opponent) {
    return opponent ? opponent->y - fighter.y : 0.0f;
}

float p2BodyDistXExpressionValue(const FighterState& fighter, const FighterState* opponent) {
    if (!opponent) {
        return 0.0f;
    }
    const float axisDistance = p2AxisDistXValue(fighter, opponent);
    const float fighterFront = fighter.playerWidthFront >= 0.0f ? fighter.playerWidthFront : 0.0f;
    const float opponentBack = opponent->playerWidthBack >= 0.0f ? opponent->playerWidthBack : 0.0f;
    return axisDistance >= 0.0f ? axisDistance - fighterFront - opponentBack : axisDistance;
}

float p2BodyDistYExpressionValue(const FighterState& fighter, const FighterState* opponent) {
    return p2AxisDistYValue(fighter, opponent);
}

float edgeDistExpressionValue(const AppState& state, const FighterState& fighter, const StageSlot* stage, bool front) {
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float visibleLeft = state.cameraX - halfWidth;
    const float visibleRight = state.cameraX + halfWidth;
    const float leftEdge = stage ? std::max(stage->leftbound, visibleLeft) : visibleLeft;
    const float rightEdge = stage ? std::min(stage->rightbound, visibleRight) : visibleRight;
    if (front) {
        return fighter.facing >= 0 ? rightEdge - fighter.x : fighter.x - leftEdge;
    }
    return fighter.facing >= 0 ? fighter.x - leftEdge : rightEdge - fighter.x;
}

float mugenRoundStateValue(const AppState& state) {
    if (!isMatchMode(state)) {
        return 2.0f;
    }
    switch (state.matchPhase) {
    case MatchPhase::RoundStart:
        return 1.0f;
    case MatchPhase::Fight:
        return 2.0f;
    case MatchPhase::RoundFinish:
        return 3.0f;
    case MatchPhase::RoundResult:
    case MatchPhase::MatchResult:
        return 4.0f;
    default:
        return 2.0f;
    }
}

std::optional<float> evalMugenExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr);

std::optional<std::pair<float, float>> parseMugenFloatRangeExpression(const std::string& expression) {
    const std::string value = trim(expression);
    if (value.size() < 5 || value.front() != '[' || value.back() != ']') {
        return std::nullopt;
    }
    const auto parts = splitCsv(std::string(std::string_view(value).substr(1, value.size() - 2)));
    if (parts.empty()) {
        return std::nullopt;
    }
    const auto minValue = parsePlainFloatValue(parts[0]);
    const auto maxValue = parts.size() >= 2 ? parsePlainFloatValue(parts[1]) : minValue;
    if (!minValue || !maxValue) {
        return std::nullopt;
    }
    return std::pair<float, float>{ std::min(*minValue, *maxValue), std::max(*minValue, *maxValue) };
}

std::optional<float> evalMugenFunctionExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent,
    const StageSlot* stage) {
    const auto open = expression.find('(');
    if (open == std::string::npos || expression.back() != ')') {
        return std::nullopt;
    }

    const std::string name = lowercaseCopy(trim(std::string_view(expression).substr(0, open)));
    const std::string body = trim(std::string_view(expression).substr(open + 1, expression.size() - open - 2));
    if (name == "floor") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::floor(*value) } : std::nullopt;
    }
    if (name == "ceil") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::ceil(*value) } : std::nullopt;
    }
    if (name == "abs") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::fabs(*value) } : std::nullopt;
    }
    if (name == "ifelse") {
        const auto parts = splitCsv(body);
        if (parts.size() < 3) {
            return std::nullopt;
        }
        const auto condition = evalMugenExpression(state, fighter, parts[0], opponent, stage);
        if (!condition) {
            return std::nullopt;
        }
        return evalMugenExpression(state, fighter, *condition != 0.0f ? parts[1] : parts[2], opponent, stage);
    }
    if (name == "const") {
        return lookupCharacterConstant(state.characterConstants, body);
    }
    if (name == "gethitvar") {
        const std::string key = lowercaseCopy(trim(body));
        if (key == "animtype") {
            return static_cast<float>(fighter.getHitAnimType);
        }
        if (key == "groundtype") {
            return static_cast<float>(fighter.getHitGroundType);
        }
        if (key == "airtype") {
            return static_cast<float>(fighter.getHitAirType);
        }
        if (key == "slidetime") {
            return static_cast<float>(fighter.getHitSlideTime);
        }
        if (key == "hittime") {
            return static_cast<float>(fighter.getHitHitTime);
        }
        if (key == "ctrltime") {
            return static_cast<float>(fighter.getHitCtrlTime);
        }
        if (key == "xvel") {
            return fighter.hitVelocityX;
        }
        if (key == "yvel") {
            return fighter.hitVelocityY;
        }
        if (key == "yaccel") {
            return fighter.hitFallYAccel;
        }
        if (key == "fall") {
            return fighter.hitFall ? 1.0f : 0.0f;
        }
        if (key == "fall.recover") {
            return fighter.hitFallRecover ? 1.0f : 0.0f;
        }
        if (key == "fall.recovertime") {
            return static_cast<float>(fighter.hitFallRecoverTime);
        }
        if (key == "fall.damage") {
            return static_cast<float>(fighter.hitFallDamage);
        }
        if (key == "fall.xvel") {
            return fighter.hitFallBounceXVelocity;
        }
        if (key == "fall.yvel") {
            return fighter.hitFallBounceYVelocity;
        }
        if (key == "hitcount") {
            return static_cast<float>(fighter.getHitHitCount);
        }
        return std::nullopt;
    }
    if (name == "selfanimexist") {
        const auto action = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!action) {
            return std::nullopt;
        }
        return findExactClip(state, static_cast<int>(std::lround(*action))) ? 1.0f : 0.0f;
    }
    if (name == "numhelper") {
        if (body.empty()) {
            return static_cast<float>(ownedHelperCount(state, fighter));
        }
        const auto helperId = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!helperId) {
            return std::nullopt;
        }
        return static_cast<float>(ownedHelperCount(state, fighter, static_cast<int>(std::lround(*helperId))));
    }
    if (name == "numprojid") {
        const auto projectileId = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!projectileId) {
            return std::nullopt;
        }
        return static_cast<float>(ownedProjectileCount(state, fighter, static_cast<int>(std::lround(*projectileId))));
    }
    if (name == "projcontacttime" || name == "projguardedtime") {
        const auto projectileId = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!projectileId) {
            return std::nullopt;
        }
        const int id = static_cast<int>(std::lround(*projectileId));
        if (name == "projcontacttime") {
            return fighter.projectileContactId == id && fighter.projectileContactTicks > 0 ? 1.0f : -1.0f;
        }
        return fighter.projectileGuardedId == id && fighter.projectileGuardedTicks > 0 ? 1.0f : -1.0f;
    }
    return std::nullopt;
}

std::optional<float> evalMugenExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent,
    const StageSlot* stage) {
    std::string value = stripOuterParens(expression);
    if (value.empty()) {
        return std::nullopt;
    }

    const size_t comma = value.find(',');
    if (comma != std::string::npos) {
        const std::string prefix = lowercaseCopy(trim(std::string_view(value).substr(0, comma)));
        if (prefix == "parent" || prefix == "root") {
            if (const FighterState* owner = fighterOwner(state, fighter)) {
                return evalMugenExpression(state, *owner, trim(std::string_view(value).substr(comma + 1)), opponent, stage);
            }
            return std::nullopt;
        }
        if (prefix == "enemy" || prefix == "enemynear") {
            if (!opponent) {
                return std::nullopt;
            }
            return evalMugenExpression(state, *opponent, trim(std::string_view(value).substr(comma + 1)), &fighter, stage);
        }
        if (prefix == "target") {
            const FighterState* target = storedTargetFighter(state, fighter);
            if (!target) {
                return std::nullopt;
            }
            return evalMugenExpression(state, *target, trim(std::string_view(value).substr(comma + 1)), &fighter, stage);
        }
        if (startsWithNoCase(prefix, "helper(") && prefix.back() == ')') {
            const std::string helperBody = trim(std::string_view(prefix).substr(std::string_view("helper(").size(), prefix.size() - std::string_view("helper(").size() - 1));
            const auto helperId = evalMugenExpression(state, fighter, helperBody, opponent, stage);
            if (!helperId) {
                return std::nullopt;
            }
            const FighterState* helper = ownedHelperById(state, fighter, static_cast<int>(std::lround(*helperId)));
            if (!helper) {
                return std::nullopt;
            }
            return evalMugenExpression(state, *helper, trim(std::string_view(value).substr(comma + 1)), opponent, stage);
        }
    }

    const auto orClauses = splitTopLevelClauses(value, "||", true);
    if (orClauses.size() > 1) {
        for (const auto& clause : orClauses) {
            const auto evaluated = evalMugenExpression(state, fighter, clause, opponent, stage);
            if (evaluated && *evaluated != 0.0f) {
                return 1.0f;
            }
        }
        return 0.0f;
    }

    const auto andClauses = splitTopLevelClauses(value, "&&");
    if (andClauses.size() > 1) {
        for (const auto& clause : andClauses) {
            const auto evaluated = evalMugenExpression(state, fighter, clause, opponent, stage);
            if (!evaluated || *evaluated == 0.0f) {
                return 0.0f;
            }
        }
        return 1.0f;
    }

    if (value.front() == '!') {
        const auto evaluated = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(1)), opponent, stage);
        if (!evaluated) {
            return std::nullopt;
        }
        return *evaluated == 0.0f ? 1.0f : 0.0f;
    }

    if (const auto compare = findTopLevelCompareOp(value)) {
        const auto [op, pos] = *compare;
        const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
        const auto lhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(0, pos)), opponent, stage);
        const std::string rhsExpression = trim(std::string_view(value).substr(pos + opLength));
        if (const auto range = parseMugenFloatRangeExpression(rhsExpression)) {
            if (!lhs || (op != CompareOp::Equal && op != CompareOp::NotEqual)) {
                return std::nullopt;
            }
            const bool inRange = *lhs >= range->first && *lhs <= range->second;
            return (op == CompareOp::Equal ? inRange : !inRange) ? 1.0f : 0.0f;
        }
        const auto rhs = evalMugenExpression(state, fighter, rhsExpression, opponent, stage);
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        return compareFloatValue(*lhs, op, *rhs) ? 1.0f : 0.0f;
    }

    if (const auto op = findTopLevelBinaryOperator(value, { "+", "-" })) {
        const char token = value[*op];
        const auto lhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(0, *op)), opponent, stage);
        const auto rhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(*op + 1)), opponent, stage);
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        return token == '+' ? *lhs + *rhs : *lhs - *rhs;
    }
    if (const auto op = findTopLevelBinaryOperator(value, { "*", "/", "%" })) {
        const char token = value[*op];
        const auto lhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(0, *op)), opponent, stage);
        const auto rhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(*op + 1)), opponent, stage);
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        if ((token == '/' || token == '%') && *rhs == 0.0f) {
            return 0.0f;
        }
        if (token == '*') {
            return *lhs * *rhs;
        }
        if (token == '%') {
            return std::fmod(*lhs, *rhs);
        }
        return *lhs / *rhs;
    }

    if (value.front() == '-') {
        const auto evaluated = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(1)), opponent, stage);
        return evaluated ? std::optional<float>{ -*evaluated } : std::nullopt;
    }

    if (const auto plain = parsePlainFloatValue(value)) {
        return plain;
    }
    if (const auto ref = parseMugenVariableRef(value)) {
        return fighterVariableValue(fighter, *ref);
    }
    if (const auto functionValue = evalMugenFunctionExpression(state, fighter, value, opponent, stage)) {
        return functionValue;
    }

    const std::string lowered = lowercaseCopy(value);
    if (lowered == "random") {
        return static_cast<float>(pseudoMugenRandom(state, fighter));
    }
    if (lowered == "numhelper") {
        return static_cast<float>(ownedHelperCount(state, fighter));
    }
    if (lowered == "numproj") {
        return static_cast<float>(ownedProjectileCount(state, fighter));
    }
    if (startsWithNoCase(lowered, "projhit")) {
        const std::string suffix = trim(std::string_view(lowered).substr(std::string_view("projhit").size()));
        const int id = suffix.empty() ? 0 : parseIntValue(suffix, 0);
        return (fighter.projectileHitTicks > 0 && (id == 0 || fighter.projectileHitId == id)) ? 1.0f : 0.0f;
    }
    if (startsWithNoCase(lowered, "projcontact")) {
        const std::string suffix = trim(std::string_view(lowered).substr(std::string_view("projcontact").size()));
        const int id = suffix.empty() ? 0 : parseIntValue(suffix, 0);
        return (fighter.projectileContactTicks > 0 && (id == 0 || fighter.projectileContactId == id)) ? 1.0f : 0.0f;
    }
    if (lowered == "ailevel") {
        return 0.0f;
    }
    if (lowered == "time") {
        return static_cast<float>(fighter.stateTime);
    }
    if (lowered == "animtime") {
        return animationTimeValue(state, fighter);
    }
    if (lowered == "stateno") {
        return static_cast<float>(fighter.stateNo);
    }
    if (lowered == "power") {
        return static_cast<float>(fighter.power);
    }
    if (lowered == "powermax") {
        return static_cast<float>(state.characterConstants.maxPower);
    }
    if (lowered == "palno") {
        return static_cast<float>(fighter.paletteNo);
    }
    if (lowered == "hitcount") {
        return static_cast<float>(fighter.hitCount);
    }
    if (lowered == "numtarget") {
        return fighter.targetTicks > 0 && fighter.targetIndex >= 0 ? 1.0f : 0.0f;
    }
    if (lowered == "movecontact") {
        return fighter.moveContact ? 1.0f : 0.0f;
    }
    if (lowered == "movehit") {
        return fighter.moveHit ? 1.0f : 0.0f;
    }
    if (lowered == "moveguarded") {
        return fighter.moveGuarded ? 1.0f : 0.0f;
    }
    if (lowered == "roundstate") {
        return mugenRoundStateValue(state);
    }
    if (lowered == "roundno") {
        return static_cast<float>(std::max(1, state.currentRound));
    }
    if (lowered == "roundsexisted") {
        return static_cast<float>(std::max(0, state.currentRound - 1));
    }
    if (lowered == "teammode") {
        return 0.0f;
    }
    if (lowered == "single") {
        return 0.0f;
    }
    if (lowered == "simul") {
        return 1.0f;
    }
    if (lowered == "turns") {
        return 2.0f;
    }
    if (lowered == "life") {
        return static_cast<float>(fighter.life);
    }
    if (lowered == "alive") {
        return fighter.life > 0 ? 1.0f : 0.0f;
    }
    if (lowered == "p2life") {
        return opponent ? static_cast<float>(opponent->life) : 0.0f;
    }
    if (lowered == "p2stateno") {
        return opponent ? static_cast<float>(opponent->stateNo) : 0.0f;
    }
    if (lowered == "p2statetype") {
        return opponent ? mugenStateTypeValue(opponent->stateType) : 0.0f;
    }
    if (lowered == "p2movetype") {
        return opponent ? mugenMoveTypeValue(opponent->moveType) : 0.0f;
    }
    if (lowered == "statetype") {
        return mugenStateTypeValue(fighter.stateType);
    }
    if (lowered == "movetype") {
        return mugenMoveTypeValue(fighter.moveType);
    }
    if (lowered == "i") {
        return mugenMoveTypeValue('I');
    }
    if (lowered == "h") {
        return mugenMoveTypeValue('H');
    }
    if (lowered == "s") {
        return mugenStateTypeValue('S');
    }
    if (lowered == "c") {
        return mugenStateTypeValue('C');
    }
    if (lowered == "a") {
        return mugenStateTypeValue('A');
    }
    if (lowered == "l") {
        return mugenStateTypeValue('L');
    }
    if (lowered == "facing") {
        return static_cast<float>(fighter.facing);
    }
    if (lowered == "pos x") {
        return fighter.x;
    }
    if (lowered == "pos y") {
        return fighter.y;
    }
    if (lowered == "vel x") {
        return fighter.vx;
    }
    if (lowered == "vel y") {
        return fighter.vy;
    }
    if (lowered == "p2bodydist x") {
        return p2BodyDistXExpressionValue(fighter, opponent);
    }
    if (lowered == "p2bodydist y") {
        return p2BodyDistYExpressionValue(fighter, opponent);
    }
    if (lowered == "p2dist x") {
        return p2AxisDistXValue(fighter, opponent);
    }
    if (lowered == "p2dist y") {
        return p2AxisDistYValue(fighter, opponent);
    }
    if (lowered == "frontedgedist" || lowered == "frontedgebodydist") {
        return edgeDistExpressionValue(state, fighter, stage, true);
    }
    if (lowered == "backedgedist" || lowered == "backedgebodydist") {
        return edgeDistExpressionValue(state, fighter, stage, false);
    }
    return std::nullopt;
}

bool evalMugenExpressionCondition(
    const AppState& state,
    const FighterState& fighter,
    const MugenExpressionCondition& condition,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    const auto lhs = evalMugenExpression(state, fighter, condition.lhs, opponent, stage);
    const auto rhs = evalMugenExpression(state, fighter, condition.rhs, opponent, stage);
    return lhs && rhs && compareFloatValue(*lhs, condition.op, *rhs);
}

bool stateTriggerGroupActive(
    const AppState& state,
    const FighterState& fighter,
    const StateTriggerGroup& group,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (group.requiresMoveContact && !fighter.moveContact) {
        return false;
    }
    if (!group.requiredCommands.empty() || !group.forbiddenCommands.empty()) {
        const auto commands = collectCurrentFighterCommands(state, fighter);
        for (const auto& required : group.requiredCommands) {
            if (!commandActive(commands, required)) {
                return false;
            }
        }
        for (const auto& forbidden : group.forbiddenCommands) {
            if (commandActive(commands, forbidden)) {
                return false;
            }
        }
    }
    for (const auto& condition : group.floatConditions) {
        if (!compareFloatValue(stateTriggerSubjectValue(state, fighter, condition.subject, opponent, stage), condition.op, condition.value)) {
            return false;
        }
    }
    for (const auto& condition : group.rangeConditions) {
        const float value = stateTriggerSubjectValue(state, fighter, condition.subject, opponent, stage);
        if (value < condition.minValue || value > condition.maxValue) {
            return false;
        }
    }
    for (const auto& condition : group.expressionConditions) {
        if (!evalMugenExpressionCondition(state, fighter, condition, opponent, stage)) {
            return false;
        }
    }
    for (const auto& expression : group.booleanExpressions) {
        const auto value = evalMugenExpression(state, fighter, expression, opponent, stage);
        if (!value || *value == 0.0f) {
            return false;
        }
    }
    for (const auto& condition : group.stateTypeConditions) {
        const bool matches = fighter.stateType == condition.stateType;
        if (condition.negated ? matches : !matches) {
            return false;
        }
    }
    for (const auto& condition : group.animElemTimeConditions) {
        if (!compareIntValue(animElemTimeForFighter(state, fighter, condition.elem), condition.op, condition.value)) {
            return false;
        }
    }
    return true;
}

bool anyStateTriggerGroupActive(
    const AppState& state,
    const FighterState& fighter,
    const std::vector<StateTriggerGroup>& groups,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    return std::any_of(groups.begin(), groups.end(), [&state, &fighter, opponent, stage](const StateTriggerGroup& group) {
        return stateTriggerGroupActive(state, fighter, group, opponent, stage);
    });
}

bool stateControllerTriggerActive(
    const AppState& state,
    const FighterState& fighter,
    const StateControllerTrigger& trigger,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (!trigger.hasTrigger) {
        return false;
    }
    for (const auto& expressionGroups : trigger.triggerAllExpressions) {
        if (!anyStateTriggerGroupActive(state, fighter, expressionGroups, opponent, stage)) {
            return false;
        }
    }
    if (trigger.triggerGroups.empty()) {
        return true;
    }
    return anyStateTriggerGroupActive(state, fighter, trigger.triggerGroups, opponent, stage);
}

bool stateRuntimeControllerAlreadyFired(const FighterState& fighter, int controllerId) {
    return std::find(
        fighter.firedStateRuntimeControllerIds.begin(),
        fighter.firedStateRuntimeControllerIds.end(),
        controllerId) != fighter.firedStateRuntimeControllerIds.end();
}

void markStateRuntimeControllerFired(FighterState& fighter, int controllerId) {
    if (!stateRuntimeControllerAlreadyFired(fighter, controllerId)) {
        fighter.firedStateRuntimeControllerIds.push_back(controllerId);
    }
}

bool shouldRunStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (!stateControllerTriggerActive(state, fighter, trigger, opponent, stage)) {
        return false;
    }
    if (trigger.persistent == 0) {
        if (stateRuntimeControllerAlreadyFired(fighter, controllerId)) {
            return false;
        }
        markStateRuntimeControllerFired(fighter, controllerId);
        return true;
    }
    if (trigger.persistent > 1 && fighter.stateTime % trigger.persistent != 0) {
        return false;
    }
    return true;
}

int fighterIndexInState(const AppState& state, const FighterState& fighter) {
    const auto* first = state.fighters.data();
    const auto* current = &fighter;
    if (current < first || current >= first + state.fighters.size()) {
        return -1;
    }
    return static_cast<int>(current - first);
}

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

void clearGlobalPause(AppState& state) {
    state.globalPauseTicks = 0;
    state.globalPauseOwnerIndex = -1;
    state.globalPauseOwnerMoveTicks = 0;
    state.globalPauseIsSuper = false;
}

void clearEnvShake(AppState& state) {
    state.envShakeTicks = 0;
    state.envShakeTotalTicks = 0;
    state.envShakeFrequency = 60;
    state.envShakeAmplitude = 0.0f;
    state.envShakePhase = 0;
    state.envShakeOffsetY = 0.0f;
}

void clearPaletteRuntime(AppState& state) {
    state.backgroundPaletteEffect = {};
    state.envColor = {};
    for (auto& fighter : state.fighters) {
        fighter.paletteEffect = {};
    }
}

void executeStopSoundController(
    AppState& state,
    FighterState& fighter,
    const StateStopSoundController& stopSound,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    bool triggerActive = false;
    if (stopSound.trigger.hasTrigger) {
        if (!shouldRunStateRuntimeController(state, fighter, stopSound.id, stopSound.trigger, opponent, stage)) {
            return;
        }
        triggerActive = true;
    } else {
        triggerActive = simpleControllerTriggerActive(state, fighter, stopSound.triggerTime, stopSound.triggerAnimElem);
        if (triggerActive) {
            if (stateRuntimeControllerAlreadyFired(fighter, stopSound.id)) {
                return;
            }
            markStateRuntimeControllerFired(fighter, stopSound.id);
        }
    }

    if (triggerActive) {
        stopSoundChannel(state.audio, stopSound.channel);
    }
}

void executeStateAudioControllers(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    for (const auto& ref : stateDef.audioControllers) {
        if (ref.kind == StateAudioControllerKind::PlaySnd) {
            if (ref.index < stateDef.sounds.size()) {
                executePlaySoundController(state, fighter, stateDef.sounds[ref.index]);
            }
        } else if (ref.index < stateDef.stopSounds.size()) {
            executeStopSoundController(state, fighter, stateDef.stopSounds[ref.index], opponent, stage);
        }
    }
}

void updateStateAudioControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    const auto stateDefs = runtimeControllerStateDefinitions(state, fighter);
    for (auto stateDefIt = stateDefs.begin(); stateDefIt != stateDefs.end(); ++stateDefIt) {
        const StateDefinition* stateDef = *stateDefIt;
        if (!stateDef || std::find(stateDefs.begin(), stateDefIt, stateDef) != stateDefIt) {
            continue;
        }
        executeStateAudioControllers(state, fighter, *stateDef, opponent, stage);
    }
}

bool fighterHasAssertSpecialFlag(const FighterState& fighter, std::string_view flag) {
    const std::string normalized = normalizeAssertSpecialFlag(flag);
    return std::find(
        fighter.assertSpecialFlags.begin(),
        fighter.assertSpecialFlags.end(),
        normalized) != fighter.assertSpecialFlags.end();
}

bool anyFighterHasAssertSpecialFlag(const AppState& state, std::string_view flag) {
    return std::any_of(
        state.fighters.begin(),
        state.fighters.end(),
        [flag](const FighterState& fighter) {
            return fighterHasAssertSpecialFlag(fighter, flag);
        });
}

void addFighterAssertSpecialFlag(FighterState& fighter, const std::string& flag) {
    if (flag.empty()
        || std::find(fighter.assertSpecialFlags.begin(), fighter.assertSpecialFlags.end(), flag) != fighter.assertSpecialFlags.end()) {
        return;
    }
    fighter.assertSpecialFlags.push_back(flag);
}

void clearFightAssertSpecialFlags(AppState& state) {
    for (auto& fighter : state.fighters) {
        fighter.assertSpecialFlags.clear();
    }
}

void updateStateAssertSpecialControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& assertSpecial : stateDef.assertSpecials) {
            if (!shouldRunStateRuntimeController(state, fighter, assertSpecial.id, assertSpecial.trigger, opponent, stage)) {
                continue;
            }
            for (const auto& flag : assertSpecial.flags) {
                addFighterAssertSpecialFlag(fighter, flag);
            }
        }
        return true;
    });
}

void updateFightAssertSpecialControllers(AppState& state, const StageSlot& stage) {
    updateStateAssertSpecialControllers(state, state.fighters[0], &state.fighters[1], &stage);
    updateStateAssertSpecialControllers(state, state.fighters[1], &state.fighters[0], &stage);
}

bool simpleControllerTriggerActive(const AppState& state, const FighterState& fighter, int triggerTime, int triggerAnimElem) {
    bool hasTrigger = false;
    if (triggerTime >= 0) {
        hasTrigger = true;
        if (fighter.stateTime != triggerTime) {
            return false;
        }
    }
    if (triggerAnimElem > 0) {
        hasTrigger = true;
        if (animElemTimeForFighter(state, fighter, triggerAnimElem) != 0) {
            return false;
        }
    }
    return hasTrigger;
}

bool changeAnimTriggerActive(const AppState& state, const FighterState& fighter, const StateChangeAnimController& controller) {
    if (controller.triggerTime >= 0 || controller.triggerAnimElem > 0) {
        if (!simpleControllerTriggerActive(state, fighter, controller.triggerTime, controller.triggerAnimElem)) {
            return false;
        }
    }
    if (controller.requiresMoveContact && !fighter.moveContact) {
        return false;
    }
    for (const auto& condition : controller.animElemTimeConditions) {
        if (!compareIntValue(animElemTimeForFighter(state, fighter, condition.elem), condition.op, condition.value)) {
            return false;
        }
    }
    return controller.requiresMoveContact
        || !controller.animElemTimeConditions.empty()
        || controller.triggerTime >= 0
        || controller.triggerAnimElem > 0;
}

void updateStateVariableControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& variable : stateDef.variableControllers) {
            if (!shouldRunStateRuntimeController(state, fighter, variable.id, variable.trigger, opponent, stage)) {
                continue;
            }

            if (variable.operation == StateVariableOperation::Random) {
                const auto minValue = evalMugenExpression(state, fighter, variable.rangeMinExpression, opponent, stage);
                const auto maxValue = evalMugenExpression(state, fighter, variable.rangeMaxExpression, opponent, stage);
                if (!minValue || !maxValue) {
                    continue;
                }
                int low = static_cast<int>(std::lround(*minValue));
                int high = static_cast<int>(std::lround(*maxValue));
                if (high < low) {
                    std::swap(low, high);
                }
                const int span = std::max(1, high - low + 1);
                const int value = low + (pseudoMugenRandom(state, fighter, variable.id) % span);
                setFighterVariableValue(fighter, variable.target, static_cast<float>(value));
                continue;
            }

            const auto value = evalMugenExpression(state, fighter, variable.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }

            if (variable.operation == StateVariableOperation::Add) {
                setFighterVariableValue(fighter, variable.target, fighterVariableValue(fighter, variable.target) + *value);
            } else {
                setFighterVariableValue(fighter, variable.target, *value);
            }
        }

        for (const auto& rangeSet : stateDef.varRangeSets) {
            if (!shouldRunStateRuntimeController(state, fighter, rangeSet.id, rangeSet.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, rangeSet.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            const int maxIndex = rangeSet.floatBank ? 39 : 59;
            const int first = std::clamp(std::min(rangeSet.first, rangeSet.last), 0, maxIndex);
            const int last = std::clamp(std::max(rangeSet.first, rangeSet.last), 0, maxIndex);
            for (int index = first; index <= last; ++index) {
                if (rangeSet.floatBank) {
                    fighter.fvars[static_cast<size_t>(index)] = *value;
                } else {
                    fighter.vars[static_cast<size_t>(index)] = static_cast<int>(std::lround(*value));
                }
            }
        }
        return true;
    });
}

void updateStateMeterControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& powerAdd : stateDef.powerAdds) {
            if (!shouldRunStateRuntimeController(state, fighter, powerAdd.id, powerAdd.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, powerAdd.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.power = std::clamp(
                fighter.power + static_cast<int>(std::lround(*value)),
                0,
                std::max(0, state.characterConstants.maxPower));
        }

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

ActorBlendMode transBlendMode(std::string_view trans) {
    const std::string value = lowercaseCopy(trim(trans));
    if (value.find("addalpha") != std::string::npos) {
        return ActorBlendMode::AddAlpha;
    }
    if (value.find("add") != std::string::npos) {
        return ActorBlendMode::Add;
    }
    return ActorBlendMode::Normal;
}

std::string decodeClipboardEscapes(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            const char next = text[++i];
            if (next == 'n') {
                out.push_back('\n');
            } else if (next == 't') {
                out.push_back('\t');
            } else {
                out.push_back(next);
            }
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

std::string formatClipboardText(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot* stage,
    const StateClipboardController& clipboard) {
    std::array<float, 5> values{};
    const size_t count = std::min<size_t>(values.size(), clipboard.params.size());
    for (size_t i = 0; i < count; ++i) {
        if (const auto value = evalMugenExpression(state, fighter, clipboard.params[i], opponent, stage)) {
            values[i] = *value;
        }
    }

    const std::string text = decodeClipboardEscapes(clipboard.text);
    std::ostringstream out;
    size_t paramIndex = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '%' || i + 1 >= text.size()) {
            out << text[i];
            continue;
        }
        const char spec = text[++i];
        if (spec == '%') {
            out << '%';
            continue;
        }
        const float value = paramIndex < values.size() ? values[paramIndex++] : 0.0f;
        if (spec == 'd' || spec == 'i') {
            out << static_cast<int>(std::lround(value));
        } else if (spec == 'f') {
            out << std::fixed << std::setprecision(3) << value;
        } else {
            out << '%' << spec;
        }
    }
    return out.str();
}

std::string selectedVictoryQuoteText(const AppState& state, const FighterState& fighter) {
    if (fighter.victoryQuote < 0 || fighter.victoryQuote >= static_cast<int>(state.victoryQuotes.size())) {
        return {};
    }
    return state.victoryQuotes[static_cast<size_t>(fighter.victoryQuote)];
}

std::optional<PaletteRemap> evaluatePaletteRemap(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot* stage,
    const PaletteRemap& remap) {
    auto evalInt = [&](const std::string& expression) -> std::optional<int> {
        const auto value = evalMugenExpression(state, fighter, expression, opponent, stage);
        return value ? std::optional<int>{ static_cast<int>(std::lround(*value)) } : std::nullopt;
    };
    const auto sourceGroup = evalInt(remap.sourceGroupExpression);
    const auto sourceItem = evalInt(remap.sourceItemExpression);
    const auto destGroup = evalInt(remap.destGroupExpression);
    const auto destItem = evalInt(remap.destItemExpression);
    if (!sourceGroup || !sourceItem || !destGroup || !destItem) {
        return std::nullopt;
    }
    PaletteRemap evaluated;
    evaluated.sourceGroupExpression = std::to_string(*sourceGroup);
    evaluated.sourceItemExpression = std::to_string(*sourceItem);
    evaluated.destGroupExpression = std::to_string(*destGroup);
    evaluated.destItemExpression = std::to_string(*destItem);
    return evaluated;
}

void updateStateVisualControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& angle : stateDef->angleControllers) {
        if (!shouldRunStateRuntimeController(state, fighter, angle.id, angle.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, angle.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        switch (angle.operation) {
        case StateAngleOperation::Set:
            fighter.drawAngle = *value;
            break;
        case StateAngleOperation::Add:
            fighter.drawAngle += *value;
            break;
        case StateAngleOperation::Mul:
            fighter.drawAngle *= *value;
            break;
        }
    }

    for (const auto& angleDraw : stateDef->angleDraws) {
        if (!shouldRunStateRuntimeController(state, fighter, angleDraw.id, angleDraw.trigger, opponent, stage)) {
            continue;
        }
        fighter.angleDrawActive = true;
    }

    for (const auto& offset : stateDef->offsets) {
        if (!shouldRunStateRuntimeController(state, fighter, offset.id, offset.trigger, opponent, stage)) {
            continue;
        }
        if (offset.hasX) {
            if (const auto value = evalMugenExpression(state, fighter, offset.xExpression, opponent, stage)) {
                fighter.displayOffsetX = *value;
            }
        }
        if (offset.hasY) {
            if (const auto value = evalMugenExpression(state, fighter, offset.yExpression, opponent, stage)) {
                fighter.displayOffsetY = *value;
            }
        }
    }

    for (const auto& clipboard : stateDef->clipboards) {
        if (!shouldRunStateRuntimeController(state, fighter, clipboard.id, clipboard.trigger, opponent, stage)) {
            continue;
        }
        const std::string text = formatClipboardText(state, fighter, opponent, stage, clipboard);
        if (clipboard.append) {
            fighter.debugClipboard += text;
        } else {
            fighter.debugClipboard = text;
        }
    }

    for (const auto& victoryQuote : stateDef->victoryQuotes) {
        if (fighter.helper || !shouldRunStateRuntimeController(state, fighter, victoryQuote.id, victoryQuote.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, victoryQuote.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        const int index = static_cast<int>(std::lround(*value));
        fighter.victoryQuote = (index >= 0 && index < 100) ? index : -1;
    }

    for (const auto& remapPal : stateDef->remapPals) {
        if (!shouldRunStateRuntimeController(state, fighter, remapPal.id, remapPal.trigger, opponent, stage)) {
            continue;
        }
        const auto evaluated = evaluatePaletteRemap(state, fighter, opponent, stage, remapPal.remap);
        if (!evaluated) {
            continue;
        }
        const bool removeMapping = evaluated->destGroupExpression == "-1"
            || (evaluated->sourceGroupExpression == evaluated->destGroupExpression
                && evaluated->sourceItemExpression == evaluated->destItemExpression);
        const auto sameSource = [&](const PaletteRemap& active) {
            return active.sourceGroupExpression == evaluated->sourceGroupExpression
                && active.sourceItemExpression == evaluated->sourceItemExpression;
        };
        fighter.paletteRemaps.erase(
            std::remove_if(fighter.paletteRemaps.begin(), fighter.paletteRemaps.end(), sameSource),
            fighter.paletteRemaps.end());
        if (!removeMapping) {
            fighter.paletteRemaps.push_back(*evaluated);
        }
    }

    for (const auto& trans : stateDef->transControllers) {
        if (!shouldRunStateRuntimeController(state, fighter, trans.id, trans.trigger, opponent, stage)) {
            continue;
        }
        const auto source = evalMugenExpression(state, fighter, trans.alphaSourceExpression, opponent, stage);
        const auto dest = evalMugenExpression(state, fighter, trans.alphaDestExpression, opponent, stage);
        fighter.transEffect.active = true;
        fighter.transEffect.mode = transBlendMode(trans.trans);
        fighter.transEffect.alphaSource = std::clamp(static_cast<int>(std::lround(source.value_or(256.0f))), 0, 256);
        fighter.transEffect.alphaDest = std::clamp(static_cast<int>(std::lround(dest.value_or(0.0f))), 0, 256);
    }

    for (const auto& afterImage : stateDef->afterImages) {
        if (!shouldRunStateRuntimeController(state, fighter, afterImage.id, afterImage.trigger, opponent, stage)) {
            continue;
        }
        fighter.afterImage.active = true;
        fighter.afterImage.ticksLeft = afterImage.time < 0 ? 9999 : std::max(0, afterImage.time);
        fighter.afterImage.length = std::max(1, afterImage.length);
        fighter.afterImage.timeGap = std::max(1, afterImage.timeGap);
        fighter.afterImage.frameGap = std::max(1, afterImage.frameGap);
        fighter.afterImage.captureCountdown = 0;
        fighter.afterImage.additive = afterImage.additive;
        fighter.afterImage.trail.clear();
    }

    for (const auto& afterImageTime : stateDef->afterImageTimes) {
        if (!shouldRunStateRuntimeController(state, fighter, afterImageTime.id, afterImageTime.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, afterImageTime.timeExpression, opponent, stage);
        if (!value) {
            continue;
        }
        const int ticks = static_cast<int>(std::lround(*value));
        fighter.afterImage.ticksLeft = ticks < 0 ? 9999 : std::max(0, ticks);
        fighter.afterImage.active = fighter.afterImage.ticksLeft > 0;
        if (!fighter.afterImage.active) {
            fighter.afterImage.trail.clear();
        }
    }
        return true;
    });
}

void updateStatePosAddControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& posAdd : stateDef->posAdds) {
        if (posAdd.trigger.hasTrigger) {
            if (!shouldRunStateRuntimeController(state, fighter, posAdd.id, posAdd.trigger, opponent, stage)) {
                continue;
            }
            fighter.x += posAdd.x * static_cast<float>(fighter.facing);
            fighter.y += posAdd.y;
            continue;
        }
        if (statePosAddAlreadyFired(fighter, posAdd.id)
            || !simpleControllerTriggerActive(state, fighter, posAdd.triggerTime, posAdd.triggerAnimElem)) {
            continue;
        }
        markStatePosAddFired(fighter, posAdd.id);
        fighter.x += posAdd.x * static_cast<float>(fighter.facing);
        fighter.y += posAdd.y;
    }
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

void runForceFeedbackController(AppState& state, const FighterState& fighter, const StateForceFeedbackController& forceFeedback) {
    const int fighterIndex = fighter.helper && fighter.ownerIndex >= 0
        ? fighter.ownerIndex
        : fighterIndexInState(state, fighter);
    const int targetPlayer = forceFeedback.self ? fighterIndex : (fighterIndex == 0 ? 1 : 0);
    const GamepadDevice* device = assignedGamepad(state, targetPlayer);
    if (!device || !device->handle) {
        return;
    }

    const std::string waveform = lowercaseCopy(forceFeedback.waveform);
    const Uint16 amplitude = static_cast<Uint16>(std::clamp(forceFeedback.amplitude, 0, 255) * 257);
    Uint16 lowFrequency = 0;
    Uint16 highFrequency = 0;
    if (waveform == "off") {
        SDL_RumbleGamepad(device->handle, 0, 0, 0);
        return;
    }
    if (waveform == "square") {
        highFrequency = amplitude;
    } else if (waveform == "sinesquare") {
        lowFrequency = amplitude;
        highFrequency = amplitude;
    } else {
        lowFrequency = amplitude;
    }

    const Uint32 durationMs = static_cast<Uint32>(std::max(0, forceFeedback.time) * 1000 / 60);
    SDL_RumbleGamepad(device->handle, lowFrequency, highFrequency, durationMs);
}

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

    for (const auto& sprPriority : stateDef->sprPriorities) {
        if (!shouldRunStateRuntimeController(state, fighter, sprPriority.id, sprPriority.trigger, opponent, stage)) {
            continue;
        }
        fighter.sprPriority = sprPriority.value;
    }

    for (const auto& posFreeze : stateDef->posFreezes) {
        if (!shouldRunStateRuntimeController(state, fighter, posFreeze.id, posFreeze.trigger, opponent, stage)) {
            continue;
        }
        fighter.posFreezeX = fighter.posFreezeX || posFreeze.freezeX;
        fighter.posFreezeY = fighter.posFreezeY || posFreeze.freezeY;
    }

    for (const auto& turn : stateDef->turns) {
        if (!shouldRunStateRuntimeController(state, fighter, turn.id, turn.trigger, opponent, stage)) {
            continue;
        }
        fighter.facing *= -1;
        fighter.jumpBaseAction = 0;
        fighter.jumpPeakActionApplied = false;
    }

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

    for (const auto& velocity : stateDef->velocityControllers) {
        if (!shouldRunStateRuntimeController(state, fighter, velocity.id, velocity.trigger, opponent, stage)) {
            continue;
        }
        switch (velocity.operation) {
        case StateVelocityOperation::Set:
            if (velocity.hasX) {
                fighter.vx = velocity.x * static_cast<float>(fighter.facing);
            }
            if (velocity.hasY) {
                fighter.vy = velocity.y;
            }
            break;
        case StateVelocityOperation::Add:
            if (velocity.hasX) {
                fighter.vx += velocity.x * static_cast<float>(fighter.facing);
            }
            if (velocity.hasY) {
                fighter.vy += velocity.y;
            }
            break;
        case StateVelocityOperation::Mul:
            if (velocity.hasX) {
                fighter.vx *= velocity.x;
            }
            if (velocity.hasY) {
                fighter.vy *= velocity.y;
            }
            break;
        }
    }

    for (const auto& posSet : stateDef->posSets) {
        if (!shouldRunStateRuntimeController(state, fighter, posSet.id, posSet.trigger, opponent, stage)) {
            continue;
        }
        if (posSet.hasX) {
            fighter.x = posSet.x;
        }
        if (posSet.hasY) {
            fighter.y = posSet.y;
            fighter.onGround = fighter.y >= 0.0f;
        }
    }

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

void updateAfterImageEffect(FighterState& fighter) {
    auto& afterImage = fighter.afterImage;
    if (!afterImage.active) {
        afterImage.trail.clear();
        return;
    }
    if (afterImage.ticksLeft <= 0) {
        afterImage.active = false;
        afterImage.trail.clear();
        return;
    }

    if (afterImage.captureCountdown <= 0) {
        afterImage.trail.push_back(AfterImageSnapshot{
            fighter.action,
            fighter.animTick,
            fighter.x,
            fighter.y,
            fighter.facing,
            0,
        });
        afterImage.captureCountdown = afterImage.timeGap;
        while (static_cast<int>(afterImage.trail.size()) > afterImage.length) {
            afterImage.trail.erase(afterImage.trail.begin());
        }
    } else {
        --afterImage.captureCountdown;
    }

    for (auto& snapshot : afterImage.trail) {
        ++snapshot.ageTicks;
    }
    --afterImage.ticksLeft;
    if (afterImage.ticksLeft <= 0) {
        afterImage.active = false;
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
    if (fighter.y >= 0.0f) {
        if (fighter.stateType == 'A' && fighter.physics == 'N') {
            fighter.onGround = true;
            return;
        }
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        fighter.jumpBaseAction = 0;
        fighter.jumpPeakActionApplied = false;
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

    if (state.pendingMode == PendingMode::Training && activeOpponentType(state) == OpponentType::Dummy) {
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

void startEnvShake(AppState& state, const EnvShakeSpec& shake) {
    if (!shake.enabled || shake.time <= 0 || std::abs(shake.amplitude) <= 0.001f) {
        return;
    }
    if (shake.time < state.envShakeTicks && std::abs(shake.amplitude) <= std::abs(state.envShakeAmplitude)) {
        return;
    }
    state.envShakeTicks = shake.time;
    state.envShakeTotalTicks = shake.time;
    state.envShakeFrequency = std::max(1, shake.frequency);
    state.envShakeAmplitude = shake.amplitude;
    state.envShakePhase = shake.phase;
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
    if (state.envShakeTicks <= 0) {
        state.envShakeTicks = 0;
        state.envShakeOffsetY = 0.0f;
        return;
    }

    const int elapsed = std::max(0, state.envShakeTotalTicks - state.envShakeTicks);
    const float progress = state.envShakeTotalTicks > 0
        ? static_cast<float>(state.envShakeTicks) / static_cast<float>(state.envShakeTotalTicks)
        : 0.0f;
    constexpr float tau = 6.28318530718f;
    const float phase = (static_cast<float>(elapsed + state.envShakePhase) * static_cast<float>(state.envShakeFrequency) / 60.0f) * tau;
    state.envShakeOffsetY = std::sin(phase) * state.envShakeAmplitude * progress;
    --state.envShakeTicks;
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
    return state.pendingMode != PendingMode::Training || state.trainingOptions.playHitSounds;
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
    state.comboCounters = {};
}

void endActiveComboForDefender(AppState& state, size_t defenderIndex) {
    if (defenderIndex >= state.comboCounters.size()) {
        return;
    }
    const size_t attackerIndex = defenderIndex == 0 ? 1 : 0;
    auto& combo = state.comboCounters[attackerIndex];
    combo.activeHits = 0;
}

void registerComboHit(AppState& state, size_t attackerIndex) {
    if (attackerIndex >= state.comboCounters.size()) {
        return;
    }

    auto& combo = state.comboCounters[attackerIndex];
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
    for (auto& combo : state.comboCounters) {
        if (combo.displayTicks <= 0) {
            combo.displayHits = 0;
            continue;
        }
        --combo.displayTicks;
        if (combo.displayTicks <= 0) {
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
        selectedStageSlot(state));
    hitDef = &resolvedHitDef;

    const bool trainingDummy = useTrainingDummyOptions(state, defenderIndex);
    GuardStance guardStance = trainingDummy
        ? chooseDummyGuardStance(state.trainingOptions, *hitDef, defender)
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
        attacker.hitPauseTicks = std::max(1, hitDef->pausetimeP1);
        const int guardDamageDone = state.trainingOptions.guardDamage
            ? scaleAttackThenDefenceDamage(hitDef->guardDamage, attacker, defender)
            : 0;
        if (!state.trainingOptions.dummyInvincible && state.trainingOptions.guardDamage) {
            defender.life = std::max(0, defender.life - guardDamageDone);
        }
        if (state.trainingOptions.dummyFrozen) {
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
        attacker.hitPauseTicks = std::max(1, hitDef->pausetimeP1);
        const int damageDone = (!trainingDummy || !state.trainingOptions.dummyInvincible)
            ? scaleAttackThenDefenceDamage(hitDef->damage, attacker, defender)
            : 0;
        if (damageDone > 0) {
            defender.life = std::max(0, defender.life - damageDone);
        }
        if (trainingDummy && state.trainingOptions.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else if (hitOverride) {
            enterState(state, defender, hitOverride->stateNo);
            defender.customHitState = true;
            defender.moveType = 'H';
            defender.ctrl = false;
            defender.hitPauseTicks = std::max(0, hitDef->pausetimeP2);
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
    state.lastHitText = hitText.str();
    state.lastHitTextTicks = 150;
    SDL_Log("%s", state.lastHitText.c_str());
}

void applyHitIfNeeded(AppState& state) {
    applyHitBetween(state, 0, 1);
    if (usesLocalP2Controls(state)) {
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
        selectedStageSlot(state));
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
        ? chooseDummyGuardStance(state.trainingOptions, hitDef, defender)
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
        owner.hitPauseTicks = std::max(1, hitDef.pausetimeP1);
        const int guardDamageDone = state.trainingOptions.guardDamage
            ? scaleAttackThenDefenceDamage(hitDef.guardDamage, owner, defender)
            : 0;
        if (!state.trainingOptions.dummyInvincible && state.trainingOptions.guardDamage) {
            defender.life = std::max(0, defender.life - guardDamageDone);
        }
        if (state.trainingOptions.dummyFrozen) {
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
        owner.hitPauseTicks = std::max(1, hitDef.pausetimeP1);
        const int damageDone = (!trainingDummy || !state.trainingOptions.dummyInvincible)
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
        if (trainingDummy && state.trainingOptions.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else if (hitOverride) {
            enterState(state, defender, hitOverride->stateNo);
            defender.customHitState = true;
            defender.moveType = 'H';
            defender.ctrl = false;
            defender.hitPauseTicks = std::max(0, hitDef.pausetimeP2);
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
    state.lastHitText = hitText.str();
    state.lastHitTextTicks = 150;
    SDL_Log("%s", state.lastHitText.c_str());
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
            const size_t defenderIndex = projectile.ownerIndex == 0 ? 1 : 0;
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

bool commandActive(const std::vector<std::string>& commands, std::string_view command) {
    return std::any_of(commands.begin(), commands.end(), [command](const std::string& active) {
        return active == command;
    });
}

void pushFighterInputFrame(FighterState& fighter, const FighterInputState& input, int tick) {
    fighter.inputHistory.push_back(CommandInputFrame{ input, tick });
    constexpr size_t maxInputHistory = 90;
    if (fighter.inputHistory.size() > maxInputHistory) {
        fighter.inputHistory.erase(fighter.inputHistory.begin(), fighter.inputHistory.end() - static_cast<std::ptrdiff_t>(maxInputHistory));
    }
}

bool buttonAtomActive(const FighterInputState& input, std::string_view symbol) {
    if (symbol == "x") {
        return input.x;
    }
    if (symbol == "y") {
        return input.y;
    }
    if (symbol == "z") {
        return input.z;
    }
    if (symbol == "a") {
        return input.a;
    }
    if (symbol == "b") {
        return input.b;
    }
    if (symbol == "c") {
        return input.c;
    }
    if (symbol == "s") {
        return input.s;
    }
    return false;
}

bool buttonAtomSymbol(std::string_view symbol) {
    return symbol == "x"
        || symbol == "y"
        || symbol == "z"
        || symbol == "a"
        || symbol == "b"
        || symbol == "c"
        || symbol == "s";
}

bool cardinalDirectionAtomSymbol(std::string_view symbol) {
    return symbol == "F" || symbol == "B" || symbol == "D" || symbol == "U";
}

bool directionAtomActive(const FighterInputState& input, std::string_view symbol, int facing, bool broadDirection) {
    const bool forward = facing >= 0 ? input.right : input.left;
    const bool back = facing >= 0 ? input.left : input.right;
    if (broadDirection) {
        if (symbol == "F") {
            return forward;
        }
        if (symbol == "B") {
            return back;
        }
        if (symbol == "D") {
            return input.down;
        }
        if (symbol == "U") {
            return input.up;
        }
    }

    if (symbol == "F") {
        return forward && !input.down && !input.up;
    }
    if (symbol == "B") {
        return back && !input.down && !input.up;
    }
    if (symbol == "D") {
        return input.down && !forward && !back;
    }
    if (symbol == "U") {
        return input.up && !forward && !back;
    }
    if (symbol == "DF") {
        return input.down && forward;
    }
    if (symbol == "DB") {
        return input.down && back;
    }
    if (symbol == "UF") {
        return input.up && forward;
    }
    if (symbol == "UB") {
        return input.up && back;
    }
    return false;
}

bool commandAtomBaseActive(const FighterInputState& input, const CommandAtom& atom, int facing, bool broadDirection) {
    if (buttonAtomActive(input, atom.symbol)) {
        return true;
    }
    return directionAtomActive(input, atom.symbol, facing, broadDirection);
}

bool commandAtomReleased(
    const std::vector<CommandInputFrame>& history,
    size_t index,
    const CommandAtom& atom,
    int facing,
    bool broadDirection) {
    if (index == 0) {
        return false;
    }
    return commandAtomBaseActive(history[index - 1].input, atom, facing, broadDirection)
        && !commandAtomBaseActive(history[index].input, atom, facing, broadDirection);
}

bool commandAtomActive(const FighterInputState& input, const CommandAtom& atom, int facing, bool terminalButtonStepLeniency = false) {
    const bool broadDirection = atom.broadDirection
        || (terminalButtonStepLeniency && !atom.hold && cardinalDirectionAtomSymbol(atom.symbol));
    return commandAtomBaseActive(input, atom, facing, broadDirection);
}

bool commandAtomFresh(
    const std::vector<CommandInputFrame>& history,
    size_t index,
    const CommandAtom& atom,
    int facing,
    bool terminalButtonStepLeniency = false) {
    if (!commandAtomActive(history[index].input, atom, facing, terminalButtonStepLeniency)) {
        const bool broadDirection = atom.broadDirection
            || (terminalButtonStepLeniency && !atom.hold && cardinalDirectionAtomSymbol(atom.symbol));
        if (!atom.release || !commandAtomReleased(history, index, atom, facing, broadDirection)) {
            return false;
        }
    }
    if (atom.hold) {
        return true;
    }
    if (atom.release && buttonAtomSymbol(atom.symbol)) {
        const bool broadDirection = atom.broadDirection
            || (terminalButtonStepLeniency && cardinalDirectionAtomSymbol(atom.symbol));
        return commandAtomReleased(history, index, atom, facing, broadDirection);
    }
    if (index == 0) {
        return true;
    }
    return !commandAtomActive(history[index - 1].input, atom, facing, terminalButtonStepLeniency);
}

bool commandStepMatches(const std::vector<CommandInputFrame>& history, size_t index, const CommandStep& step, int facing) {
    bool hasPressAtom = false;
    bool hasFreshPressAtom = false;
    const bool terminalButtonStepLeniency = std::any_of(step.atoms.begin(), step.atoms.end(), [](const CommandAtom& atom) {
        return buttonAtomSymbol(atom.symbol) && !atom.hold;
    });
    for (const auto& atom : step.atoms) {
        const bool broadDirection = atom.broadDirection
            || (terminalButtonStepLeniency && !atom.hold && cardinalDirectionAtomSymbol(atom.symbol));
        const bool active = commandAtomActive(history[index].input, atom, facing, terminalButtonStepLeniency);
        const bool released = atom.release && commandAtomReleased(history, index, atom, facing, broadDirection);
        if (!active && !released) {
            return false;
        }
        if (!atom.hold) {
            hasPressAtom = true;
            if (released || commandAtomFresh(history, index, atom, facing, terminalButtonStepLeniency)) {
                hasFreshPressAtom = true;
            }
        }
    }
    return !hasPressAtom || hasFreshPressAtom;
}

bool commandDefinitionActive(const CommandDefinition& definition, const FighterState& fighter) {
    if (definition.steps.empty() || fighter.inputHistory.empty()) {
        return false;
    }

    int searchIndex = static_cast<int>(fighter.inputHistory.size()) - 1;
    int newestTick = fighter.inputHistory.back().tick;
    int lastMatchedTick = -1;
    for (int stepIndex = static_cast<int>(definition.steps.size()) - 1; stepIndex >= 0; --stepIndex) {
        bool matched = false;
        for (int frameIndex = searchIndex; frameIndex >= 0; --frameIndex) {
            const int elapsed = newestTick - fighter.inputHistory[static_cast<size_t>(frameIndex)].tick;
            if (stepIndex == static_cast<int>(definition.steps.size()) - 1 && elapsed > definition.bufferTime) {
                break;
            }
            if (elapsed > definition.maxTime) {
                break;
            }
            if (commandStepMatches(fighter.inputHistory, static_cast<size_t>(frameIndex), definition.steps[static_cast<size_t>(stepIndex)], fighter.facing)) {
                matched = true;
                if (lastMatchedTick < 0) {
                    lastMatchedTick = fighter.inputHistory[static_cast<size_t>(frameIndex)].tick;
                }
                searchIndex = frameIndex - 1;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    return lastMatchedTick >= 0 && newestTick - lastMatchedTick <= definition.bufferTime;
}

void appendBufferedCommands(std::vector<std::string>& commands, const FighterState& fighter, const std::vector<CommandDefinition>& definitions) {
    for (const auto& definition : definitions) {
        if (commandDefinitionActive(definition, fighter)) {
            addUniqueCommand(commands, definition.name);
        }
    }
}

std::vector<std::string> collectFighterCommands(
    const FighterInputState& input,
    const FighterState& fighter,
    const std::vector<CommandDefinition>& definitions) {
    std::vector<std::string> commands;
    if (input.x) {
        commands.push_back("x");
    }
    if (input.y) {
        commands.push_back("y");
    }
    if (input.z) {
        commands.push_back("z");
    }
    if (input.a) {
        commands.push_back("a");
    }
    if (input.b) {
        commands.push_back("b");
    }
    if (input.c) {
        commands.push_back("c");
    }
    if (input.down) {
        commands.push_back("holddown");
    }
    if (input.up) {
        commands.push_back("holdup");
    }

    const bool holdingLeft = input.left;
    const bool holdingRight = input.right;
    if ((fighter.facing >= 0 && holdingRight) || (fighter.facing < 0 && holdingLeft)) {
        commands.push_back("holdfwd");
    }
    if ((fighter.facing >= 0 && holdingLeft) || (fighter.facing < 0 && holdingRight)) {
        commands.push_back("holdback");
    }
    appendBufferedCommands(commands, fighter, definitions);
    return commands;
}

int commandConditionValue(const FighterState& fighter, CommandConditionSubject subject) {
    switch (subject) {
    case CommandConditionSubject::StateNo:
        return fighter.stateNo;
    case CommandConditionSubject::Time:
        return fighter.stateTime;
    case CommandConditionSubject::Power:
        return fighter.power;
    case CommandConditionSubject::RoundState:
        return 2;
    case CommandConditionSubject::AiLevel:
        return 0;
    default:
        return 0;
    }
}

std::string replaceCommandComparisonsInExpression(std::string expression, const std::vector<std::string>& commands) {
    size_t searchFrom = 0;
    while (true) {
        const std::string lowered = lowercaseCopy(expression);
        const size_t commandPos = lowered.find("command", searchFrom);
        if (commandPos == std::string::npos) {
            break;
        }

        size_t opPos = commandPos + std::string_view("command").size();
        while (opPos < expression.size() && std::isspace(static_cast<unsigned char>(expression[opPos]))) {
            ++opPos;
        }

        bool negate = false;
        if (opPos + 1 < expression.size() && expression[opPos] == '!' && expression[opPos + 1] == '=') {
            negate = true;
            opPos += 2;
        } else if (opPos < expression.size() && expression[opPos] == '=') {
            ++opPos;
        } else {
            searchFrom = commandPos + 1;
            continue;
        }

        while (opPos < expression.size() && std::isspace(static_cast<unsigned char>(expression[opPos]))) {
            ++opPos;
        }
        if (opPos >= expression.size() || expression[opPos] != '"') {
            searchFrom = commandPos + 1;
            continue;
        }

        const size_t quoteStart = opPos + 1;
        const size_t quoteEnd = expression.find('"', quoteStart);
        if (quoteEnd == std::string::npos) {
            break;
        }

        const std::string commandName = expression.substr(quoteStart, quoteEnd - quoteStart);
        const bool active = commandActive(commands, commandName);
        const std::string replacement = (negate ? !active : active) ? "1" : "0";
        expression.replace(commandPos, quoteEnd + 1 - commandPos, replacement);
        searchFrom = commandPos + replacement.size();
    }
    return expression;
}

std::optional<int> resolveCommandTargetState(
    const AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const CommandStateEntry& entry,
    const std::vector<std::string>& commands) {
    std::string expression = entry.targetStateExpression.empty()
        ? std::to_string(entry.targetState)
        : entry.targetStateExpression;
    expression = replaceCommandComparisonsInExpression(std::move(expression), commands);
    const auto value = evalMugenExpression(state, fighter, expression, opponent, nullptr);
    if (!value) {
        return std::nullopt;
    }
    return static_cast<int>(std::lround(*value));
}

bool canEnterCommandState(
    const AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const CommandStateEntry& entry,
    const std::vector<std::string>& commands) {
    for (const auto& required : entry.requiredCommands) {
        if (!commandActive(commands, required)) {
            return false;
        }
    }
    for (const auto& optionGroup : entry.commandOptionGroups) {
        bool matched = false;
        for (const auto& option : optionGroup) {
            if (commandActive(commands, option)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    for (const auto& forbidden : entry.forbiddenCommands) {
        if (commandActive(commands, forbidden)) {
            return false;
        }
    }
    if (entry.requiredStateType != 0 && fighter.stateType != entry.requiredStateType) {
        return false;
    }
    for (const char forbiddenStateType : entry.forbiddenStateTypes) {
        if (fighter.stateType == forbiddenStateType) {
            return false;
        }
    }
    if (entry.requiresCtrl && !fighter.ctrl) {
        return false;
    }
    if (entry.requiresMoveContact && !fighter.moveContact) {
        return false;
    }
    for (const auto& condition : entry.intConditions) {
        if (!compareIntValue(commandConditionValue(fighter, condition.subject), condition.op, condition.value)) {
            return false;
        }
    }
    for (const auto& condition : entry.intRangeConditions) {
        const int value = commandConditionValue(fighter, condition.subject);
        const bool inRange = value >= condition.minValue && value <= condition.maxValue;
        if (condition.op == CompareOp::Equal && !inRange) {
            return false;
        }
        if (condition.op == CompareOp::NotEqual && inRange) {
            return false;
        }
    }
    for (const auto& condition : entry.expressionConditions) {
        if (!evalMugenExpressionCondition(state, fighter, condition, opponent, nullptr)) {
            return false;
        }
    }
    for (const auto& expression : entry.booleanExpressions) {
        const auto value = evalMugenExpression(state, fighter, expression, opponent, nullptr);
        if (!value || *value == 0.0f) {
            return false;
        }
    }

    const auto targetState = resolveCommandTargetState(state, fighter, opponent, entry, commands);
    if (!targetState) {
        return false;
    }
    const StateDefinition* stateDef = findStateDefinition(state, *targetState);
    return stateDef && (!stateDef->hasAnim || findExactClip(state, stateDef->anim));
}

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
        } else {
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

    if (target.hitFall && target.stateNo == 5070 && target.stateTime >= 8) {
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

    if (fighter.hitPauseTicks > 0) {
        fighter.vx = 0.0f;
        --fighter.hitPauseTicks;
        return;
    }

    if (fighter.guarding) {
        updateGroundGuardState(state, fighter);
        return;
    }

    if (fighter.moveType == 'H' && !fighter.customHitState) {
        updateGroundGetHitState(state, fighter);
        return;
    }
    if (fighter.moveType == 'H' && fighter.customHitState) {
        return;
    }

    updateStateZeroFromMovement(state, fighter);
    const bool holdingDown = input.down && fighter.onGround;
    updatePlayerCrouchInput(state, fighter, holdingDown);
    const auto commands = collectFighterCommands(input, fighter, state.commandDefinitions);
    const bool changedStateFromCommand = applyCommandState(state, fighter, opponent, commands);
    const bool movementLocked = fighterHasAssertSpecialFlag(fighter, "nowalk");
    const bool holdingHorizontal = input.left != input.right;
    const int heldWalkAction = ((fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left)) ? 20 : 21;

    if (!changedStateFromCommand && fighter.stateNo == 20 && (!holdingHorizontal || holdingDown || !fighter.ctrl)) {
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
            if (!changedStateFromCommand && !movementLocked && !holdingDown && fighter.ctrl && holdingHorizontal) {
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
        if (!changedStateFromCommand && !movementLocked && !holdingDown && fighter.ctrl && input.up && fighter.onGround) {
            const int heldDirection = input.left == input.right ? 0 : (input.left ? -1 : 1);
            fighter.vy = -7.8f;
            fighter.onGround = false;
            fighter.jumpBaseAction =
                heldDirection == 0 ? 41 : (heldDirection * fighter.facing > 0 ? 42 : 43);
            fighter.jumpPeakActionApplied = false;
        }
    }
}

void updateTrainingDummy(AppState& state, FighterState& dummy) {
    if (state.trainingOptions.dummyFrozen) {
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
        const GuardStance idleGuard = dummyGuardIdleStance(state.trainingOptions.dummyGuardMode);
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

void updateCpuOpponent(AppState& state, FighterState& opponent) {
    if (opponent.guarding) {
        updateGroundGuardState(state, opponent);
    } else if (opponent.moveType == 'H' && opponent.customHitState) {
        return;
    } else if (opponent.moveType == 'H') {
        updateGroundGetHitState(state, opponent);
    } else {
        opponent.vx = 0.0f;
        opponent.vy = 0.0f;
        opponent.y = 0.0f;
        opponent.onGround = true;
        if (opponent.stateNo == 130 || opponent.stateNo == 131) {
            enterState(state, opponent, 0);
        }
    }
}

std::string fighterResultName(const AppState& state, int winner) {
    switch (winner) {
    case 1:
        return selectedCharacterName(state);
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
    if (state.roundWins[0] == matchWinsRequired(state) - 1
        && state.roundWins[1] == matchWinsRequired(state) - 1) {
        return "FINAL ROUND";
    }
    return "ROUND " + std::to_string(state.currentRound);
}

int matchWinner(const AppState& state) {
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
        state.lastHitText = roundFinishCalloutText(state);
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
    state.lastHitText = roundResultText(state);
    state.lastHitTextTicks = singleFightRoundResultHoldTicks(state);
}

void startSingleFightRoundFinish(AppState& state, int winner, RoundEndReason reason) {
    state.matchPhase = MatchPhase::RoundFinish;
    state.matchPhaseTicks = 0;
    state.roundWinner = winner;
    state.roundEndReason = reason;
    state.roundScoreApplied = false;
    state.roundPoseApplied = false;
    state.matchComplete = false;
    state.lastHitText = roundFinishCalloutText(state);
    state.lastHitTextTicks = singleFightRoundFinishHoldTicks(state);
}

void updateSingleFightRules(AppState& state) {
    if (!isMatchMode(state) || state.matchPhase != MatchPhase::Fight) {
        return;
    }

    if (state.matchTimerTicks > 0 && !anyFighterHasAssertSpecialFlag(state, "timerfreeze")) {
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
    } else if (state.matchTimerTicks <= 0) {
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
    if (state.lastHitTextTicks > 0) {
        --state.lastHitTextTicks;
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
                state.selectedMatchResultOption = 0;
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

void updateFight(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state) ? *selectedStageSlot(state) : fallbackStage;
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
    } else if (activeOpponentType(state) == OpponentType::Dummy) {
        updateTrainingDummy(state, p2);
    } else {
        updateCpuOpponent(state, p2);
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
    if (state.pendingMode == PendingMode::Training
        && activeOpponentType(state) == OpponentType::Dummy
        && (state.trainingOptions.dummyAutoLife || state.trainingOptions.dummyInvincible)) {
        p2.life = 1000;
    }
    updateSingleFightRules(state);
    updateCamera(state, stage);
    applyScreenBounds(state, stage);
    if (state.lastHitTextTicks > 0) {
        --state.lastHitTextTicks;
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

void drawStageSelect(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    setColor(renderer, 36, 34, 30);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    setColor(renderer, 24, 30, 38);
    fillRect(renderer, 0, 176, widthF, 64);
    setColor(renderer, 94, 78, 54);
    fillRect(renderer, 0, 174, widthF, 1);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 220, 178, 112);
    debugText(renderer, 20, 30, "STAGE SELECT");
    setColor(renderer, 155, 164, 174);
    debugText(renderer, 210, 30, "2/2 ARENA");

    drawPanel(renderer, 18, 52, 284, 108);

    if (state.stages.empty()) {
        setColor(renderer, 230, 130, 120);
        debugText(renderer, 32, 72, "No stages found in game/stages");
    } else {
        for (int i = 0; i < static_cast<int>(state.stages.size()); ++i) {
            const float y = 66.0f + static_cast<float>(i * 20);
            const auto& stage = state.stages[static_cast<size_t>(i)];
            if (i == state.selectedStage) {
                const int pulse = 150 + ((state.frame / 8) % 55);
                setColor(renderer, static_cast<Uint8>(pulse), 124, 58);
                fillRect(renderer, 28, y - 3, 140, 15);
                setColor(renderer, 8, 12, 16);
                debugText(renderer, 36, y, stage.displayName);
            } else {
                setColor(renderer, 184, 178, 168);
                debugText(renderer, 36, y, stage.displayName);
            }
        }

        const auto& selected = state.stages[static_cast<size_t>(state.selectedStage)];
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 182, 68, selected.displayName);
        setColor(renderer, 155, 164, 174);
        debugText(renderer, 182, 84, "id: " + selected.id);
        debugText(renderer, 182, 96, "author: " + selected.author);
        debugText(renderer, 182, 116, "fighter: " + selectedCharacterName(state));
        debugText(renderer, 182, 132, "opponent: " + compactSettingText(opponentDisplayName(state), 11));
    }

    setColor(renderer, 118, 126, 138);
    debugText(renderer, 20, 204, "UP/DOWN choose  ENTER start  ESC fighter select");
    SDL_RenderPresent(renderer);
}

void drawVersusScreen(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    setColor(renderer, 28, 26, 36);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    setColor(renderer, 42, 35, 52);
    fillRect(renderer, 0, 100, widthF, 42);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 20, 30, std::string(pendingModeTitle(state.pendingMode)) + " VS");

    drawPanel(renderer, 22, 62, 108, 96);
    drawPanel(renderer, widthF - 130.0f, 62, 108, 96);

    const TextureSprite* p1Portrait = state.characterLargePortrait.texture
        ? &state.characterLargePortrait
        : spriteAt(state.characterFaceSprites, sessionP1CharacterIndex(state));

    if (p1Portrait) {
        const float scale = std::min({ 1.0f, 92.0f / static_cast<float>(p1Portrait->width), 96.0f / static_cast<float>(p1Portrait->height) });
        drawSpriteTopLeft(
            renderer,
            *p1Portrait,
            76.0f - static_cast<float>(p1Portrait->width) * scale * 0.5f,
            76,
            scale);
    } else {
        setColor(renderer, 70, 132, 190);
        fillRect(renderer, 50, 86, 52, 48);
    }
    drawFixedOpponentSlot(renderer, widthF - 122.0f, 72, 76, 76, opponentSlotLabel(state));

    setColor(renderer, 222, 226, 232);
    debugText(renderer, 34, 144, compactSettingText(selectedCharacterName(state), 13));
    debugText(renderer, widthF - 100.0f, 144, compactSettingText(opponentDisplayName(state), 10));

    setColor(renderer, 230, 130, 120);
    debugTextCentered(renderer, centerX, 98, "VS");

    setColor(renderer, 155, 164, 174);
    debugText(renderer, 20, 184, "Stage: " + compactSettingText(selectedStageName(state), 26));
    if (state.fightSessionLoadFailed) {
        setColor(renderer, 230, 130, 120);
        debugText(renderer, 20, 204, "Load failed. ESC stage select");
    } else if (state.fightSessionPrepared) {
        debugText(renderer, 20, 204, "Ready. ENTER start now");
        debugText(renderer, 20, 216, "Auto-start after a short pause");
    } else {
        debugText(renderer, 20, 204, "Loading fighter and stage...");
        debugText(renderer, 20, 216, "Please wait");
    }

    SDL_RenderPresent(renderer);
}

bool hasSelectedStageBackground(const AppState& state) {
    return !state.stageBackground.empty();
}

void drawStageLayer(SDL_Renderer* renderer, const AppState& state, int layerNo) {
    if (!hasSelectedStageBackground(state)) {
        return;
    }

    for (const auto& element : state.stageBackground) {
        if (!element.sprite.texture || element.layerNo != layerNo) {
            continue;
        }

        const float baseX = screenCenterX(state)
            + element.x
            - static_cast<float>(element.sprite.axisX)
            - state.cameraX * element.deltaX;
        const float baseY = element.y
            - static_cast<float>(element.sprite.axisY)
            - state.cameraY * element.deltaY;
        const int repeatX = element.tileX ? 6 : 1;
        const int repeatY = element.tileY ? 3 : 1;
        const float firstX = element.tileX ? baseX - static_cast<float>(element.sprite.width * 2) : baseX;
        const float firstY = element.tileY ? baseY - static_cast<float>(element.sprite.height) : baseY;

        for (int ty = 0; ty < repeatY; ++ty) {
            for (int tx = 0; tx < repeatX; ++tx) {
                SDL_FRect dst{
                    firstX + static_cast<float>(tx * element.sprite.width),
                    firstY + static_cast<float>(ty * element.sprite.height),
                    static_cast<float>(element.sprite.width),
                    static_cast<float>(element.sprite.height),
                };
                SDL_RenderTexture(renderer, element.sprite.texture, nullptr, &dst);
            }
        }
    }
}

void drawFallbackStage(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, float cameraY) {
    const float widthF = logicalWidthF(state);
    setColor(renderer, 74, 100, 128);
    fillRect(renderer, 0, 0, widthF, 96);
    setColor(renderer, 86, 112, 88);
    fillRect(renderer, 0, 96, widthF, 54);
    setColor(renderer, 82, 62, 44);
    fillRect(renderer, 0, 150, widthF, 90);
    setColor(renderer, 58, 46, 38);
    fillRect(renderer, 0, stage.zoffset - cameraY, widthF, 10);
}

int paletteEffectSinValue(int value, int period, int elapsedTicks) {
    if (value == 0 || period <= 0) {
        return 0;
    }
    constexpr float tau = 6.28318530718f;
    return static_cast<int>(std::lround(std::sin((static_cast<float>(elapsedTicks) / static_cast<float>(period)) * tau) * static_cast<float>(value)));
}

std::array<int, 3> paletteEffectAdd(const ActivePaletteEffect& effect) {
    if (effect.ticksLeft <= 0) {
        return { 0, 0, 0 };
    }
    return {
        effect.spec.addR + paletteEffectSinValue(effect.spec.sinAddR, effect.spec.sinPeriod, effect.elapsedTicks),
        effect.spec.addG + paletteEffectSinValue(effect.spec.sinAddG, effect.spec.sinPeriod, effect.elapsedTicks),
        effect.spec.addB + paletteEffectSinValue(effect.spec.sinAddB, effect.spec.sinPeriod, effect.elapsedTicks),
    };
}

std::array<Uint8, 3> paletteEffectColorMod(const ActivePaletteEffect& effect) {
    if (effect.ticksLeft <= 0) {
        return { 255, 255, 255 };
    }

    auto channel = [](int mul, int add) -> Uint8 {
        const int value = (255 * std::max(0, mul)) / 256 + std::min(0, add);
        return static_cast<Uint8>(std::clamp(value, 0, 255));
    };

    const auto add = paletteEffectAdd(effect);
    return {
        channel(effect.spec.mulR, add[0]),
        channel(effect.spec.mulG, add[1]),
        channel(effect.spec.mulB, add[2]),
    };
}

void drawPaletteOverlay(SDL_Renderer* renderer, const AppState& state, const ActivePaletteEffect& effect, int alphaLimit) {
    if (effect.ticksLeft <= 0) {
        return;
    }
    const auto add = paletteEffectAdd(effect);
    const int alpha = std::clamp(std::max({ std::abs(add[0]), std::abs(add[1]), std::abs(add[2]) }), 0, alphaLimit);
    if (alpha <= 0) {
        return;
    }
    setColor(
        renderer,
        static_cast<Uint8>(std::clamp(std::max(0, add[0]), 0, 255)),
        static_cast<Uint8>(std::clamp(std::max(0, add[1]), 0, 255)),
        static_cast<Uint8>(std::clamp(std::max(0, add[2]), 0, 255)),
        static_cast<Uint8>(alpha));
    fillRect(renderer, 0, 0, logicalWidthF(state), static_cast<float>(kLogicalHeight));
}

void drawActor(SDL_Renderer* renderer, const AppState& state, const FighterState& fighter, size_t actorIndex) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state) ? *selectedStageSlot(state) : fallbackStage;
    if (fighterHasAssertSpecialFlag(fighter, "invisible")) {
        return;
    }

    auto drawActorSprite = [&](int action, int animTick, float x, float y, int facing, int alpha, bool additive, const ActivePaletteEffect* palette) -> bool {
        const AnimationClip* clip = findClip(state, action);
        const AnimationFrame* frame = clip ? frameForClip(*clip, animTick) : nullptr;
        if (!frame || !frame->sprite.texture) {
            return false;
        }

        const float originX = screenCenterX(state) + x - state.cameraX;
        const float originY = stage.zoffset + y - state.cameraY;
        const bool facingLeft = facing < 0;
        const bool flipH = frame->flipX != facingLeft;
        const float drawX = facingLeft
            ? originX - static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.width - frame->sprite.axisX)
            : originX + static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX);
        const float drawY = originY + static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY);

        SDL_FRect dst{
            drawX,
            drawY,
            static_cast<float>(frame->sprite.width),
            static_cast<float>(frame->sprite.height),
        };
        int flipMode = SDL_FLIP_NONE;
        if (flipH) {
            flipMode |= SDL_FLIP_HORIZONTAL;
        }
        if (frame->flipY) {
            flipMode |= SDL_FLIP_VERTICAL;
        }
        SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
        Uint8 previousR = 255;
        Uint8 previousG = 255;
        Uint8 previousB = 255;
        Uint8 previousA = 255;
        SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
        SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
        SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);

        std::array<Uint8, 3> colorMod{ 255, 255, 255 };
        if (palette) {
            colorMod = paletteEffectColorMod(*palette);
        }
        SDL_SetTextureBlendMode(frame->sprite.texture, additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(frame->sprite.texture, colorMod[0], colorMod[1], colorMod[2]);
        SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(std::clamp(alpha, 0, 255)));
        SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));

        SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
        SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
        SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
        return true;
    };

    if (fighter.afterImage.active || !fighter.afterImage.trail.empty()) {
        for (const auto& snapshot : fighter.afterImage.trail) {
            const int alpha = std::clamp(150 - snapshot.ageTicks * 10, 20, 150);
            drawActorSprite(
                snapshot.action,
                snapshot.animTick,
                snapshot.x,
                snapshot.y,
                snapshot.facing,
                alpha,
                fighter.afterImage.additive,
                nullptr);
        }
    }

    const AnimationClip* clip = findClip(state, fighter.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, fighter.animTick) : nullptr;
    const bool drawHitFeedback = (state.pendingMode != PendingMode::Training || state.trainingOptions.showHitFlash)
        && (fighter.moveType == 'H' || fighter.hitPauseTicks > 0);

    if (frame && frame->sprite.texture) {
        const float originX = screenCenterX(state) + fighter.x - state.cameraX;
        const float originY = stage.zoffset + fighter.y - state.cameraY;
        const float displayOriginX = originX + fighter.displayOffsetX * static_cast<float>(fighter.facing);
        const float displayOriginY = originY + fighter.displayOffsetY;
        const bool facingLeft = fighter.facing < 0;
        const bool flipH = frame->flipX != facingLeft;
        const float drawX = facingLeft
            ? displayOriginX - static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.width - frame->sprite.axisX)
            : displayOriginX + static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX);
        const float drawY = displayOriginY + static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY);

        SDL_FRect dst{
            drawX,
            drawY,
            static_cast<float>(frame->sprite.width),
            static_cast<float>(frame->sprite.height),
        };
        int flipMode = SDL_FLIP_NONE;
        if (flipH) {
            flipMode |= SDL_FLIP_HORIZONTAL;
        }
        if (frame->flipY) {
            flipMode |= SDL_FLIP_VERTICAL;
        }
        const double angle = fighter.angleDrawActive
            ? static_cast<double>(fighter.facing >= 0 ? -fighter.drawAngle : fighter.drawAngle)
            : 0.0;
        SDL_FPoint rotationCenter{
            originX - drawX,
            originY - drawY,
        };
        SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
        Uint8 previousR = 255;
        Uint8 previousG = 255;
        Uint8 previousB = 255;
        SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
        SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
        Uint8 previousA = 255;
        SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);
        const auto colorMod = paletteEffectColorMod(fighter.paletteEffect);
        const bool additiveTrans = fighter.transEffect.active
            && (fighter.transEffect.mode == ActorBlendMode::Add || fighter.transEffect.mode == ActorBlendMode::AddAlpha);
        const int sourceAlpha = fighter.transEffect.active ? fighter.transEffect.alphaSource : 256;
        SDL_SetTextureBlendMode(frame->sprite.texture, additiveTrans ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(frame->sprite.texture, colorMod[0], colorMod[1], colorMod[2]);
        SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(std::clamp(sourceAlpha, 0, 256) * 255 / 256));
        SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
        const auto add = paletteEffectAdd(fighter.paletteEffect);
        const int additiveAlpha = std::clamp(std::max({ add[0], add[1], add[2] }), 0, 180);
        if (additiveAlpha > 0) {
            SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_ADD);
            SDL_SetTextureColorMod(
                frame->sprite.texture,
                static_cast<Uint8>(std::clamp(add[0], 0, 255)),
                static_cast<Uint8>(std::clamp(add[1], 0, 255)),
                static_cast<Uint8>(std::clamp(add[2], 0, 255)));
            SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
        }
        SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
        SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
        SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
        if (drawHitFeedback) {
            setColor(renderer, 255, 245, 170);
            drawRect(renderer, dst.x - 1.0f, dst.y - 1.0f, dst.w + 2.0f, dst.h + 2.0f);
        }
        return;
    }

    const float originX = screenCenterX(state) + fighter.x - state.cameraX;
    const float originY = (selectedStageSlot(state) ? selectedStageSlot(state)->zoffset : 168.0f) + fighter.y - state.cameraY;
    if (actorIndex == 0) {
        setColor(renderer, 62, 118, 184);
    } else {
        setColor(renderer, 218, 174, 100);
    }
    fillRect(renderer, originX - 14.0f, originY - 58.0f, 28, 58);
    fillRect(renderer, originX - 22.0f, originY - 38.0f, 44, 12);
    if (drawHitFeedback) {
        setColor(renderer, 255, 245, 170);
        drawRect(renderer, originX - 22.0f, originY - 58.0f, 44.0f, 58.0f);
    }
}

void drawFighter(SDL_Renderer* renderer, const AppState& state, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }
    drawActor(renderer, state, state.fighters[fighterIndex], fighterIndex);
}

void drawRuntimeEffect(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, const RuntimeEffect& effect) {
    const AnimationClip* clip = effect.fromFightFx ? findFightFxClip(state, effect.action) : findExactClip(state, effect.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, effect.animTick) : nullptr;
    if (!frame || !frame->sprite.texture) {
        return;
    }

    SDL_SetTextureBlendMode(frame->sprite.texture, frame->additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    const float originX = screenCenterX(state) + effect.x - state.cameraX;
    const float originY = stage.zoffset + effect.y - state.cameraY;
    SDL_FRect dst{
        originX + (static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX)) * effect.scaleX,
        originY + (static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY)) * effect.scaleY,
        static_cast<float>(frame->sprite.width) * effect.scaleX,
        static_cast<float>(frame->sprite.height) * effect.scaleY,
    };

    int flipMode = SDL_FLIP_NONE;
    if (frame->flipX) {
        flipMode |= SDL_FLIP_HORIZONTAL;
    }
    if (frame->flipY) {
        flipMode |= SDL_FLIP_VERTICAL;
    }
    SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
}

void drawRuntimeProjectile(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, const RuntimeProjectile& projectile) {
    const AnimationClip* clip = findExactClip(state, projectile.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, projectile.animTick) : nullptr;
    if (!frame || !frame->sprite.texture) {
        return;
    }

    const float originX = screenCenterX(state) + projectile.x - state.cameraX;
    const float originY = stage.zoffset + projectile.y - state.cameraY;
    const bool facingLeft = projectile.facing < 0;
    const bool flipH = frame->flipX != facingLeft;
    const float drawX = facingLeft
        ? originX - static_cast<float>(frame->offsetX) * projectile.scaleX - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * projectile.scaleX
        : originX + static_cast<float>(frame->offsetX) * projectile.scaleX - static_cast<float>(frame->sprite.axisX) * projectile.scaleX;
    const float drawY = originY + static_cast<float>(frame->offsetY) * projectile.scaleY - static_cast<float>(frame->sprite.axisY) * projectile.scaleY;
    SDL_FRect dst{
        drawX,
        drawY,
        static_cast<float>(frame->sprite.width) * projectile.scaleX,
        static_cast<float>(frame->sprite.height) * projectile.scaleY,
    };

    int flipMode = SDL_FLIP_NONE;
    if (flipH) {
        flipMode |= SDL_FLIP_HORIZONTAL;
    }
    if (frame->flipY) {
        flipMode |= SDL_FLIP_VERTICAL;
    }
    SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
    Uint8 previousR = 255;
    Uint8 previousG = 255;
    Uint8 previousB = 255;
    Uint8 previousA = 255;
    SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
    SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
    SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);

    if (projectile.shadowEnabled && projectile.y < 0.0f) {
        SDL_FRect shadowDst{
            dst.x,
            stage.zoffset - state.cameraY - 3.0f,
            dst.w,
            std::max(2.0f, dst.h * 0.18f),
        };
        SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(
            frame->sprite.texture,
            static_cast<Uint8>(std::clamp(255 - projectile.shadowR, 0, 255)),
            static_cast<Uint8>(std::clamp(255 - projectile.shadowG, 0, 255)),
            static_cast<Uint8>(std::clamp(255 - projectile.shadowB, 0, 255)));
        SDL_SetTextureAlphaMod(frame->sprite.texture, 96);
        SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &shadowDst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
    }

    SDL_SetTextureBlendMode(frame->sprite.texture, frame->additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(frame->sprite.texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(frame->sprite.texture, 255);
    SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));

    SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
    SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
    SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
}

void drawWorldActors(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    struct DrawItem {
        int priority = 0;
        int kind = 0;
        size_t index = 0;
    };

    std::vector<DrawItem> items;
    items.reserve(state.fighters.size() + state.helpers.size() + state.runtimeEffects.size() + state.projectiles.size());
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        items.push_back(DrawItem{ state.fighters[i].sprPriority, 0, i });
    }
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        if (!state.helpers[i].destroyRequested) {
            items.push_back(DrawItem{ state.helpers[i].sprPriority, 1, i });
        }
    }
    for (size_t i = 0; i < state.projectiles.size(); ++i) {
        items.push_back(DrawItem{ 3, 3, i });
    }
    if (state.pendingMode != PendingMode::Training || state.trainingOptions.showHitSparks) {
        for (size_t i = 0; i < state.runtimeEffects.size(); ++i) {
            items.push_back(DrawItem{ state.runtimeEffects[i].sprPriority, 2, i });
        }
    }

    std::stable_sort(items.begin(), items.end(), [](const DrawItem& lhs, const DrawItem& rhs) {
        return lhs.priority < rhs.priority;
    });

    for (const auto& item : items) {
        if (item.kind == 3) {
            drawRuntimeProjectile(renderer, state, stage, state.projectiles[item.index]);
        } else if (item.kind == 2) {
            drawRuntimeEffect(renderer, state, stage, state.runtimeEffects[item.index]);
        } else if (item.kind == 1) {
            drawActor(renderer, state, state.helpers[item.index], state.helpers[item.index].ownerIndex >= 0 ? static_cast<size_t>(state.helpers[item.index].ownerIndex) : 0);
        } else {
            drawFighter(renderer, state, item.index);
        }
    }
}

#include "TrainingDebugOverlay.h"

std::string fitDebugText(const std::string& value, size_t maxChars) {
    if (value.size() <= maxChars) {
        return value;
    }
    if (maxChars <= 1) {
        return value.substr(0, maxChars);
    }
    return value.substr(0, maxChars - 1) + "~";
}

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
        if (!commandEntryMatchesMoveCategory(entry, state.trainingOptions.moveCategory)) {
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

const CommandStateEntry* activeCommandEntry(
    const AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const std::vector<std::string>& commands) {
    for (const auto& entry : state.commandEntries) {
        if (canEnterCommandState(state, fighter, opponent, entry, commands)) {
            return &entry;
        }
    }
    return nullptr;
}

std::string commandEntryTargetLabel(const CommandStateEntry& entry) {
    if (const auto literalTarget = parsePlainIntValue(entry.targetStateExpression)) {
        return std::to_string(*literalTarget);
    }
    return fitDebugText(entry.targetStateExpression.empty() ? std::to_string(entry.targetState) : entry.targetStateExpression, 17);
}

#include "TrainingCommandOverlay.h"

#include "TrainingOptionsOverlay.h"

std::string_view matchResultLabel(int option) {
    static constexpr std::array<std::string_view, kMatchResultOptionCount> labels{
        "REMATCH",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMatchResultOptionCount - 1))];
}

std::string singleFightStatusLine(const AppState& state) {
    if (state.matchPhase == MatchPhase::RoundStart) {
        return roundStartCalloutText(state);
    }
    if (state.matchPhase == MatchPhase::RoundFinish) {
        if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
            return roundResultText(state);
        }
        return roundFinishCalloutText(state);
    }
    if (state.matchPhase == MatchPhase::RoundResult) {
        return roundResultText(state);
    }
    if (state.matchPhase == MatchPhase::MatchResult) {
        return "MATCH COMPLETE";
    }
    if (!state.gamepads.empty()) {
        return "Pads " + gamepadActionLayoutText(state, 0) + "  Start pause";
    }
    if (state.pendingMode == PendingMode::SinglePlayer) {
        return "P1 arrows A/S/D Z/X/C  CPU opponent";
    }
    return "P1 arrows A/S/D Z/X/C  P2 I/J/K/L U/O/P N/M/,";
}

#include "PauseMenuOverlay.h"

void drawRoundCalloutBand(SDL_Renderer* renderer, const AppState& state, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    setColor(renderer, 6, 8, 14, 210);
    fillRect(renderer, centerX - 116.0f, 82, 232, 34);
    setColor(renderer, r, g, b);
    fillRect(renderer, centerX - 114.0f, 84, 228, 2);
    fillRect(renderer, centerX - 114.0f, 114, 228, 2);
    debugTextCentered(renderer, centerX, 96, fitDebugText(text, 28));
}

void drawRoundWinPips(
    SDL_Renderer* renderer,
    float x,
    float y,
    int wins,
    int required,
    bool rightAligned = false,
    float size = 5.0f) {
    required = std::clamp(required, 1, 5);
    wins = std::clamp(wins, 0, required);
    const float gap = 3.0f;
    const float totalWidth = static_cast<float>(required) * size + static_cast<float>(required - 1) * gap;
    float startX = rightAligned ? x - totalWidth : x;
    for (int i = 0; i < required; ++i) {
        const float pipX = startX + static_cast<float>(i) * (size + gap);
        if (i < wins) {
            setColor(renderer, 230, 190, 105);
            fillRect(renderer, pipX, y, size, size);
            setColor(renderer, 42, 32, 12);
            drawRect(renderer, pipX, y, size, size);
        } else {
            setColor(renderer, 42, 48, 58, 210);
            fillRect(renderer, pipX, y, size, size);
            setColor(renderer, 118, 130, 148);
            drawRect(renderer, pipX, y, size, size);
        }
    }
}

void drawRoundStartOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (state.matchPhaseTicks < state.fightRoundSettings.startWaitTime) {
        return;
    }

    const bool fightText = state.matchPhaseTicks >= singleFightRoundDisplayEndTick(state);
    drawRoundCalloutBand(renderer, state, fightText ? "FIGHT" : roundStartCalloutText(state), 230, 220, 172);
}

void drawRoundFinishOverlay(SDL_Renderer* renderer, const AppState& state) {
    if (state.matchPhaseTicks < singleFightRoundFinishCalloutTicks(state)) {
        drawRoundCalloutBand(renderer, state, roundFinishCalloutText(state), 230, 190, 105);
        return;
    }
    if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
        drawRoundCalloutBand(renderer, state, roundResultText(state), 222, 226, 232);
    }
}

void drawRoundResultOverlay(SDL_Renderer* renderer, const AppState& state) {
    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    setColor(renderer, 6, 8, 14, 224);
    fillRect(renderer, centerX - 98.0f, 76, 196, 70);
    setColor(renderer, 230, 190, 105);
    drawRect(renderer, centerX - 98.0f, 76, 196, 70);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, centerX - 96.0f, 78, 192, 2);

    setColor(renderer, 222, 226, 232);
    debugTextCentered(renderer, centerX, 94, fitDebugText(roundResultText(state), 24));
    setColor(renderer, 230, 220, 172);
    drawRoundWinPips(renderer, centerX - 42.0f, 112, state.roundWins[0], matchWinsRequired(state), false, 6.0f);
    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, centerX, 111, "-");
    drawRoundWinPips(renderer, centerX + 42.0f, 112, state.roundWins[1], matchWinsRequired(state), true, 6.0f);
    setColor(renderer, 130, 142, 156);
    debugTextCentered(renderer, centerX, 130, state.matchComplete ? "MATCH COMPLETE" : "NEXT ROUND");
}

void drawMatchResultScreen(SDL_Renderer* renderer, const AppState& state) {
    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    setColor(renderer, 6, 8, 14, 238);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    setColor(renderer, 34, 40, 52);
    fillRect(renderer, 0, 0, widthF, 52);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 0, 52, widthF, 2);

    const int winner = matchWinner(state);
    const std::string winnerText = winner == 0 ? "DRAW GAME" : uppercaseCopy(fighterResultName(state, winner));

    setColor(renderer, 230, 220, 172);
    debugText(renderer, 22, 18, "MATCH COMPLETE");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 198, 18, isMatchMode(state) ? std::string(pendingModeTitle(state.pendingMode)) : "");

    setColor(renderer, 222, 226, 232);
    debugTextCentered(renderer, centerX, 72, fitDebugText(winnerText, 28));
    setColor(renderer, 230, 190, 105);
    debugTextCentered(renderer, centerX, 94, singleFightScoreText(state));
    setColor(renderer, 174, 184, 196);
    debugTextCentered(renderer, centerX, 112, matchWinMethodText(state));
    const std::string quote = winner > 0 && winner <= static_cast<int>(state.fighters.size())
        ? selectedVictoryQuoteText(state, state.fighters[static_cast<size_t>(winner - 1)])
        : std::string{};
    if (!quote.empty()) {
        debugTextCentered(renderer, centerX, 128, fitDebugText("\"" + quote + "\"", 40));
        debugTextCentered(renderer, centerX, 142, fitDebugText("Stage: " + selectedStageName(state), 34));
    } else {
        debugTextCentered(renderer, centerX, 128, fitDebugText("Stage: " + selectedStageName(state), 34));
    }

    for (int i = 0; i < kMatchResultOptionCount; ++i) {
        const float y = (quote.empty() ? 154.0f : 166.0f) + static_cast<float>(i * 16);
        if (i == state.selectedMatchResultOption) {
            setColor(renderer, 74, 170, 134);
            fillRect(renderer, centerX - 64.0f, y - 3.0f, 128, 13);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 174, 184, 196);
        }
        debugTextCentered(renderer, centerX, y, std::string(matchResultLabel(i)));
    }

    setColor(renderer, 130, 142, 156);
    debugTextCentered(renderer, centerX, 224, "ENTER select");
}

std::string formatComboLabel(const FightComboSettings& settings, int hits) {
    std::string label = settings.text;
    const std::string count = std::to_string(hits);
    size_t at = 0;
    while ((at = label.find("%i", at)) != std::string::npos) {
        label.replace(at, 2, count);
        at += count.size();
    }
    return label;
}

void setFightFontPaletteColor(SDL_Renderer* renderer, int palette, bool counter) {
    switch (palette) {
    case 1:
        setColor(renderer, 128, 171, 225);
        break;
    case 2:
        setColor(renderer, 82, 190, 112);
        break;
    case 3:
        setColor(renderer, 230, 130, 120);
        break;
    case 4:
        setColor(renderer, 230, 190, 105);
        break;
    case 0:
    default:
        if (counter) {
            setColor(renderer, 238, 226, 188);
        } else {
            setColor(renderer, 222, 226, 232);
        }
        break;
    }
}

void drawComboCounter(SDL_Renderer* renderer, const AppState& state, size_t attackerIndex) {
    if (attackerIndex >= state.comboCounters.size()) {
        return;
    }

    const auto& combo = state.comboCounters[attackerIndex];
    if (combo.displayTicks <= 0 || combo.displayHits < 2) {
        return;
    }

    const auto& settings = state.fightRoundSettings.combo;
    const bool labelIncludesCount = settings.text.find("%i") != std::string::npos;
    const std::string countText = labelIncludesCount ? std::string{} : std::to_string(combo.displayHits);
    const std::string label = formatComboLabel(settings, combo.displayHits);
    const float countWidth = static_cast<float>(countText.size()) * 8.0f;
    const float labelWidth = static_cast<float>(label.size()) * 8.0f;
    const float labelOffsetX = countText.empty() ? 0.0f : settings.textOffsetX;
    const float totalWidth = countWidth + labelOffsetX + labelWidth;
    const int displayTime = std::max(1, settings.displayTime);
    const int age = std::clamp(displayTime - combo.displayTicks, 0, displayTime);
    const float introT = std::clamp(static_cast<float>(age) / 8.0f, 0.0f, 1.0f);
    const float sideSign = attackerIndex == 0 ? 1.0f : -1.0f;
    const float slideOffset = settings.startX * (1.0f - introT) * sideSign;
    const float shakeOffset = settings.counterShake && age < 12
        ? static_cast<float>((state.frame / 2) % 3 - 1)
        : 0.0f;
    float x = attackerIndex == 0
        ? settings.posX
        : logicalWidthF(state) - settings.posX - totalWidth;
    x += slideOffset + shakeOffset;
    const float y = settings.posY;

    if (!countText.empty()) {
        setFightFontPaletteColor(renderer, settings.counterFontPalette, true);
        debugText(renderer, x, y, countText);
    }
    setFightFontPaletteColor(renderer, settings.textFontPalette, false);
    debugText(renderer, x + countWidth + labelOffsetX, y + settings.textOffsetY, label);
}

void drawPowerGauge(SDL_Renderer* renderer, const AppState& state, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }

    const bool p2 = fighterIndex == 1;
    const auto& settings = state.fightRoundSettings.powerbar;
    const float anchorX = motifOriginX(state) + (p2 ? settings.p2PosX : settings.p1PosX);
    const float y = p2 ? settings.p2PosY : settings.p1PosY;
    const float rangeStart = p2 ? settings.p2RangeStart : settings.p1RangeStart;
    const float rangeEnd = p2 ? settings.p2RangeEnd : settings.p1RangeEnd;
    const float barStart = anchorX + rangeStart;
    const float barEnd = anchorX + rangeEnd;
    const float barLeft = std::min(barStart, barEnd);
    const float barWidth = std::max(1.0f, std::abs(barEnd - barStart));
    constexpr float gaugeH = 5.0f;
    const int maxPower = std::max(1, state.characterConstants.maxPower);
    const float ratio = std::clamp(static_cast<float>(state.fighters[fighterIndex].power) / static_cast<float>(maxPower), 0.0f, 1.0f);
    const float fill = barWidth * ratio;

    setColor(renderer, 8, 10, 12, 230);
    fillRect(renderer, barLeft - 4.0f, y - 2.0f, barWidth + 8.0f, gaugeH + 4.0f);
    setColor(renderer, 60, 70, 88);
    drawRect(renderer, barLeft - 4.0f, y - 2.0f, barWidth + 8.0f, gaugeH + 4.0f);
    setColor(renderer, 236, 198, 74);
    if (barEnd < barStart) {
        fillRect(renderer, barStart - fill, y, fill, gaugeH);
    } else {
        fillRect(renderer, barStart, y, fill, gaugeH);
    }

    setColor(renderer, 178, 188, 204);
    debugText(renderer, p2 ? barEnd - 44.0f : barEnd + 6.0f, y - 3.0f, "POWER");
    const int stocks = state.fighters[fighterIndex].power / 1000;
    for (int i = 0; i < 3; ++i) {
        if (i < stocks) {
            setColor(renderer, 236, 198, 74);
        } else {
            setColor(renderer, 56, 62, 76);
        }
        const float pipX = p2 ? barEnd - 7.0f - static_cast<float>(i * 8) : barEnd + 6.0f + static_cast<float>(i * 8);
        fillRect(renderer, pipX, y + 7.0f, 5.0f, 3.0f);
    }
}

void drawFightView(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state) ? *selectedStageSlot(state) : fallbackStage;
    const bool hasStageBackground = hasSelectedStageBackground(state);
    const bool hideBackground = anyFighterHasAssertSpecialFlag(state, "nobg");
    const bool hideForeground = anyFighterHasAssertSpecialFlag(state, "nofg");
    const bool hideHud = anyFighterHasAssertSpecialFlag(state, "nobardisplay");
    SDL_Rect defaultViewport;
    SDL_GetRenderViewport(renderer, &defaultViewport);
    const int shakeOffsetY = static_cast<int>(std::lround(state.envShakeOffsetY));
    if (shakeOffsetY != 0) {
        SDL_Rect shakeViewport{ 0, shakeOffsetY, logicalWidth(state), kLogicalHeight };
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

    if (shakeOffsetY != 0) {
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

    if (!hideHud && state.pendingMode == PendingMode::Training) {
        drawDebugOverlay(renderer, state, stage);
    }

    if (!hideHud) {
        drawComboCounter(renderer, state, 0);
        drawComboCounter(renderer, state, 1);
        drawPowerGauge(renderer, state, 0);
        drawPowerGauge(renderer, state, 1);
        setColor(renderer, 8, 10, 12);
        constexpr float kLifeBarWidth = 130.0f;
        constexpr float kLifeBarFillWidth = 126.0f;
        constexpr float kLifeBarInset = 20.0f;
        const float p1BarX = 18.0f;
        const float p2BarX = widthF - p1BarX - kLifeBarWidth;
        fillRect(renderer, p1BarX, 10, kLifeBarWidth, 9);
        fillRect(renderer, p2BarX, 10, kLifeBarWidth, 9);
        setColor(renderer, 82, 190, 112);
        const float p1LifeWidth = kLifeBarFillWidth * std::clamp(static_cast<float>(state.fighters[0].life) / 1000.0f, 0.0f, 1.0f);
        const float p2LifeWidth = kLifeBarFillWidth * std::clamp(static_cast<float>(state.fighters[1].life) / 1000.0f, 0.0f, 1.0f);
        fillRect(renderer, kLifeBarInset, 12, p1LifeWidth, 5);
        fillRect(renderer, p2BarX + kLifeBarWidth - 2.0f - p2LifeWidth, 12, p2LifeWidth, 5);

        setColor(renderer, 222, 226, 232);
        debugText(renderer, 20, 24, selectedCharacterName(state));
        debugText(renderer, widthF - 106.0f, 24, compactSettingText(opponentDisplayName(state), 12));

        if (isMatchMode(state)) {
            const int seconds = std::max(0, (state.matchTimerTicks + 59) / 60);
            setColor(renderer, 8, 10, 12);
            fillRect(renderer, centerX - 28.0f, 22, 56, 13);
            setColor(renderer, 230, 220, 172);
            debugText(renderer, centerX - 20.0f, 25, std::to_string(seconds));
            setColor(renderer, 155, 164, 174);
            debugText(renderer, 82, 24, "R" + std::to_string(state.currentRound));
            drawRoundWinPips(renderer, 108, 27, state.roundWins[0], matchWinsRequired(state));
            drawRoundWinPips(renderer, widthF - 108.0f, 27, state.roundWins[1], matchWinsRequired(state), true);
        }

        setColor(renderer, 155, 164, 174);
        debugText(renderer, 20, 206,
            "P1 " + compactSettingText(selectedCharacterName(state), 11)
            + " vs " + compactSettingText(opponentDisplayName(state), 9));
        if (state.pendingMode == PendingMode::Training
            && state.trainingOptions.showHitLog
            && state.lastHitTextTicks > 0
            && !state.lastHitText.empty()) {
            setColor(renderer, 230, 190, 105);
            debugText(renderer, 20, 218, state.lastHitText);
        } else if (isMatchMode(state)) {
            const bool resultActive = isSingleFightResultPhase(state);
            setColor(renderer, resultActive ? 230 : 155, resultActive ? 190 : 164, resultActive ? 105 : 174);
            debugText(renderer, 20, 218, singleFightStatusLine(state));
        } else {
            if (!state.gamepads.empty()) {
                debugText(renderer, 20, 218, "Keys A/S/D Z/X/C  Pad " + gamepadActionLayoutText(state, 0));
            } else if (state.pendingMode == PendingMode::Training && state.trainingOptions.p2Controlled) {
                debugText(renderer, 20, 218, "P1 arrows A/S/D Z/X/C  P2 I/J/K/L U/O/P N/M/,");
            } else {
                debugText(renderer, 20, 218, "A/S/D Z/X/C  R reset  F1 boxes  F2 options");
            }
        }
    }

    if (state.pendingMode == PendingMode::Training && !state.trainingOptions.menuOpen && !hideHud) {
        drawTrainingCommandHud(renderer, state);
    }

    if (state.pendingMode == PendingMode::Training && state.trainingOptions.menuOpen) {
        drawTrainingOptionsMenu(renderer, state);
    } else if (isMatchMode(state) && state.singleFightPauseOpen) {
        drawSingleFightPauseMenu(renderer, state);
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

void handleKey(SDL_Renderer* renderer, AppState& state, SDL_Keycode key) {
    constexpr int modeCount = 5;
    if (state.screen == Screen::ModeSelect) {
        switch (key) {
        case SDLK_ESCAPE:
            state.running = false;
            break;
        case SDLK_UP:
            state.selectedMode = (state.selectedMode + modeCount - 1) % modeCount;
            break;
        case SDLK_DOWN:
            state.selectedMode = (state.selectedMode + 1) % modeCount;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (state.selectedMode == 0) {
                unloadCharacterRuntime(state);
                state.pendingMode = PendingMode::Training;
                state.screen = Screen::CharacterSelect;
            } else if (state.selectedMode == 1) {
                unloadCharacterRuntime(state);
                state.pendingMode = PendingMode::SinglePlayer;
                state.screen = Screen::CharacterSelect;
            } else if (state.selectedMode == 2) {
                unloadCharacterRuntime(state);
                state.pendingMode = PendingMode::SingleFight;
                state.screen = Screen::CharacterSelect;
            } else if (state.selectedMode == 3) {
                state.screen = Screen::MainSettings;
            } else if (state.selectedMode == 4) {
                state.running = false;
            }
            break;
        default:
            break;
        }
        return;
    }

    if (state.screen == Screen::MainSettings) {
        switch (key) {
        case SDLK_ESCAPE:
            state.screen = Screen::ModeSelect;
            break;
        case SDLK_UP:
            state.mainSettings.selectedOption =
                (state.mainSettings.selectedOption + kMainSettingsCount - 1) % kMainSettingsCount;
            break;
        case SDLK_DOWN:
            state.mainSettings.selectedOption =
                (state.mainSettings.selectedOption + 1) % kMainSettingsCount;
            break;
        case SDLK_LEFT:
            if (state.mainSettings.selectedOption == 0) {
                cycleMatchTimerSetting(state.mainSettings, -1);
            } else if (state.mainSettings.selectedOption == 1) {
                cycleCanvasSizeSetting(state.mainSettings, -1);
            } else if (state.mainSettings.selectedOption == 2) {
                cycleUiScaleSetting(state.mainSettings, -1);
            } else if (state.mainSettings.selectedOption == 3) {
                cycleGamepadPromptStyle(state.mainSettings, -1);
            } else if (state.mainSettings.selectedOption == 4) {
                cycleGamepadAssignment(state.mainSettings, 0, static_cast<int>(state.gamepads.size()), -1);
            } else if (state.mainSettings.selectedOption == 5) {
                cycleGamepadAssignment(state.mainSettings, 1, static_cast<int>(state.gamepads.size()), -1);
            }
            break;
        case SDLK_RIGHT:
            if (state.mainSettings.selectedOption == 0) {
                cycleMatchTimerSetting(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 1) {
                cycleCanvasSizeSetting(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 2) {
                cycleUiScaleSetting(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 3) {
                cycleGamepadPromptStyle(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 4) {
                cycleGamepadAssignment(state.mainSettings, 0, static_cast<int>(state.gamepads.size()), 1);
            } else if (state.mainSettings.selectedOption == 5) {
                cycleGamepadAssignment(state.mainSettings, 1, static_cast<int>(state.gamepads.size()), 1);
            }
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            if (state.mainSettings.selectedOption == 0) {
                cycleMatchTimerSetting(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 1) {
                cycleCanvasSizeSetting(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 2) {
                cycleUiScaleSetting(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 3) {
                cycleGamepadPromptStyle(state.mainSettings, 1);
            } else if (state.mainSettings.selectedOption == 4) {
                cycleGamepadAssignment(state.mainSettings, 0, static_cast<int>(state.gamepads.size()), 1);
            } else if (state.mainSettings.selectedOption == 5) {
                cycleGamepadAssignment(state.mainSettings, 1, static_cast<int>(state.gamepads.size()), 1);
            } else {
                state.screen = Screen::ModeSelect;
            }
            break;
        default:
            break;
        }
        return;
    }

    if (state.screen == Screen::CharacterSelect) {
        const int characterCount = static_cast<int>(state.characters.size());
        switch (key) {
        case SDLK_ESCAPE:
            unloadCharacterRuntime(state);
            state.screen = Screen::ModeSelect;
            break;
        case SDLK_UP:
            moveCharacterSelectCursor(state, 0, -1);
            break;
        case SDLK_DOWN:
            moveCharacterSelectCursor(state, 0, 1);
            break;
        case SDLK_LEFT:
            moveCharacterSelectCursor(state, -1, 0);
            break;
        case SDLK_RIGHT:
            moveCharacterSelectCursor(state, 1, 0);
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (characterCount > 0) {
                state.selectedCharacter = std::clamp(state.selectedCharacter, 0, characterCount - 1);
                configureFightSessionSlotsFromSelection(state);
                selectPreferredStage(state);
                state.screen = Screen::StageSelect;
            }
            break;
        default:
            break;
        }
        return;
    }

    if (state.screen == Screen::StageSelect) {
        const int stageCount = static_cast<int>(state.stages.size());
        switch (key) {
        case SDLK_ESCAPE:
            unloadCharacterRuntime(state);
            state.screen = Screen::CharacterSelect;
            break;
        case SDLK_UP:
            if (stageCount > 0) {
                state.selectedStage = (state.selectedStage + stageCount - 1) % stageCount;
            }
            break;
        case SDLK_DOWN:
            if (stageCount > 0) {
                state.selectedStage = (state.selectedStage + 1) % stageCount;
            }
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (stageCount > 0) {
                if (!characterSlotAt(state, state.sessionSlots.p1Character)) {
                    configureFightSessionSlotsFromSelection(state);
                }
                unloadCharacterRuntime(state);
                state.screen = Screen::VersusScreen;
                state.screenFrame = 0;
                state.fightSessionPrepared = false;
                state.fightSessionLoadFailed = false;
            }
            break;
        default:
            break;
        }
        return;
    }

    if (state.screen == Screen::VersusScreen) {
        switch (key) {
        case SDLK_ESCAPE:
            unloadCharacterRuntime(state);
            state.screen = Screen::StageSelect;
            state.screenFrame = 0;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (state.fightSessionPrepared || prepareFightSession(renderer, state)) {
                beginFight(state);
            }
            break;
        default:
            break;
        }
        return;
    }

    if (state.screen == Screen::FightView) {
        if (isMatchMode(state)) {
            if (state.singleFightPauseOpen) {
                switch (key) {
                case SDLK_ESCAPE:
                case SDLK_F2:
                    state.singleFightPauseOpen = false;
                    break;
                case SDLK_UP:
                    state.selectedSingleFightPauseOption =
                        (state.selectedSingleFightPauseOption + kSingleFightPauseOptionCount - 1) % kSingleFightPauseOptionCount;
                    break;
                case SDLK_DOWN:
                    state.selectedSingleFightPauseOption =
                        (state.selectedSingleFightPauseOption + 1) % kSingleFightPauseOptionCount;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    switch (state.selectedSingleFightPauseOption) {
                    case 0:
                        state.singleFightPauseOpen = false;
                        break;
                    case 1:
                        resetFightState(state);
                        break;
                    case 2:
                        state.singleFightPauseOpen = false;
                        unloadCharacterRuntime(state);
                        state.screen = Screen::CharacterSelect;
                        state.screenFrame = 0;
                        break;
                    case 3:
                        state.singleFightPauseOpen = false;
                        unloadCharacterRuntime(state);
                        state.screen = Screen::StageSelect;
                        state.screenFrame = 0;
                        break;
                    case 4:
                        state.singleFightPauseOpen = false;
                        unloadCharacterRuntime(state);
                        state.screen = Screen::ModeSelect;
                        state.screenFrame = 0;
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                return;
            }

            if (state.matchPhase == MatchPhase::MatchResult) {
                switch (key) {
                case SDLK_UP:
                    state.selectedMatchResultOption =
                        (state.selectedMatchResultOption + kMatchResultOptionCount - 1) % kMatchResultOptionCount;
                    break;
                case SDLK_DOWN:
                    state.selectedMatchResultOption =
                        (state.selectedMatchResultOption + 1) % kMatchResultOptionCount;
                    break;
                case SDLK_R:
                    resetFightState(state);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    switch (state.selectedMatchResultOption) {
                    case 0:
                        resetFightState(state);
                        break;
                    case 1:
                        unloadCharacterRuntime(state);
                        state.screen = Screen::CharacterSelect;
                        state.screenFrame = 0;
                        break;
                    case 2:
                        unloadCharacterRuntime(state);
                        state.screen = Screen::StageSelect;
                        state.screenFrame = 0;
                        break;
                    case 3:
                        unloadCharacterRuntime(state);
                        state.screen = Screen::ModeSelect;
                        state.screenFrame = 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case SDLK_ESCAPE:
                case SDLK_F2:
                    unloadCharacterRuntime(state);
                    state.screen = Screen::ModeSelect;
                    state.screenFrame = 0;
                    break;
                default:
                    break;
                }
                return;
            }

            if (key == SDLK_ESCAPE || key == SDLK_F2) {
                state.singleFightPauseOpen = true;
                state.selectedSingleFightPauseOption = 0;
            } else if (key == SDLK_R) {
                resetFightState(state);
            }
            return;
        }

        if (state.trainingOptions.menuOpen) {
            if (state.trainingOptions.moveListOpen) {
                const auto entries = displayableMoveListEntries(state);
                constexpr int visibleRows = 7;
                const int maxScroll = std::max(0, static_cast<int>(entries.size()) - visibleRows);
                const int maxSelected = std::max(0, static_cast<int>(entries.size()) - 1);
                switch (key) {
                case SDLK_ESCAPE:
                case SDLK_F2:
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    state.trainingOptions.moveListOpen = false;
                    break;
                case SDLK_UP:
                    state.trainingOptions.selectedMoveListEntry =
                        std::max(0, state.trainingOptions.selectedMoveListEntry - 1);
                    if (state.trainingOptions.selectedMoveListEntry < state.trainingOptions.moveListScroll) {
                        state.trainingOptions.moveListScroll = state.trainingOptions.selectedMoveListEntry;
                    }
                    break;
                case SDLK_DOWN:
                    state.trainingOptions.selectedMoveListEntry =
                        std::min(maxSelected, state.trainingOptions.selectedMoveListEntry + 1);
                    if (state.trainingOptions.selectedMoveListEntry >= state.trainingOptions.moveListScroll + visibleRows) {
                        state.trainingOptions.moveListScroll =
                            std::min(maxScroll, state.trainingOptions.selectedMoveListEntry - visibleRows + 1);
                    }
                    break;
                default:
                    break;
                }
                state.trainingOptions.moveListScroll = std::clamp(state.trainingOptions.moveListScroll, 0, maxScroll);
                return;
            }

            switch (key) {
            case SDLK_ESCAPE:
            case SDLK_F2:
                state.trainingOptions.menuOpen = false;
                state.trainingOptions.moveListOpen = false;
                break;
            case SDLK_UP:
                state.trainingOptions.selectedOption =
                    (state.trainingOptions.selectedOption + kTrainingOptionCount - 1) % kTrainingOptionCount;
                break;
            case SDLK_DOWN:
                state.trainingOptions.selectedOption =
                    (state.trainingOptions.selectedOption + 1) % kTrainingOptionCount;
                break;
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_SPACE:
                if (state.trainingOptions.selectedOption == kTrainingResetOption) {
                    resetTrainingPositions(state);
                } else {
                    const int toggledOption = state.trainingOptions.selectedOption;
                    toggleTrainingOption(state.trainingOptions, state.trainingOptions.selectedOption);
                    if (toggledOption == kTrainingP2ControlOption) {
                        clearFighterHitRuntime(state.fighters[1]);
                        enterState(state, state.fighters[1], 0);
                        state.lastHitText = state.trainingOptions.p2Controlled ? "P2 control enabled" : "P2 dummy enabled";
                        state.lastHitTextTicks = 90;
                    } else if (toggledOption == kTrainingPowerOption) {
                        applyTrainingPowerMode(state);
                        state.lastHitText = state.trainingOptions.powerMode == TrainingPowerMode::Max ? "Training power max" : "Training power normal";
                        state.lastHitTextTicks = 90;
                    }
                }
                break;
            default:
                break;
            }
            return;
        }

        if (key == SDLK_ESCAPE) {
            unloadCharacterRuntime(state);
            state.screen = Screen::StageSelect;
            state.screenFrame = 0;
        } else if (key == SDLK_F1) {
            state.trainingOptions.showHitboxes = !state.trainingOptions.showHitboxes;
        } else if (key == SDLK_F2) {
            state.trainingOptions.menuOpen = true;
        } else if (key == SDLK_R) {
            resetFightState(state);
        }
        return;
    }

}

std::optional<SDL_Keycode> gamepadMenuKeyForButton(const AppState& state, SDL_GamepadButton button) {
    const bool fightOverlayOpen =
        state.screen == Screen::FightView
        && ((state.pendingMode == PendingMode::Training && state.trainingOptions.menuOpen)
            || (isMatchMode(state) && state.singleFightPauseOpen));

    if (state.screen == Screen::FightView && !fightOverlayOpen) {
        if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
            if (button == SDL_GAMEPAD_BUTTON_SOUTH || button == SDL_GAMEPAD_BUTTON_START) {
                return SDLK_RETURN;
            }
            if (button == SDL_GAMEPAD_BUTTON_BACK || button == SDL_GAMEPAD_BUTTON_EAST) {
                return SDLK_ESCAPE;
            }
        }
        if (button == SDL_GAMEPAD_BUTTON_START) {
            return SDLK_F2;
        }
        if (button == SDL_GAMEPAD_BUTTON_BACK) {
            return SDLK_ESCAPE;
        }
        return std::nullopt;
    }

    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return SDLK_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return SDLK_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return SDLK_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return SDLK_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH:
    case SDL_GAMEPAD_BUTTON_START:
        return SDLK_RETURN;
    case SDL_GAMEPAD_BUTTON_EAST:
    case SDL_GAMEPAD_BUTTON_BACK:
        return SDLK_ESCAPE;
    default:
        return std::nullopt;
    }
}

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
            if (const auto key = gamepadMenuKeyForButton(state, static_cast<SDL_GamepadButton>(event.gbutton.button))) {
                handleKey(renderer, state, *key);
            }
            break;
        default:
            break;
        }
    }
}

void fixedUpdate(AppState& state) {
    ++state.frame;
    ++state.screenFrame;
    if (state.screen == Screen::VersusScreen && state.fightSessionPrepared && state.screenFrame > 120) {
        beginFight(state);
    }
    const bool fightPaused =
        (state.pendingMode == PendingMode::Training && state.trainingOptions.menuOpen)
        || (isMatchMode(state) && state.singleFightPauseOpen);
    if (state.screen == Screen::FightView && !fightPaused) {
        updateFight(state);
        applyTrainingPowerMode(state);
    }
    updateAudioMixer(state);
}

void prepareVersusSessionAfterPresent(SDL_Renderer* renderer, AppState& state) {
    if (state.screen != Screen::VersusScreen
        || state.fightSessionPrepared
        || state.fightSessionLoadFailed
        || state.screenFrame < kVersusPrepareStartFrames) {
        return;
    }

    prepareFightSession(renderer, state);
}

void applyLogicalPresentation(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderLogicalPresentation(renderer, logicalWidth(state), kLogicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
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
    initAudio(state);
    state.fightRoundSettings = loadFightRoundSettings(gameRoot);
    state.characters = loadCharacters(gameRoot);
    state.stages = loadStages(gameRoot);
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

        applyLogicalPresentation(renderer, state);

        if (state.screen == Screen::ModeSelect) {
            drawModeSelect(renderer, state);
        } else if (state.screen == Screen::MainSettings) {
            drawMainSettings(renderer, state);
        } else if (state.screen == Screen::CharacterSelect) {
            drawCharacterSelect(renderer, state);
        } else if (state.screen == Screen::StageSelect) {
            drawStageSelect(renderer, state);
        } else if (state.screen == Screen::VersusScreen) {
            drawVersusScreen(renderer, state);
            prepareVersusSessionAfterPresent(renderer, state);
        } else if (state.screen == Screen::FightView) {
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
