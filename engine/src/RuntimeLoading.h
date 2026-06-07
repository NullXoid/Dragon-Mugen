#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local AppState, TextureSprite, AnimationClip,
// SystemScreenAssets, StageBackgroundElement, character/stage loading helpers,
// and M.U.G.E.N runtime resource types. Include only from App.cpp after those
// dependencies are defined.

TextureSprite loadCharacterSprite(SDL_Renderer* renderer, const CharacterFiles& files, int group, int image) {
    if (files.sprite.empty() || !std::filesystem::exists(files.sprite)) {
        return {};
    }

    const auto sff = loadSffArchive(files.sprite);
    const auto* sprite = findSprite(sff, group, image);
    if (!sprite) {
        return {};
    }

    DecodeOptions options;
    options.transparentColorZero = true;
    std::optional<Palette> palette;
    if (!files.palette.empty() && std::filesystem::exists(files.palette)) {
        palette = loadActPalette(files.palette);
        options.fallbackPalette = &*palette;
        options.reverseFallbackPalette = true;
    }
    const auto decoded = decodeSffSprite(sff, *sprite, options);
    if (!decoded) {
        return {};
    }
    return makeTextureSprite(renderer, *decoded);
}

TextureSprite loadSystemSpriteFromArchive(
    SDL_Renderer* renderer,
    const SffArchive& sff,
    int group,
    int image,
    bool transparentColorZero) {
    const auto* sprite = findSprite(sff, group, image);
    if (!sprite) {
        return {};
    }
    DecodeOptions options;
    options.transparentColorZero = transparentColorZero;
    const auto decoded = decodeSffSprite(sff, *sprite, options);
    return decoded ? makeTextureSprite(renderer, *decoded) : TextureSprite{};
}

SystemScreenAssets loadSystemScreenAssets(SDL_Renderer* renderer, const std::filesystem::path& gameRoot) {
    SystemScreenAssets assets;
    const auto systemSff = gameRoot / "data" / "system.sff";
    if (!std::filesystem::exists(systemSff)) {
        return assets;
    }

    try {
        const auto sff = loadSffArchive(systemSff);
        assets.titleLogo = loadSystemSpriteFromArchive(renderer, sff, 0, 0, true);
        assets.titleTop = loadSystemSpriteFromArchive(renderer, sff, 5, 0, false);
        assets.titleFloor = loadSystemSpriteFromArchive(renderer, sff, 5, 1, false);
        assets.titleShade = loadSystemSpriteFromArchive(renderer, sff, 5, 2, true);
        assets.selectBackdrop = loadSystemSpriteFromArchive(renderer, sff, 100, 0, false);
        assets.selectShadow = loadSystemSpriteFromArchive(renderer, sff, 100, 1, true);
        assets.selectTitleA = loadSystemSpriteFromArchive(renderer, sff, 102, 0, true);
        assets.selectTitleB = loadSystemSpriteFromArchive(renderer, sff, 102, 1, true);
        assets.selectTitleC = loadSystemSpriteFromArchive(renderer, sff, 102, 2, true);
        assets.selectCell = loadSystemSpriteFromArchive(renderer, sff, 150, 0, true);
        assets.selectP1Cursor = loadSystemSpriteFromArchive(renderer, sff, 160, 0, true);
        assets.selectP1Done = loadSystemSpriteFromArchive(renderer, sff, 161, 0, true);
    } catch (const std::exception& ex) {
        SDL_Log("system.sff load failed: %s", ex.what());
    }
    return assets;
}

TextureSprite loadCharacterIconSprite(SDL_Renderer* renderer, const std::filesystem::path& gameRoot, const CharacterSlot& character) {
    try {
        const CharacterFiles files = resolveCharacterFiles(gameRoot, character);
        return loadCharacterSprite(renderer, files, 9000, 0);
    } catch (const std::exception& ex) {
        SDL_Log("Character icon load failed %s: %s", character.displayName.c_str(), ex.what());
        return {};
    }
}

TextureSprite loadCharacterFaceSprite(SDL_Renderer* renderer, const std::filesystem::path& gameRoot, const CharacterSlot& character) {
    try {
        const CharacterFiles files = resolveCharacterFiles(gameRoot, character);
        return loadCharacterSprite(renderer, files, 9000, 1);
    } catch (const std::exception& ex) {
        SDL_Log("Character face load failed %s: %s", character.displayName.c_str(), ex.what());
        return {};
    }
}

std::vector<StageBackgroundElement> loadStageBackground(SDL_Renderer* renderer, const StageSlot& stage) {
    auto sffPath = stage.defPath;
    sffPath.replace_extension(".sff");
    if (!std::filesystem::exists(sffPath)) {
        return {};
    }

    const auto sff = loadSffArchive(sffPath);
    const auto doc = parseMugenTextFile(stage.defPath);
    std::vector<StageBackgroundElement> elements;
    for (const auto& section : doc.sections) {
        if (!(section.name == "BG" || startsWithNoCase(section.name, "BG "))) {
            continue;
        }
        const auto* spriteNo = findProperty(section, "spriteno");
        if (!spriteNo) {
            continue;
        }
        const auto pair = parsePair(spriteNo->value);
        if (!pair) {
            continue;
        }
        const auto* sprite = findSprite(sff, static_cast<int>(pair->first), static_cast<int>(pair->second));
        if (!sprite) {
            continue;
        }

        DecodeOptions options;
        options.transparentColorZero = true;
        if (const auto* mask = findProperty(section, "mask")) {
            options.transparentColorZero = trim(mask->value) != "0";
        }
        const auto decoded = decodeSffSprite(sff, *sprite, options);
        if (!decoded) {
            continue;
        }

        StageBackgroundElement element;
        element.sprite = makeTextureSprite(renderer, *decoded);
        if (const auto* start = findProperty(section, "start")) {
            if (const auto startPair = parsePair(start->value)) {
                element.x = startPair->first;
                element.y = startPair->second;
            }
        }
        if (const auto* delta = findProperty(section, "delta")) {
            if (const auto deltaPair = parsePair(delta->value)) {
                element.deltaX = deltaPair->first;
                element.deltaY = deltaPair->second;
            }
        }
        if (const auto* tile = findProperty(section, "tile")) {
            if (const auto tilePair = parsePair(tile->value)) {
                element.tileX = tilePair->first != 0;
                element.tileY = tilePair->second != 0;
            }
        }
        if (const auto* layerNo = findProperty(section, "layerno")) {
            try {
                element.layerNo = std::stoi(layerNo->value);
            } catch (...) {
                element.layerNo = 0;
            }
        }
        elements.push_back(element);
    }
    return elements;
}

void destroyTextureSprite(TextureSprite& sprite);
void destroyAnimationClips(std::vector<AnimationClip>& clips);
void destroyArenaFighterClips(AppState& state);

LoadedContentSummary buildContentSummary(
    const CharacterSlot& character,
    const CharacterFiles& files,
    const StageSlot* stage) {
    LoadedContentSummary summary;
    summary.characterName = character.displayName;
    summary.characterAuthor = character.author;
    summary.stageName = stage ? stage->displayName : "Unknown";

    try {
        if (!files.anim.empty() && std::filesystem::exists(files.anim)) {
            summary.airActions = countSectionsWithPrefix(parseMugenTextFile(files.anim), "Begin Action");
        }
        for (const auto& stateFile : files.stateFiles) {
            summary.cnsStates += countSectionsWithPrefix(parseMugenTextFile(stateFile), "Statedef");
        }
        if (!files.cmd.empty() && std::filesystem::exists(files.cmd)) {
            summary.cmdCommands = countSectionsWithPrefix(parseMugenTextFile(files.cmd), "Command");
        }
        if (stage && std::filesystem::exists(stage->defPath)) {
            const auto stageDoc = parseMugenTextFile(stage->defPath);
            for (const auto& section : stageDoc.sections) {
                if (section.name == "BG" || startsWithNoCase(section.name, "BG ")) {
                    ++summary.stageBackgrounds;
                }
            }
        }
    } catch (const std::exception& ex) {
        SDL_Log("Content summary failed: %s", ex.what());
    }

    return summary;
}

void selectPreferredStage(AppState& state) {
    const CharacterSlot* character = sessionP1CharacterSlot(state.selection);
    if (!character) {
        return;
    }
    const int stageIndex = findStageIndexByDefPath(state.selection, character->preferredStagePath);
    if (stageIndex >= 0) {
        state.selection.selectedStage = stageIndex;
    }
}

bool loadSelectedCharacterRuntime(SDL_Renderer* renderer, AppState& state) {
    const int p1Index = sessionP1CharacterIndex(state.selection);
    const CharacterSlot* character = characterSlotAt(state.selection, p1Index);
    if (!character) {
        return false;
    }

    std::vector<AnimationClip> clips;
    std::vector<AnimationClip> opponentClips;
    std::vector<std::vector<AnimationClip>> arenaClips;
    TextureSprite largePortrait;
    std::vector<StateDefinition> stateDefs;
    std::vector<HitDefinition> hitDefs;
    std::vector<CommandStateEntry> commandEntries;
    std::vector<CommandDefinition> commandDefinitions;
    std::vector<DecodedSoundSample> characterSamples;
    std::vector<std::string> victoryQuotes;
    CharacterConstants constants;

    try {
        const CharacterFiles files = resolveCharacterFiles(state.gameRoot, *character);
        constants = loadCharacterConstants(files);
        stateDefs = loadStateDefinitions(files, constants);
        hitDefs = loadHitDefinitions(files);
        commandDefinitions = loadCommandDefinitions(files);
        commandEntries = loadCommandStateEntries(files);
        victoryQuotes = loadVictoryQuotes(files);
        if (state.audio.stream) {
            characterSamples = loadDecodedSoundSamples(files.sound, state.audio.playbackSpec);
        }
        clips = loadCharacterClips(renderer, files);
        largePortrait = loadCharacterSprite(renderer, files, 9000, 1);
        if (state.frontend.pendingMode == PendingMode::SingleFight) {
            if (const CharacterSlot* opponent = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
                try {
                    const CharacterFiles opponentFiles = resolveCharacterFiles(state.gameRoot, *opponent);
                    opponentClips = loadCharacterClips(renderer, opponentFiles);
                } catch (const std::exception& ex) {
                    SDL_Log("Opponent visual load failed %s: %s", opponent->displayName.c_str(), ex.what());
                }
            }
        } else if (state.frontend.pendingMode == PendingMode::Arena) {
            arenaClips.resize(static_cast<size_t>(arenaFighterCount(state)));
            for (size_t i = 1; i < arenaClips.size(); ++i) {
                if (const CharacterSlot* cpu = characterSlotAt(state.selection, arenaFighterCharacterIndex(state, i))) {
                    try {
                        const CharacterFiles cpuFiles = resolveCharacterFiles(state.gameRoot, *cpu);
                        arenaClips[i] = loadCharacterClips(renderer, cpuFiles);
                    } catch (const std::exception& ex) {
                        SDL_Log("Arena CPU visual load failed %s: %s", cpu->displayName.c_str(), ex.what());
                    }
                }
            }
        }

        destroyAnimationClips(state.characterClips);
        destroyAnimationClips(state.opponentCharacterClips);
        destroyArenaFighterClips(state);
        destroyTextureSprite(state.characterLargePortrait);
        state.runtimeEffects.clear();
        state.characterClips = std::move(clips);
        state.opponentCharacterClips = std::move(opponentClips);
        state.arenaFighterClips = std::move(arenaClips);
        state.characterLargePortrait = largePortrait;
        largePortrait = {};
        state.stateDefs = std::move(stateDefs);
        state.hitDefs = std::move(hitDefs);
        state.commandEntries = std::move(commandEntries);
        state.commandDefinitions = std::move(commandDefinitions);
        state.victoryQuotes = std::move(victoryQuotes);
        state.characterConstants = constants;
        state.audio.activeVoices.clear();
        if (state.audio.stream) {
            SDL_ClearAudioStream(state.audio.stream);
        }
        state.audio.characterSamples = std::move(characterSamples);
        state.content = buildContentSummary(*character, files, selectedStageSlot(state.selection));
        state.selection.loadedP1Character = p1Index;

        SDL_Log(
            "Character loaded: %s actions=%zu states=%zu hitdefs=%zu command-defs=%zu command-states=%zu sounds=%zu",
            character->displayName.c_str(),
            state.characterClips.size(),
            state.stateDefs.size(),
            state.hitDefs.size(),
            state.commandDefinitions.size(),
            state.commandEntries.size(),
            state.audio.characterSamples.size());
        if (const CharacterSlot* opponent = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
            SDL_Log("Opponent visuals loaded: %s actions=%zu", opponent->displayName.c_str(), state.opponentCharacterClips.size());
        }
        if (state.frontend.pendingMode == PendingMode::Arena) {
            for (size_t i = 1; i < state.arenaFighterClips.size(); ++i) {
                SDL_Log(
                    "Arena CPU visuals loaded: %s actions=%zu",
                    arenaFighterName(state, i).c_str(),
                    state.arenaFighterClips[i].size());
            }
        }
        return true;
    } catch (const std::exception& ex) {
        destroyAnimationClips(clips);
        destroyAnimationClips(opponentClips);
        for (auto& clipSet : arenaClips) {
            destroyAnimationClips(clipSet);
        }
        destroyTextureSprite(largePortrait);
        SDL_Log("Selected character load failed %s: %s", character->displayName.c_str(), ex.what());
        return false;
    }
}

void unloadCharacterRuntime(AppState& state) {
    destroyAnimationClips(state.characterClips);
    destroyAnimationClips(state.opponentCharacterClips);
    destroyArenaFighterClips(state);
    destroyTextureSprite(state.characterLargePortrait);
    for (auto& element : state.stageBackground) {
        destroyTextureSprite(element.sprite);
    }
    state.stageBackground.clear();
    state.stageBackgroundStageIndex = -1;
    state.stateDefs.clear();
    state.hitDefs.clear();
    state.commandEntries.clear();
    state.commandDefinitions.clear();
    state.victoryQuotes.clear();
    state.characterConstants = CharacterConstants{};
    state.audio.activeVoices.clear();
    if (state.audio.stream) {
        SDL_ClearAudioStream(state.audio.stream);
    }
    state.audio.characterSamples.clear();
    state.runtimeEffects.clear();
    state.content = LoadedContentSummary{};
    state.selection.loadedP1Character = -1;
    state.fightSessionPrepared = false;
    state.fightSessionLoadFailed = false;
}

void destroySystemScreenAssets(SystemScreenAssets& assets) {
    destroyTextureSprite(assets.titleLogo);
    destroyTextureSprite(assets.titleTop);
    destroyTextureSprite(assets.titleFloor);
    destroyTextureSprite(assets.titleShade);
    destroyTextureSprite(assets.selectBackdrop);
    destroyTextureSprite(assets.selectShadow);
    destroyTextureSprite(assets.selectTitleA);
    destroyTextureSprite(assets.selectTitleB);
    destroyTextureSprite(assets.selectTitleC);
    destroyTextureSprite(assets.selectCell);
    destroyTextureSprite(assets.selectP1Cursor);
    destroyTextureSprite(assets.selectP1Done);
}

void destroyStageBackground(std::vector<StageBackgroundElement>& background) {
    for (auto& element : background) {
        destroyTextureSprite(element.sprite);
    }
    background.clear();
}

void loadVisualAssets(SDL_Renderer* renderer, AppState& state) {
    try {
        destroySystemScreenAssets(state.systemScreens);
        state.systemScreens = loadSystemScreenAssets(renderer, state.gameRoot);
        state.fightFxClips = loadFightFxClips(renderer, state.gameRoot);
        for (auto& sprite : state.characterIconSprites) {
            destroyTextureSprite(sprite);
        }
        for (auto& sprite : state.characterFaceSprites) {
            destroyTextureSprite(sprite);
        }
        state.characterIconSprites.clear();
        state.characterFaceSprites.clear();
        state.characterIconSprites.reserve(state.selection.characters.size());
        state.characterFaceSprites.reserve(state.selection.characters.size());
        for (const auto& character : state.selection.characters) {
            state.characterIconSprites.push_back(loadCharacterIconSprite(renderer, state.gameRoot, character));
            state.characterFaceSprites.push_back(loadCharacterFaceSprite(renderer, state.gameRoot, character));
        }
    } catch (const std::exception& ex) {
        SDL_Log("Visual asset load failed: %s", ex.what());
    }
}

void destroyTextureSprite(TextureSprite& sprite) {
    if (sprite.texture) {
        SDL_DestroyTexture(sprite.texture);
        sprite.texture = nullptr;
    }
}

void destroyAnimationClips(std::vector<AnimationClip>& clips) {
    for (auto& clip : clips) {
        for (auto& frame : clip.frames) {
            destroyTextureSprite(frame.sprite);
        }
    }
    clips.clear();
}

void destroyArenaFighterClips(AppState& state) {
    for (auto& clipSet : state.arenaFighterClips) {
        destroyAnimationClips(clipSet);
    }
    state.arenaFighterClips.clear();
}

void destroyVisualAssets(AppState& state) {
    destroyAnimationClips(state.characterClips);
    destroyAnimationClips(state.opponentCharacterClips);
    destroyArenaFighterClips(state);
    destroyAnimationClips(state.fightFxClips);
    state.runtimeEffects.clear();
    destroySystemScreenAssets(state.systemScreens);
    destroyTextureSprite(state.characterLargePortrait);
    for (auto& sprite : state.characterIconSprites) {
        destroyTextureSprite(sprite);
    }
    state.characterIconSprites.clear();
    for (auto& sprite : state.characterFaceSprites) {
        destroyTextureSprite(sprite);
    }
    state.characterFaceSprites.clear();
    destroyStageBackground(state.stageBackground);
    state.stageBackgroundStageIndex = -1;
}
