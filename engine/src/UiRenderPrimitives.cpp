#include "UiRenderPrimitives.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace dragon {
namespace {

void renderDebugTextRaw(SDL_Renderer* renderer, float x, float y, const char* text) {
    if (!text || text[0] == '\0') {
        return;
    }
    SDL_RenderDebugText(renderer, x, y, text);
}

bool textColorNeedsShadow(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (a == 0) {
        return false;
    }
    const int luminance = (static_cast<int>(r) * 299 + static_cast<int>(g) * 587 + static_cast<int>(b) * 114) / 1000;
    return luminance >= 96;
}

} // namespace

void setColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void fillRect(SDL_Renderer* renderer, float x, float y, float w, float h) {
    SDL_FRect rect{ x, y, w, h };
    SDL_RenderFillRect(renderer, &rect);
}

void drawRect(SDL_Renderer* renderer, float x, float y, float w, float h) {
    SDL_FRect rect{ x, y, w, h };
    SDL_RenderRect(renderer, &rect);
}

float uiScaleFromPercent(int percent) {
    return std::clamp(static_cast<float>(percent) / 100.0f, 0.6f, 1.0f);
}

void fillScaledRect(SDL_Renderer* renderer, float scale, float x, float y, float w, float h) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    fillRect(renderer, x / scale, y / scale, w / scale, h / scale);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

void drawScaledRect(SDL_Renderer* renderer, float scale, float x, float y, float w, float h) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    drawRect(renderer, x / scale, y / scale, w / scale, h / scale);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

void debugText(SDL_Renderer* renderer, float x, float y, const std::string& text) {
    debugText(renderer, x, y, text.c_str());
}

void debugText(SDL_Renderer* renderer, float x, float y, const char* text) {
    Uint8 r = 255;
    Uint8 g = 255;
    Uint8 b = 255;
    Uint8 a = 255;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    if (textColorNeedsShadow(r, g, b, a)) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, std::min<Uint8>(190, a));
        renderDebugTextRaw(renderer, x + 1.0f, y + 1.0f, text);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
    }
    renderDebugTextRaw(renderer, x, y, text);
}

void scaledDebugText(SDL_Renderer* renderer, float scale, float x, float y, const std::string& text) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    debugText(renderer, x / scale, y / scale, text);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

ScopedUiScale::ScopedUiScale(const UiRenderContext& context, float virtualWidth, float virtualHeight)
    : renderer(context.renderer) {
    SDL_GetRenderViewport(renderer, &oldViewport);
    SDL_GetRenderScale(renderer, &oldScaleX, &oldScaleY);

    const float width = virtualWidth * context.scale;
    const float height = virtualHeight * context.scale;
    SDL_Rect viewport{
        static_cast<int>(std::lround((static_cast<float>(context.logicalWidth) - width) * 0.5f)),
        static_cast<int>(std::lround((static_cast<float>(context.logicalHeight) - height) * 0.5f)),
        static_cast<int>(std::lround(width)),
        static_cast<int>(std::lround(height)),
    };
    SDL_SetRenderViewport(renderer, &viewport);
    SDL_SetRenderScale(renderer, context.scale, context.scale);
}

ScopedUiScale::~ScopedUiScale() {
    SDL_SetRenderScale(renderer, oldScaleX, oldScaleY);
    SDL_SetRenderViewport(renderer, &oldViewport);
}

void debugTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text) {
    debugText(renderer, centerX - static_cast<float>(text.size()) * 4.0f, y, text);
}

std::string fitDebugText(const std::string& value, std::size_t maxChars) {
    if (value.size() <= maxChars) {
        return value;
    }
    if (maxChars <= 1) {
        return value.substr(0, maxChars);
    }
    return value.substr(0, maxChars - 1) + "~";
}

void drawPanel(SDL_Renderer* renderer, float x, float y, float w, float h) {
    setColor(renderer, 18, 20, 24, 232);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 78, 90, 112);
    drawRect(renderer, x, y, w, h);
}

} // namespace dragon
