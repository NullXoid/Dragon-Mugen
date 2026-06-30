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
    const float s = std::clamp(uiScale, 0.60f, 1.75f);
    return {
        24.0f * s,
        22.0f * s,
        18.0f * s,
        14.0f * s,
        s >= 1.5f ? 2.0f : 1.0f,
        6.0f * s,
        s,
    };
}

float dragonUiDensityForDimensions(CanvasDimensions dimensions) {
    static_cast<void>(dimensions);
    return 1.0f;
}

DragonUiMetrics dragonUiMetricsForCanvas(CanvasDimensions dimensions, float uiScale) {
    return dragonUiMetricsForScale(uiScale * dragonUiDensityForDimensions(dimensions));
}

DragonUiMetrics dragonUiMetricsForContext(const UiRenderContext& ui) {
    return dragonUiMetricsForCanvas(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight }, ui.scale);
}

DragonUiMetrics dragonUiMetricsForLayout(DragonLayoutClass layoutClass) {
    switch (layoutClass) {
    case DragonLayoutClass::StandardDefinition:
        return dragonUiMetricsForCanvas(dimensionsForPreset(CanvasPreset::Sd854x480), 1.0f);
    case DragonLayoutClass::HighDefinition:
        return dragonUiMetricsForCanvas(dimensionsForPreset(CanvasPreset::Hd1280x720), 1.0f);
    case DragonLayoutClass::Classic:
    case DragonLayoutClass::WideLowRes:
    case DragonLayoutClass::ExtraLowRes:
    default:
        return dragonUiMetricsForScale(1.0f);
    }
}

DragonUiMetrics dragonUiMetricsForPreset(CanvasPreset preset) {
    return dragonUiMetricsForCanvas(dimensionsForPreset(preset), 1.0f);
}

SDL_FRect dragonPixelUiSafeArea(CanvasDimensions dimensions) {
    return {
        0.0f,
        0.0f,
        static_cast<float>(dimensions.width),
        static_cast<float>(dimensions.height),
    };
}

} // namespace dragon
