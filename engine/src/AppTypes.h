#pragma once

#include "FramePerformance.h"
#include "StoryModeTypes.h"

#include <array>
#include <string>

namespace dragon {

inline constexpr int kWindowWidth = 1280;
inline constexpr int kWindowHeight = 720;
inline constexpr int kClassicLogicalWidth = 320;
inline constexpr int kDefaultLogicalWidth = 426;
inline constexpr int kExtraWideLogicalWidth = 480;
inline constexpr int kSdLogicalWidth = 854;
inline constexpr int kHdLogicalWidth = 1280;
inline constexpr int kLogicalHeight = 240;
inline constexpr int kSdLogicalHeight = 480;
inline constexpr int kHdLogicalHeight = 720;
inline constexpr int kLogicalWidth = kDefaultLogicalWidth;
inline constexpr int kDesignLogicalWidth = kDefaultLogicalWidth;
inline constexpr int kDesignLogicalHeight = kLogicalHeight;
inline constexpr int kPresentationLogicalWidth = kDesignLogicalWidth;
inline constexpr int kPresentationLogicalHeight = kDesignLogicalHeight;
inline constexpr int kTrainingOptionCount = 20;
inline constexpr int kTrainingOptionRows = 10;
inline constexpr int kTrainingMoveListRows = 10;
inline constexpr int kTrainingP2ControlOption = 13;
inline constexpr int kTrainingCommandHudOption = 14;
inline constexpr int kTrainingInputHudOption = 15;
inline constexpr int kTrainingPowerOption = 16;
inline constexpr int kTrainingMoveTypeOption = 17;
inline constexpr int kTrainingMoveListOption = 18;
inline constexpr int kTrainingResetOption = 19;
inline constexpr int kSingleFightPauseOptionCount = 5;
inline constexpr int kMatchResultOptionCount = 4;
inline constexpr int kArenaSetupOptionCount = 10;
inline constexpr int kMainSettingsCount = 11;
inline constexpr int kOptionsRootCount = 4;
inline constexpr int kOptionsGameplayCount = 7;
inline constexpr int kOptionsVideoCount = 5;
inline constexpr int kOptionsControlsCount = 9;
inline constexpr int kOptionsKeyboardSetupCount = 6;
inline constexpr int kOptionsControllerSetupCount = 8;
inline constexpr int kOptionsInputTestCount = 1;
inline constexpr int kOptionsRestoreDefaultsCount = 3;
inline constexpr int kControlPlayerStaticRows = 4;
inline constexpr int kMainSettingP1ProfileOption = 4;
inline constexpr int kMainSettingP2ProfileOption = 5;
inline constexpr int kMainSettingBackOption = kMainSettingsCount - 1;
inline constexpr int kVersusPrepareStartFrames = 2;
inline constexpr int kCharacterSelectColumns = 5;
inline constexpr int kCharacterSelectRows = 2;
inline constexpr int kCharacterSelectPageSize = kCharacterSelectColumns * kCharacterSelectRows;

enum class Screen {
    ModeSelect,
    CharacterSelect,
    ArenaSetup,
    StageSelect,
    VersusScreen,
    FightView,
    ShopDemo,
    MainSettings,
};

enum class PendingMode {
    Training,
    SinglePlayer,
    SingleFight,
    Arena,
    Story,
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

enum class TrainingMoveListTab {
    Main,
    All,
};

enum class GamepadPromptStyle {
    Auto,
    Xbox,
    Playstation,
};

enum class CanvasPreset {
    Classic320x240,
    Wide426x240,
    Extra480x240,
    Sd854x480,
    Hd1280x720,
};

inline constexpr CanvasPreset kStandardCanvasPreset = CanvasPreset::Hd1280x720;

struct CanvasDimensions {
    int width = kDefaultLogicalWidth;
    int height = kLogicalHeight;
};

inline CanvasDimensions dimensionsForPreset(CanvasPreset preset) {
    switch (preset) {
    case CanvasPreset::Classic320x240:
        return { kClassicLogicalWidth, kLogicalHeight };
    case CanvasPreset::Extra480x240:
        return { kExtraWideLogicalWidth, kLogicalHeight };
    case CanvasPreset::Sd854x480:
        return { kSdLogicalWidth, kSdLogicalHeight };
    case CanvasPreset::Hd1280x720:
        return { kHdLogicalWidth, kHdLogicalHeight };
    case CanvasPreset::Wide426x240:
    default:
        return { kDefaultLogicalWidth, kLogicalHeight };
    }
}

inline CanvasDimensions outputDimensionsForPreset(CanvasPreset preset) {
    return dimensionsForPreset(preset);
}

inline CanvasDimensions presentationDimensions() {
    return { kPresentationLogicalWidth, kPresentationLogicalHeight };
}

inline bool canvasPresetChangesLayout(CanvasPreset preset) {
    static_cast<void>(preset);
    return false;
}

enum class DragonLayoutClass {
    Classic,
    WideLowRes,
    ExtraLowRes,
    StandardDefinition,
    HighDefinition,
};

inline DragonLayoutClass layoutClassForPreset(CanvasPreset preset) {
    switch (preset) {
    case CanvasPreset::Classic320x240:
        return DragonLayoutClass::Classic;
    case CanvasPreset::Extra480x240:
        return DragonLayoutClass::ExtraLowRes;
    case CanvasPreset::Sd854x480:
        return DragonLayoutClass::StandardDefinition;
    case CanvasPreset::Hd1280x720:
        return DragonLayoutClass::HighDefinition;
    case CanvasPreset::Wide426x240:
    default:
        return DragonLayoutClass::WideLowRes;
    }
}

inline DragonLayoutClass layoutClassForDimensions(CanvasDimensions dimensions) {
    if (dimensions.width >= kHdLogicalWidth || dimensions.height >= kHdLogicalHeight) {
        return DragonLayoutClass::HighDefinition;
    }
    if (dimensions.width >= kSdLogicalWidth || dimensions.height >= kSdLogicalHeight) {
        return DragonLayoutClass::StandardDefinition;
    }
    if (dimensions.width <= kClassicLogicalWidth) {
        return DragonLayoutClass::Classic;
    }
    if (dimensions.width <= kDefaultLogicalWidth) {
        return DragonLayoutClass::WideLowRes;
    }
    return DragonLayoutClass::ExtraLowRes;
}

enum class OptionsMenuScreen {
    Root,
    Gameplay,
    Video,
    Controls,
    PlayerControls,
    KeyboardSetup,
    ControllerSetup,
    InputTest,
    RestoreDefaults,
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
    TrainingMoveListTab moveListTab = TrainingMoveListTab::All;
};

struct MainSettings {
    int selectedOption = 0;
    OptionsMenuScreen optionsScreen = OptionsMenuScreen::Root;
    int selectedRootOption = 0;
    int selectedGameplayOption = 0;
    int selectedVideoOption = 0;
    int selectedControlsOption = 0;
    int selectedPlayerControlsOption = 0;
    int selectedKeyboardSetupOption = 0;
    int selectedControllerSetupOption = 0;
    int selectedInputTestOption = 0;
    int selectedRestoreDefaultsOption = 0;
    int selectedControlPlayer = 0;
    bool awaitingControlBinding = false;
    bool guidedControlSetup = false;
    int controlBindingActionIndex = 0;
    std::string controlStatusMessage;
    int matchTimerSeconds = 99;
    CanvasPreset canvasPreset = kStandardCanvasPreset;
    int uiScalePercent = 80;
    bool fpsCapEnabled = true;
    PerformanceHudMode performanceHudMode = PerformanceHudMode::Fps;
    GamepadPromptStyle gamepadPromptStyle = GamepadPromptStyle::Auto;
    int p1GamepadAssignment = 0;
    int p2GamepadAssignment = 0;
    bool fallFallbacksEnabled = true;
};

struct WindowPresentationState {
    bool fullscreen = true;
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

struct ComboCounterState {
    int activeHits = 0;
    int displayHits = 0;
    int displayTicks = 0;
};

struct FightSessionSlots {
    int p1Character = -1;
    int opponentCharacter = -1;
    OpponentType opponentType = OpponentType::Dummy;
    int arenaCpuCount = 1;
    std::array<int, 3> arenaCpuCharacters{ -1, -1, -1 };
    int arenaTimerSeconds = 0;
    bool arenaZAxisEnabled = true;
    bool arenaCameraRotationEnabled = false;
};

} // namespace dragon
