#pragma once

#include <SDL3_image/SDL_image.h>

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

TextureSprite loadUiPngSprite(
    SDL_Renderer* renderer,
    const std::filesystem::path& gameRoot,
    const std::filesystem::path& relativePath,
    TextureFilter filter = TextureFilter::Nearest) {
    TextureSprite sprite;
    const auto path = gameRoot / relativePath;
    if (!std::filesystem::exists(path)) {
        SDL_Log("UI PNG missing: %s", path.string().c_str());
        return sprite;
    }

    SDL_Surface* surface = IMG_Load(path.string().c_str());
    if (!surface) {
        SDL_Log("UI PNG load failed %s: %s", path.string().c_str(), SDL_GetError());
        return sprite;
    }

    sprite.texture = SDL_CreateTextureFromSurface(renderer, surface);
    sprite.width = surface->w;
    sprite.height = surface->h;
    SDL_DestroySurface(surface);
    if (!sprite.texture) {
        SDL_Log("UI PNG texture failed %s: %s", path.string().c_str(), SDL_GetError());
        sprite.width = 0;
        sprite.height = 0;
        return sprite;
    }
    SDL_SetTextureBlendMode(sprite.texture, SDL_BLENDMODE_BLEND);
    setTextureSpriteFilterIntent(sprite, filter);
    return sprite;
}

CommandInputIconAtlas loadCommandInputIconAtlas(SDL_Renderer* renderer, const std::filesystem::path& gameRoot) {
    CommandInputIconAtlas atlas;
    const auto path = gameRoot / "data" / "ui" / "command_input_icons.png";
    if (!std::filesystem::exists(path)) {
        SDL_Log("Command input icon atlas missing: %s", path.string().c_str());
        return atlas;
    }

    SDL_Surface* surface = IMG_Load(path.string().c_str());
    if (!surface) {
        SDL_Log("Command input icon atlas load failed: %s", SDL_GetError());
        return atlas;
    }

    atlas.texture = SDL_CreateTextureFromSurface(renderer, surface);
    atlas.textureWidth = surface->w;
    atlas.textureHeight = surface->h;
    SDL_DestroySurface(surface);
    if (!atlas.texture) {
        SDL_Log("Command input icon atlas texture failed: %s", SDL_GetError());
        atlas.textureWidth = 0;
        atlas.textureHeight = 0;
        return atlas;
    }

    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    atlas.cellWidth = 24;
    atlas.cellHeight = 16;
    atlas.columns = 8;
    return atlas;
}

std::vector<StageBackgroundElement> loadStageBackground(SDL_Renderer* renderer, const StageSlot& stage) {
    auto sffPath = stage.defPath;
    sffPath.replace_extension(".sff");
    const auto doc = parseMugenTextFile(stage.defPath);
    if (const auto* bgDef = findSection(doc, "BGDef")) {
        if (const auto* spr = findProperty(*bgDef, "spr")) {
            const std::filesystem::path configured = unquote(trim(spr->value));
            if (!configured.empty()) {
                const auto direct = (stage.defPath.parent_path() / configured).lexically_normal();
                const auto fromRoot = (stage.defPath.parent_path().parent_path() / configured).lexically_normal();
                if (std::filesystem::exists(direct)) {
                    sffPath = direct;
                } else {
                    sffPath = fromRoot;
                }
            }
        }
    }
    if (!std::filesystem::exists(sffPath)) {
        return {};
    }

    const auto sff = loadSffArchive(sffPath);
    std::vector<StageBackgroundElement> elements;
    for (const auto& section : doc.sections) {
        if (!(section.name == "BG" || startsWithNoCase(section.name, "BG "))) {
            continue;
        }

        DecodeOptions options;
        options.transparentColorZero = true;
        if (const auto* mask = findProperty(section, "mask")) {
            options.transparentColorZero = trim(mask->value) != "0";
        }

        StageBackgroundElement element;
        const auto* type = findProperty(section, "type");
        const bool animated = type && equalsNoCase(trim(type->value), "anim");
        if (animated) {
            const auto* actionNo = findProperty(section, "actionno");
            if (!actionNo) {
                continue;
            }
            try {
                element.animation = loadSffClip(renderer, sff, doc, std::stoi(actionNo->value), options);
                if (element.animation.frames.empty()) {
                    continue;
                }
                element.animated = true;
            } catch (...) {
                continue;
            }
        } else {
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
            const auto decoded = decodeSffSprite(sff, *sprite, options);
            if (!decoded) {
                continue;
            }
            element.sprite = makeTextureSprite(renderer, *decoded);
        }

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
void destroyCommandInputIconAtlas(CommandInputIconAtlas& atlas);
void destroyAnimationClips(std::vector<AnimationClip>& clips);
void destroyCharacterRuntime(ArenaCharacterRuntime& runtime);
void destroyArenaFighterClips(AppState& state);
void destroyArenaFighterRuntimes(AppState& state);

CharacterFiles characterFilesWithPalette(CharacterFiles files, int requestedPaletteNo) {
    const int requested = std::clamp(requestedPaletteNo, 1, 12);
    if (!files.palettes.empty()) {
        const size_t requestedIndex = static_cast<size_t>(requested - 1);
        if (requestedIndex < files.palettes.size()
            && !files.palettes[requestedIndex].empty()
            && std::filesystem::exists(files.palettes[requestedIndex])) {
            files.palette = files.palettes[requestedIndex];
            files.paletteNo = requested;
            return files;
        }
        for (size_t i = 0; i < files.palettes.size(); ++i) {
            if (!files.palettes[i].empty() && std::filesystem::exists(files.palettes[i])) {
                files.palette = files.palettes[i];
                files.paletteNo = static_cast<int>(i + 1);
                return files;
            }
        }
    }
    files.paletteNo = requested;
    return files;
}

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

CompatibilityContext makeLoadedCompatibilityContext(const AppState& state, const CharacterSlot& character) {
    const StageSlot* stage = selectedStageSlot(state.selection);
    return makeCompatibilityContext(
        state.runtimeMode,
        character.compatibilityProfile,
        character.localCoord,
        hasGameDragonSidecar(state.gameRoot),
        character.dragonSidecarAvailable,
        stage ? stage->dragonSidecarAvailable : false,
        stage ? stage->legacyOpenBorSection : false);
}

bool loadSelectedCharacterRuntime(SDL_Renderer* renderer, AppState& state) {
    const int p1Index = sessionP1CharacterIndex(state.selection);
    const CharacterSlot* character = characterSlotAt(state.selection, p1Index);
    if (!character) {
        return false;
    }

    std::vector<AnimationClip> clips;
    std::vector<AnimationClip> opponentClips;
    ArenaCharacterRuntime opponentRuntime;
    std::vector<std::vector<AnimationClip>> arenaClips;
    std::vector<ArenaCharacterRuntime> arenaRuntimes;
    TextureSprite largePortrait;
    std::vector<StateDefinition> stateDefs;
    std::vector<HitDefinition> hitDefs;
    std::vector<CommandStateEntry> commandEntries;
    std::vector<CommandDefinition> commandDefinitions;
    std::vector<DecodedSoundSample> characterSamples;
    std::vector<std::string> victoryQuotes;
    CompatibilityContext compatibility;
    CharacterConstants constants;
    int characterPaletteNo = 1;
    int opponentPaletteNo = 1;
    std::filesystem::path characterPalettePath;
    std::filesystem::path opponentPalettePath;

    try {
        updateLoadingProgress(state.loadingProgress, 0.10f, "Resolving player files");
        presentVersusLoadingProgress(renderer, state);
        const CharacterFiles files = characterFilesWithPalette(resolveCharacterFiles(state.gameRoot, *character), 1);
        characterPaletteNo = files.paletteNo;
        characterPalettePath = files.palette;
        compatibility = makeLoadedCompatibilityContext(state, *character);
        constants = loadCharacterConstants(files);
        updateLoadingProgress(state.loadingProgress, 0.18f, "Loading player data");
        presentVersusLoadingProgress(renderer, state);
        stateDefs = loadStateDefinitions(files, constants);
        hitDefs = loadHitDefinitions(files);
        commandDefinitions = loadCommandDefinitions(files);
        commandEntries = loadCommandStateEntries(files);
        victoryQuotes = loadVictoryQuotes(files);
        updateLoadingProgress(state.loadingProgress, 0.28f, "Loading player media");
        presentVersusLoadingProgress(renderer, state);
        if (state.audio.stream) {
            characterSamples = loadDecodedSoundSamples(files.sound, state.audio.playbackSpec);
        }
        clips = loadCharacterClips(renderer, files);
        largePortrait = loadCharacterSprite(renderer, files, 9000, 1);
        updateLoadingProgress(state.loadingProgress, 0.38f, "Player ready");
        presentVersusLoadingProgress(renderer, state);
        if (state.frontend.pendingMode == PendingMode::Training) {
            updateLoadingProgress(state.loadingProgress, 0.46f, "Loading training dummy");
            presentVersusLoadingProgress(renderer, state);
            const CharacterFiles dummyFiles = characterFilesWithPalette(resolveCharacterFiles(state.gameRoot, *character), 2);
            opponentPaletteNo = dummyFiles.paletteNo;
            opponentPalettePath = dummyFiles.palette;
            opponentClips = loadCharacterClips(renderer, dummyFiles);
            updateLoadingProgress(state.loadingProgress, 0.62f, "Training dummy ready");
            presentVersusLoadingProgress(renderer, state);
        } else if (state.frontend.pendingMode == PendingMode::SingleFight) {
            if (const CharacterSlot* opponent = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
                updateLoadingProgress(state.loadingProgress, 0.44f, "Loading opponent data");
                presentVersusLoadingProgress(renderer, state);
                const CharacterFiles opponentFiles = characterFilesWithPalette(resolveCharacterFiles(state.gameRoot, *opponent), 2);
                opponentPaletteNo = opponentFiles.paletteNo;
                opponentPalettePath = opponentFiles.palette;
                opponentRuntime.name = opponent->displayName;
                opponentRuntime.paletteNo = opponentFiles.paletteNo;
                opponentRuntime.palettePath = opponentFiles.palette;
                opponentRuntime.compatibility = makeLoadedCompatibilityContext(state, *opponent);
                opponentRuntime.constants = loadCharacterConstants(opponentFiles);
                opponentRuntime.stateDefs = loadStateDefinitions(opponentFiles, opponentRuntime.constants);
                opponentRuntime.hitDefs = loadHitDefinitions(opponentFiles);
                opponentRuntime.commandDefinitions = loadCommandDefinitions(opponentFiles);
                opponentRuntime.commandEntries = loadCommandStateEntries(opponentFiles);
                opponentRuntime.victoryQuotes = loadVictoryQuotes(opponentFiles);
                if (state.audio.stream) {
                    opponentRuntime.samples = loadDecodedSoundSamples(opponentFiles.sound, state.audio.playbackSpec);
                }
                updateLoadingProgress(state.loadingProgress, 0.56f, "Loading opponent media");
                presentVersusLoadingProgress(renderer, state);
                opponentRuntime.clips = loadCharacterClips(renderer, opponentFiles);
                updateLoadingProgress(state.loadingProgress, 0.66f, "Opponent ready");
                presentVersusLoadingProgress(renderer, state);
            }
        } else if (state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story) {
            if (state.frontend.pendingMode == PendingMode::Story) {
                chooseStoryEnemyCharacters(state);
            }
            const int fighterCount = state.frontend.pendingMode == PendingMode::Story
                ? storyFighterCount()
                : arenaFighterCount(state);
            arenaClips.resize(static_cast<size_t>(fighterCount));
            arenaRuntimes.resize(static_cast<size_t>(fighterCount));
            for (int i = 0; i < fighterCount; ++i) {
                const float start = 0.40f + (0.22f * static_cast<float>(i) / static_cast<float>(std::max(1, fighterCount)));
                updateLoadingProgress(
                    state.loadingProgress,
                    start,
                    (state.frontend.pendingMode == PendingMode::Story ? "Loading story fighter " : "Loading arena fighter ")
                        + std::to_string(i + 1)
                        + "/"
                        + std::to_string(fighterCount));
                presentVersusLoadingProgress(renderer, state);
                const int characterIndex = state.frontend.pendingMode == PendingMode::Story
                    ? storyFighterCharacterIndex(state, static_cast<size_t>(i))
                    : arenaFighterCharacterIndex(state, static_cast<size_t>(i));
                const CharacterSlot* arenaCharacter = characterSlotAt(state.selection, characterIndex);
                if (!arenaCharacter) {
                    throw std::runtime_error(
                        state.frontend.pendingMode == PendingMode::Story
                            ? "Story fighter slot missing character"
                            : "Arena fighter slot missing character");
                }

                const CharacterFiles arenaFiles = characterFilesWithPalette(resolveCharacterFiles(state.gameRoot, *arenaCharacter), i + 1);
                auto& runtime = arenaRuntimes[static_cast<size_t>(i)];
                runtime.name = arenaCharacter->displayName;
                runtime.paletteNo = arenaFiles.paletteNo;
                runtime.palettePath = arenaFiles.palette;
                runtime.compatibility = makeLoadedCompatibilityContext(state, *arenaCharacter);
                runtime.constants = loadCharacterConstants(arenaFiles);
                runtime.stateDefs = loadStateDefinitions(arenaFiles, runtime.constants);
                runtime.hitDefs = loadHitDefinitions(arenaFiles);
                runtime.commandDefinitions = loadCommandDefinitions(arenaFiles);
                runtime.commandEntries = loadCommandStateEntries(arenaFiles);
                runtime.victoryQuotes = loadVictoryQuotes(arenaFiles);
                if (state.audio.stream) {
                    runtime.samples = loadDecodedSoundSamples(arenaFiles.sound, state.audio.playbackSpec);
                }
                runtime.clips = loadCharacterClips(renderer, arenaFiles);
                const float done = 0.42f + (0.24f * static_cast<float>(i + 1) / static_cast<float>(std::max(1, fighterCount)));
                updateLoadingProgress(
                    state.loadingProgress,
                    done,
                    (state.frontend.pendingMode == PendingMode::Story ? "Story fighter ready " : "Arena fighter ready ")
                        + std::to_string(i + 1)
                        + "/"
                        + std::to_string(fighterCount));
                presentVersusLoadingProgress(renderer, state);
            }
        }

        destroyAnimationClips(state.characterClips);
        destroyAnimationClips(state.opponentCharacterClips);
        destroyCharacterRuntime(state.opponentRuntime);
        destroyArenaFighterClips(state);
        destroyArenaFighterRuntimes(state);
        destroyTextureSprite(state.characterLargePortrait);
        state.runtimeEffects.clear();
        state.characterClips = std::move(clips);
        state.opponentCharacterClips = std::move(opponentClips);
        state.opponentRuntime = std::move(opponentRuntime);
        state.arenaFighterClips = std::move(arenaClips);
        state.arenaRuntimes = std::move(arenaRuntimes);
        state.characterLargePortrait = largePortrait;
        largePortrait = {};
        state.stateDefs = std::move(stateDefs);
        state.hitDefs = std::move(hitDefs);
        state.commandEntries = std::move(commandEntries);
        state.commandDefinitions = std::move(commandDefinitions);
        state.trainingMoveListEntriesCache.clear();
        state.trainingMoveListCacheValid = false;
        state.trainingMoveListCacheData = nullptr;
        state.trainingMoveListCacheCount = 0;
        state.victoryQuotes = std::move(victoryQuotes);
        state.characterConstants = constants;
        state.characterCompatibility = compatibility;
        state.characterPaletteNo = characterPaletteNo;
        state.characterPalettePath = characterPalettePath;
        state.opponentPaletteNo = opponentPaletteNo;
        state.opponentPalettePath = opponentPalettePath;
        state.audio.activeVoices.clear();
        state.audio.stageMusicSample.audio.clear();
        state.audio.stageMusicPath.clear();
        if (state.audio.stream) {
            SDL_ClearAudioStream(state.audio.stream);
        }
        state.audio.characterSamples = std::move(characterSamples);
        state.content = buildContentSummary(*character, files, selectedStageSlot(state.selection));
        state.selection.loadedP1Character = p1Index;

        SDL_Log(
            "Character loaded: %s pal=%d actions=%zu states=%zu hitdefs=%zu command-defs=%zu command-states=%zu sounds=%zu",
            character->displayName.c_str(),
            state.characterPaletteNo,
            state.characterClips.size(),
            state.stateDefs.size(),
            state.hitDefs.size(),
            state.commandDefinitions.size(),
            state.commandEntries.size(),
            state.audio.characterSamples.size());
        if (const CharacterSlot* opponent = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
            SDL_Log(
                "Opponent runtime loaded: %s pal=%d actions=%zu states=%zu hitdefs=%zu command-defs=%zu command-states=%zu sounds=%zu",
                opponent->displayName.c_str(),
                state.opponentRuntime.paletteNo,
                state.opponentRuntime.clips.size(),
                state.opponentRuntime.stateDefs.size(),
                state.opponentRuntime.hitDefs.size(),
                state.opponentRuntime.commandDefinitions.size(),
                state.opponentRuntime.commandEntries.size(),
                state.opponentRuntime.samples.size());
        }
        if (state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story) {
            for (size_t i = 0; i < state.arenaRuntimes.size(); ++i) {
                const auto& runtime = state.arenaRuntimes[i];
                SDL_Log(
                    "%s runtime loaded: %s pal=%d actions=%zu states=%zu hitdefs=%zu command-defs=%zu command-states=%zu sounds=%zu",
                    state.frontend.pendingMode == PendingMode::Story ? "Story" : "Arena",
                    runtime.name.c_str(),
                    runtime.paletteNo,
                    runtime.clips.size(),
                    runtime.stateDefs.size(),
                    runtime.hitDefs.size(),
                    runtime.commandDefinitions.size(),
                    runtime.commandEntries.size(),
                    runtime.samples.size());
            }
        }
        return true;
    } catch (const std::exception& ex) {
        destroyAnimationClips(clips);
        destroyAnimationClips(opponentClips);
        destroyCharacterRuntime(opponentRuntime);
        for (auto& clipSet : arenaClips) {
            destroyAnimationClips(clipSet);
        }
        for (auto& runtime : arenaRuntimes) {
            destroyAnimationClips(runtime.clips);
        }
        destroyTextureSprite(largePortrait);
        SDL_Log("Selected character load failed %s: %s", character->displayName.c_str(), ex.what());
        return false;
    }
}

void unloadCharacterRuntime(AppState& state) {
    destroyAnimationClips(state.characterClips);
    destroyAnimationClips(state.opponentCharacterClips);
    destroyCharacterRuntime(state.opponentRuntime);
    destroyArenaFighterClips(state);
    destroyArenaFighterRuntimes(state);
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
    state.trainingMoveListEntriesCache.clear();
    state.trainingMoveListCacheValid = false;
    state.trainingMoveListCacheData = nullptr;
    state.trainingMoveListCacheCount = 0;
    state.victoryQuotes.clear();
    state.characterCompatibility = CompatibilityContext{};
    state.characterConstants = CharacterConstants{};
    state.characterPaletteNo = 1;
    state.characterPalettePath.clear();
    state.opponentPaletteNo = 1;
    state.opponentPalettePath.clear();
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
    resetLoadingProgress(state.loadingProgress);
}

void destroySystemScreenAssets(SystemScreenAssets& assets) {
    destroyTextureSprite(assets.mainMenuBackground);
    destroyTextureSprite(assets.mainMenuLogo);
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
        for (auto& frame : element.animation.frames) {
            destroyTextureSprite(frame.sprite);
        }
    }
    background.clear();
}

void loadVisualAssets(SDL_Renderer* renderer, AppState& state) {
    try {
        destroySystemScreenAssets(state.systemScreens);
        state.systemScreens = loadSystemScreenAssets(renderer, state.gameRoot);
        state.systemScreens.mainMenu = loadMainMenuPresentationConfig(state.gameRoot);
        if (state.systemScreens.mainMenu.backgroundMode == MainMenuBackgroundMode::Image) {
            state.systemScreens.mainMenuBackground = loadUiPngSprite(
                renderer,
                state.gameRoot,
                state.systemScreens.mainMenu.backgroundPath,
                TextureFilter::Linear);
            if (!state.systemScreens.mainMenuBackground.texture) {
                state.systemScreens.mainMenu.backgroundMode = state.systemScreens.mainMenu.fallbackGrid
                    ? MainMenuBackgroundMode::Fallback
                    : MainMenuBackgroundMode::None;
            }
        }
        if (state.systemScreens.mainMenu.logoMode == MainMenuLogoMode::Image) {
            state.systemScreens.mainMenuLogo = loadUiPngSprite(
                renderer,
                state.gameRoot,
                state.systemScreens.mainMenu.logoPath,
                TextureFilter::Linear);
            if (!state.systemScreens.mainMenuLogo.texture) {
                state.systemScreens.mainMenu.logoMode = MainMenuLogoMode::None;
            }
        }
        destroyCommandInputIconAtlas(state.commandInputIcons);
        state.commandInputIcons = loadCommandInputIconAtlas(renderer, state.gameRoot);
        destroyTextureSprite(state.commandCompleteCheck);
        state.commandCompleteCheck = loadUiPngSprite(renderer, state.gameRoot, "data/ui/command_complete_check.png");
        destroyTextureSprite(state.storyForwardCueImage);
        std::filesystem::path storyForwardCueImagePath = "data/story/wave_clear_arrow.png";
        const std::filesystem::path storyBoardRoutePath = state.gameRoot / "data" / "story_boards.def";
        if (std::filesystem::exists(storyBoardRoutePath)) {
            try {
                StoryBoardRoute route = loadStoryBoardRouteFile(storyBoardRoutePath);
                if (!route.forwardCueImagePath.empty()) {
                    storyForwardCueImagePath = route.forwardCueImagePath;
                }
            } catch (const std::exception& ex) {
                SDL_Log("Story cue image route parse failed %s: %s", storyBoardRoutePath.string().c_str(), ex.what());
            }
        }
        if (std::filesystem::exists(state.gameRoot / storyForwardCueImagePath)) {
            state.storyForwardCueImage = loadUiPngSprite(
                renderer,
                state.gameRoot,
                storyForwardCueImagePath,
                TextureFilter::Linear);
        }
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

void destroyCommandInputIconAtlas(CommandInputIconAtlas& atlas) {
    if (atlas.texture) {
        SDL_DestroyTexture(atlas.texture);
    }
    atlas = CommandInputIconAtlas{};
}

void destroyAnimationClips(std::vector<AnimationClip>& clips) {
    for (auto& clip : clips) {
        for (auto& frame : clip.frames) {
            destroyTextureSprite(frame.sprite);
        }
    }
    clips.clear();
}

void destroyCharacterRuntime(ArenaCharacterRuntime& runtime) {
    destroyAnimationClips(runtime.clips);
    runtime = ArenaCharacterRuntime{};
}

void destroyArenaFighterClips(AppState& state) {
    for (auto& clipSet : state.arenaFighterClips) {
        destroyAnimationClips(clipSet);
    }
    state.arenaFighterClips.clear();
}

void destroyArenaFighterRuntimes(AppState& state) {
    for (auto& runtime : state.arenaRuntimes) {
        destroyCharacterRuntime(runtime);
    }
    state.arenaRuntimes.clear();
}

void destroyVisualAssets(AppState& state) {
    destroyAnimationClips(state.characterClips);
    destroyAnimationClips(state.opponentCharacterClips);
    destroyCharacterRuntime(state.opponentRuntime);
    destroyArenaFighterClips(state);
    destroyArenaFighterRuntimes(state);
    destroyAnimationClips(state.fightFxClips);
    state.runtimeEffects.clear();
    destroySystemScreenAssets(state.systemScreens);
    destroyCommandInputIconAtlas(state.commandInputIcons);
    destroyTextureSprite(state.commandCompleteCheck);
    destroyTextureSprite(state.storyForwardCueImage);
    destroyTextureSprite(state.shopDemo.shopBackdrop);
    destroyTextureSprite(state.shopDemo.shopCounterFront);
    destroyTextureSprite(state.shopDemo.shopkeeperPose);
    destroyTextureSprite(state.shopDemo.shopPlayerPose);
    for (auto& frame : state.shopDemo.shopPlayerWalkFrames) {
        destroyTextureSprite(frame);
    }
    destroyTextureSprite(state.shopDemo.trainingWeightIcon);
    destroyTextureSprite(state.shopDemo.guardCharmIcon);
    destroyTextureSprite(state.shopDemo.dragonSashIcon);
    state.shopDemo.assetsLoaded = false;
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
