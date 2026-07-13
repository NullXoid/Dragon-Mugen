#include "DragonUi.h"

#include <algorithm>
#include <cmath>

namespace dragon {

const DragonUiTokens& dragonUiTokens() {
    static const DragonUiTokens tokens;
    return tokens;
}

SDL_Color dragonTextColor(DragonTypographyRole role) {
    const auto& tokens = dragonUiTokens();
    switch (role) {
    case DragonTypographyRole::DisplayTitle:
    case DragonTypographyRole::PanelTitle:
    case DragonTypographyRole::CurrencyText:
        return tokens.mutedGold;
    case DragonTypographyRole::SectionTitle:
    case DragonTypographyRole::MetadataValue:
        return tokens.primaryTeal;
    case DragonTypographyRole::ListSecondary:
    case DragonTypographyRole::MetadataLabel:
    case DragonTypographyRole::HelpText:
    case DragonTypographyRole::StatusText:
        return tokens.mutedText;
    case DragonTypographyRole::ListPrimary:
    default:
        return tokens.primaryText;
    }
}

DragonUiMetrics dragonUiMetricsForScale(float uiScale) {
    const float s = std::clamp(uiScale, 0.60f, 3.0f);
    return {
        24.0f * s,
        22.0f * s,
        18.0f * s,
        14.0f * s,
        std::max(1.0f, std::round(s)),
        6.0f * s,
        s,
    };
}

float dragonUiDensityForDimensions(CanvasDimensions dimensions) {
    if (dimensions.width >= kHdLogicalWidth || dimensions.height >= kHdLogicalHeight) {
        return 3.0f;
    }
    if (dimensions.width >= kSdLogicalWidth || dimensions.height >= kSdLogicalHeight) {
        return 2.0f;
    }
    return 1.0f;
}

DragonUiMetrics dragonUiMetricsForCanvas(CanvasDimensions dimensions, float uiScale) {
    return dragonUiMetricsForScale(uiScale * dragonUiDensityForDimensions(dimensions));
}

DragonUiMetrics dragonUiMetricsForContext(const UiRenderContext& ui) {
    return dragonUiMetricsForCanvas(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight }, ui.scale);
}

DragonUiMetrics dragonUiMetricsForPreset(CanvasPreset preset) {
    return dragonUiMetricsForCanvas(dimensionsForPreset(preset), 1.0f);
}

SDL_FRect dragonPixelUiSafeArea(CanvasDimensions dimensions) {
    constexpr float targetAspect = 16.0f / 9.0f;
    const float width = static_cast<float>(std::max(1, dimensions.width));
    const float height = static_cast<float>(std::max(1, dimensions.height));
    const float aspect = width / height;

    if (std::fabs(aspect - targetAspect) <= 0.01f) {
        return {
            0.0f,
            0.0f,
            width,
            height,
        };
    }

    if (aspect > targetAspect) {
        const float safeW = std::floor(height * targetAspect);
        return {
            std::floor((width - safeW) * 0.5f),
            0.0f,
            safeW,
            height,
        };
    }

    const float safeH = std::floor(width / targetAspect);
    return {
        0.0f,
        std::floor((height - safeH) * 0.5f),
        width,
        safeH,
    };
}

} // namespace dragon
