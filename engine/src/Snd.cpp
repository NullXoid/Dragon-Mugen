#include "dragon/Snd.h"

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

std::uint32_t u32le(const std::vector<std::uint8_t>& bytes, size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

SndArchive loadSndArchive(const std::filesystem::path& path) {
    SndArchive archive;
    archive.path = path;
    archive.bytes = readAllBytes(path);
    if (archive.bytes.size() < 512 || std::memcmp(archive.bytes.data(), "ElecbyteSnd\0", 12) != 0) {
        throw std::runtime_error("Invalid SND archive: " + path.string());
    }

    const auto sampleCount = u32le(archive.bytes, 16);
    auto subfileOffset = u32le(archive.bytes, 20);
    constexpr std::uint32_t subheaderSize = 16;

    for (std::uint32_t i = 0; i < sampleCount && subfileOffset != 0; ++i) {
        if (subfileOffset + subheaderSize > archive.bytes.size()) {
            throw std::runtime_error("SND subheader points past end of file: " + path.string());
        }

        const auto nextOffset = u32le(archive.bytes, subfileOffset);
        SndSample sample;
        sample.dataLength = u32le(archive.bytes, subfileOffset + 4);
        sample.group = static_cast<int>(u32le(archive.bytes, subfileOffset + 8));
        sample.index = static_cast<int>(u32le(archive.bytes, subfileOffset + 12));
        sample.dataOffset = subfileOffset + subheaderSize;
        if (sample.dataOffset + sample.dataLength > archive.bytes.size()) {
            throw std::runtime_error("SND sample points past end of file: " + path.string());
        }
        archive.samples.push_back(sample);
        subfileOffset = nextOffset;
    }

    return archive;
}

const SndSample* findSndSample(const SndArchive& archive, int group, int index) {
    for (const auto& sample : archive.samples) {
        if (sample.group == group && sample.index == index) {
            return &sample;
        }
    }
    return nullptr;
}

} // namespace dragon
