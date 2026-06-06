#include "UiSpriteView.h"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

namespace dragon {

bool hasTexture(const UiSpriteView& sprite) {
    return sprite.texture != nullptr;
}

void drawSpriteTopLeft(
    SDL_Renderer* renderer,
    const UiSpriteView& sprite,
    float x,
    float y,
    float scale,
    SDL_FlipMode flip) {
    if (!hasTexture(sprite) || sprite.width <= 0 || sprite.height <= 0) {
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

} // namespace dragon
