#pragma once

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
    std::string music;
    int order = 1;
    bool includeStage = true;
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
    int maxPower = 3000;
    int attackDistance = 160;
    float velocityRunFwdX = 4.6f;
    float velocityRunFwdY = 0.0f;
    float velocityRunBackX = -4.5f;
    float velocityRunBackY = -3.8f;
    float movementYAccel = 0.42f;
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

} // namespace dragon
