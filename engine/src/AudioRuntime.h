#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local AudioState, decoded sound sample types,
// SDL audio handles, and runtime sound helpers. Include only from App.cpp after
// those dependencies are defined.

std::vector<DecodedSoundSample> loadDecodedSoundSamples(const std::filesystem::path& path, const SDL_AudioSpec& playbackSpec) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    const auto archive = loadSndArchive(path);
    std::vector<DecodedSoundSample> decodedSamples;
    decodedSamples.reserve(archive.samples.size());
    for (const auto& sample : archive.samples) {
        const auto* sampleData = archive.bytes.data() + sample.dataOffset;
        SDL_IOStream* io = SDL_IOFromConstMem(sampleData, sample.dataLength);
        if (!io) {
            SDL_Log("SND sample IO failed %s %d,%d: %s", path.string().c_str(), sample.group, sample.index, SDL_GetError());
            continue;
        }

        SDL_AudioSpec sourceSpec{};
        Uint8* wavData = nullptr;
        Uint32 wavLength = 0;
        if (!SDL_LoadWAV_IO(io, true, &sourceSpec, &wavData, &wavLength)) {
            SDL_Log("SND WAV decode failed %s %d,%d: %s", path.string().c_str(), sample.group, sample.index, SDL_GetError());
            continue;
        }

        Uint8* convertedData = nullptr;
        int convertedLength = 0;
        const bool converted = SDL_ConvertAudioSamples(
            &sourceSpec,
            wavData,
            static_cast<int>(wavLength),
            &playbackSpec,
            &convertedData,
            &convertedLength);
        SDL_free(wavData);
        if (!converted || !convertedData || convertedLength <= 0) {
            SDL_Log("SND sample convert failed %s %d,%d: %s", path.string().c_str(), sample.group, sample.index, SDL_GetError());
            continue;
        }

        DecodedSoundSample out;
        out.group = sample.group;
        out.index = sample.index;
        const int floatCount = convertedLength / static_cast<int>(sizeof(float));
        const auto* convertedFloats = reinterpret_cast<const float*>(convertedData);
        out.audio.assign(convertedFloats, convertedFloats + floatCount);
        SDL_free(convertedData);
        decodedSamples.push_back(std::move(out));
    }
    return decodedSamples;
}

void applyMenuSoundConfig(const MugenSection& section, std::string_view key, int& group, int& index) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return;
    }
    const auto parsed = parseSoundValue(property->value);
    if (!parsed || parsed->group < 0 || parsed->index < 0) {
        return;
    }
    group = parsed->group;
    index = parsed->index;
}

void loadSystemAudio(AppState& state) {
    const std::filesystem::path dataRoot = state.gameRoot / "data";
    const std::filesystem::path systemDefPath = dataRoot / "system.def";
    std::filesystem::path systemSndPath = dataRoot / "system.snd";

    if (std::filesystem::exists(systemDefPath)) {
        const auto systemDef = parseMugenTextFile(systemDefPath);
        if (const auto* files = findSection(systemDef, "Files")) {
            if (const auto* snd = findProperty(*files, "snd")) {
                systemSndPath = resolveContentPath(dataRoot, snd->value);
            }
        }
        if (const auto* titleInfo = findSection(systemDef, "Title Info")) {
            applyMenuSoundConfig(
                *titleInfo,
                "cursor.move.snd",
                state.audio.menuCursorMoveSoundGroup,
                state.audio.menuCursorMoveSoundIndex);
            applyMenuSoundConfig(
                *titleInfo,
                "cursor.done.snd",
                state.audio.menuCursorDoneSoundGroup,
                state.audio.menuCursorDoneSoundIndex);
            applyMenuSoundConfig(
                *titleInfo,
                "cancel.snd",
                state.audio.menuCancelSoundGroup,
                state.audio.menuCancelSoundIndex);
        }
    }

    state.audio.systemSamples = loadDecodedSoundSamples(systemSndPath, state.audio.playbackSpec);
}

bool initAudio(AppState& state) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("SDL audio init failed: %s", SDL_GetError());
        return false;
    }
    state.audio.subsystemInitialized = true;

    state.audio.stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &state.audio.playbackSpec,
        nullptr,
        nullptr);
    if (!state.audio.stream) {
        SDL_Log("SDL audio stream failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(state.audio.stream)) {
        SDL_Log("SDL audio resume failed: %s", SDL_GetError());
        return false;
    }

    try {
        loadSystemAudio(state);
        state.audio.commonSamples = loadDecodedSoundSamples(state.gameRoot / "data" / "common.snd", state.audio.playbackSpec);
        state.audio.fightSamples = loadDecodedSoundSamples(state.gameRoot / "data" / "fight.snd", state.audio.playbackSpec);
        SDL_Log(
            "SND loaded: system %zu common %zu fight %zu",
            state.audio.systemSamples.size(),
            state.audio.commonSamples.size(),
            state.audio.fightSamples.size());
    } catch (const std::exception& ex) {
        SDL_Log("SND load failed: %s", ex.what());
    }
    return true;
}

void destroyAudioAssets(AppState& state) {
    if (state.audio.stream) {
        SDL_ClearAudioStream(state.audio.stream);
        SDL_DestroyAudioStream(state.audio.stream);
        state.audio.stream = nullptr;
    }
    state.audio.activeVoices.clear();
    state.audio.mixBuffer.clear();
    state.audio.characterSamples.clear();
    state.audio.systemSamples.clear();
    state.audio.commonSamples.clear();
    state.audio.fightSamples.clear();
    if (state.audio.subsystemInitialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        state.audio.subsystemInitialized = false;
    }
}

const DecodedSoundSample* findDecodedSoundSample(const std::vector<DecodedSoundSample>& samples, int group, int index) {
    for (const auto& sample : samples) {
        if (sample.group == group && sample.index == index) {
            return &sample;
        }
    }
    return nullptr;
}

const DecodedSoundSample* findPlaybackSound(
    const AppState& state,
    int group,
    int index,
    bool forceCommon = false,
    int ownerIndex = -1) {
    if (forceCommon) {
        if (const auto* sample = findDecodedSoundSample(state.audio.commonSamples, group, index)) {
            return sample;
        }
        return findDecodedSoundSample(state.audio.fightSamples, group, index);
    }
    if (const auto* arenaSamples = arenaCharacterSamplesForOwner(state, ownerIndex)) {
        if (const auto* sample = findDecodedSoundSample(*arenaSamples, group, index)) {
            return sample;
        }
    }
    if (const auto* sample = findDecodedSoundSample(state.audio.characterSamples, group, index)) {
        return sample;
    }
    if (const auto* sample = findDecodedSoundSample(state.audio.commonSamples, group, index)) {
        return sample;
    }
    return findDecodedSoundSample(state.audio.fightSamples, group, index);
}

int soundSampleFrames(const DecodedSoundSample& sample) {
    return static_cast<int>(sample.audio.size() / 2);
}

void stopSoundChannel(AudioState& audio, int channel) {
    if (channel < 0) {
        return;
    }
    audio.activeVoices.erase(
        std::remove_if(audio.activeVoices.begin(), audio.activeVoices.end(), [channel](const ActiveSoundVoice& voice) {
            return voice.channel == channel;
        }),
        audio.activeVoices.end());
}

bool soundChannelActive(const AudioState& audio, int channel) {
    return channel >= 0 && std::any_of(audio.activeVoices.begin(), audio.activeVoices.end(), [channel](const ActiveSoundVoice& voice) {
        return voice.channel == channel;
    });
}

bool sameSoundTriggeredRecently(const AppState& state, int group, int index, int channel) {
    constexpr int kDuplicateSoundWindowFrames = 2;
    return std::any_of(state.audio.activeVoices.begin(), state.audio.activeVoices.end(), [&state, group, index, channel](const ActiveSoundVoice& voice) {
        return voice.group == group
            && voice.index == index
            && voice.channel == channel
            && state.frame - voice.startedFrame <= kDuplicateSoundWindowFrames;
    });
}

void capActiveSoundVoices(AudioState& audio) {
    constexpr size_t kMaxActiveSfxVoices = 32;
    while (audio.activeVoices.size() >= kMaxActiveSfxVoices) {
        auto oldest = std::max_element(audio.activeVoices.begin(), audio.activeVoices.end(), [](const ActiveSoundVoice& lhs, const ActiveSoundVoice& rhs) {
            return lhs.frameOffset < rhs.frameOffset;
        });
        if (oldest == audio.activeVoices.end()) {
            break;
        }
        audio.activeVoices.erase(oldest);
    }
}

void playSystemSound(AppState& state, int group, int index) {
    if (!state.audio.stream || group < 0 || index < 0) {
        return;
    }

    const DecodedSoundSample* sample = findDecodedSoundSample(state.audio.systemSamples, group, index);
    if (!sample || sample->audio.empty()) {
        SDL_Log("System SND sample not found: %d,%d", group, index);
        return;
    }

    constexpr int kUiSoundChannel = 64;
    if (sameSoundTriggeredRecently(state, group, index, kUiSoundChannel)) {
        return;
    }

    stopSoundChannel(state.audio, kUiSoundChannel);
    capActiveSoundVoices(state.audio);
    state.audio.activeVoices.push_back(ActiveSoundVoice{
        sample,
        group,
        index,
        kUiSoundChannel,
        0,
        state.frame,
        1.0f,
        false,
    });
}

void playMenuCursorMoveSound(AppState& state) {
    playSystemSound(state, state.audio.menuCursorMoveSoundGroup, state.audio.menuCursorMoveSoundIndex);
}

void playMenuCursorDoneSound(AppState& state) {
    playSystemSound(state, state.audio.menuCursorDoneSoundGroup, state.audio.menuCursorDoneSoundIndex);
}

void playMenuCancelSound(AppState& state) {
    playSystemSound(state, state.audio.menuCancelSoundGroup, state.audio.menuCancelSoundIndex);
}

void playSound(
    AppState& state,
    int group,
    int index,
    bool forceCommon = false,
    int channel = -1,
    bool lowPriority = false,
    float gain = 1.0f,
    bool loop = false,
    int ownerIndex = -1) {
    if (!state.audio.stream || group < 0 || index < 0) {
        return;
    }

    const DecodedSoundSample* sample = findPlaybackSound(state, group, index, forceCommon, ownerIndex);
    if (!sample || sample->audio.empty()) {
        SDL_Log("SND sample not found: %d,%d", group, index);
        return;
    }

    if (sameSoundTriggeredRecently(state, group, index, channel)) {
        return;
    }

    if (channel >= 0) {
        if (lowPriority && soundChannelActive(state.audio, channel)) {
            return;
        }
        stopSoundChannel(state.audio, channel);
    }

    capActiveSoundVoices(state.audio);
    state.audio.activeVoices.push_back(ActiveSoundVoice{
        sample,
        group,
        index,
        channel,
        0,
        state.frame,
        gain,
        loop,
    });
}

void mixActiveSoundVoices(AppState& state, int framesToWrite) {
    if (!state.audio.stream || framesToWrite <= 0) {
        return;
    }

    constexpr int kChannels = 2;
    state.audio.mixBuffer.assign(static_cast<size_t>(framesToWrite * kChannels), 0.0f);

    for (auto voiceIt = state.audio.activeVoices.begin(); voiceIt != state.audio.activeVoices.end();) {
        ActiveSoundVoice& voice = *voiceIt;
        if (!voice.sample || voice.sample->audio.empty()) {
            voiceIt = state.audio.activeVoices.erase(voiceIt);
            continue;
        }

        const int sampleFrames = soundSampleFrames(*voice.sample);
        if (sampleFrames <= 0) {
            voiceIt = state.audio.activeVoices.erase(voiceIt);
            continue;
        }

        bool finished = false;
        for (int frame = 0; frame < framesToWrite; ++frame) {
            if (voice.frameOffset >= sampleFrames) {
                if (voice.loop) {
                    voice.frameOffset = 0;
                } else {
                    finished = true;
                    break;
                }
            }

            const size_t source = static_cast<size_t>(voice.frameOffset * kChannels);
            const size_t dest = static_cast<size_t>(frame * kChannels);
            state.audio.mixBuffer[dest] += voice.sample->audio[source] * voice.gain;
            state.audio.mixBuffer[dest + 1] += voice.sample->audio[source + 1] * voice.gain;
            ++voice.frameOffset;
        }

        if (finished) {
            voiceIt = state.audio.activeVoices.erase(voiceIt);
        } else {
            ++voiceIt;
        }
    }

    for (float& sample : state.audio.mixBuffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    const int byteCount = static_cast<int>(state.audio.mixBuffer.size() * sizeof(float));
    if (!SDL_PutAudioStreamData(state.audio.stream, state.audio.mixBuffer.data(), byteCount)) {
        SDL_Log("SND mixer queue failed: %s", SDL_GetError());
    }
}

void updateAudioMixer(AppState& state) {
    if (!state.audio.stream || state.audio.activeVoices.empty()) {
        return;
    }

    constexpr int kChannels = 2;
    constexpr int kBytesPerFrame = static_cast<int>(sizeof(float)) * kChannels;
    constexpr int kTargetQueuedFrames = 2048;
    constexpr int kMixChunkFrames = 512;

    int queuedBytes = SDL_GetAudioStreamQueued(state.audio.stream);
    if (queuedBytes < 0) {
        queuedBytes = 0;
    }
    int queuedFrames = queuedBytes / kBytesPerFrame;

    while (queuedFrames < kTargetQueuedFrames && !state.audio.activeVoices.empty()) {
        const int framesToWrite = std::min(kMixChunkFrames, kTargetQueuedFrames - queuedFrames);
        mixActiveSoundVoices(state, framesToWrite);
        queuedFrames += framesToWrite;
    }
}
