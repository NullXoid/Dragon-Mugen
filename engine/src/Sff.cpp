#include "dragon/Sff.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace dragon {
namespace {

std::vector<std::uint8_t> readAllBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Could not open binary file: " + path.string());
    }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

std::uint16_t u16le(const std::vector<std::uint8_t>& bytes, size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

std::int16_t i16le(const std::vector<std::uint8_t>& bytes, size_t offset) {
    return static_cast<std::int16_t>(u16le(bytes, offset));
}

std::uint32_t u32le(const std::vector<std::uint8_t>& bytes, size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::vector<std::uint8_t> reversedPalette(const Palette& palette) {
    std::vector<std::uint8_t> reversed(768);
    for (int i = 0; i < 256; ++i) {
        const auto src = static_cast<size_t>((255 - i) * 3);
        const auto dst = static_cast<size_t>(i * 3);
        reversed[dst + 0] = palette.rgb[src + 0];
        reversed[dst + 1] = palette.rgb[src + 1];
        reversed[dst + 2] = palette.rgb[src + 2];
    }
    return reversed;
}

std::vector<std::uint8_t> paletteFromPcx(const std::vector<std::uint8_t>& pcx, const DecodeOptions& options) {
    if (options.fallbackPalette && options.preferFallbackPalette) {
        return options.reverseFallbackPalette
            ? reversedPalette(*options.fallbackPalette)
            : options.fallbackPalette->rgb;
    }
    if (pcx.size() >= 769 && pcx[pcx.size() - 769] == 0x0C) {
        return std::vector<std::uint8_t>(pcx.end() - 768, pcx.end());
    }
    if (options.fallbackPalette && options.fallbackPalette->rgb.size() >= 768) {
        return options.reverseFallbackPalette
            ? reversedPalette(*options.fallbackPalette)
            : options.fallbackPalette->rgb;
    }
    std::vector<std::uint8_t> grayscale(768);
    for (int i = 0; i < 256; ++i) {
        grayscale[static_cast<size_t>(i * 3 + 0)] = static_cast<std::uint8_t>(i);
        grayscale[static_cast<size_t>(i * 3 + 1)] = static_cast<std::uint8_t>(i);
        grayscale[static_cast<size_t>(i * 3 + 2)] = static_cast<std::uint8_t>(i);
    }
    return grayscale;
}

} // namespace

Palette loadActPalette(const std::filesystem::path& path) {
    Palette palette;
    palette.rgb = readAllBytes(path);
    if (palette.rgb.size() < 768) {
        throw std::runtime_error("ACT palette is too small: " + path.string());
    }
    palette.rgb.resize(768);
    return palette;
}

SffArchive loadSffArchive(const std::filesystem::path& path) {
    SffArchive archive;
    archive.path = path;
    archive.bytes = readAllBytes(path);
    if (archive.bytes.size() < 512 || std::memcmp(archive.bytes.data(), "ElecbyteSpr\0", 12) != 0) {
        throw std::runtime_error("Invalid SFF v1 archive: " + path.string());
    }

    archive.groups = static_cast<int>(u32le(archive.bytes, 16));
    const auto spriteCount = u32le(archive.bytes, 20);
    auto subfileOffset = u32le(archive.bytes, 24);
    const auto subheaderSize = u32le(archive.bytes, 28);
    if (subheaderSize < 32) {
        throw std::runtime_error("Unsupported SFF subheader size: " + path.string());
    }

    for (std::uint32_t index = 0; index < spriteCount && subfileOffset != 0; ++index) {
        if (subfileOffset + 32 > archive.bytes.size()) {
            throw std::runtime_error("SFF subheader points past end of file: " + path.string());
        }

        SffSprite sprite;
        const auto nextOffset = u32le(archive.bytes, subfileOffset);
        sprite.dataLength = u32le(archive.bytes, subfileOffset + 4);
        sprite.axisX = i16le(archive.bytes, subfileOffset + 8);
        sprite.axisY = i16le(archive.bytes, subfileOffset + 10);
        sprite.group = i16le(archive.bytes, subfileOffset + 12);
        sprite.image = i16le(archive.bytes, subfileOffset + 14);
        sprite.linkedIndex = i16le(archive.bytes, subfileOffset + 16);
        sprite.sharedPalette = archive.bytes[subfileOffset + 18] != 0;
        sprite.dataOffset = subfileOffset + subheaderSize;
        archive.sprites.push_back(sprite);
        subfileOffset = nextOffset;
    }

    return archive;
}

const SffSprite* findSprite(const SffArchive& archive, int group, int image) {
    for (const auto& sprite : archive.sprites) {
        if (sprite.group == group && sprite.image == image) {
            return &sprite;
        }
    }
    return nullptr;
}

std::optional<DecodedSprite> decodeSffSprite(const SffArchive& archive, const SffSprite& sprite, DecodeOptions options) {
    const SffSprite* source = &sprite;
    if (source->dataLength == 0 && source->linkedIndex >= 0 && source->linkedIndex < static_cast<int>(archive.sprites.size())) {
        source = &archive.sprites[static_cast<size_t>(source->linkedIndex)];
    }
    if (source->dataLength == 0 || source->dataOffset + source->dataLength > archive.bytes.size()) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> pcx(
        archive.bytes.begin() + source->dataOffset,
        archive.bytes.begin() + source->dataOffset + source->dataLength);

    if (pcx.size() < 128 || pcx[0] != 0x0A || pcx[2] != 1 || pcx[3] != 8) {
        return std::nullopt;
    }

    const int xmin = u16le(pcx, 4);
    const int ymin = u16le(pcx, 6);
    const int xmax = u16le(pcx, 8);
    const int ymax = u16le(pcx, 10);
    const int width = xmax - xmin + 1;
    const int height = ymax - ymin + 1;
    const int planes = pcx[65];
    const int bytesPerLine = u16le(pcx, 66);
    if (width <= 0 || height <= 0 || planes != 1 || bytesPerLine <= 0) {
        return std::nullopt;
    }

    const size_t paletteMarker = pcx.size() >= 769 && pcx[pcx.size() - 769] == 0x0C
        ? pcx.size() - 769
        : pcx.size();
    std::vector<std::uint8_t> indices(static_cast<size_t>(bytesPerLine * height));
    size_t read = 128;
    size_t write = 0;
    while (read < paletteMarker && write < indices.size()) {
        const std::uint8_t value = pcx[read++];
        if ((value & 0xC0) == 0xC0 && read < paletteMarker) {
            const int count = value & 0x3F;
            const std::uint8_t runValue = pcx[read++];
            for (int i = 0; i < count && write < indices.size(); ++i) {
                indices[write++] = runValue;
            }
        } else {
            indices[write++] = value;
        }
    }

    const auto palette = paletteFromPcx(pcx, options);
    DecodedSprite decoded;
    decoded.width = width;
    decoded.height = height;
    decoded.axisX = sprite.axisX;
    decoded.axisY = sprite.axisY;
    decoded.rgba.resize(static_cast<size_t>(width * height * 4));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::uint8_t color = indices[static_cast<size_t>(y * bytesPerLine + x)];
            const size_t rgba = static_cast<size_t>((y * width + x) * 4);
            decoded.rgba[rgba + 0] = palette[static_cast<size_t>(color * 3 + 0)];
            decoded.rgba[rgba + 1] = palette[static_cast<size_t>(color * 3 + 1)];
            decoded.rgba[rgba + 2] = palette[static_cast<size_t>(color * 3 + 2)];
            decoded.rgba[rgba + 3] = options.transparentColorZero && color == 0 ? 0 : 255;
        }
    }

    return decoded;
}

} // namespace dragon
