#pragma once

#include <SDL3/SDL_render.h>

namespace dragon {

struct UiSpriteView {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    int axisX = 0;
    int axisY = 0;
};

bool hasTexture(const UiSpriteView& sprite);

void drawSpriteTopLeft(
    SDL_Renderer* renderer,
    const UiSpriteView& sprite,
    float x,
    float y,
    float scale = 1.0f,
    SDL_FlipMode flip = SDL_FLIP_NONE);

} // namespace dragon
