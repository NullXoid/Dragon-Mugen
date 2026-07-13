#pragma once

// Internal App.cpp implementation shard.
// UI context, Arena projection, sprite drawing, and versus loading helpers.

CanvasDimensions activeCanvasDimensions(const AppState& state) {
    (void)state;
    return presentationDimensions();
}

CanvasDimensions selectedOutputDimensions(const AppState& state) {
    return outputDimensionsForPreset(state.mainSettings.canvasPreset);
}

void beginPresentationFrame(SDL_Renderer* renderer, const AppState& state);
void presentPresentationFrame(SDL_Renderer* renderer, const AppState& state);
void destroyPresentationFrameTarget();

int logicalWidth(const AppState& state) {
    return activeCanvasDimensions(state).width;
}

int logicalHeight(const AppState& state) {
    return activeCanvasDimensions(state).height;
}

float logicalWidthF(const AppState& state) {
    return static_cast<float>(logicalWidth(state));
}

float logicalHeightF(const AppState& state) {
    return static_cast<float>(logicalHeight(state));
}

float screenCenterX(const AppState& state) {
    return logicalWidthF(state) * 0.5f;
}

float worldRenderScale(const AppState& state) {
    return logicalHeightF(state) / static_cast<float>(kDesignLogicalHeight);
}

float worldViewportWidth(const AppState& state) {
    const float scale = worldRenderScale(state);
    return scale > 0.0f ? logicalWidthF(state) / scale : logicalWidthF(state);
}

float worldViewportHalfWidth(const AppState& state) {
    return worldViewportWidth(state) * 0.5f;
}

float motifOriginX(const AppState& state) {
    return (logicalWidthF(state) - static_cast<float>(kClassicLogicalWidth)) * 0.5f;
}

float uiScale(const AppState& state) {
    return uiScaleFromPercent(state.mainSettings.uiScalePercent);
}

UiRenderContext uiRenderContext(SDL_Renderer* renderer, const AppState& state) {
    const CanvasDimensions canvas = activeCanvasDimensions(state);
    const CanvasDimensions output = selectedOutputDimensions(state);
    return UiRenderContext{
        renderer,
        canvas.width,
        canvas.height,
        uiScale(state),
        output.width,
        output.height,
    };
}

PerformanceHudMode effectivePerformanceHudMode(const AppState& state) {
    if (performanceOverlayEnvEnabled()) {
        return PerformanceHudMode::Perf;
    }
    return state.mainSettings.performanceHudMode;
}

void drawFpsCounter(SDL_Renderer* renderer, const AppState& state) {
    drawFramePerformanceHud(uiRenderContext(renderer, state), state.framePerf, effectivePerformanceHudMode(state), state.suppressFpsCounter);
}

void clearComboCounters(AppState& state);
void startEnvShake(AppState& state, const EnvShakeSpec& shake);
void startPaletteEffect(ActivePaletteEffect& active, const PaletteEffectSpec& effect);
void startEnvColor(AppState& state, const StateEnvColorController& envColor);

std::string unquote(std::string value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

bool startsWithNoCase(std::string_view value, std::string_view prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); ++i) {
        const char a = static_cast<char>(SDL_tolower(static_cast<unsigned char>(value[i])));
        const char b = static_cast<char>(SDL_tolower(static_cast<unsigned char>(prefix[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    return lhs.size() == rhs.size() && startsWithNoCase(lhs, rhs);
}

std::string lowercaseCopy(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        out.push_back(static_cast<char>(SDL_tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

std::string uppercaseCopy(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        out.push_back(static_cast<char>(SDL_toupper(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool isIdentifierChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

int countSectionsWithPrefix(const MugenDocument& doc, std::string_view prefix) {
    int count = 0;
    for (const auto& section : doc.sections) {
        if (startsWithNoCase(section.name, prefix)) {
            ++count;
        }
    }
    return count;
}

std::vector<std::string> splitCsv(const std::string& line);
int parseIntValue(const std::string& value, int fallback);
size_t findNoCase(std::string_view value, std::string_view needle, size_t start);

std::filesystem::path resolveContentPath(const std::filesystem::path& base, std::string value) {
    value = unquote(trim(value));
    if (value.empty()) {
        return {};
    }
    std::filesystem::path path(value);
    if (path.is_absolute()) {
        return path;
    }
    return base / path;
}

bool isMatchMode(const AppState& state) {
    return isMatchMode(state.frontend.pendingMode);
}

OpponentType activeOpponentType(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Training && state.training.options.p2Controlled) {
        return OpponentType::LocalP2;
    }
    return state.selection.sessionSlots.opponentType;
}

bool usesLocalP2Controls(const AppState& state) {
    return activeOpponentType(state) == OpponentType::LocalP2;
}

#include "ArenaModeState.h"

bool arenaDepthActive(const AppState& state);

#include "StoryModeState.h"

std::string opponentDisplayName(const AppState& state) {
    if (isArenaMode(state)) {
        return std::to_string(arenaCpuCount(state)) + " CPU";
    }
    if (isStoryMode(state)) {
        return "Enemy Waves";
    }
    if (const CharacterSlot* character = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter)) {
        return character->displayName;
    }
    switch (activeOpponentType(state)) {
    case OpponentType::Dummy:
        return "Dummy";
    case OpponentType::Cpu:
        return "CPU";
    case OpponentType::LocalP2:
    default:
        return "Player 2";
    }
}

bool arenaDepthActive(const AppState& state) {
    return (isArenaMode(state) && arenaZAxisEnabled(state))
        || isStoryMode(state);
}

float arenaDepthProjectionOffset(const AppState& state, float depthZ) {
    return arenaDepthActive(state) ? depthZ * state.arenaConfig.depthProjectionScale : 0.0f;
}

bool arenaCameraRotationActive(const AppState& state) {
    return arenaDepthActive(state) && arenaCameraRotationSelected(state);
}

float arenaRotationDepthExtent(const AppState& state) {
    return std::max({ std::fabs(state.arenaConfig.depthMin), std::fabs(state.arenaConfig.depthMax), 1.0f });
}

float arenaCameraYawRadians(const AppState& state) {
    constexpr float degToRad = 0.017453292519943295f;
    return state.arenaCameraYawDeg * degToRad;
}

struct ArenaProjectedPoint {
    float screenX = 0.0f;
    float screenY = 0.0f;
    float viewZ = 0.0f;
};

ArenaProjectedPoint projectArenaWorldPoint(
    const AppState& state,
    const StageSlot& stage,
    float x,
    float y,
    float depthZ) {
    const float effectiveDepth = arenaDepthActive(state) ? depthZ : 0.0f;
    const float worldX = x - state.cameraX;
    float viewX = worldX;
    float viewZ = effectiveDepth;
    if (arenaCameraRotationActive(state)) {
        const float yaw = arenaCameraYawRadians(state);
        const float c = std::cos(yaw);
        const float s = std::sin(yaw);
        viewX = worldX * c - effectiveDepth * s;
        viewZ = worldX * s + effectiveDepth * c;
    }
    const float scale = worldRenderScale(state);
    return ArenaProjectedPoint{
        screenCenterX(state) + viewX * scale,
        (stage.zoffset + y + viewZ * state.arenaConfig.depthProjectionScale - state.cameraY) * scale,
        viewZ,
    };
}

float arenaProjectedViewDepth(const AppState& state, float x, float depthZ) {
    const float effectiveDepth = arenaDepthActive(state) ? depthZ : 0.0f;
    if (!arenaCameraRotationActive(state)) {
        return effectiveDepth;
    }
    const float yaw = arenaCameraYawRadians(state);
    return (x - state.cameraX) * std::sin(yaw) + effectiveDepth * std::cos(yaw);
}

float arenaActorDepth(const AppState& state, const FighterState& actor) {
    if (arenaDepthActive(state)) {
        return actor.depthZ;
    }
    return 0.0f;
}

float arenaProjectileDepth(const AppState& state, const RuntimeProjectile& projectile) {
    if (arenaDepthActive(state)) {
        return projectile.depthZ;
    }
    return 0.0f;
}

float arenaEffectDepth(const AppState& state, const RuntimeEffect& effect) {
    if (arenaDepthActive(state)) {
        return effect.depthZ;
    }
    return 0.0f;
}

void drawSpriteTopLeft(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    float scale = 1.0f,
    SDL_FlipMode flip = SDL_FLIP_NONE) {
    if (!sprite.texture) {
        return;
    }
    SDL_FRect dst{
        x,
        y,
        static_cast<float>(sprite.width) * scale,
        static_cast<float>(sprite.height) * scale,
    };
    SDL_RenderTextureRotated(renderer, sprite.texture, nullptr, &dst, 0.0, nullptr, flip);
}

void drawSpriteAtAxis(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    float scale = 1.0f,
    SDL_FlipMode flip = SDL_FLIP_NONE) {
    drawSpriteTopLeft(
        renderer,
        sprite,
        x - static_cast<float>(sprite.axisX) * scale,
        y - static_cast<float>(sprite.axisY) * scale,
        scale,
        flip);
}

void drawTiledSprite(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    int repeatX,
    int repeatY) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }
    for (int ty = 0; ty < repeatY; ++ty) {
        for (int tx = 0; tx < repeatX; ++tx) {
            drawSpriteTopLeft(
                renderer,
                sprite,
                x + static_cast<float>(tx * sprite.width),
                y + static_cast<float>(ty * sprite.height));
        }
    }
}

void drawTiledSpriteCoverX(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    int logicalWidth,
    int repeatY) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }
    while (x > 0.0f) {
        x -= static_cast<float>(sprite.width);
    }
    const int repeatX = (logicalWidth / sprite.width) + 3;
    drawTiledSprite(renderer, sprite, x, y, repeatX, repeatY);
}

void drawParallaxFloorSprite(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    float topWidth,
    float bottomWidth,
    int logicalWidth,
    int frame) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }

    const float sourceWidth = static_cast<float>(sprite.width);
    const float sourceHeight = static_cast<float>(sprite.height);
    const float scroll = std::fmod(static_cast<float>(frame), sourceWidth);
    for (int row = 0; row < sprite.height; ++row) {
        const float t = sourceHeight <= 1.0f ? 0.0f : static_cast<float>(row) / (sourceHeight - 1.0f);
        const float rowWidth = topWidth + (bottomWidth - topWidth) * t;
        const float rowScroll = std::fmod(scroll * (rowWidth / sourceWidth), rowWidth);
        float drawX = x - rowScroll;
        while (drawX > 0.0f) {
            drawX -= rowWidth;
        }

        SDL_FRect src{ 0.0f, static_cast<float>(row), sourceWidth, 1.0f };
        for (float x = drawX; x < static_cast<float>(logicalWidth); x += rowWidth) {
            SDL_FRect dst{ x, y + static_cast<float>(row), rowWidth, 1.0f };
            SDL_RenderTexture(renderer, sprite.texture, &src, &dst);
        }
    }
}

void drawSpriteTopLeftWithBlend(
    SDL_Renderer* renderer,
    const TextureSprite& sprite,
    float x,
    float y,
    SDL_BlendMode blendMode,
    Uint8 alpha) {
    if (!sprite.texture) {
        return;
    }

    SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
    Uint8 previousAlpha = 255;
    SDL_GetTextureBlendMode(sprite.texture, &previousBlend);
    SDL_GetTextureAlphaMod(sprite.texture, &previousAlpha);
    SDL_SetTextureBlendMode(sprite.texture, blendMode);
    SDL_SetTextureAlphaMod(sprite.texture, alpha);
    drawSpriteTopLeft(renderer, sprite, x, y);
    SDL_SetTextureBlendMode(sprite.texture, previousBlend);
    SDL_SetTextureAlphaMod(sprite.texture, previousAlpha);
}

const TextureSprite* spriteAt(const std::vector<TextureSprite>& sprites, int index) {
    if (index < 0 || index >= static_cast<int>(sprites.size())) {
        return nullptr;
    }
    const auto& sprite = sprites[static_cast<size_t>(index)];
    return sprite.texture ? &sprite : nullptr;
}

UiSpriteView uiSpriteView(const TextureSprite* sprite) {
    if (!sprite || !sprite->texture) {
        return {};
    }
    return UiSpriteView{
        sprite->texture,
        sprite->width,
        sprite->height,
        sprite->axisX,
        sprite->axisY,
    };
}

VsScreenLoadStatus vsScreenLoadStatus(const AppState& state) {
    if (state.fightSessionLoadFailed || state.loadingProgress.failed) {
        return VsScreenLoadStatus::Failed;
    }
    if (state.fightSessionPrepared) {
        return VsScreenLoadStatus::Ready;
    }
    return VsScreenLoadStatus::Loading;
}

std::string_view loadingOpponentSlotLabel(const AppState& state) {
    if (isStoryMode(state)) {
        return storyWaveRoleLabel(storyWaveRole(state, state.story.waveIndex));
    }
    return opponentTypeLabel(activeOpponentType(state));
}

std::string loadingOpponentDisplayName(const AppState& state) {
    if (isStoryMode(state)) {
        return std::string(storyWaveRoleLabel(storyWaveRole(state, state.story.waveIndex)));
    }
    return opponentDisplayName(state);
}

VsScreenView versusScreenView(const AppState& state) {
    const TextureSprite* p1Portrait = state.characterLargePortrait.texture
        ? &state.characterLargePortrait
        : spriteAt(state.characterFaceSprites, sessionP1CharacterIndex(state.selection));
    const int opponentPortraitIndex = isStoryMode(state)
        ? storyFighterCharacterIndex(state, 1)
        : state.selection.sessionSlots.opponentCharacter;
    const TextureSprite* opponentPortrait =
        spriteAt(state.characterFaceSprites, opponentPortraitIndex);

    const VsScreenLoadStatus status = vsScreenLoadStatus(state);
    const float progress = status == VsScreenLoadStatus::Ready || status == VsScreenLoadStatus::Failed
        ? 1.0f
        : loadingProgressFraction(state.loadingProgress);
    const std::string phase = state.loadingProgress.active
        ? state.loadingProgress.phase
        : std::string("Waiting to load");
    const std::string progressText = std::to_string(
        status == VsScreenLoadStatus::Ready || status == VsScreenLoadStatus::Failed
            ? 100
            : loadingProgressPercent(state.loadingProgress))
        + "%";

    return VsScreenView{
        std::string(pendingModeTitle(state.frontend.pendingMode)),
        compactSettingText(selectedCharacterName(state.selection), 13),
        compactSettingText(loadingOpponentDisplayName(state), 12),
        std::string(loadingOpponentSlotLabel(state)),
        compactSettingText(selectedStageName(state.selection), 26),
        phase,
        progressText,
        status,
        progress,
        uiSpriteView(p1Portrait),
        uiSpriteView(opponentPortrait),
    };
}

void presentVersusLoadingProgress(SDL_Renderer* renderer, AppState& state) {
    if (!renderer || state.frontend.screen != Screen::VersusScreen) {
        return;
    }
    SDL_PumpEvents();
    beginPresentationFrame(renderer, state);
    drawVersusScreenOverlay(uiRenderContext(renderer, state), versusScreenView(state));
    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

SDL_Texture* createTexture(SDL_Renderer* renderer, const DecodedSprite& sprite) {
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        sprite.width,
        sprite.height);
    if (!texture) {
        return nullptr;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_UpdateTexture(texture, nullptr, sprite.rgba.data(), sprite.width * 4);
    return texture;
}

void setTextureSpriteFilterIntent(TextureSprite& sprite, TextureFilter filter) {
    sprite.filter = filter;
    if (!sprite.texture) {
        return;
    }
    SDL_SetTextureScaleMode(
        sprite.texture,
        filter == TextureFilter::Linear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

TextureSprite makeTextureSprite(SDL_Renderer* renderer, const DecodedSprite& sprite) {
    TextureSprite out;
    out.texture = createTexture(renderer, sprite);
    out.width = sprite.width;
    out.height = sprite.height;
    out.axisX = sprite.axisX;
    out.axisY = sprite.axisY;
    setTextureSpriteFilterIntent(out, TextureFilter::Nearest);
    return out;
}
