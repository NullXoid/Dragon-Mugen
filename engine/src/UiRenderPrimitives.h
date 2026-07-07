#pragma once

#include "UiRenderContext.h"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

#include <cstddef>
#include <string>

namespace dragon {

void setColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
void setColor(SDL_Renderer* renderer, SDL_Color color);
void setColor(SDL_Renderer* renderer, SDL_Color color, Uint8 alpha);
void fillRect(SDL_Renderer* renderer, float x, float y, float w, float h);
void drawRect(SDL_Renderer* renderer, float x, float y, float w, float h);

float uiScaleFromPercent(int percent);

void fillScaledRect(SDL_Renderer* renderer, float scale, float x, float y, float w, float h);
void drawScaledRect(SDL_Renderer* renderer, float scale, float x, float y, float w, float h);

void debugText(SDL_Renderer* renderer, float x, float y, const std::string& text);
void debugText(SDL_Renderer* renderer, float x, float y, const char* text);
void scaledDebugText(SDL_Renderer* renderer, float scale, float x, float y, const std::string& text);

struct ScopedUiScale {
    SDL_Renderer* renderer = nullptr;
    SDL_Rect oldViewport{};
    float oldScaleX = 1.0f;
    float oldScaleY = 1.0f;

    ScopedUiScale(const UiRenderContext& context, float virtualWidth, float virtualHeight);
    ScopedUiScale(const ScopedUiScale&) = delete;
    ScopedUiScale& operator=(const ScopedUiScale&) = delete;
    ~ScopedUiScale();
};

struct ScopedVirtualCanvas {
    SDL_Renderer* renderer = nullptr;
    SDL_Rect oldViewport{};
    float oldScaleX = 1.0f;
    float oldScaleY = 1.0f;

    ScopedVirtualCanvas(const UiRenderContext& context, float virtualWidth, float virtualHeight);
    ScopedVirtualCanvas(const ScopedVirtualCanvas&) = delete;
    ScopedVirtualCanvas& operator=(const ScopedVirtualCanvas&) = delete;
    ~ScopedVirtualCanvas();
};

void debugTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text);
std::string fitDebugText(const std::string& value, std::size_t maxChars);
void drawPanel(SDL_Renderer* renderer, float x, float y, float w, float h);

} // namespace dragon
