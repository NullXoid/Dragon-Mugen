#include "dragon/Sff.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

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

void requireRange(const std::vector<std::uint8_t>& bytes, size_t offset, size_t length, const std::filesystem::path& path) {
    if (offset > bytes.size() || length > bytes.size() - offset) {
        throw std::runtime_error("SFF data points past end of file: " + path.string());
    }
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

std::optional<size_t> findPngPayloadOffset(const std::vector<std::uint8_t>& bytes, size_t offset, size_t length) {
    static constexpr std::uint8_t kPngSignature[] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    const size_t searchEnd = std::min(offset + length, offset + 64);
    for (size_t i = offset; i + sizeof(kPngSignature) <= searchEnd; ++i) {
        if (std::memcmp(bytes.data() + i, kPngSignature, sizeof(kPngSignature)) == 0) {
            return i;
        }
    }
    return std::nullopt;
}

const SffPalette* paletteForSprite(const SffArchive& archive, const SffSprite& sprite) {
    if (sprite.paletteIndex >= 0 && sprite.paletteIndex < static_cast<int>(archive.palettes.size())) {
        const auto& palette = archive.palettes[static_cast<size_t>(sprite.paletteIndex)];
        if (!palette.rgba.empty()) {
            return &palette;
        }
    }
    return nullptr;
}

std::optional<DecodedSprite> decodeIndexedPngSprite(
    SDL_Surface* surface,
    const SffSprite& sprite,
    const SffPalette& palette,
    const DecodeOptions& options) {
    if (surface->format != SDL_PIXELFORMAT_INDEX8 || !surface->pixels) {
        return std::nullopt;
    }

    DecodedSprite decoded;
    decoded.width = surface->w;
    decoded.height = surface->h;
    decoded.axisX = sprite.axisX;
    decoded.axisY = sprite.axisY;
    decoded.rgba.resize(static_cast<size_t>(decoded.width * decoded.height * 4));

    const auto* pixels = static_cast<const std::uint8_t*>(surface->pixels);
    const size_t maxColors = palette.rgba.size() / 4;
    for (int y = 0; y < decoded.height; ++y) {
        const auto* src = pixels + static_cast<size_t>(y * surface->pitch);
        auto* dst = decoded.rgba.data() + static_cast<size_t>(y * decoded.width * 4);
        for (int x = 0; x < decoded.width; ++x) {
            const std::uint8_t index = src[x];
            if (index < maxColors) {
                const auto* color = palette.rgba.data() + static_cast<size_t>(index) * 4;
                dst[x * 4 + 0] = color[0];
                dst[x * 4 + 1] = color[1];
                dst[x * 4 + 2] = color[2];
                dst[x * 4 + 3] = options.transparentColorZero && index == 0 ? 0 : color[3];
            } else {
                dst[x * 4 + 0] = 0;
                dst[x * 4 + 1] = 0;
                dst[x * 4 + 2] = 0;
                dst[x * 4 + 3] = 0;
            }
        }
    }
    return decoded;
}

std::optional<DecodedSprite> decodePngSprite(
    const SffArchive& archive,
    const SffSprite& sprite,
    const SffSprite& source,
    const DecodeOptions& options) {
    const auto pngOffset = findPngPayloadOffset(archive.bytes, source.dataOffset, source.dataLength);
    if (!pngOffset) {
        return std::nullopt;
    }

    const size_t payloadLength = source.dataOffset + source.dataLength - *pngOffset;
    SDL_IOStream* io = SDL_IOFromConstMem(archive.bytes.data() + *pngOffset, payloadLength);
    if (!io) {
        return std::nullopt;
    }

    SDL_Surface* surface = IMG_Load_IO(io, true);
    if (!surface) {
        return std::nullopt;
    }

    if (const auto* palette = paletteForSprite(archive, source)) {
        if (source.colorDepth <= 8) {
            if (auto indexed = decodeIndexedPngSprite(surface, sprite, *palette, options)) {
                SDL_DestroySurface(surface);
                return indexed;
            }
        }
    }

    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!converted) {
        return std::nullopt;
    }

    DecodedSprite decoded;
    decoded.width = converted->w;
    decoded.height = converted->h;
    decoded.axisX = sprite.axisX;
    decoded.axisY = sprite.axisY;
    decoded.rgba.resize(static_cast<size_t>(decoded.width * decoded.height * 4));
    const auto* pixels = static_cast<const std::uint8_t*>(converted->pixels);
    for (int y = 0; y < decoded.height; ++y) {
        const auto* src = pixels + static_cast<size_t>(y * converted->pitch);
        auto* dst = decoded.rgba.data() + static_cast<size_t>(y * decoded.width * 4);
        std::memcpy(dst, src, static_cast<size_t>(decoded.width * 4));
    }
    SDL_DestroySurface(converted);
    return decoded;
}

void loadSffV1Archive(SffArchive& archive) {
    archive.version = SffArchiveVersion::V1;
    archive.versionMajor = 1;

    archive.groups = static_cast<int>(u32le(archive.bytes, 16));
    const auto spriteCount = u32le(archive.bytes, 20);
    auto subfileOffset = u32le(archive.bytes, 24);
    const auto subheaderSize = u32le(archive.bytes, 28);
    if (subheaderSize < 32) {
        throw std::runtime_error("Unsupported SFF v1 subheader size: " + archive.path.string());
    }

    for (std::uint32_t index = 0; index < spriteCount && subfileOffset != 0; ++index) {
        requireRange(archive.bytes, subfileOffset, 32, archive.path);

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
        sprite.encoding = SffSpriteEncoding::Pcx8;
        archive.sprites.push_back(sprite);
        subfileOffset = nextOffset;
    }
}

SffSpriteEncoding sffV2Encoding(std::uint8_t format) {
    switch (format) {
    case 10:
        return SffSpriteEncoding::Png;
    default:
        return SffSpriteEncoding::Unsupported;
    }
}

void loadSffV2Archive(SffArchive& archive) {
    archive.version = SffArchiveVersion::V2;
    archive.versionMajor = 2;
    archive.versionMinor = archive.bytes[14];
    archive.versionPatch = archive.bytes[13];
    archive.versionBuild = archive.bytes[12];

    const auto spriteOffset = u32le(archive.bytes, 0x24);
    const auto spriteCount = u32le(archive.bytes, 0x28);
    const auto paletteOffset = u32le(archive.bytes, 0x2c);
    const auto paletteCount = u32le(archive.bytes, 0x30);
    const auto ldataOffset = u32le(archive.bytes, 0x34);
    const auto ldataLength = u32le(archive.bytes, 0x38);
    const auto tdataOffset = u32le(archive.bytes, 0x3c);
    const auto tdataLength = u32le(archive.bytes, 0x40);
    requireRange(archive.bytes, spriteOffset, static_cast<size_t>(spriteCount) * 28, archive.path);
    if (paletteCount > 0) {
        requireRange(archive.bytes, paletteOffset, static_cast<size_t>(paletteCount) * 16, archive.path);
    }
    if (ldataOffset != 0) {
        requireRange(archive.bytes, ldataOffset, ldataLength, archive.path);
    }
    if (tdataOffset != 0) {
        requireRange(archive.bytes, tdataOffset, tdataLength, archive.path);
    }

    for (std::uint32_t index = 0; index < paletteCount; ++index) {
        const size_t offset = static_cast<size_t>(paletteOffset) + static_cast<size_t>(index) * 16;
        SffPalette palette;
        palette.group = u16le(archive.bytes, offset);
        palette.image = u16le(archive.bytes, offset + 2);
        palette.colorCount = u16le(archive.bytes, offset + 4);
        palette.linkedIndex = u16le(archive.bytes, offset + 6);
        const auto dataOffset = u32le(archive.bytes, offset + 8);
        palette.dataLength = u32le(archive.bytes, offset + 12);
        palette.dataOffset = ldataOffset + dataOffset;
        if (palette.dataLength > 0) {
            requireRange(archive.bytes, palette.dataOffset, palette.dataLength, archive.path);
            palette.rgba.assign(
                archive.bytes.begin() + palette.dataOffset,
                archive.bytes.begin() + palette.dataOffset + palette.dataLength);
        }
        archive.palettes.push_back(std::move(palette));
    }

    for (auto& palette : archive.palettes) {
        if (!palette.rgba.empty()) {
            continue;
        }
        if (palette.linkedIndex >= 0 && palette.linkedIndex < static_cast<int>(archive.palettes.size())) {
            palette.rgba = archive.palettes[static_cast<size_t>(palette.linkedIndex)].rgba;
        }
    }

    for (std::uint32_t index = 0; index < spriteCount; ++index) {
        const size_t offset = static_cast<size_t>(spriteOffset) + static_cast<size_t>(index) * 28;
        SffSprite sprite;
        sprite.group = u16le(archive.bytes, offset);
        sprite.image = u16le(archive.bytes, offset + 2);
        sprite.width = u16le(archive.bytes, offset + 4);
        sprite.height = u16le(archive.bytes, offset + 6);
        sprite.axisX = i16le(archive.bytes, offset + 8);
        sprite.axisY = i16le(archive.bytes, offset + 10);
        sprite.linkedIndex = u16le(archive.bytes, offset + 12);
        const auto format = archive.bytes[offset + 14];
        sprite.colorDepth = archive.bytes[offset + 15];
        const auto dataOffset = u32le(archive.bytes, offset + 16);
        sprite.dataLength = u32le(archive.bytes, offset + 20);
        sprite.paletteIndex = u16le(archive.bytes, offset + 24);
        const auto flags = u16le(archive.bytes, offset + 26);
        const bool usesTData = (flags & 0x01) != 0;
        const auto baseOffset = usesTData ? tdataOffset : ldataOffset;
        sprite.dataOffset = baseOffset + dataOffset;
        sprite.sharedPalette = true;
        sprite.encoding = sffV2Encoding(format);
        if (sprite.dataLength > 0) {
            requireRange(archive.bytes, sprite.dataOffset, sprite.dataLength, archive.path);
        }
        archive.sprites.push_back(sprite);
    }

    std::vector<int> groups;
    for (const auto& sprite : archive.sprites) {
        if (std::find(groups.begin(), groups.end(), sprite.group) == groups.end()) {
            groups.push_back(sprite.group);
        }
    }
    archive.groups = static_cast<int>(groups.size());
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
        throw std::runtime_error("Invalid SFF archive: " + path.string());
    }

    if (archive.bytes[15] >= 2) {
        loadSffV2Archive(archive);
    } else {
        archive.versionMinor = archive.bytes[14];
        archive.versionPatch = archive.bytes[13];
        archive.versionBuild = archive.bytes[12];
        loadSffV1Archive(archive);
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

    if (source->encoding == SffSpriteEncoding::Png) {
        return decodePngSprite(archive, sprite, *source, options);
    }

    if (source->encoding != SffSpriteEncoding::Pcx8) {
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
