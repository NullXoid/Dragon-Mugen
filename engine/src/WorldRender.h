#pragma once

// Internal App.cpp implementation header.
// This depends on App.cpp-local runtime/render types and helpers; include it only
// from App.cpp after those dependencies are defined.

bool renderCullingEnabled() {
    const char* disabled = SDL_getenv("DRAGON_DISABLE_RENDER_CULLING");
    return !disabled || disabled[0] == '\0' || disabled[0] == '0';
}

bool renderRectVisible(const AppState& state, const SDL_FRect& rect, float pad = 64.0f) {
    if (!renderCullingEnabled()) {
        return true;
    }
    const float right = static_cast<float>(logicalWidth(state)) + pad;
    const float bottom = logicalHeightF(state) + pad;
    return rect.x + rect.w >= -pad
        && rect.x <= right
        && rect.y + rect.h >= -pad
        && rect.y <= bottom;
}

bool skipInvisibleRenderRect(const AppState& state, const SDL_FRect& rect, float pad = 64.0f) {
    if (renderRectVisible(state, rect, pad)) {
        return false;
    }
    state.framePerf.addSkippedDraw();
    return true;
}

void renderTexturePerf(SDL_Renderer* renderer, const AppState& state, SDL_Texture* texture, const SDL_FRect& dst) {
    state.framePerf.addDrawCall();
    SDL_RenderTexture(renderer, texture, nullptr, &dst);
}

void renderTextureRotatedPerf(
    SDL_Renderer* renderer,
    const AppState& state,
    SDL_Texture* texture,
    const SDL_FRect& dst,
    double angle,
    const SDL_FPoint* center,
    SDL_FlipMode flipMode) {
    state.framePerf.addDrawCall();
    SDL_RenderTextureRotated(renderer, texture, nullptr, &dst, angle, center, flipMode);
}

void drawStageLayer(SDL_Renderer* renderer, const AppState& state, int layerNo) {
    if (!hasSelectedStageBackground(state)) {
        return;
    }

    for (const auto& element : state.stageBackground) {
        if (element.layerNo != layerNo) {
            continue;
        }

        const TextureSprite* sprite = &element.sprite;
        float frameOffsetX = 0.0f;
        float frameOffsetY = 0.0f;
        if (element.animated) {
            const AnimationFrame* frame = frameForClip(element.animation, state.frame);
            if (!frame || !frame->sprite.texture) {
                continue;
            }
            sprite = &frame->sprite;
            frameOffsetX = static_cast<float>(frame->offsetX);
            frameOffsetY = static_cast<float>(frame->offsetY);
        } else if (!sprite->texture) {
            continue;
        }

        const float renderScale = worldRenderScale(state);
        float baseX = screenCenterX(state)
            + (element.x
                + frameOffsetX
                - static_cast<float>(sprite->axisX)
                - state.cameraX * element.deltaX) * renderScale;
        const float baseY = (element.y
            + frameOffsetY
            - static_cast<float>(sprite->axisY)
            - state.cameraY * element.deltaY) * renderScale;
        float drawWidth = static_cast<float>(sprite->width) * renderScale;
        const float drawHeight = static_cast<float>(sprite->height) * renderScale;
        const float tileWidth = static_cast<float>(sprite->width) * renderScale;
        const float tileHeight = static_cast<float>(sprite->height) * renderScale;
        const float widthF = logicalWidthF(state);
        const bool fixedWideBackdrop = layerNo == 0
            && !element.tileX
            && element.deltaX > -0.001f
            && element.deltaX < 0.001f
            && drawWidth >= widthF * 0.8f
            && (baseX > 0.0f || baseX + drawWidth < widthF);
        if (fixedWideBackdrop) {
            baseX = 0.0f;
            drawWidth = widthF;
        }
        const int repeatX = element.tileX && tileWidth > 0.0f
            ? std::max(6, static_cast<int>(std::ceil(widthF / tileWidth)) + 5)
            : 1;
        const int repeatY = element.tileY && tileHeight > 0.0f
            ? std::max(3, static_cast<int>(std::ceil(logicalHeightF(state) / tileHeight)) + 3)
            : 1;
        const float firstX = element.tileX ? baseX - tileWidth * 2.0f : baseX;
        const float firstY = element.tileY ? baseY - tileHeight : baseY;

        int startTx = 0;
        int endTx = repeatX;
        if (element.tileX && tileWidth > 0.0f) {
            constexpr float pad = 72.0f;
            startTx = std::clamp(static_cast<int>(std::floor((-pad - firstX) / tileWidth)), 0, repeatX - 1);
            endTx = std::clamp(static_cast<int>(std::ceil((widthF + pad - firstX) / tileWidth)) + 1, startTx + 1, repeatX);
        }
        int startTy = 0;
        int endTy = repeatY;
        if (element.tileY && tileHeight > 0.0f) {
            constexpr float pad = 72.0f;
            startTy = std::clamp(static_cast<int>(std::floor((-pad - firstY) / tileHeight)), 0, repeatY - 1);
            endTy = std::clamp(static_cast<int>(std::ceil((logicalHeightF(state) + pad - firstY) / tileHeight)) + 1, startTy + 1, repeatY);
        }

        for (int ty = startTy; ty < endTy; ++ty) {
            for (int tx = startTx; tx < endTx; ++tx) {
                SDL_FRect dst{
                    firstX + static_cast<float>(tx) * tileWidth,
                    firstY + static_cast<float>(ty) * tileHeight,
                    drawWidth,
                    drawHeight,
                };
                if (skipInvisibleRenderRect(state, dst, 72.0f)) {
                    continue;
                }
                renderTexturePerf(renderer, state, sprite->texture, dst);
            }
        }
    }
}

void drawFallbackStage(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, float cameraY) {
    const float widthF = logicalWidthF(state);
    setColor(renderer, 74, 100, 128);
    fillRect(renderer, 0, 0, widthF, 96);
    setColor(renderer, 86, 112, 88);
    fillRect(renderer, 0, 96, widthF, 54);
    setColor(renderer, 82, 62, 44);
    fillRect(renderer, 0, 150, widthF, 90);
    setColor(renderer, 58, 46, 38);
    fillRect(renderer, 0, stage.zoffset - cameraY, widthF, 10);
}

int paletteEffectSinValue(int value, int period, int elapsedTicks) {
    if (value == 0 || period <= 0) {
        return 0;
    }
    constexpr float tau = 6.28318530718f;
    return static_cast<int>(std::lround(std::sin((static_cast<float>(elapsedTicks) / static_cast<float>(period)) * tau) * static_cast<float>(value)));
}

std::array<int, 3> paletteEffectAdd(const ActivePaletteEffect& effect) {
    if (effect.ticksLeft <= 0) {
        return { 0, 0, 0 };
    }
    return {
        effect.spec.addR + paletteEffectSinValue(effect.spec.sinAddR, effect.spec.sinPeriod, effect.elapsedTicks),
        effect.spec.addG + paletteEffectSinValue(effect.spec.sinAddG, effect.spec.sinPeriod, effect.elapsedTicks),
        effect.spec.addB + paletteEffectSinValue(effect.spec.sinAddB, effect.spec.sinPeriod, effect.elapsedTicks),
    };
}

std::array<Uint8, 3> paletteEffectColorMod(const ActivePaletteEffect& effect) {
    if (effect.ticksLeft <= 0) {
        return { 255, 255, 255 };
    }

    auto channel = [](int mul, int add) -> Uint8 {
        const int value = (255 * std::max(0, mul)) / 256 + std::min(0, add);
        return static_cast<Uint8>(std::clamp(value, 0, 255));
    };

    const auto add = paletteEffectAdd(effect);
    return {
        channel(effect.spec.mulR, add[0]),
        channel(effect.spec.mulG, add[1]),
        channel(effect.spec.mulB, add[2]),
    };
}

ActivePaletteEffect afterImagePaletteEffect(const ActiveAfterImageEffect& afterImage, int step) {
    ActivePaletteEffect effect;
    effect.ticksLeft = 1;
    effect.spec.enabled = true;

    std::array<int, 3> add = afterImage.palBright;
    std::array<int, 3> mul = afterImage.palContrast;
    for (int i = 1; i <= step; ++i) {
        for (size_t channel = 0; channel < add.size(); ++channel) {
            add[channel] += afterImage.palAdd[channel];
            if (i == 1) {
                add[channel] += afterImage.palPostBright[channel];
            }
            mul[channel] = static_cast<int>(std::lround(static_cast<float>(mul[channel]) * afterImage.palMul[channel]));
        }
    }

    effect.spec.addR = add[0];
    effect.spec.addG = add[1];
    effect.spec.addB = add[2];
    effect.spec.mulR = mul[0];
    effect.spec.mulG = mul[1];
    effect.spec.mulB = mul[2];
    return effect;
}

int afterImageAlpha(const ActiveAfterImageEffect& afterImage, int step) {
    int baseAlpha = 190;
    switch (afterImage.blendMode) {
    case ActorBlendMode::Add1:
        baseAlpha = 82;
        break;
    case ActorBlendMode::Add:
    case ActorBlendMode::AddAlpha:
        baseAlpha = 150;
        break;
    case ActorBlendMode::Normal:
    default:
        baseAlpha = 190;
        break;
    }
    const float fade = std::pow(0.86f, static_cast<float>(std::max(0, step)));
    return std::clamp(static_cast<int>(std::lround(static_cast<float>(baseAlpha) * fade)), 0, 255);
}

bool actorBlendModeIsAdditive(ActorBlendMode mode) {
    return mode == ActorBlendMode::Add || mode == ActorBlendMode::Add1 || mode == ActorBlendMode::AddAlpha;
}

void drawPaletteOverlay(SDL_Renderer* renderer, const AppState& state, const ActivePaletteEffect& effect, int alphaLimit) {
    if (effect.ticksLeft <= 0) {
        return;
    }
    const auto add = paletteEffectAdd(effect);
    const int alpha = std::clamp(std::max({ std::abs(add[0]), std::abs(add[1]), std::abs(add[2]) }), 0, alphaLimit);
    if (alpha <= 0) {
        return;
    }
    setColor(
        renderer,
        static_cast<Uint8>(std::clamp(std::max(0, add[0]), 0, 255)),
        static_cast<Uint8>(std::clamp(std::max(0, add[1]), 0, 255)),
        static_cast<Uint8>(std::clamp(std::max(0, add[2]), 0, 255)),
        static_cast<Uint8>(alpha));
    fillRect(renderer, 0, 0, logicalWidthF(state), logicalHeightF(state));
}

struct ActorVisualFrame {
    int action = 0;
    int animTick = 0;
};

ActorVisualFrame visualFrameForFighter(const FighterState& fighter) {
    return ActorVisualFrame{ fighter.action, fighter.animTick };
}

void drawActor(SDL_Renderer* renderer, const AppState& state, const FighterState& fighter, size_t actorIndex) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    if (fighterHasAssertSpecialFlag(fighter, "invisible")) {
        return;
    }

    auto drawActorSprite = [&](int action, int actionClipOwnerIndex, int animTick, float x, float y, float depthZ, int facing, float scaleX, float scaleY, int alpha, bool additive, const ActivePaletteEffect* palette) -> bool {
        const AnimationClip* clip = nullptr;
        if (actionClipOwnerIndex >= 0 && actionClipOwnerIndex < static_cast<int>(state.fighters.size())) {
            clip = findClipForFighter(state, static_cast<size_t>(actionClipOwnerIndex), action);
        } else {
            clip = findClipForActor(state, fighter, action);
        }
        const AnimationFrame* frame = clip ? frameForClip(*clip, animTick) : nullptr;
        if (!frame || !frame->sprite.texture) {
            return false;
        }

        const ArenaProjectedPoint projected = projectArenaWorldPoint(state, stage, x, y, depthZ);
        const float originX = projected.screenX;
        const float originY = projected.screenY;
        const float renderScale = worldRenderScale(state);
        const float drawScaleX = scaleX * renderScale;
        const float drawScaleY = scaleY * renderScale;
        const bool facingLeft = facing < 0;
        const bool flipH = frame->flipX != facingLeft;
        const float drawX = facingLeft
            ? originX - static_cast<float>(frame->offsetX) * drawScaleX - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * drawScaleX
            : originX + static_cast<float>(frame->offsetX) * drawScaleX - static_cast<float>(frame->sprite.axisX) * drawScaleX;
        const float drawY = originY + static_cast<float>(frame->offsetY) * drawScaleY - static_cast<float>(frame->sprite.axisY) * drawScaleY;

        SDL_FRect dst{
            drawX,
            drawY,
            static_cast<float>(frame->sprite.width) * drawScaleX,
            static_cast<float>(frame->sprite.height) * drawScaleY,
        };
        if (skipInvisibleRenderRect(state, dst, 96.0f)) {
            return false;
        }
        int flipMode = SDL_FLIP_NONE;
        if (flipH) {
            flipMode |= SDL_FLIP_HORIZONTAL;
        }
        if (frame->flipY) {
            flipMode |= SDL_FLIP_VERTICAL;
        }
        SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
        Uint8 previousR = 255;
        Uint8 previousG = 255;
        Uint8 previousB = 255;
        Uint8 previousA = 255;
        SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
        SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
        SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);

        std::array<Uint8, 3> colorMod{ 255, 255, 255 };
        if (palette) {
            colorMod = paletteEffectColorMod(*palette);
        }
        SDL_SetTextureBlendMode(frame->sprite.texture, additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(frame->sprite.texture, colorMod[0], colorMod[1], colorMod[2]);
        SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(std::clamp(alpha, 0, 255)));
        renderTextureRotatedPerf(renderer, state, frame->sprite.texture, dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
        if (palette) {
            const auto add = paletteEffectAdd(*palette);
            const int overlayAlpha = std::clamp(std::max({ add[0], add[1], add[2] }), 0, std::clamp(alpha, 0, 255));
            if (overlayAlpha > 0) {
                SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_ADD);
                SDL_SetTextureColorMod(
                    frame->sprite.texture,
                    static_cast<Uint8>(std::clamp(add[0], 0, 255)),
                    static_cast<Uint8>(std::clamp(add[1], 0, 255)),
                    static_cast<Uint8>(std::clamp(add[2], 0, 255)));
                SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(overlayAlpha));
                renderTextureRotatedPerf(renderer, state, frame->sprite.texture, dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
            }
        }

        SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
        SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
        SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
        return true;
    };

    if (fighter.afterImage.active || !fighter.afterImage.trail.empty()) {
        const int frameGap = std::max(1, fighter.afterImage.frameGap);
        const int sampleCount = std::min(
            static_cast<int>(fighter.afterImage.trail.size()),
            std::max(1, fighter.afterImage.length));
        const int end = (sampleCount / frameGap) * frameGap;
        for (int offset = end; offset >= frameGap; offset -= frameGap) {
            const size_t snapshotIndex = fighter.afterImage.trail.size() - static_cast<size_t>(offset);
            const auto& snapshot = fighter.afterImage.trail[snapshotIndex];
            const int step = offset / frameGap - 1;
            ActivePaletteEffect palette = afterImagePaletteEffect(fighter.afterImage, step);
            const int alpha = afterImageAlpha(fighter.afterImage, step);
            if (alpha <= 0) {
                continue;
            }
            drawActorSprite(
                snapshot.action,
                snapshot.actionClipOwnerIndex,
                snapshot.animTick,
                snapshot.x,
                snapshot.y,
                snapshot.depthZ,
                snapshot.facing,
                fighter.scaleX,
                fighter.scaleY,
                alpha,
                actorBlendModeIsAdditive(fighter.afterImage.blendMode),
                &palette);
        }
    }

    const ActorVisualFrame visual = visualFrameForFighter(fighter);
    const AnimationClip* clip = findClipForActor(state, fighter, visual.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, visual.animTick) : nullptr;
    const bool drawHitFeedback = state.frontend.pendingMode == PendingMode::Training
        && state.training.options.showHitFlash
        && (fighter.moveType == 'H' || fighter.hitPauseTicks > 0);

    if (frame && frame->sprite.texture) {
        const ArenaProjectedPoint projected = projectArenaWorldPoint(state, stage, fighter.x, fighter.y, fighter.depthZ);
        const float originX = projected.screenX;
        const float originY = projected.screenY;
        const float renderScale = worldRenderScale(state);
        const float drawScaleX = fighter.scaleX * renderScale;
        const float drawScaleY = fighter.scaleY * renderScale;
        const float displayOriginX = originX + fighter.displayOffsetX * static_cast<float>(fighter.facing) * renderScale;
        const float displayOriginY = originY + fighter.displayOffsetY * renderScale;
        const bool facingLeft = fighter.facing < 0;
        const bool flipH = frame->flipX != facingLeft;
        const float drawX = facingLeft
            ? displayOriginX - static_cast<float>(frame->offsetX) * drawScaleX - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * drawScaleX
            : displayOriginX + static_cast<float>(frame->offsetX) * drawScaleX - static_cast<float>(frame->sprite.axisX) * drawScaleX;
        const float drawY = displayOriginY + static_cast<float>(frame->offsetY) * drawScaleY - static_cast<float>(frame->sprite.axisY) * drawScaleY;

        SDL_FRect dst{
            drawX,
            drawY,
            static_cast<float>(frame->sprite.width) * drawScaleX,
            static_cast<float>(frame->sprite.height) * drawScaleY,
        };
        if (skipInvisibleRenderRect(state, dst, 96.0f)) {
            return;
        }
        int flipMode = SDL_FLIP_NONE;
        if (flipH) {
            flipMode |= SDL_FLIP_HORIZONTAL;
        }
        if (frame->flipY) {
            flipMode |= SDL_FLIP_VERTICAL;
        }
        const double angle = fighter.angleDrawActive
            ? static_cast<double>(fighter.facing >= 0 ? -fighter.drawAngle : fighter.drawAngle)
            : 0.0;
        SDL_FPoint rotationCenter{
            originX - drawX,
            originY - drawY,
        };
        SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
        Uint8 previousR = 255;
        Uint8 previousG = 255;
        Uint8 previousB = 255;
        SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
        SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
        Uint8 previousA = 255;
        SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);
        const auto colorMod = paletteEffectColorMod(fighter.paletteEffect);
        const bool additiveTrans = fighter.transEffect.active && actorBlendModeIsAdditive(fighter.transEffect.mode);
        const int sourceAlpha = fighter.transEffect.active ? fighter.transEffect.alphaSource : 256;
        SDL_SetTextureBlendMode(frame->sprite.texture, additiveTrans ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(frame->sprite.texture, colorMod[0], colorMod[1], colorMod[2]);
        SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(std::clamp(sourceAlpha, 0, 256) * 255 / 256));
        renderTextureRotatedPerf(renderer, state, frame->sprite.texture, dst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
        const auto add = paletteEffectAdd(fighter.paletteEffect);
        const int additiveAlpha = std::clamp(std::max({ add[0], add[1], add[2] }), 0, 180);
        if (additiveAlpha > 0) {
            SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_ADD);
            SDL_SetTextureColorMod(
                frame->sprite.texture,
                static_cast<Uint8>(std::clamp(add[0], 0, 255)),
                static_cast<Uint8>(std::clamp(add[1], 0, 255)),
                static_cast<Uint8>(std::clamp(add[2], 0, 255)));
            renderTextureRotatedPerf(renderer, state, frame->sprite.texture, dst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
        }
        SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
        SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
        SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
        if (drawHitFeedback) {
            const int feedbackAlpha = fighter.hitPauseTicks > 0 ? 150 : 94;
            SDL_FRect flashDst{
                dst.x - 2.0f,
                dst.y - 2.0f,
                dst.w + 4.0f,
                dst.h + 4.0f,
            };
            SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_ADD);
            SDL_SetTextureColorMod(frame->sprite.texture, 255, 178, 86);
            SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(feedbackAlpha));
            renderTextureRotatedPerf(renderer, state, frame->sprite.texture, flashDst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
            SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
            SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
            SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
            setColor(renderer, 255, 204, 96, 210);
            drawRect(renderer, dst.x - 2.0f, dst.y - 2.0f, dst.w + 4.0f, dst.h + 4.0f);
            setColor(renderer, 255, 244, 190, 168);
            drawRect(renderer, dst.x - 4.0f, dst.y - 4.0f, dst.w + 8.0f, dst.h + 8.0f);
        }
        return;
    }

    const ArenaProjectedPoint projected = projectArenaWorldPoint(
        state,
        selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : stage,
        fighter.x,
        fighter.y,
        fighter.depthZ);
    const float originX = projected.screenX;
    const float originY = projected.screenY;
    const float renderScale = worldRenderScale(state);
    if (actorIndex == 0) {
        setColor(renderer, 62, 118, 184);
    } else {
        setColor(renderer, 218, 174, 100);
    }
    fillRect(renderer, originX - 14.0f * renderScale, originY - 58.0f * renderScale, 28.0f * renderScale, 58.0f * renderScale);
    fillRect(renderer, originX - 22.0f * renderScale, originY - 38.0f * renderScale, 44.0f * renderScale, 12.0f * renderScale);
    if (drawHitFeedback) {
        setColor(renderer, 255, 178, 86, fighter.hitPauseTicks > 0 ? 112 : 72);
        fillRect(renderer, originX - 24.0f * renderScale, originY - 60.0f * renderScale, 48.0f * renderScale, 62.0f * renderScale);
        setColor(renderer, 255, 244, 190, 190);
        drawRect(renderer, originX - 24.0f * renderScale, originY - 60.0f * renderScale, 48.0f * renderScale, 62.0f * renderScale);
    }
}

void drawFighter(SDL_Renderer* renderer, const AppState& state, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }
    drawActor(renderer, state, state.fighters[fighterIndex], fighterIndex);
}

void drawRuntimeEffect(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, const RuntimeEffect& effect) {
    const AnimationClip* clip = findExactClipForRuntimeEffect(state, effect);
    const AnimationFrame* frame = clip ? frameForClip(*clip, effect.animTick) : nullptr;
    if (!frame || !frame->sprite.texture) {
        return;
    }

    const ArenaProjectedPoint projected = projectArenaWorldPoint(state, stage, effect.x, effect.y, arenaEffectDepth(state, effect));
    const float originX = projected.screenX;
    const float originY = projected.screenY;
    const float renderScale = worldRenderScale(state);
    const float drawScaleX = effect.scaleX * renderScale;
    const float drawScaleY = effect.scaleY * renderScale;
    SDL_FRect dst{
        originX + (static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX)) * drawScaleX,
        originY + (static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY)) * drawScaleY,
        static_cast<float>(frame->sprite.width) * drawScaleX,
        static_cast<float>(frame->sprite.height) * drawScaleY,
    };
    if (skipInvisibleRenderRect(state, dst, 96.0f)) {
        return;
    }

    int flipMode = SDL_FLIP_NONE;
    if (frame->flipX) {
        flipMode |= SDL_FLIP_HORIZONTAL;
    }
    if (frame->flipY) {
        flipMode |= SDL_FLIP_VERTICAL;
    }
    SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
    Uint8 previousR = 255;
    Uint8 previousG = 255;
    Uint8 previousB = 255;
    Uint8 previousA = 255;
    SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
    SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
    SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);

    const bool earlyFightFx = effect.fromFightFx && effect.ageTicks <= 5;
    if (earlyFightFx) {
        const float grow = 1.28f + std::max(0, 5 - effect.ageTicks) * 0.045f;
        SDL_FRect glowDst{
            dst.x - (dst.w * (grow - 1.0f)) * 0.5f,
            dst.y - (dst.h * (grow - 1.0f)) * 0.5f,
            dst.w * grow,
            dst.h * grow,
        };
        SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(frame->sprite.texture, 255, 204, 96);
        SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(150 - effect.ageTicks * 18));
        renderTextureRotatedPerf(renderer, state, frame->sprite.texture, glowDst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
    }

    SDL_SetTextureBlendMode(frame->sprite.texture, frame->additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(frame->sprite.texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(frame->sprite.texture, 255);
    renderTextureRotatedPerf(renderer, state, frame->sprite.texture, dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));

    SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
    SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
    SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
}

void drawRuntimeProjectile(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, const RuntimeProjectile& projectile) {
    if (projectile.ownerIndex < 0 || projectile.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return;
    }
    const AnimationClip* clip = findClipForFighter(state, static_cast<size_t>(projectile.ownerIndex), projectile.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, projectile.animTick) : nullptr;
    if (!frame || !frame->sprite.texture) {
        return;
    }

    const ArenaProjectedPoint projected = projectArenaWorldPoint(
        state,
        stage,
        projectile.x,
        projectile.y,
        arenaProjectileDepth(state, projectile));
    const float originX = projected.screenX;
    const float originY = projected.screenY;
    const float renderScale = worldRenderScale(state);
    const float drawScaleX = projectile.scaleX * renderScale;
    const float drawScaleY = projectile.scaleY * renderScale;
    const bool facingLeft = projectile.facing < 0;
    const bool flipH = frame->flipX != facingLeft;
    const float drawX = facingLeft
        ? originX - static_cast<float>(frame->offsetX) * drawScaleX - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * drawScaleX
        : originX + static_cast<float>(frame->offsetX) * drawScaleX - static_cast<float>(frame->sprite.axisX) * drawScaleX;
    const float drawY = originY + static_cast<float>(frame->offsetY) * drawScaleY - static_cast<float>(frame->sprite.axisY) * drawScaleY;
    SDL_FRect dst{
        drawX,
        drawY,
        static_cast<float>(frame->sprite.width) * drawScaleX,
        static_cast<float>(frame->sprite.height) * drawScaleY,
    };
    if (skipInvisibleRenderRect(state, dst, 96.0f)) {
        return;
    }

    int flipMode = SDL_FLIP_NONE;
    if (flipH) {
        flipMode |= SDL_FLIP_HORIZONTAL;
    }
    if (frame->flipY) {
        flipMode |= SDL_FLIP_VERTICAL;
    }
    SDL_BlendMode previousBlend = SDL_BLENDMODE_BLEND;
    Uint8 previousR = 255;
    Uint8 previousG = 255;
    Uint8 previousB = 255;
    Uint8 previousA = 255;
    SDL_GetTextureBlendMode(frame->sprite.texture, &previousBlend);
    SDL_GetTextureColorMod(frame->sprite.texture, &previousR, &previousG, &previousB);
    SDL_GetTextureAlphaMod(frame->sprite.texture, &previousA);

    if (projectile.shadowEnabled && projectile.y < 0.0f) {
        const ArenaProjectedPoint shadowProjected = projectArenaWorldPoint(
            state,
            stage,
            projectile.x,
            0.0f,
            arenaProjectileDepth(state, projectile));
        SDL_FRect shadowDst{
            dst.x,
            shadowProjected.screenY - 3.0f * renderScale,
            dst.w,
            std::max(2.0f * renderScale, dst.h * 0.18f),
        };
        SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(
            frame->sprite.texture,
            static_cast<Uint8>(std::clamp(255 - projectile.shadowR, 0, 255)),
            static_cast<Uint8>(std::clamp(255 - projectile.shadowG, 0, 255)),
            static_cast<Uint8>(std::clamp(255 - projectile.shadowB, 0, 255)));
        SDL_SetTextureAlphaMod(frame->sprite.texture, 96);
        if (!skipInvisibleRenderRect(state, shadowDst, 96.0f)) {
            renderTextureRotatedPerf(renderer, state, frame->sprite.texture, shadowDst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
        }
    }

    SDL_SetTextureBlendMode(frame->sprite.texture, frame->additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(frame->sprite.texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(frame->sprite.texture, 255);
    renderTextureRotatedPerf(renderer, state, frame->sprite.texture, dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));

    SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
    SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
    SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
}

void drawWorldActors(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    struct DrawItem {
        int priority = 0;
        int kind = 0;
        float depth = 0.0f;
        size_t index = 0;
    };

    std::vector<DrawItem> items;
    items.reserve(state.fighters.size() + state.helpers.size() + state.runtimeEffects.size() + state.projectiles.size());
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (isStoryMode(state) && i > 0 && !storyEnemySlotActive(state, i)) {
            continue;
        }
        items.push_back(DrawItem{
            state.fighters[i].sprPriority,
            0,
            arenaProjectedViewDepth(state, state.fighters[i].x, arenaActorDepth(state, state.fighters[i])),
            i });
    }
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        if (!state.helpers[i].destroyRequested) {
            items.push_back(DrawItem{
                state.helpers[i].sprPriority,
                1,
                arenaProjectedViewDepth(state, state.helpers[i].x, arenaActorDepth(state, state.helpers[i])),
                i });
        }
    }
    for (size_t i = 0; i < state.projectiles.size(); ++i) {
        items.push_back(DrawItem{
            3,
            3,
            arenaProjectedViewDepth(state, state.projectiles[i].x, arenaProjectileDepth(state, state.projectiles[i])),
            i });
    }
    if (state.frontend.pendingMode != PendingMode::Training || state.training.options.showHitSparks) {
        for (size_t i = 0; i < state.runtimeEffects.size(); ++i) {
            items.push_back(DrawItem{
                state.runtimeEffects[i].sprPriority,
                2,
                arenaProjectedViewDepth(state, state.runtimeEffects[i].x, arenaEffectDepth(state, state.runtimeEffects[i])),
                i });
        }
    }

    std::stable_sort(items.begin(), items.end(), [&state](const DrawItem& lhs, const DrawItem& rhs) {
        if (lhs.priority != rhs.priority) {
            return lhs.priority < rhs.priority;
        }
        if (arenaDepthActive(state) && std::fabs(lhs.depth - rhs.depth) > 0.001f) {
            return lhs.depth < rhs.depth;
        }
        return false;
    });

    for (const auto& item : items) {
        if (item.kind == 3) {
            drawRuntimeProjectile(renderer, state, stage, state.projectiles[item.index]);
        } else if (item.kind == 2) {
            drawRuntimeEffect(renderer, state, stage, state.runtimeEffects[item.index]);
        } else if (item.kind == 1) {
            drawActor(renderer, state, state.helpers[item.index], state.helpers[item.index].ownerIndex >= 0 ? static_cast<size_t>(state.helpers[item.index].ownerIndex) : 0);
        } else {
            drawFighter(renderer, state, item.index);
        }
    }
}
