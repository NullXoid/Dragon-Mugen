#include "dragon/Compatibility.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

namespace dragon {
namespace {

std::string trimCopy(std::string_view value) {
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return std::string(value.substr(first, last - first));
}

std::string lowercaseCopy(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

bool contains(std::string_view value, std::string_view needle) {
    return value.find(needle) != std::string_view::npos;
}

int parsePositiveInt(std::string_view value, int fallback) {
    try {
        size_t consumed = 0;
        const int parsed = std::stoi(trimCopy(value), &consumed, 10);
        return consumed == 0 || parsed <= 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

} // namespace

const char* compatibilityProfileName(CompatibilityProfile profile) {
    switch (profile) {
    case CompatibilityProfile::Mugen2001:
        return "Mugen2001";
    case CompatibilityProfile::Mugen10:
        return "Mugen10";
    case CompatibilityProfile::Mugen11:
        return "Mugen11";
    case CompatibilityProfile::Dragon:
        return "Dragon";
    }
    return "Mugen11";
}

const char* dragonRuntimeModeName(DragonRuntimeMode mode) {
    switch (mode) {
    case DragonRuntimeMode::Dragon:
        return "Dragon";
    case DragonRuntimeMode::ClassicMugen:
        return "ClassicMugen";
    }
    return "Dragon";
}

CompatibilityProfile resolveCompatibilityProfile(std::string_view mugenVersion) {
    const std::string value = lowercaseCopy(trimCopy(mugenVersion));
    if (value.empty()) {
        return CompatibilityProfile::Mugen11;
    }
    if (contains(value, "dragon")) {
        return CompatibilityProfile::Dragon;
    }
    if (contains(value, "1.1")) {
        return CompatibilityProfile::Mugen11;
    }
    if (contains(value, "1.0")) {
        return CompatibilityProfile::Mugen10;
    }
    if (contains(value, "2001") || contains(value, "04,14,2001") || contains(value, "04.14.2001")) {
        return CompatibilityProfile::Mugen2001;
    }
    return CompatibilityProfile::Mugen11;
}

LocalCoord parseLocalCoord(std::string_view value, LocalCoord fallback) {
    const std::string text = trimCopy(value);
    const auto comma = text.find(',');
    if (comma == std::string::npos) {
        return fallback;
    }
    return LocalCoord{
        parsePositiveInt(std::string_view(text).substr(0, comma), fallback.width),
        parsePositiveInt(std::string_view(text).substr(comma + 1), fallback.height),
    };
}

CompatibilityContext makeCompatibilityContext(
    DragonRuntimeMode mode,
    CompatibilityProfile contentProfile,
    LocalCoord localCoord,
    bool gameSidecarAvailable,
    bool characterSidecarAvailable,
    bool stageSidecarAvailable,
    bool legacyOpenBorStageSection) {
    CompatibilityContext context;
    context.runtimeMode = mode;
    context.contentProfile = contentProfile;
    context.localCoord = localCoord;
    context.gameSidecarAvailable = gameSidecarAvailable;
    context.characterSidecarAvailable = characterSidecarAvailable;
    context.stageSidecarAvailable = stageSidecarAvailable;
    context.legacyOpenBorStageSection = legacyOpenBorStageSection;
    context.dragonExtensionsAvailable =
        mode == DragonRuntimeMode::Dragon
        && (gameSidecarAvailable || characterSidecarAvailable || stageSidecarAvailable || legacyOpenBorStageSection);
    context.arenaExtensionsAvailable =
        context.dragonExtensionsAvailable && (stageSidecarAvailable || legacyOpenBorStageSection);
    return context;
}

bool usesMugenSemantics(const CompatibilityContext& context) {
    return context.contentProfile != CompatibilityProfile::Dragon;
}

bool allowsDragonExtensions(const CompatibilityContext& context) {
    return context.dragonExtensionsAvailable;
}

bool allowsArenaExtensions(const CompatibilityContext& context) {
    return context.arenaExtensionsAvailable;
}

float resolveLocalCoordScaleX(const CompatibilityContext& context, float targetWidth) {
    return context.localCoord.width > 0 ? targetWidth / static_cast<float>(context.localCoord.width) : 1.0f;
}

float resolveLocalCoordScaleY(const CompatibilityContext& context, float targetHeight) {
    return context.localCoord.height > 0 ? targetHeight / static_cast<float>(context.localCoord.height) : 1.0f;
}

std::filesystem::path characterDragonSidecarPath(const std::filesystem::path& characterDef) {
    if (characterDef.empty()) {
        return {};
    }
    return characterDef.parent_path() / (characterDef.stem().string() + ".dragon.def");
}

std::filesystem::path stageDragonSidecarPath(const std::filesystem::path& stageDef) {
    if (stageDef.empty()) {
        return {};
    }
    return stageDef.parent_path() / (stageDef.stem().string() + ".dragon.def");
}

std::filesystem::path gameDragonSidecarPath(const std::filesystem::path& gameRoot) {
    if (gameRoot.empty()) {
        return {};
    }
    return gameRoot / "data" / "dragon.def";
}

} // namespace dragon
