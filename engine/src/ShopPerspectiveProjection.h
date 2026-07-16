#pragma once

#include <algorithm>
#include <cmath>

namespace dragon::shop_demo {

// Pinhole camera used only by the Dragon Shop Hub. Gameplay collision remains
// on the existing X/depth plane; this camera projects that plane for rendering.
struct ShopPerspectiveCamera {
    float viewportCenterX = 0.0f;
    float horizonY = 0.0f;
    float cameraX = 0.0f;
    float cameraDepth = 620.0f;
    float focalLength = 620.0f;
    float cameraHeight = 520.0f;
    float yawRadians = 0.0f;
};

struct ShopPerspectivePoint {
    float x = 0.0f;
    float floorY = 0.0f;
    float scale = 1.0f;
    float cameraDepth = 1.0f;
};

inline ShopPerspectivePoint projectShopGroundPoint(
    const ShopPerspectiveCamera& camera,
    float worldX,
    float worldDepth) {
    const float dx = worldX - camera.cameraX;
    const float cosine = std::cos(camera.yawRadians);
    const float sine = std::sin(camera.yawRadians);
    const float cameraSpaceX = cosine * dx - sine * worldDepth;
    const float cameraSpaceForward = sine * dx + cosine * worldDepth;
    const float distance = std::max(24.0f, camera.cameraDepth - cameraSpaceForward);
    const float scale = camera.focalLength / distance;
    return {
        camera.viewportCenterX + cameraSpaceX * scale,
        camera.horizonY + camera.cameraHeight * scale,
        scale,
        distance,
    };
}

} // namespace dragon::shop_demo
