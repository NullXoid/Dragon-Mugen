#pragma once

// Internal App.cpp implementation shard.
// Command, animation, stage, effect, and audio data used by App.cpp internals.

struct CommandStateEntry {
    std::string label;
    std::string displayLabel;
    std::string displayInput;
    bool presentationOverride = false;
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
    bool infiniteDuration = false;
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
    bool hasInfiniteDuration = false;
    int infiniteStartTick = 0;
    std::vector<AnimationFrame> frames;
};

struct StageBackgroundElement {
    TextureSprite sprite;
    AnimationClip animation;
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 1.0f;
    float deltaY = 1.0f;
    bool tileX = false;
    bool tileY = false;
    int layerNo = 0;
    bool animated = false;
};

enum class MainMenuBackgroundMode {
    Motif,
    Image,
    Fallback,
    None,
};

struct MainMenuPresentationConfig {
    MainMenuBackgroundMode backgroundMode = MainMenuBackgroundMode::Motif;
    std::filesystem::path backgroundPath;
    bool fallbackGrid = true;
    float backgroundPanX = 0.5f;
    int backgroundDimAlpha = 0;
    std::array<std::string, kMainMenuOptionCount> labels{};
};

struct SystemScreenAssets {
    MainMenuPresentationConfig mainMenu;
    TextureSprite mainMenuBackground;
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
    int clipOwnerIndex = -1;
    int action = 0;
    int animTick = 0;
    int ageTicks = 0;
    float x = 0.0f;
    float y = 0.0f;
    float depthZ = 0.0f;
    float bindOffsetX = 0.0f;
    float bindOffsetY = 0.0f;
    int bindTicks = 0;
    int removeTime = -2;
    bool fromFightFx = true;
    int sprPriority = 0;
    int pauseMoveTime = 0;
    int superMoveTime = 0;
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
    float pan = 0.0f;
};

struct AudioState {
    SDL_AudioSpec playbackSpec{ SDL_AUDIO_F32, 2, 44100 };
    SDL_AudioStream* stream = nullptr;
    bool subsystemInitialized = false;
    bool mixerInitialized = false;
    std::vector<DecodedSoundSample> characterSamples;
    std::vector<DecodedSoundSample> systemSamples;
    std::vector<DecodedSoundSample> commonSamples;
    std::vector<DecodedSoundSample> fightSamples;
    DecodedSoundSample stageMusicSample;
    std::filesystem::path stageMusicPath;
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

