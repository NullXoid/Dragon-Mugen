#pragma once

#include "dragon/Compatibility.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dragon {

struct CharacterSlot {
    std::string id;
    std::string displayName;
    std::string author;
    std::filesystem::path folder;
    std::filesystem::path defPath;
    std::filesystem::path preferredStagePath;
    std::filesystem::path dragonSidecarPath;
    std::string music;
    std::string mugenVersion;
    CompatibilityProfile compatibilityProfile = CompatibilityProfile::Mugen11;
    LocalCoord localCoord;
    int order = 1;
    bool includeStage = true;
    bool dragonSidecarAvailable = false;
};

struct CharacterFiles {
    std::filesystem::path root;
    std::filesystem::path def;
    std::filesystem::path cmd;
    std::vector<std::filesystem::path> stateFiles;
    std::filesystem::path sprite;
    std::filesystem::path anim;
    std::filesystem::path sound;
    std::filesystem::path palette;
};

struct CharacterSize {
    float groundBack = 15.0f;
    float groundFront = 16.0f;
    float airBack = 12.0f;
    float airFront = 12.0f;
    float height = 60.0f;
};

struct CharacterConstants {
    CharacterSize size;
    float sizeScaleX = 1.0f;
    float sizeScaleY = 1.0f;
    int maxPower = 3000;
    int liedownTime = 60;
    int attackDistance = 160;
    float velocityWalkFwdX = 2.4f;
    float velocityWalkBackX = -2.2f;
    float velocityRunFwdX = 4.6f;
    float velocityRunFwdY = 0.0f;
    float velocityRunBackX = -4.5f;
    float velocityRunBackY = -3.8f;
    float velocityJumpNeuX = 0.0f;
    float velocityJumpY = -8.4f;
    float velocityJumpFwdX = 2.5f;
    float velocityJumpBackX = -2.55f;
    float velocityRunJumpFwdX = 4.0f;
    float velocityRunJumpFwdY = -8.1f;
    float velocityRunJumpBackX = -2.55f;
    float velocityRunJumpBackY = -8.1f;
    float velocityAirJumpNeuX = 0.0f;
    float velocityAirJumpY = -8.1f;
    float velocityAirJumpFwdX = 2.5f;
    float velocityAirJumpBackX = -2.55f;
    int movementAirJumpNum = 0;
    int movementAirJumpHeight = 35;
    float movementStandFriction = 0.85f;
    float movementCrouchFriction = 0.82f;
    float movementStandFrictionThreshold = 2.0f;
    float movementCrouchFrictionThreshold = 2.0f;
    float movementYAccel = 0.42f;
    float movementDownBounceOffsetX = 0.0f;
    float movementDownBounceOffsetY = 20.0f;
    float movementDownBounceYAccel = 0.4f;
    float movementDownBounceGroundLevel = 12.0f;
    float movementDownFrictionThreshold = 0.05f;
};

struct StageSlot {
    std::string id;
    std::string displayName;
    std::string author;
    std::filesystem::path defPath;
    float cameraStartx = 0.0f;
    float cameraStarty = 0.0f;
    float cameraBoundleft = -150.0f;
    float cameraBoundright = 150.0f;
    float cameraBoundhigh = -25.0f;
    float cameraBoundlow = 0.0f;
    float cameraVerticalfollow = 0.2f;
    float cameraFloortension = 0.0f;
    float cameraTension = 60.0f;
    float p1startx = -70.0f;
    float p1starty = 0.0f;
    float p2startx = 70.0f;
    float p2starty = 0.0f;
    float zoffset = 200.0f;
    float leftbound = -1000.0f;
    float rightbound = 1000.0f;
    float screenleft = 15.0f;
    float screenright = 15.0f;
    bool openborScrolling = false;
    float openborScrollStartx = 0.0f;
    float openborScrollEndx = 0.0f;
    float openborScrollSpeed = 2.4f;
    float openborScrollLead = 96.0f;
    std::filesystem::path dragonSidecarPath;
    bool dragonSidecarAvailable = false;
    bool legacyOpenBorSection = false;
};

inline bool hasMugenRuntimeRootFiles(const std::filesystem::path& gameRoot) {
    return std::filesystem::exists(gameRoot / "data" / "select.def")
        && std::filesystem::exists(gameRoot / "data" / "system.def");
}

inline const char* mugenRuntimeRootRequirementText() {
    return "data/select.def and data/system.def";
}

std::vector<CharacterSlot> loadCharacters(const std::filesystem::path& gameRoot);
std::vector<StageSlot> loadStages(const std::filesystem::path& gameRoot);
CharacterFiles resolveCharacterFiles(const std::filesystem::path& gameRoot, const CharacterSlot& character);
CharacterConstants loadCharacterConstants(const CharacterFiles& files);
bool hasGameDragonSidecar(const std::filesystem::path& gameRoot);

} // namespace dragon
