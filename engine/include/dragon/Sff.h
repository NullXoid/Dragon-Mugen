#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dragon {

struct Palette {
    std::vector<std::uint8_t> rgb;
};

enum class SffArchiveVersion {
    V1,
    V2,
};

enum class SffSpriteEncoding {
    Pcx8,
    Png,
    Unsupported,
};

struct SffSprite {
    int group = 0;
    int image = 0;
    int axisX = 0;
    int axisY = 0;
    int linkedIndex = 0;
    bool sharedPalette = false;
    std::uint32_t dataOffset = 0;
    std::uint32_t dataLength = 0;
    int width = 0;
    int height = 0;
    int colorDepth = 0;
    int paletteIndex = -1;
    SffSpriteEncoding encoding = SffSpriteEncoding::Pcx8;
};

struct SffPalette {
    int group = 0;
    int image = 0;
    int colorCount = 0;
    int linkedIndex = 0;
    std::uint32_t dataOffset = 0;
    std::uint32_t dataLength = 0;
    std::vector<std::uint8_t> rgba;
};

struct SffArchive {
    std::filesystem::path path;
    SffArchiveVersion version = SffArchiveVersion::V1;
    int versionMajor = 1;
    int versionMinor = 0;
    int versionPatch = 0;
    int versionBuild = 0;
    int groups = 0;
    std::vector<SffSprite> sprites;
    std::vector<SffPalette> palettes;
    std::vector<std::uint8_t> bytes;
};

struct DecodedSprite {
    int width = 0;
    int height = 0;
    int axisX = 0;
    int axisY = 0;
    std::vector<std::uint8_t> rgba;
};

struct DecodeOptions {
    const Palette* fallbackPalette = nullptr;
    bool preferFallbackPalette = false;
    bool reverseFallbackPalette = false;
    bool transparentColorZero = true;
};

Palette loadActPalette(const std::filesystem::path& path);
SffArchive loadSffArchive(const std::filesystem::path& path);
const SffSprite* findSprite(const SffArchive& archive, int group, int image);
std::optional<DecodedSprite> decodeSffSprite(const SffArchive& archive, const SffSprite& sprite, DecodeOptions options = {});

} // namespace dragon
