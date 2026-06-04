#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace dragon {

struct SndSample {
    int group = 0;
    int index = 0;
    std::uint32_t dataOffset = 0;
    std::uint32_t dataLength = 0;
};

struct SndArchive {
    std::filesystem::path path;
    std::vector<SndSample> samples;
    std::vector<std::uint8_t> bytes;
};

SndArchive loadSndArchive(const std::filesystem::path& path);
const SndSample* findSndSample(const SndArchive& archive, int group, int index);

} // namespace dragon
