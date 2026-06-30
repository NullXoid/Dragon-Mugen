#pragma once

#include <SDL3/SDL_render.h>

namespace dragon {

struct UiRenderContext {
    SDL_Renderer* renderer = nullptr;
    int logicalWidth = 0;
    int logicalHeight = 0;
    float scale = 1.0f;
    int outputWidth = 0;
    int outputHeight = 0;
};

} // namespace dragon
