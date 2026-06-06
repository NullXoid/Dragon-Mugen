#pragma once

// Internal App.cpp implementation header.
// This depends on App.cpp-local runtime/render types and helpers; include it only
// from App.cpp after those dependencies are defined.

void drawStageLayer(SDL_Renderer* renderer, const AppState& state, int layerNo) {
    if (!hasSelectedStageBackground(state)) {
        return;
    }

    for (const auto& element : state.stageBackground) {
        if (!element.sprite.texture || element.layerNo != layerNo) {
            continue;
        }

        const float baseX = screenCenterX(state)
            + element.x
            - static_cast<float>(element.sprite.axisX)
            - state.cameraX * element.deltaX;
        const float baseY = element.y
            - static_cast<float>(element.sprite.axisY)
            - state.cameraY * element.deltaY;
        const int repeatX = element.tileX ? 6 : 1;
        const int repeatY = element.tileY ? 3 : 1;
        const float firstX = element.tileX ? baseX - static_cast<float>(element.sprite.width * 2) : baseX;
        const float firstY = element.tileY ? baseY - static_cast<float>(element.sprite.height) : baseY;

        for (int ty = 0; ty < repeatY; ++ty) {
            for (int tx = 0; tx < repeatX; ++tx) {
                SDL_FRect dst{
                    firstX + static_cast<float>(tx * element.sprite.width),
                    firstY + static_cast<float>(ty * element.sprite.height),
                    static_cast<float>(element.sprite.width),
                    static_cast<float>(element.sprite.height),
                };
                SDL_RenderTexture(renderer, element.sprite.texture, nullptr, &dst);
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
    fillRect(renderer, 0, 0, logicalWidthF(state), static_cast<float>(kLogicalHeight));
}

void drawActor(SDL_Renderer* renderer, const AppState& state, const FighterState& fighter, size_t actorIndex) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    if (fighterHasAssertSpecialFlag(fighter, "invisible")) {
        return;
    }

    auto drawActorSprite = [&](int action, int animTick, float x, float y, int facing, int alpha, bool additive, const ActivePaletteEffect* palette) -> bool {
        const AnimationClip* clip = findClipForFighter(state, actorIndex, action);
        const AnimationFrame* frame = clip ? frameForClip(*clip, animTick) : nullptr;
        if (!frame || !frame->sprite.texture) {
            return false;
        }

        const float originX = screenCenterX(state) + x - state.cameraX;
        const float originY = stage.zoffset + y - state.cameraY;
        const bool facingLeft = facing < 0;
        const bool flipH = frame->flipX != facingLeft;
        const float drawX = facingLeft
            ? originX - static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.width - frame->sprite.axisX)
            : originX + static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX);
        const float drawY = originY + static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY);

        SDL_FRect dst{
            drawX,
            drawY,
            static_cast<float>(frame->sprite.width),
            static_cast<float>(frame->sprite.height),
        };
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
        SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));

        SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
        SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
        SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
        return true;
    };

    if (fighter.afterImage.active || !fighter.afterImage.trail.empty()) {
        for (const auto& snapshot : fighter.afterImage.trail) {
            const int alpha = std::clamp(150 - snapshot.ageTicks * 10, 20, 150);
            drawActorSprite(
                snapshot.action,
                snapshot.animTick,
                snapshot.x,
                snapshot.y,
                snapshot.facing,
                alpha,
                fighter.afterImage.additive,
                nullptr);
        }
    }

    const AnimationClip* clip = findClipForFighter(state, actorIndex, fighter.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, fighter.animTick) : nullptr;
    const bool drawHitFeedback = (state.frontend.pendingMode != PendingMode::Training || state.training.options.showHitFlash)
        && (fighter.moveType == 'H' || fighter.hitPauseTicks > 0);

    if (frame && frame->sprite.texture) {
        const float originX = screenCenterX(state) + fighter.x - state.cameraX;
        const float originY = stage.zoffset + fighter.y - state.cameraY;
        const float displayOriginX = originX + fighter.displayOffsetX * static_cast<float>(fighter.facing);
        const float displayOriginY = originY + fighter.displayOffsetY;
        const bool facingLeft = fighter.facing < 0;
        const bool flipH = frame->flipX != facingLeft;
        const float drawX = facingLeft
            ? displayOriginX - static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.width - frame->sprite.axisX)
            : displayOriginX + static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX);
        const float drawY = displayOriginY + static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY);

        SDL_FRect dst{
            drawX,
            drawY,
            static_cast<float>(frame->sprite.width),
            static_cast<float>(frame->sprite.height),
        };
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
        const bool additiveTrans = fighter.transEffect.active
            && (fighter.transEffect.mode == ActorBlendMode::Add || fighter.transEffect.mode == ActorBlendMode::AddAlpha);
        const int sourceAlpha = fighter.transEffect.active ? fighter.transEffect.alphaSource : 256;
        SDL_SetTextureBlendMode(frame->sprite.texture, additiveTrans ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(frame->sprite.texture, colorMod[0], colorMod[1], colorMod[2]);
        SDL_SetTextureAlphaMod(frame->sprite.texture, static_cast<Uint8>(std::clamp(sourceAlpha, 0, 256) * 255 / 256));
        SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
        const auto add = paletteEffectAdd(fighter.paletteEffect);
        const int additiveAlpha = std::clamp(std::max({ add[0], add[1], add[2] }), 0, 180);
        if (additiveAlpha > 0) {
            SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_ADD);
            SDL_SetTextureColorMod(
                frame->sprite.texture,
                static_cast<Uint8>(std::clamp(add[0], 0, 255)),
                static_cast<Uint8>(std::clamp(add[1], 0, 255)),
                static_cast<Uint8>(std::clamp(add[2], 0, 255)));
            SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, angle, &rotationCenter, static_cast<SDL_FlipMode>(flipMode));
        }
        SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
        SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
        SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
        if (drawHitFeedback) {
            setColor(renderer, 255, 245, 170);
            drawRect(renderer, dst.x - 1.0f, dst.y - 1.0f, dst.w + 2.0f, dst.h + 2.0f);
        }
        return;
    }

    const float originX = screenCenterX(state) + fighter.x - state.cameraX;
    const float originY = (selectedStageSlot(state.selection) ? selectedStageSlot(state.selection)->zoffset : 168.0f) + fighter.y - state.cameraY;
    if (actorIndex == 0) {
        setColor(renderer, 62, 118, 184);
    } else {
        setColor(renderer, 218, 174, 100);
    }
    fillRect(renderer, originX - 14.0f, originY - 58.0f, 28, 58);
    fillRect(renderer, originX - 22.0f, originY - 38.0f, 44, 12);
    if (drawHitFeedback) {
        setColor(renderer, 255, 245, 170);
        drawRect(renderer, originX - 22.0f, originY - 58.0f, 44.0f, 58.0f);
    }
}

void drawFighter(SDL_Renderer* renderer, const AppState& state, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }
    drawActor(renderer, state, state.fighters[fighterIndex], fighterIndex);
}

void drawRuntimeEffect(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, const RuntimeEffect& effect) {
    const AnimationClip* clip = effect.fromFightFx ? findFightFxClip(state, effect.action) : findExactClip(state, effect.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, effect.animTick) : nullptr;
    if (!frame || !frame->sprite.texture) {
        return;
    }

    SDL_SetTextureBlendMode(frame->sprite.texture, frame->additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    const float originX = screenCenterX(state) + effect.x - state.cameraX;
    const float originY = stage.zoffset + effect.y - state.cameraY;
    SDL_FRect dst{
        originX + (static_cast<float>(frame->offsetX) - static_cast<float>(frame->sprite.axisX)) * effect.scaleX,
        originY + (static_cast<float>(frame->offsetY) - static_cast<float>(frame->sprite.axisY)) * effect.scaleY,
        static_cast<float>(frame->sprite.width) * effect.scaleX,
        static_cast<float>(frame->sprite.height) * effect.scaleY,
    };

    int flipMode = SDL_FLIP_NONE;
    if (frame->flipX) {
        flipMode |= SDL_FLIP_HORIZONTAL;
    }
    if (frame->flipY) {
        flipMode |= SDL_FLIP_VERTICAL;
    }
    SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
}

void drawRuntimeProjectile(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, const RuntimeProjectile& projectile) {
    const AnimationClip* clip = findExactClip(state, projectile.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, projectile.animTick) : nullptr;
    if (!frame || !frame->sprite.texture) {
        return;
    }

    const float originX = screenCenterX(state) + projectile.x - state.cameraX;
    const float originY = stage.zoffset + projectile.y - state.cameraY;
    const bool facingLeft = projectile.facing < 0;
    const bool flipH = frame->flipX != facingLeft;
    const float drawX = facingLeft
        ? originX - static_cast<float>(frame->offsetX) * projectile.scaleX - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * projectile.scaleX
        : originX + static_cast<float>(frame->offsetX) * projectile.scaleX - static_cast<float>(frame->sprite.axisX) * projectile.scaleX;
    const float drawY = originY + static_cast<float>(frame->offsetY) * projectile.scaleY - static_cast<float>(frame->sprite.axisY) * projectile.scaleY;
    SDL_FRect dst{
        drawX,
        drawY,
        static_cast<float>(frame->sprite.width) * projectile.scaleX,
        static_cast<float>(frame->sprite.height) * projectile.scaleY,
    };

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
        SDL_FRect shadowDst{
            dst.x,
            stage.zoffset - state.cameraY - 3.0f,
            dst.w,
            std::max(2.0f, dst.h * 0.18f),
        };
        SDL_SetTextureBlendMode(frame->sprite.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(
            frame->sprite.texture,
            static_cast<Uint8>(std::clamp(255 - projectile.shadowR, 0, 255)),
            static_cast<Uint8>(std::clamp(255 - projectile.shadowG, 0, 255)),
            static_cast<Uint8>(std::clamp(255 - projectile.shadowB, 0, 255)));
        SDL_SetTextureAlphaMod(frame->sprite.texture, 96);
        SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &shadowDst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));
    }

    SDL_SetTextureBlendMode(frame->sprite.texture, frame->additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(frame->sprite.texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(frame->sprite.texture, 255);
    SDL_RenderTextureRotated(renderer, frame->sprite.texture, nullptr, &dst, 0.0, nullptr, static_cast<SDL_FlipMode>(flipMode));

    SDL_SetTextureBlendMode(frame->sprite.texture, previousBlend);
    SDL_SetTextureColorMod(frame->sprite.texture, previousR, previousG, previousB);
    SDL_SetTextureAlphaMod(frame->sprite.texture, previousA);
}

void drawWorldActors(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    struct DrawItem {
        int priority = 0;
        int kind = 0;
        size_t index = 0;
    };

    std::vector<DrawItem> items;
    items.reserve(state.fighters.size() + state.helpers.size() + state.runtimeEffects.size() + state.projectiles.size());
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        items.push_back(DrawItem{ state.fighters[i].sprPriority, 0, i });
    }
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        if (!state.helpers[i].destroyRequested) {
            items.push_back(DrawItem{ state.helpers[i].sprPriority, 1, i });
        }
    }
    for (size_t i = 0; i < state.projectiles.size(); ++i) {
        items.push_back(DrawItem{ 3, 3, i });
    }
    if (state.frontend.pendingMode != PendingMode::Training || state.training.options.showHitSparks) {
        for (size_t i = 0; i < state.runtimeEffects.size(); ++i) {
            items.push_back(DrawItem{ state.runtimeEffects[i].sprPriority, 2, i });
        }
    }

    std::stable_sort(items.begin(), items.end(), [](const DrawItem& lhs, const DrawItem& rhs) {
        return lhs.priority < rhs.priority;
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
