#pragma once

#include "AppTypes.h"
#include "UiRenderContext.h"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>

namespace dragon {

enum class DragonTypographyRole {
    DisplayTitle,
    PanelTitle,
    SectionTitle,
    ListPrimary,
    ListSecondary,
    MetadataLabel,
    MetadataValue,
    HelpText,
    StatusText,
    CurrencyText,
};

struct DragonUiTokens {
    SDL_Color panelBase{ 0x07, 0x10, 0x19, 255 };
    SDL_Color secondaryPanel{ 0x10, 0x1A, 0x27, 255 };
    SDL_Color primaryTeal{ 0x51, 0xD2, 0xC6, 255 };
    SDL_Color mutedGold{ 0xE7, 0xC3, 0x5A, 255 };
    SDL_Color characterPurple{ 0x7A, 0x4D, 0xD8, 255 };
    SDL_Color separatorRed{ 0xC6, 0x4F, 0x55, 255 };
    SDL_Color primaryText{ 0xE9, 0xED, 0xF3, 255 };
    SDL_Color mutedText{ 0x89, 0x96, 0xA7, 255 };
};

struct DragonUiMetrics {
    float topBarH = 24.0f;
    float helpBarH = 22.0f;
    float rowH = 18.0f;
    float itemIcon = 14.0f;
    float border = 1.0f;
    float padding = 6.0f;
    float pixelScale = 1.0f;
};

const DragonUiTokens& dragonUiTokens();
SDL_Color dragonTextColor(DragonTypographyRole role);
DragonUiMetrics dragonUiMetricsForScale(float uiScale);
float dragonUiDensityForDimensions(CanvasDimensions dimensions);
DragonUiMetrics dragonUiMetricsForCanvas(CanvasDimensions dimensions, float uiScale);
DragonUiMetrics dragonUiMetricsForContext(const UiRenderContext& ui);
DragonUiMetrics dragonUiMetricsForPreset(CanvasPreset preset);
SDL_FRect dragonPixelUiSafeArea(CanvasDimensions dimensions);

} // namespace dragon
