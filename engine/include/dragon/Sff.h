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

struct SffSprite {
    int group = 0;
    int image = 0;
    int axisX = 0;
    int axisY = 0;
    int linkedIndex = 0;
    bool sharedPalette = false;
    std::uint32_t dataOffset = 0;
    std::uint32_t dataLength = 0;
};

struct SffArchive {
    std::filesystem::path path;
    int groups = 0;
    std::vector<SffSprite> sprites;
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
