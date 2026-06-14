#pragma once

#include <filesystem>
#include <string_view>

namespace dragon {

enum class CompatibilityProfile {
    Mugen2001,
    Mugen10,
    Mugen11,
    Dragon,
};

enum class DragonRuntimeMode {
    Dragon,
    ClassicMugen,
};

struct LocalCoord {
    int width = 320;
    int height = 240;
};

struct CompatibilityContext {
    DragonRuntimeMode runtimeMode = DragonRuntimeMode::Dragon;
    CompatibilityProfile contentProfile = CompatibilityProfile::Mugen11;
    LocalCoord localCoord;
    bool gameSidecarAvailable = false;
    bool characterSidecarAvailable = false;
    bool stageSidecarAvailable = false;
    bool legacyOpenBorStageSection = false;
    bool dragonExtensionsAvailable = false;
    bool arenaExtensionsAvailable = false;
};

const char* compatibilityProfileName(CompatibilityProfile profile);
const char* dragonRuntimeModeName(DragonRuntimeMode mode);

CompatibilityProfile resolveCompatibilityProfile(std::string_view mugenVersion);
LocalCoord parseLocalCoord(std::string_view value, LocalCoord fallback = {});

CompatibilityContext makeCompatibilityContext(
    DragonRuntimeMode mode,
    CompatibilityProfile contentProfile,
    LocalCoord localCoord,
    bool gameSidecarAvailable,
    bool characterSidecarAvailable,
    bool stageSidecarAvailable,
    bool legacyOpenBorStageSection);

bool usesMugenSemantics(const CompatibilityContext& context);
bool allowsDragonExtensions(const CompatibilityContext& context);
bool allowsArenaExtensions(const CompatibilityContext& context);
float resolveLocalCoordScaleX(const CompatibilityContext& context, float targetWidth = 320.0f);
float resolveLocalCoordScaleY(const CompatibilityContext& context, float targetHeight = 240.0f);

std::filesystem::path characterDragonSidecarPath(const std::filesystem::path& characterDef);
std::filesystem::path stageDragonSidecarPath(const std::filesystem::path& stageDef);
std::filesystem::path gameDragonSidecarPath(const std::filesystem::path& gameRoot);

} // namespace dragon
