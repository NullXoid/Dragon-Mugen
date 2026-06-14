#include "UiRenderPrimitives.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string_view>

namespace dragon {
namespace {

using GlyphRows = std::array<Uint8, 7>;

bool textColorNeedsShadow(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (a == 0) {
        return false;
    }
    const int luminance = (static_cast<int>(r) * 299 + static_cast<int>(g) * 587 + static_cast<int>(b) * 114) / 1000;
    return luminance >= 96;
}

GlyphRows glyphRows(char raw) {
    unsigned char c = static_cast<unsigned char>(raw);
    if (std::islower(c)) {
        c = static_cast<unsigned char>(std::toupper(c));
    }

    switch (static_cast<char>(c)) {
    case ' ': return { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 };
    case '!': return { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 };
    case '"': return { 0b01010, 0b01010, 0b01010, 0b00000, 0b00000, 0b00000, 0b00000 };
    case '#': return { 0b01010, 0b01010, 0b11111, 0b01010, 0b11111, 0b01010, 0b01010 };
    case '$': return { 0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100 };
    case '%': return { 0b11001, 0b11010, 0b00010, 0b00100, 0b01000, 0b01011, 0b10011 };
    case '&': return { 0b01100, 0b10010, 0b10100, 0b01000, 0b10101, 0b10010, 0b01101 };
    case '\'': return { 0b00100, 0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000 };
    case '(': return { 0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010 };
    case ')': return { 0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000 };
    case '*': return { 0b00000, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00000 };
    case '+': return { 0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000 };
    case ',': return { 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100, 0b01000 };
    case '-': return { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 };
    case '.': return { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100 };
    case '/': return { 0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000 };
    case '0': return { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 };
    case '1': return { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 };
    case '2': return { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 };
    case '3': return { 0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110 };
    case '4': return { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 };
    case '5': return { 0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110 };
    case '6': return { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 };
    case '7': return { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 };
    case '8': return { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 };
    case '9': return { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 };
    case ':': return { 0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00000 };
    case ';': return { 0b00000, 0b01100, 0b01100, 0b00000, 0b00100, 0b00100, 0b01000 };
    case '<': return { 0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010 };
    case '=': return { 0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000 };
    case '>': return { 0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000 };
    case '?': return { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100 };
    case '@': return { 0b01110, 0b10001, 0b10111, 0b10101, 0b10111, 0b10000, 0b01110 };
    case 'A': return { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 };
    case 'B': return { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 };
    case 'C': return { 0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111 };
    case 'D': return { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 };
    case 'E': return { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 };
    case 'F': return { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 };
    case 'G': return { 0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 };
    case 'H': return { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 };
    case 'I': return { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 };
    case 'J': return { 0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100 };
    case 'K': return { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 };
    case 'L': return { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 };
    case 'M': return { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 };
    case 'N': return { 0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001 };
    case 'O': return { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
    case 'P': return { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 };
    case 'Q': return { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 };
    case 'R': return { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 };
    case 'S': return { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 };
    case 'T': return { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
    case 'U': return { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
    case 'V': return { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 };
    case 'W': return { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 };
    case 'X': return { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 };
    case 'Y': return { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 };
    case 'Z': return { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 };
    case '[': return { 0b01110, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01110 };
    case '\\': return { 0b10000, 0b01000, 0b01000, 0b00100, 0b00010, 0b00010, 0b00001 };
    case ']': return { 0b01110, 0b00010, 0b00010, 0b00010, 0b00010, 0b00010, 0b01110 };
    case '^': return { 0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000, 0b00000 };
    case '_': return { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111 };
    case '`': return { 0b01000, 0b00100, 0b00010, 0b00000, 0b00000, 0b00000, 0b00000 };
    case '{': return { 0b00010, 0b00100, 0b00100, 0b01000, 0b00100, 0b00100, 0b00010 };
    case '|': return { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
    case '}': return { 0b01000, 0b00100, 0b00100, 0b00010, 0b00100, 0b00100, 0b01000 };
    case '~': return { 0b00000, 0b00000, 0b01000, 0b10101, 0b00010, 0b00000, 0b00000 };
    default: return glyphRows('?');
    }
}

void renderGlyph(SDL_Renderer* renderer, float x, float y, char c) {
    const GlyphRows rows = glyphRows(c);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if ((rows[static_cast<size_t>(row)] & (1u << (4 - col))) == 0) {
                continue;
            }
            fillRect(renderer, x + 1.0f + static_cast<float>(col), y + static_cast<float>(row), 1.0f, 1.0f);
        }
    }
}

void renderDebugTextRaw(SDL_Renderer* renderer, float x, float y, const char* text) {
    if (!text || text[0] == '\0') {
        return;
    }
    float penX = x;
    float penY = y;
    for (char c : std::string_view(text)) {
        if (c == '\n') {
            penX = x;
            penY += 9.0f;
            continue;
        }
        renderGlyph(renderer, penX, penY, c);
        penX += 8.0f;
    }
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
