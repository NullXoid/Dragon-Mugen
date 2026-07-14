#pragma once

#include "FramePerformance.h"

#include <cstdint>
#include <iosfwd>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace dragon::verification {

enum class ScenarioMode {
    Training,
    SinglePlayer,
    Versus,
    Arena,
    Story,
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
    int maxLife = 1000;
    int power = 0;
    float attackMultiplier = 1.0f;
    float defenceMultiplier = 1.0f;
    int targetIndex = -1;
    int targetTicks = 0;
    int targetHitId = -1;
    int hitCount = 0;
    int paletteNo = 1;
    int hitPauseTicks = 0;
    int hitStunTicks = 0;
    float hitDownVelocityX = 0.0f;
    float hitDownVelocityY = 0.0f;
    float displayOffsetX = 0.0f;
    float displayOffsetY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float screenX = 0.0f;
    float screenY = 0.0f;
    float viewDepth = 0.0f;
    float visualScreenLeft = 0.0f;
    float visualScreenRight = 0.0f;
    int facing = 1;
    char stateType = 'S';
    char moveType = 'I';
    char physics = 'S';
    bool ctrl = false;
    bool onGround = true;
    bool moveContact = false;
    bool moveHit = false;
    bool moveGuarded = false;
    bool afterImageActive = false;
    int afterImageTrailCount = 0;
};

struct RuntimeSnapshot {
    int frame = 0;
    int logicalWidth = 426;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float arenaCameraYawDeg = 0.0f;
    float arenaCameraTargetYawDeg = 0.0f;
    int matchTimerTicks = 0;
    int screen = 0;
    int pendingMode = 0;
    int selectedStageIndex = 0;
    int stageCount = 0;
    int stageBackgroundCount = 0;
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
    int roundEndReason = 0;
    int roundWinsP1 = 0;
    int roundWinsP2 = 0;
    int matchWinner = 0;
    int selectedSingleFightPauseOption = 0;
    int selectedMatchResultOption = 0;
    int arenaRuntimeCount = 0;
    int storyWaveIndex = 0;
    int storyActiveEnemies = 0;
    int storyLivingEnemies = 0;
    int storyEnemiesDefeated = 0;
    int storyTotalEnemies = 0;
    int storyDifficulty = 1;
    int storyBoardNodeCount = 0;
    int storySelectableBoardNodeCount = 0;
    int storySelectedBoardNode = 0;
    int storyActiveBoardNode = 0;
    int storyNextRouteBoardNode = -1;
    int storySelectedBoardWaves = 0;
    bool storySelectedBoardShop = false;
    bool storyCanContinueRoute = false;
    bool storyForwardCueVisible = false;
    bool storyForwardCueImageLoaded = false;
    bool storyShopDoorAvailable = false;
    bool storyShopDoorPromptVisible = false;
    bool storyShopDoorTransitionPending = false;
    bool storyResumeRouteAfterShop = false;
    int storyResumeBoardNodeAfterShop = -1;
    float storyShopDoorX = 0.0f;
    bool matchComplete = false;
    bool storyStageClear = false;
    bool storyStageFailed = false;
    bool fightPauseOpen = false;
    bool singleFightPauseOpen = false;
    bool trainingOptionsOpen = false;
    bool loadingProgressActive = false;
    bool loadingProgressFailed = false;
    float loadingProgressFraction = 0.0f;
    bool arenaZAxisEnabled = false;
    bool arenaCameraRotationSelected = false;
    bool arenaCameraRotationActive = false;
    bool p1P2BoxesOverlap = false;
    bool p1ActiveHitDef = false;
    bool p1HitFlagAllowsP2 = false;
    bool p2HittableByP1 = false;
    int p1LocalCoordWidth = 320;
    int p1LocalCoordHeight = 240;
    int p2LocalCoordWidth = 320;
    int p2LocalCoordHeight = 240;
    bool p1UsesMugenSemantics = true;
    bool p1AllowsDragonExtensions = false;
    bool p1AllowsArenaExtensions = false;
    bool p2UsesMugenSemantics = true;
    bool selectedStageDragonSidecarAvailable = false;
    bool selectedStageLegacyOpenBorSection = false;
    bool selectedStageHasMusic = false;
    double perfFps = 0.0;
    double perfAvgFrameMs = 0.0;
    double perfP95FrameMs = 0.0;
    double perfWorstFrameMs = 0.0;
    double perfFpsEquivalent = 0.0;
    int perfDrawCalls = 0;
    int perfSkippedDraws = 0;
    int perfFixedSteps = 0;
    std::string perfDominantSection;
    int p1AnimElem = 0;
    int p2AnimElem = 0;
    int p1Clsn1Count = 0;
    int p1Clsn2Count = 0;
    int p2Clsn1Count = 0;
    int p2Clsn2Count = 0;
    float p1P2BodyDistX = 0.0f;
    std::string arenaDrawOrder;
    std::string runtimeMode;
    std::string p1CompatibilityProfile;
    std::string p2CompatibilityProfile;
    std::string lastHitText;
    std::string progressionAwardText;
    std::string storySelectedBoardKind;
    std::string storySelectedBoardTitle;
    std::string storySelectedBoardTarget;
    std::string p1CharacterId;
    std::string p1CharacterName;
    std::string p2CharacterId;
    std::string p2CharacterName;
    int progressionGoldBalance = 0;
    int storyRewardPopups = 0;
    int storyRewardCoins = 0;
    std::string loadingProgressPhase;
    std::string p1Commands;
    std::string p2Commands;
    std::string selectedTrainingMoveLabel;
    std::string selectedStageMusicPath;
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
    std::string section;
};

struct RuntimePerformanceResult {
    bool ran = false;
    int warmupFrames = 0;
    int measuredFrames = 0;
    double fpsEquivalent = 0.0;
    double avgFrameMs = 0.0;
    double p95FrameMs = 0.0;
    double worstFrameMs = 0.0;
    double pauseFrameAvgMs = 0.0;
    int gameplayFrames = 0;
    int pauseFrames = 0;
    FramePerfCounters counters;
    std::string dominantSection;
};

struct RosterCharacterInfo {
    std::string id;
    std::string displayName;
    std::string defPath;
    std::string compatibilityProfile;
};

struct UiGeometryProbe {
    bool ok = false;
    bool visible = false;
    bool secondaryVisible = false;
    bool tertiaryVisible = false;
    std::string detail;
};

struct PresentationFrameProbe {
    bool readbackOk = false;
    int selectedOutputWidth = 0;
    int selectedOutputHeight = 0;
    int renderTargetWidth = 0;
    int renderTargetHeight = 0;
    int physicalWidth = 0;
    int physicalHeight = 0;
    int readbackWidth = 0;
    int readbackHeight = 0;
    int sampledDistinctByteValues = 0;
    std::uint64_t staticUiHash = 0;
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
    virtual bool setupStageSelect(std::string_view p1Id, ScenarioMode mode, std::ostream& out) = 0;
    virtual bool setupArenaSetupScreen(std::string_view p1Id, std::ostream& out) = 0;
    virtual bool setupVideoOptions(std::ostream& out) = 0;
    virtual void step(const SymbolicInput& p1Input, int frames) = 0;
    virtual void step(const SymbolicInput& p1Input, const SymbolicInput& p2Input, int frames) = 0;
    virtual void pressKey(std::string_view key) = 0;
    virtual bool preparePendingFight() = 0;
    virtual void positionFighters(float p1X, float p2X) = 0;
    virtual void setFighterPosition(int fighterIndex, float x, float y) = 0;
    virtual void setFighterDepth(int fighterIndex, float depthZ) = 0;
    virtual void setFighterLife(int fighterIndex, int life) = 0;
    virtual void setFighterPower(int fighterIndex, int power) = 0;
    virtual void setFighterVar(int fighterIndex, int varIndex, int value) = 0;
    virtual int fighterVar(int fighterIndex, int varIndex) const = 0;
    virtual void setFighterMoveContact(int fighterIndex, bool hit, bool guarded) = 0;
    virtual void setFighterControl(int fighterIndex, bool enabled) = 0;
    virtual void setMatchTimerTicks(int ticks) = 0;
    virtual void setTrainingDummyGuardMode(std::string_view mode) = 0;
    virtual void setArenaZAxisEnabled(bool enabled) = 0;
    virtual void setArenaCameraRotationEnabled(bool enabled) = 0;
    virtual void setArenaCpuFrozen(bool frozen) = 0;
    virtual void setStoryWave(int waveIndex) = 0;
    virtual void setFightPaused(bool paused) = 0;
    virtual void setFighterHitPause(int fighterIndex, int ticks) = 0;
    virtual void setFighterHitStun(int fighterIndex, int ticks) = 0;
    virtual void forceFighterState(int fighterIndex, int stateNo) = 0;
    virtual void forceFighterLiedown(int fighterIndex, int hitStunTicks) = 0;
    virtual void spawnHelper(int ownerIndex, int helperId, int stateNo, int pauseMoveTime = 0, int superMoveTime = 0) = 0;
    virtual std::vector<RosterCharacterInfo> selectableCharacters() const = 0;
    virtual std::vector<TrainingMoveInfo> trainingMoves() const = 0;
    virtual std::vector<TrainingMoveInfo> trainingMovesForPromptStyle(std::string_view style) const = 0;
    virtual void setTrainingMoveCategory(std::string_view category) = 0;
    virtual std::string trainingMoveListTab() const = 0;
    virtual void setTrainingMoveListTab(std::string_view tab) = 0;
    virtual bool trainingMoveListSelectedRowVisible() const = 0;
    virtual bool commandIconAtlasLoaded() const = 0;
    virtual std::string trainingCurrentInputDisplay() const = 0;
    virtual std::string trainingExpectedInputDisplay() const = 0;
    virtual std::string trainingDirectionGuideState() const = 0;
    virtual std::string trainingButtonGuideState() const = 0;
    virtual bool trainingCommandCompleteFlash() const = 0;
    virtual UiGeometryProbe trainingCommandHudGeometry(int logicalWidth) const = 0;
    virtual UiGeometryProbe trainingPauseHelpGeometry(int logicalWidth, bool optionsOpen) const = 0;
    virtual bool selectTrainingMoveIndex(int index) = 0;
    virtual bool selectTrainingMove(std::string_view label) = 0;
    virtual void startTrainingCommandDemo() = 0;
    virtual void pressTrainingShowShortcut() = 0;
    virtual void holdTrainingShowSelect(bool held, int frames) = 0;
    virtual RuntimePerformanceResult measurePerformance(int warmupFrames, int measuredFrames, bool renderEachFrame, bool stressInputs) = 0;
    virtual bool captureScreenshot(const std::filesystem::path& path) = 0;
    virtual PresentationFrameProbe videoOptionsPresentation(int outputProfileIndex) = 0;
    virtual RuntimeSnapshot snapshot() const = 0;
    virtual std::string rootText() const = 0;
    virtual std::string stageName() const = 0;
    virtual std::string p1Name() const = 0;
};

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out);

} // namespace dragon::verification
