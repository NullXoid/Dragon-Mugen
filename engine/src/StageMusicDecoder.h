#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

// Internal stage-music decoder helper. Include only after DecodedSoundSample is
// declared by App.cpp.

inline std::optional<DecodedSoundSample> loadDecodedStageMusicSample(
    const std::filesystem::path& path,
    const SDL_AudioSpec& playbackSpec,
    int group,
    int index) {
    if (path.empty() || !std::filesystem::exists(path)) {
        return std::nullopt;
    }

    const std::string pathText = path.string();
    MIX_AudioDecoder* decoder = MIX_CreateAudioDecoder(pathText.c_str(), 0);
    if (!decoder) {
        SDL_Log("Stage BGM decode open failed %s: %s", pathText.c_str(), SDL_GetError());
        return std::nullopt;
    }

    DecodedSoundSample out;
    out.group = group;
    out.index = index;

    std::vector<std::uint8_t> buffer(64 * 1024);
    for (;;) {
        const int decodedBytes = MIX_DecodeAudio(decoder, buffer.data(), static_cast<int>(buffer.size()), &playbackSpec);
        if (decodedBytes < 0) {
            SDL_Log("Stage BGM decode failed %s: %s", pathText.c_str(), SDL_GetError());
            out.audio.clear();
            break;
        }
        if (decodedBytes == 0) {
            break;
        }
        const int floatCount = decodedBytes / static_cast<int>(sizeof(float));
        const auto* floats = reinterpret_cast<const float*>(buffer.data());
        out.audio.insert(out.audio.end(), floats, floats + floatCount);
    }

    MIX_DestroyAudioDecoder(decoder);
    if (out.audio.empty()) {
        return std::nullopt;
    }
    return out;
}
