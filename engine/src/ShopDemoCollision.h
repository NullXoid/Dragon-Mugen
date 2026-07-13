#pragma once

#include <algorithm>
#include <cmath>

#include <SDL3/SDL_rect.h>

namespace dragon::shop_demo {

struct ShopDemoLayoutRects {
    SDL_FRect topBar{};
    SDL_FRect world{};
    SDL_FRect helpBar{};
};

inline ShopDemoLayoutRects makeShopDemoLayoutRects(
    float width,
    float height,
    float topBarHeight,
    float helpBarHeight) {
    return {
        SDL_FRect{ 0.0f, 0.0f, width, topBarHeight },
        SDL_FRect{ 0.0f, topBarHeight, width, std::max(1.0f, height - topBarHeight - helpBarHeight) },
        SDL_FRect{ 0.0f, height - helpBarHeight, width, helpBarHeight },
    };
}

inline float clampShopDemoWorldZoom(float zoom) {
    return std::clamp(zoom, 1.0f, 1.72f);
}

inline float shopDemoWorldFocusY240(float zoom) {
    return 120.0f + (clampShopDemoWorldZoom(zoom) - 1.0f) * 62.0f;
}

inline float projectShopDemoSceneY(const SDL_FRect& world, float zoom, float y240) {
    const float safeZoom = clampShopDemoWorldZoom(zoom);
    return world.y + world.h * 0.5f
        + ((y240 - shopDemoWorldFocusY240(safeZoom)) / 240.0f) * world.h * safeZoom;
}

inline float projectShopDemoSceneHeight(const SDL_FRect& world, float zoom, float height240) {
    return (height240 / 240.0f) * world.h * clampShopDemoWorldZoom(zoom);
}

inline float shopDemoPlayerTargetHeight(const SDL_FRect& world, float zoom, float openShotScale) {
    return world.h * 0.33f * clampShopDemoWorldZoom(zoom) * openShotScale;
}

inline float shopDemoShopkeeperTargetHeight(const SDL_FRect& world, float zoom) {
    return world.h * 0.27f * clampShopDemoWorldZoom(zoom);
}

struct ShopCounterCollisionBounds {
    float playerMinX = 0.0f;
    float playerMaxX = 0.0f;
    float playerMinDepth = 0.0f;
    float playerMaxDepth = 0.0f;
    float solidLeft = 0.0f;
    float solidRight = 0.0f;
    float solidBackDepth = 0.0f;
    float solidFrontDepth = 0.0f;
    float epsilon = 0.5f;
};

struct ShopInteractionVolume {
    float left = 0.0f;
    float right = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 0.0f;
};

inline bool shopDemoInsideInteractionVolume(const ShopInteractionVolume& volume, float x, float depthZ) {
    return x >= volume.left
        && x <= volume.right
        && depthZ >= volume.minDepth
        && depthZ <= volume.maxDepth;
}

inline bool shopDemoInsideCounterSolid(const ShopCounterCollisionBounds& bounds, float x, float depthZ) {
    return x >= bounds.solidLeft
        && x <= bounds.solidRight
        && depthZ >= bounds.solidBackDepth
        && depthZ <= bounds.solidFrontDepth;
}

inline float shopDemoCounterPenetrationScore(const ShopCounterCollisionBounds& bounds, float x, float depthZ) {
    if (!shopDemoInsideCounterSolid(bounds, x, depthZ)) {
        return 0.0f;
    }
    const float xEscape = std::min(x - bounds.solidLeft, bounds.solidRight - x);
    const float zEscape = std::min(depthZ - bounds.solidBackDepth, bounds.solidFrontDepth - depthZ);
    return std::min(xEscape, zEscape);
}

inline void shopDemoSnapOutOfCounterSolid(const ShopCounterCollisionBounds& bounds, float& x, float& depthZ) {
    if (!shopDemoInsideCounterSolid(bounds, x, depthZ)) {
        return;
    }

    const float escapeLeft = x - bounds.solidLeft;
    const float escapeRight = bounds.solidRight - x;
    const float escapeBack = depthZ - bounds.solidBackDepth;
    const float escapeFront = bounds.solidFrontDepth - depthZ;
    float best = escapeLeft;
    enum class Edge { Left, Right, Back, Front } edge = Edge::Left;
    if (escapeRight < best) {
        best = escapeRight;
        edge = Edge::Right;
    }
    if (escapeBack < best) {
        best = escapeBack;
        edge = Edge::Back;
    }
    if (escapeFront < best) {
        edge = Edge::Front;
    }

    switch (edge) {
    case Edge::Left:
        x = bounds.solidLeft - bounds.epsilon;
        break;
    case Edge::Right:
        x = bounds.solidRight + bounds.epsilon;
        break;
    case Edge::Back:
        depthZ = bounds.solidBackDepth - bounds.epsilon;
        break;
    case Edge::Front:
        depthZ = bounds.solidFrontDepth + bounds.epsilon;
        break;
    }
}

inline bool shopDemoSegmentCrossesCounterSolid(
    const ShopCounterCollisionBounds& bounds,
    float oldX,
    float oldDepthZ,
    float nextX,
    float nextDepthZ) {
    const float minX = std::min(oldX, nextX);
    const float maxX = std::max(oldX, nextX);
    const float minDepth = std::min(oldDepthZ, nextDepthZ);
    const float maxDepth = std::max(oldDepthZ, nextDepthZ);
    if (maxX < bounds.solidLeft || minX > bounds.solidRight
        || maxDepth < bounds.solidBackDepth || minDepth > bounds.solidFrontDepth) {
        return false;
    }

    const float distance = std::max(std::abs(nextX - oldX), std::abs(nextDepthZ - oldDepthZ));
    const int steps = std::clamp(static_cast<int>(std::ceil(distance / 4.0f)), 2, 48);
    for (int i = 1; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const float sampleX = oldX + (nextX - oldX) * t;
        const float sampleDepth = oldDepthZ + (nextDepthZ - oldDepthZ) * t;
        if (shopDemoInsideCounterSolid(bounds, sampleX, sampleDepth)) {
            return true;
        }
    }
    return false;
}

inline void shopDemoResolveCounterCollision(
    const ShopCounterCollisionBounds& bounds,
    float oldX,
    float oldDepthZ,
    float dx,
    float dz,
    float& nextX,
    float& nextDepthZ) {
    const bool finalInside = shopDemoInsideCounterSolid(bounds, nextX, nextDepthZ);
    const bool crossedCounter = !finalInside
        && shopDemoSegmentCrossesCounterSolid(bounds, oldX, oldDepthZ, nextX, nextDepthZ);
    if (!finalInside && !crossedCounter) {
        return;
    }

    float safeOldX = oldX;
    float safeOldDepth = oldDepthZ;
    shopDemoSnapOutOfCounterSolid(bounds, safeOldX, safeOldDepth);

    const float xOnly = std::clamp(safeOldX + dx, bounds.playerMinX, bounds.playerMaxX);
    const float depthOnly = std::clamp(safeOldDepth + dz, bounds.playerMinDepth, bounds.playerMaxDepth);
    const bool xOnlySafe = !shopDemoInsideCounterSolid(bounds, xOnly, safeOldDepth);
    const bool depthOnlySafe = !shopDemoInsideCounterSolid(bounds, safeOldX, depthOnly);
    if (xOnlySafe && depthOnlySafe) {
        const float xScore = shopDemoCounterPenetrationScore(bounds, xOnly, safeOldDepth);
        const float zScore = shopDemoCounterPenetrationScore(bounds, safeOldX, depthOnly);
        if (xScore <= zScore) {
            nextX = xOnly;
            nextDepthZ = safeOldDepth;
        } else {
            nextX = safeOldX;
            nextDepthZ = depthOnly;
        }
        return;
    }
    if (xOnlySafe) {
        nextX = xOnly;
        nextDepthZ = safeOldDepth;
        return;
    }
    if (depthOnlySafe) {
        nextX = safeOldX;
        nextDepthZ = depthOnly;
        return;
    }

    nextX = safeOldX;
    nextDepthZ = safeOldDepth;
    if (std::abs(dx) >= std::abs(dz) && dx != 0.0f) {
        nextX = dx < 0.0f
            ? bounds.solidRight + bounds.epsilon
            : bounds.solidLeft - bounds.epsilon;
    } else if (dz != 0.0f) {
        nextDepthZ = dz < 0.0f
            ? bounds.solidFrontDepth + bounds.epsilon
            : bounds.solidBackDepth - bounds.epsilon;
    }
    nextX = std::clamp(nextX, bounds.playerMinX, bounds.playerMaxX);
    nextDepthZ = std::clamp(nextDepthZ, bounds.playerMinDepth, bounds.playerMaxDepth);
}

} // namespace dragon::shop_demo
