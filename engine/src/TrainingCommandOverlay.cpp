#include "TrainingCommandOverlay.h"

#include "DragonUi.h"
#include "TrainingCommandInputRenderer.h"
#include "UiRenderPrimitives.h"
#include "UiSpriteView.h"

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <string>
#include <type_traits>
#include <utility>

namespace dragon {

namespace {

constexpr int kCommandCompletionFlashTicks = 72;

CommandInputChipTone commandStepTone(TrainingCommandStepStatus status) {
    switch (status) {
    case TrainingCommandStepStatus::Matched:
        return CommandInputChipTone::Matched;
    case TrainingCommandStepStatus::Current:
        return CommandInputChipTone::Current;
    case TrainingCommandStepStatus::Pending:
    default:
        return CommandInputChipTone::Pending;
    }
}

CommandInputRenderOptions commandInputOptions(
    float scale,
    CommandInputChipTone tone,
    const TrainingCommandHudView& view,
    float visualScale = 1.0f) {
    CommandInputRenderOptions options;
    options.scale = scale;
    options.tone = tone;
    options.iconAtlas = view.commandIcons;
    options.visualScale = visualScale * scale;
    if (view.physicalDirections) {
        options.directionPresentation = CommandInputDirectionPresentation::Physical;
        options.facing = view.facing;
    }
    return options;
}

CommandInputRenderOptions liveInputOptions(
    float scale,
    CommandInputChipTone tone,
    const TrainingCommandHudView& view) {
    CommandInputRenderOptions options;
    options.scale = scale;
    options.tone = tone;
    options.iconAtlas = view.commandIcons;
    options.visualScale = scale;
    return options;
}

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size()) * 8.0f;
}

bool emptyInputHistoryPlaceholder(const std::string& input) {
    return input.empty() || input == "-";
}

void drawInputHistoryValue(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    float w,
    const std::string& input,
    const TrainingCommandHudView& view) {
    if (emptyInputHistoryPlaceholder(input)) {
        setColor(renderer, 154, 166, 184, 210);
        scaledDebugText(renderer, scale, x, y + 1.0f, "- - -");
        return;
    }

    drawCommandInputChips(
        renderer,
        x,
        y,
        w,
        input,
        liveInputOptions(scale, CommandInputChipTone::Pending, view));
}

void fillScaledCircle(SDL_Renderer* renderer, float scale, float centerX, float centerY, float radius) {
    const int rowCount = std::max(1, static_cast<int>(std::ceil(radius * 2.0f)));
    for (int row = 0; row < rowCount; ++row) {
        const float y = centerY - radius + static_cast<float>(row);
        const float sampleY = y + 0.5f;
        const float dy = sampleY - centerY;
        const float span = std::sqrt(std::max(0.0f, radius * radius - dy * dy));
        fillScaledRect(renderer, scale, centerX - span, y, span * 2.0f, 1.0f);
    }
}

void drawScaledUiSprite(
    SDL_Renderer* renderer,
    float scale,
    const UiSpriteView& sprite,
    float x,
    float y,
    float w,
    float h,
    Uint8 alpha = 255) {
    if (!hasTexture(sprite) || sprite.width <= 0 || sprite.height <= 0 || w <= 0.0f || h <= 0.0f) {
        return;
    }

    float oldScaleX = 1.0f;
    float oldScaleY = 1.0f;
    SDL_BlendMode oldBlend = SDL_BLENDMODE_BLEND;
    Uint8 oldAlpha = 255;
    SDL_GetRenderScale(renderer, &oldScaleX, &oldScaleY);
    SDL_GetTextureBlendMode(sprite.texture, &oldBlend);
    SDL_GetTextureAlphaMod(sprite.texture, &oldAlpha);
    SDL_SetTextureBlendMode(sprite.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(sprite.texture, alpha);
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_FRect dst{ x / scale, y / scale, w / scale, h / scale };
    SDL_RenderTexture(renderer, sprite.texture, nullptr, &dst);
    SDL_SetRenderScale(renderer, oldScaleX, oldScaleY);
    SDL_SetTextureBlendMode(sprite.texture, oldBlend);
    SDL_SetTextureAlphaMod(sprite.texture, oldAlpha);
}

void drawFadedHorizontalBand(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    float w,
    float h,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint8 alpha,
    float fadeW) {
    if (w <= 0.0f || h <= 0.0f || alpha == 0) {
        return;
    }

    const float edgeW = std::clamp(fadeW, 0.0f, w * 0.5f);
    const float centerW = std::max(0.0f, w - edgeW * 2.0f);
    if (centerW > 0.0f) {
        setColor(renderer, r, g, b, alpha);
        fillScaledRect(renderer, scale, x + edgeW, y, centerW, h);
    }

    const int steps = std::max(1, static_cast<int>(std::ceil(edgeW)));
    const float stripW = edgeW / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        const float t = static_cast<float>(i + 1) / static_cast<float>(steps);
        const Uint8 leftAlpha = static_cast<Uint8>(std::clamp(static_cast<int>(std::round(static_cast<float>(alpha) * t)), 0, 255));
        const Uint8 rightAlpha = static_cast<Uint8>(std::clamp(static_cast<int>(std::round(static_cast<float>(alpha) * (1.0f - (static_cast<float>(i) / static_cast<float>(steps))))), 0, 255));
        setColor(renderer, r, g, b, leftAlpha);
        fillScaledRect(renderer, scale, x + stripW * static_cast<float>(i), y, stripW + 0.25f, h);
        setColor(renderer, r, g, b, rightAlpha);
        fillScaledRect(renderer, scale, x + w - edgeW + stripW * static_cast<float>(i), y, stripW + 0.25f, h);
    }
}

float completionProgress(const TrainingCommandHudView& view) {
    if (!view.completionVisible || view.completionTicks <= 0) {
        return 0.0f;
    }
    const int ticks = std::clamp(view.completionTicks, 0, kCommandCompletionFlashTicks);
    return 1.0f - (static_cast<float>(ticks) / static_cast<float>(kCommandCompletionFlashTicks));
}

void drawCompletionSweep(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    float w,
    float h,
    float progress,
    bool flashOn) {
    if (w <= 0.0f || h <= 0.0f || progress <= 0.0f) {
        return;
    }

    setColor(renderer, flashOn ? 34 : 18, flashOn ? 118 : 88, flashOn ? 78 : 58, flashOn ? 112 : 72);
    fillScaledRect(renderer, scale, x, y, w, h);

    const float sweepW = 34.0f * scale;
    const float sweepX = x - sweepW + (w + sweepW * 1.6f) * std::clamp(progress, 0.0f, 1.0f);
    setColor(renderer, 255, 236, 142, 80);
    fillScaledRect(renderer, scale, sweepX, y, sweepW, h);
    setColor(renderer, 136, 255, 192, 112);
    fillScaledRect(renderer, scale, sweepX + sweepW * 0.35f, y, sweepW * 0.32f, h);
    setColor(renderer, 238, 255, 246, 120);
    fillScaledRect(renderer, scale, sweepX + sweepW * 0.50f, y + 1.0f * scale, 2.0f * scale, h - 2.0f * scale);
}

void drawCompletionCheckBadge(
    SDL_Renderer* renderer,
    float scale,
    const UiSpriteView& check,
    float centerX,
    float centerY,
    float progress,
    bool flashOn) {
    const float pulse = 1.0f + (flashOn ? 0.12f : 0.0f) + std::max(0.0f, 1.0f - progress * 3.0f) * 0.18f;
    const float glowRadius = 12.0f * scale * pulse;
    setColor(renderer, 34, 245, 146, flashOn ? 94 : 54);
    fillScaledCircle(renderer, scale, centerX, centerY, glowRadius);
    setColor(renderer, 255, 224, 132, flashOn ? 120 : 72);
    fillScaledCircle(renderer, scale, centerX, centerY, glowRadius * 0.72f);

    const float size = 18.0f * scale * pulse;
    if (hasTexture(check)) {
        drawScaledUiSprite(renderer, scale, check, centerX - size * 0.5f, centerY - size * 0.5f, size, size);
        return;
    }

    setColor(renderer, 130, 255, 190, 255);
    fillScaledRect(renderer, scale, centerX - 6.0f * scale, centerY + 1.0f * scale, 4.0f * scale, 4.0f * scale);
    fillScaledRect(renderer, scale, centerX - 2.0f * scale, centerY + 3.0f * scale, 4.0f * scale, 4.0f * scale);
    fillScaledRect(renderer, scale, centerX + 2.0f * scale, centerY - 1.0f * scale, 4.0f * scale, 4.0f * scale);
    fillScaledRect(renderer, scale, centerX + 6.0f * scale, centerY - 5.0f * scale, 4.0f * scale, 4.0f * scale);
}

void setGuideButtonColors(
    SDL_Renderer* renderer,
    const TrainingCommandButtonGuideButtonView& button,
    bool outer,
    bool flash) {
    if (button.matched || (flash && button.required)) {
        setColor(renderer, outer ? 168 : 24, outer ? 244 : 92, outer ? 196 : 70, outer ? 248 : 242);
        return;
    }
    if (button.required) {
        setColor(renderer, outer ? 238 : 76, outer ? 202 : 54, outer ? 118 : 22, outer ? 232 : 238);
        return;
    }
    if (button.pressed) {
        setColor(renderer, outer ? 126 : 30, outer ? 164 : 42, outer ? 214 : 64, outer ? 232 : 238);
        return;
    }
    setColor(renderer, outer ? 74 : 14, outer ? 88 : 20, outer ? 112 : 32, outer ? 220 : 232);
}

bool drawGuideGlyph(
    SDL_Renderer* renderer,
    float scale,
    float centerX,
    float centerY,
    const std::string& token,
    const CommandInputIconAtlasView& commandIcons,
    float w,
    float h) {
    CommandInputRenderOptions options;
    options.scale = scale;
    options.iconAtlas = commandIcons;
    options.visualScale = scale;
    const float glyphW = w * scale;
    const float glyphH = h * scale;
    return drawCommandInputIconGlyph(renderer, centerX - glyphW * 0.5f, centerY - glyphH * 0.5f, glyphW, glyphH, token, options);
}

void drawGuideButton(
    SDL_Renderer* renderer,
    float scale,
    float centerX,
    float centerY,
    const TrainingCommandButtonGuideButtonView& button,
    const CommandInputIconAtlasView& commandIcons,
    bool flash,
    float radius = 6.2f) {
    const float r = radius * scale;
    setGuideButtonColors(renderer, button, true, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, r);
    setGuideButtonColors(renderer, button, false, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, std::max(1.0f * scale, r - 1.0f * scale));

    if (button.matched || (flash && button.required)) {
        setColor(renderer, 210, 255, 226);
    } else if (button.required) {
        setColor(renderer, 255, 238, 178);
    } else if (button.pressed) {
        setColor(renderer, 212, 228, 255);
    } else {
        setColor(renderer, 154, 166, 184);
    }
    std::string label = button.label;
    if (drawGuideGlyph(renderer, scale, centerX, centerY, label, commandIcons, 12.0f, 8.0f)) {
        return;
    }
    if (label.rfind("BTN_", 0) == 0 && label.size() > 4) {
        label = label.substr(4);
    }
    if (label == "TRI") {
        label = "TR";
    } else if (label == "START") {
        label = "ST";
    } else {
        label = fitDebugText(label, 2);
    }
    scaledDebugText(renderer, scale, centerX - debugTextWidth(label) * scale * 0.5f, centerY - 3.0f * scale, label);
}

std::string directionGuideIconToken(const std::string& label) {
    if (label == "^") {
        return "U";
    }
    if (label == "v") {
        return "D";
    }
    if (label == "<") {
        return "B";
    }
    if (label == ">") {
        return "F";
    }
    return label;
}

void drawGuideDirectionArrow(SDL_Renderer* renderer, float scale, float centerX, float centerY, const std::string& label) {
    if (label == "^") {
        fillScaledRect(renderer, scale, centerX - 4.0f * scale, centerY - 4.0f * scale, 9.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 3.0f * scale, centerY - 3.0f * scale, 7.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 2.0f * scale, centerY - 2.0f * scale, 5.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 1.0f * scale, centerY - 1.0f * scale, 3.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX, centerY, 1.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 1.0f * scale, centerY + 1.0f * scale, 3.0f * scale, 4.0f * scale);
        return;
    }
    if (label == "v") {
        fillScaledRect(renderer, scale, centerX - 1.0f * scale, centerY - 5.0f * scale, 3.0f * scale, 4.0f * scale);
        fillScaledRect(renderer, scale, centerX, centerY, 1.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 1.0f * scale, centerY + 1.0f * scale, 3.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 2.0f * scale, centerY + 2.0f * scale, 5.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 3.0f * scale, centerY + 3.0f * scale, 7.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 4.0f * scale, centerY + 4.0f * scale, 9.0f * scale, 1.0f * scale);
        return;
    }
    if (label == "<") {
        fillScaledRect(renderer, scale, centerX - 4.0f * scale, centerY, 1.0f * scale, 1.0f * scale);
        fillScaledRect(renderer, scale, centerX - 3.0f * scale, centerY - 1.0f * scale, 1.0f * scale, 3.0f * scale);
        fillScaledRect(renderer, scale, centerX - 2.0f * scale, centerY - 2.0f * scale, 1.0f * scale, 5.0f * scale);
        fillScaledRect(renderer, scale, centerX - 1.0f * scale, centerY - 3.0f * scale, 1.0f * scale, 7.0f * scale);
        fillScaledRect(renderer, scale, centerX, centerY - 4.0f * scale, 1.0f * scale, 9.0f * scale);
        fillScaledRect(renderer, scale, centerX + 1.0f * scale, centerY - 1.0f * scale, 4.0f * scale, 3.0f * scale);
        return;
    }
    if (label == ">") {
        fillScaledRect(renderer, scale, centerX - 5.0f * scale, centerY - 1.0f * scale, 4.0f * scale, 3.0f * scale);
        fillScaledRect(renderer, scale, centerX, centerY - 4.0f * scale, 1.0f * scale, 9.0f * scale);
        fillScaledRect(renderer, scale, centerX + 1.0f * scale, centerY - 3.0f * scale, 1.0f * scale, 7.0f * scale);
        fillScaledRect(renderer, scale, centerX + 2.0f * scale, centerY - 2.0f * scale, 1.0f * scale, 5.0f * scale);
        fillScaledRect(renderer, scale, centerX + 3.0f * scale, centerY - 1.0f * scale, 1.0f * scale, 3.0f * scale);
        fillScaledRect(renderer, scale, centerX + 4.0f * scale, centerY, 1.0f * scale, 1.0f * scale);
        return;
    }

    scaledDebugText(renderer, scale, centerX - debugTextWidth(label) * scale * 0.5f, centerY - 3.0f * scale, label);
}

void drawDirectionGuideButton(
    SDL_Renderer* renderer,
    float scale,
    float centerX,
    float centerY,
    const TrainingCommandDirectionGuideButtonView& direction,
    const CommandInputIconAtlasView& commandIcons,
    bool flash) {
    const TrainingCommandButtonGuideButtonView button{
        direction.label,
        direction.pressed,
        direction.required,
        direction.matched,
    };
    const float radius = 5.6f * scale;
    setGuideButtonColors(renderer, button, true, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, radius);
    setGuideButtonColors(renderer, button, false, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, radius - 1.0f * scale);

    if (button.matched || (flash && button.required)) {
        setColor(renderer, 210, 255, 226);
    } else if (button.required) {
        setColor(renderer, 255, 238, 178);
    } else if (button.pressed) {
        setColor(renderer, 212, 228, 255);
    } else {
        setColor(renderer, 154, 166, 184);
    }
    if (drawGuideGlyph(renderer, scale, centerX, centerY, directionGuideIconToken(direction.label), commandIcons, 10.0f, 8.0f)) {
        return;
    }
    drawGuideDirectionArrow(renderer, scale, centerX, centerY, direction.label);
}

TrainingCommandButtonGuideButtonView asButtonGuideButton(const TrainingCommandDirectionGuideButtonView& direction) {
    return TrainingCommandButtonGuideButtonView{
        direction.label,
        direction.pressed,
        direction.required,
        direction.matched,
    };
}

template <typename ButtonView>
void drawGuideCluster(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    const std::array<ButtonView, 4>& buttons,
    const CommandInputIconAtlasView& commandIcons,
    bool flash) {
    const std::array<std::pair<float, float>, 4> centers{
        std::pair<float, float>{ x + 9.0f * scale, y + 22.0f * scale },
        std::pair<float, float>{ x + 19.0f * scale, y + 12.0f * scale },
        std::pair<float, float>{ x + 19.0f * scale, y + 32.0f * scale },
        std::pair<float, float>{ x + 29.0f * scale, y + 22.0f * scale },
    };
    for (std::size_t i = 0; i < centers.size(); ++i) {
        if constexpr (std::is_same_v<ButtonView, TrainingCommandDirectionGuideButtonView>) {
            drawDirectionGuideButton(renderer, scale, centers[i].first, centers[i].second, buttons[i], commandIcons, flash);
        } else {
            drawGuideButton(renderer, scale, centers[i].first, centers[i].second, buttons[i], commandIcons, flash);
        }
    }
}

struct TrainingHudRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct TrainingCommandHudLayout {
    TrainingHudRect objective;
    TrainingHudRect input;
    TrainingHudRect guide;
    float stepX = 0.0f;
    float stepY = 0.0f;
    float stepRight = 0.0f;
    bool objectiveVisible = false;
    bool inputVisible = false;
    bool guideVisible = false;
    bool commandIconsVisible = false;
};

float clampUi(float value, float minValue, float maxValue) {
    return std::clamp(value, minValue, std::max(minValue, maxValue));
}

std::pair<int, int> practiceStepProgress(const TrainingCommandHudView& view) {
    int matched = 0;
    int total = 0;
    for (const auto& step : view.practiceSteps) {
        ++total;
        if (step.status == TrainingCommandStepStatus::Matched) {
            ++matched;
        }
    }
    return { matched, total };
}

bool rectIntersects(const TrainingHudRect& a, const TrainingHudRect& b) {
    if (a.w <= 0.0f || a.h <= 0.0f || b.w <= 0.0f || b.h <= 0.0f) {
        return false;
    }
    return a.x < b.x + b.w
        && a.x + a.w > b.x
        && a.y < b.y + b.h
        && a.y + a.h > b.y;
}

float dynamicInputHudWidth(const TrainingCommandHudView& view) {
    const auto actualOptions = liveInputOptions(1.0f, CommandInputChipTone::Normal, view);
    const auto expectedOptions = commandInputOptions(1.0f, CommandInputChipTone::Current, view);
    const float currentW = commandInputWidth(view.input.currentInput, actualOptions);
    const float recentW = commandInputWidth(view.input.recentInputs, actualOptions);
    const float expectedW = commandInputWidth(view.input.expectedInput.empty() ? "-" : view.input.expectedInput, expectedOptions);
    const float labelW = debugTextWidth("INPUT HISTORY") + 18.0f;
    const float expectedLabelW = debugTextWidth("EXPECTED") + 18.0f;
    const float desired = std::max({
        124.0f,
        labelW,
        expectedLabelW,
        currentW + 18.0f,
        recentW + 18.0f,
        expectedW + 18.0f,
    });
    return std::clamp(desired, 124.0f, 212.0f);
}

TrainingCommandHudLayout trainingCommandHudLayout(
    const TrainingCommandHudView& view,
    float widthF,
    float heightF,
    float scale = 1.0f,
    float originX = 0.0f,
    float originY = 0.0f) {
    TrainingCommandHudLayout layout;
    const auto scaledRect = [scale, originX, originY](float x, float y, float w, float h) {
        return TrainingHudRect{ originX + x * scale, originY + y * scale, w * scale, h * scale };
    };

    if (view.commandsVisible) {
        const float commandW = clampUi(widthF - 126.0f, 246.0f, 306.0f);
        const float commandX = std::max(10.0f, (widthF - commandW) * 0.5f);
        const float commandY = 49.0f;
        const float commandH = 50.0f;
        const float statusW = widthF < 360.0f ? 42.0f : 64.0f;
        layout.objective = scaledRect(commandX, commandY, commandW, commandH);
        layout.stepX = originX + (commandX + 8.0f) * scale;
        layout.stepY = originY + (commandY + 28.0f) * scale;
        layout.stepRight = originX + (commandX + commandW - statusW - 8.0f) * scale;
        layout.objectiveVisible = true;
        layout.commandIconsVisible = (commandW - statusW - 16.0f) >= 54.0f;
    }

    const bool showAnyGuide = (view.buttonGuide.visible || view.directionGuide.visible) && !view.paused;
    constexpr float inputX = 24.0f;
    constexpr float inputH = 64.0f;
    constexpr float guideW = 104.0f;
    constexpr float guideH = 64.0f;
    const float inputY = clampUi(heightF - guideH - 9.0f, 104.0f, 158.0f);
    if (showAnyGuide) {
        const float desiredGuideX = widthF - guideW - 20.0f;
        float guideX = clampUi(desiredGuideX, 8.0f, widthF - guideW - 8.0f);
        const float desiredInputW = view.input.visible ? dynamicInputHudWidth(view) : 0.0f;
        float inputW = view.input.visible
            ? std::min(desiredInputW, std::max(112.0f, guideX - inputX - 20.0f))
            : 0.0f;
        if (view.input.visible && inputX + inputW + 12.0f > guideX) {
            guideX = clampUi(inputX + inputW + 12.0f, 8.0f, widthF - guideW - 8.0f);
            if (inputX + inputW + 12.0f > guideX) {
                inputW = std::clamp(guideX - inputX - 12.0f, 112.0f, desiredInputW);
            }
        }
        layout.guide = scaledRect(guideX, inputY - 1.0f, guideW, guideH);
        layout.guideVisible = guideX >= 8.0f
            && guideX + guideW <= widthF - 8.0f
            && inputY - 1.0f + guideH <= heightF - 8.0f;
        if (view.input.visible) {
            layout.input = scaledRect(inputX - 8.0f, inputY - 7.0f, inputW + 6.0f, inputH);
            layout.inputVisible = true;
        }
    } else if (view.input.visible) {
        const float inputW = std::min(dynamicInputHudWidth(view), widthF - inputX - 18.0f);
        layout.input = scaledRect(inputX - 8.0f, inputY - 7.0f, inputW + 6.0f, inputH);
        layout.inputVisible = true;
    }

    return layout;
}

void drawCornerAccents(SDL_Renderer* renderer, float scale, const TrainingHudRect& rect, Uint8 alpha) {
    const float tick = 9.0f * scale;
    const float line = std::max(1.0f, scale);
    setColor(renderer, 72, 208, 246, alpha);
    fillScaledRect(renderer, scale, rect.x, rect.y, tick, line);
    fillScaledRect(renderer, scale, rect.x, rect.y, line, tick);
    fillScaledRect(renderer, scale, rect.x + rect.w - tick, rect.y, tick, line);
    fillScaledRect(renderer, scale, rect.x + rect.w - line, rect.y, line, tick);
    fillScaledRect(renderer, scale, rect.x, rect.y + rect.h - line, tick, line);
    fillScaledRect(renderer, scale, rect.x, rect.y + rect.h - tick, line, tick);
    fillScaledRect(renderer, scale, rect.x + rect.w - tick, rect.y + rect.h - line, tick, line);
    fillScaledRect(renderer, scale, rect.x + rect.w - line, rect.y + rect.h - tick, line, tick);
}

void drawTrainingGuideDock(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    const TrainingCommandDirectionGuideView& directionGuide,
    const TrainingCommandButtonGuideView& buttonGuide,
    const CommandInputIconAtlasView& commandIcons,
    bool flash) {
    if (!directionGuide.visible && !buttonGuide.visible) {
        return;
    }

    constexpr float dockH = 64.0f;
    setColor(renderer, 170, 178, 188, 22);
    fillScaledRect(renderer, scale, x + 51.0f * scale, y + 7.0f * scale, 1.0f * scale, (dockH - 14.0f) * scale);

    if (directionGuide.visible) {
        drawGuideCluster(renderer, scale, x + 8.0f * scale, y + 6.0f * scale, directionGuide.directions, commandIcons, flash);
    }
    if (buttonGuide.visible) {
        drawGuideCluster(renderer, scale, x + 60.0f * scale, y + 6.0f * scale, buttonGuide.buttons, commandIcons, flash);
        if (buttonGuide.systemButtonVisible) {
            drawGuideButton(
                renderer,
                scale,
                x + 79.0f * scale,
                y + 51.0f * scale,
                buttonGuide.systemButton,
                commandIcons,
                flash,
                5.4f);
        }
    }
}

void drawPauseLegendRow(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    const std::string& label,
    const std::string& text,
    Uint8 r,
    Uint8 g,
    Uint8 b) {
    setColor(renderer, r, g, b, 238);
    scaledDebugText(renderer, scale, x, y, label);
    setColor(renderer, 176, 188, 204, 224);
    scaledDebugText(renderer, scale, x + 88.0f * scale, y, text);
}

std::string liveStatusText(const TrainingCommandHudView& view) {
    if (view.completeFlash || view.completionVisible) {
        return "COMPLETE";
    }
    return {};
}

} // namespace

void drawTrainingCommandOverlay(const UiRenderContext& ui, const TrainingCommandHudView& view) {
    if (!view.input.visible && !view.commandsVisible) {
        return;
    }

    SDL_Renderer* renderer = ui.renderer;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const SDL_FRect safe = dragonPixelUiSafeArea(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight });
    const float scale = metrics.pixelScale;
    const float widthF = safe.w / std::max(0.01f, scale);
    const float heightF = safe.h / std::max(0.01f, scale);
    const TrainingCommandHudLayout layout = trainingCommandHudLayout(view, widthF, heightF, scale, safe.x, safe.y);

    if (view.commandsVisible) {
        const bool flashOn = view.completeFlash && ((SDL_GetTicks() / 120) % 2 == 0);
        const float completeProgress = completionProgress(view);
        constexpr float objectiveIconScale = 1.25f;
        const TrainingHudRect& command = layout.objective;
        const float statusDividerX = layout.stepRight + 4.0f * scale;
        const float bandY = command.y + command.h * 0.05f;
        const float bandH = command.h * 0.90f;
        const float fadeW = std::max(24.0f * scale, command.w * 0.12f);
        drawFadedHorizontalBand(renderer, scale, command.x, bandY, command.w, bandH, 3, 6, 12, view.paused ? 84 : 116, fadeW);
        drawFadedHorizontalBand(renderer, scale, command.x + 1.0f * scale, bandY + 1.0f * scale, command.w - 2.0f * scale, bandH - 2.0f * scale, 28, 42, 62, view.paused ? 54 : 74, fadeW - 1.0f * scale);
        drawFadedHorizontalBand(renderer, scale, command.x + 10.0f * scale, bandY, command.w - 20.0f * scale, 1.0f * scale, 66, 202, 246, view.paused ? 28 : 38, fadeW);
        drawFadedHorizontalBand(renderer, scale, command.x + 8.0f * scale, command.y + 20.0f * scale, command.w - 16.0f * scale, 1.0f * scale, 224, 190, 82, view.paused ? 104 : 168, fadeW);

        if (view.completeFlash) {
            drawFadedHorizontalBand(
                renderer,
                scale,
                command.x + 1.0f * scale,
                bandY + 1.0f * scale,
                command.w - 2.0f * scale,
                17.0f * scale,
                flashOn ? 52 : 26,
                flashOn ? 162 : 108,
                flashOn ? 118 : 92,
                flashOn ? 164 : 104,
                fadeW - 1.0f * scale);
            setColor(renderer, 216, 255, 230);
        } else if (view.demoActive) {
            drawFadedHorizontalBand(renderer, scale, command.x + 1.0f * scale, bandY + 1.0f * scale, command.w - 2.0f * scale, 17.0f * scale, 34, 78, 132, 94, fadeW - 1.0f * scale);
            setColor(renderer, 222, 236, 252);
        } else {
            setColor(renderer, 128, 216, 242);
        }
        scaledDebugText(renderer, scale, command.x + 8.0f * scale, command.y + 7.0f * scale, "MOVE:");
        setColor(renderer, view.completeFlash ? 184 : 90, view.completeFlash ? 255 : 226, view.completeFlash ? 212 : 246);
        const float nameX = command.x + 48.0f * scale;
        const std::size_t nameChars = static_cast<std::size_t>(
            std::max(6.0f, (statusDividerX - nameX - 5.0f * scale) / (8.0f * scale)));
        scaledDebugText(renderer, scale, nameX, command.y + 7.0f * scale, fitDebugText(view.currentMoveName, nameChars));
        const std::string statusText = liveStatusText(view);
        const bool completeStatus = view.completeFlash || view.completionVisible;
        if (!statusText.empty()) {
            setColor(renderer, 52, 118, 144, view.paused ? 54 : 86);
            fillScaledRect(renderer, scale, statusDividerX, command.y + 7.0f * scale, 1.0f * scale, command.h - 14.0f * scale);
            if (completeStatus) {
                setColor(renderer, 116, 244, 176, view.paused ? 144 : 238);
            } else {
                setColor(renderer, 246, 218, 82, view.paused ? 128 : 220);
            }
            const std::size_t statusChars = command.w < 300.0f * scale ? 5u : 8u;
            scaledDebugText(renderer, scale, statusDividerX + 6.0f * scale, command.y + 7.0f * scale, fitDebugText(statusText, statusChars));
        }
        if (view.completionVisible) {
            drawCompletionCheckBadge(
                renderer,
                scale,
                view.completionCheck,
                command.x + command.w - 18.0f * scale,
                command.y + 31.0f * scale,
                completeProgress,
                flashOn);
        }

        float stepX = layout.stepX;
        const float stepY = layout.stepY;
        const float stepRight = layout.stepRight;
        int stepsDrawn = 0;
        if (view.completionVisible) {
            drawCompletionSweep(
                renderer,
                scale,
                command.x + 7.0f * scale,
                stepY - 4.0f * scale,
                stepRight - command.x - 12.0f * scale,
                13.0f * scale,
                completeProgress,
                flashOn);
        }
        const std::size_t visibleStepCount = std::min<std::size_t>(view.practiceSteps.size(), 16u);
        const bool roomyCommand = visibleStepCount > 0 && visibleStepCount <= 5
            && (stepRight - layout.stepX) >= 150.0f * scale;
        const float separatorW = (roomyCommand ? 16.0f : 12.0f) * scale;
        const float stepGap = (roomyCommand ? 9.0f : 6.0f) * scale;
        for (const auto& step : view.practiceSteps) {
            if (stepsDrawn >= 16) {
                break;
            }
            const auto stepOptions = commandInputOptions(scale, commandStepTone(step.status), view, objectiveIconScale);
            const float stepW = std::max(10.0f, commandInputWidth(step.label, stepOptions));
            if (stepsDrawn > 0) {
                if (stepX + separatorW + stepW > stepRight) {
                    break;
                }
                setColor(renderer, 255, 226, 64, 246);
                scaledDebugText(renderer, scale, stepX + 2.0f * scale, stepY + 1.0f * scale, ">");
                scaledDebugText(renderer, scale, stepX + 3.0f * scale, stepY + 1.0f * scale, ">");
                stepX += separatorW;
            }
            if (stepX + stepW > stepRight) {
                break;
            }
            drawCommandInputChips(
                renderer,
                stepX,
                stepY - 2.0f * scale,
                stepRight - stepX,
                step.label,
                stepOptions);
            stepX += stepW + stepGap;
            ++stepsDrawn;
        }
        if (stepsDrawn == 0) {
            drawCommandInputChips(
                renderer,
                command.x + 6.0f * scale,
                stepY - 2.0f * scale,
                stepRight - command.x - 6.0f * scale,
                view.currentMoveInput,
                commandInputOptions(scale, CommandInputChipTone::Current, view, objectiveIconScale));
        }

        if (layout.inputVisible) {
            const TrainingHudRect& input = layout.input;
            const float inputX = input.x + 8.0f * scale;
            const float inputY = input.y + 8.0f * scale;
            const float inputW = input.w - 12.0f * scale;
            setColor(renderer, 102, 210, 246, 224);
            scaledDebugText(renderer, scale, inputX, inputY, "INPUT HISTORY");
            const std::string actualInput = !view.input.recentInputs.empty()
                ? view.input.recentInputs
                : view.input.currentInput;
            drawInputHistoryValue(
                renderer,
                scale,
                inputX,
                inputY + 15.0f * scale,
                inputW,
                actualInput,
                view);
            setColor(renderer, 176, 182, 190, 96);
            fillScaledRect(renderer, scale, inputX, inputY + 31.0f * scale, inputW, 1.0f * scale);
            setColor(renderer, 244, 212, 102, 235);
            scaledDebugText(renderer, scale, inputX, inputY + 38.0f * scale, "EXPECTED");
            drawCommandInputChips(
                renderer,
                inputX,
                inputY + 49.0f * scale,
                inputW,
                view.input.expectedInput.empty() ? "-" : view.input.expectedInput,
                commandInputOptions(scale, CommandInputChipTone::Current, view));
        }
        if (layout.guideVisible) {
            drawTrainingGuideDock(
                renderer,
                scale,
                layout.guide.x,
                layout.guide.y,
                view.directionGuide,
                view.buttonGuide,
                view.commandIcons,
                flashOn);
        }
        return;
    }

    if (view.input.visible) {
        const float panelW = std::min(190.0f * scale, safe.w - 18.0f * scale);
        const float panelX = safe.x + safe.w - panelW - 8.0f * scale;
        const float panelY = safe.y + 42.0f * scale;

        setColor(renderer, 5, 7, 12, 158);
        fillScaledRect(renderer, scale, panelX, panelY, panelW, 50.0f * scale);
        setColor(renderer, 32, 152, 214, 176);
        fillScaledRect(renderer, scale, panelX, panelY, panelW, 1.0f * scale);
        fillScaledRect(renderer, scale, panelX, panelY, 1.0f * scale, 50.0f * scale);
        setColor(renderer, 102, 210, 246, 224);
        scaledDebugText(renderer, scale, panelX + 7.0f * scale, panelY + 5.0f * scale, "INPUT HISTORY");

        float y = panelY + 23.0f * scale;
        setColor(renderer, 18, 24, 34, 230);
        fillScaledRect(renderer, scale, panelX + 8.0f * scale, y - 3.0f * scale, panelW - 16.0f * scale, 11.0f * scale);
        drawInputHistoryValue(
            renderer,
            scale,
            panelX + 12.0f * scale,
            y - 2.0f * scale,
            panelW - 24.0f * scale,
            view.input.currentInput,
            view);
        y += 12.0f * scale;

        drawCommandInputChips(
            renderer,
            panelX + 8.0f * scale,
            y - 2.0f * scale,
            panelW - 16.0f * scale,
            view.input.recentInputs,
            liveInputOptions(scale, CommandInputChipTone::Pending, view));
    }
}

void drawTrainingPauseHelpOverlay(const UiRenderContext& ui, const TrainingPauseHelpView& view) {
    if (!view.visible) {
        return;
    }

    SDL_Renderer* renderer = ui.renderer;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const SDL_FRect safe = dragonPixelUiSafeArea(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight });
    const float scale = metrics.pixelScale;
    const float designWidth = safe.w / std::max(0.01f, scale);
    const float panelDesignW = clampUi(designWidth - 44.0f, 238.0f, 292.0f);
    const float panelW = std::min(panelDesignW * scale, safe.w - 16.0f * scale);
    const float panelH = 121.0f * scale;
    const float x = safe.x + std::max(8.0f * scale, (safe.w - panelW) * 0.5f);
    const float y = safe.y + 61.0f * scale;
    const TrainingHudRect panel{ x, y, panelW, panelH };

    setColor(renderer, 4, 7, 12, 210);
    fillScaledRect(renderer, scale, panel.x, panel.y, panel.w, panel.h);
    setColor(renderer, 20, 30, 48, 224);
    fillScaledRect(renderer, scale, panel.x + 1.0f * scale, panel.y + 1.0f * scale, panel.w - 2.0f * scale, 18.0f * scale);
    setColor(renderer, 66, 202, 246, 198);
    fillScaledRect(renderer, scale, panel.x, panel.y, panel.w, std::max(1.0f, scale));
    setColor(renderer, 224, 190, 82, 190);
    fillScaledRect(renderer, scale, panel.x + 2.0f * scale, panel.y + 19.0f * scale, panel.w - 4.0f * scale, std::max(1.0f, scale));
    drawCornerAccents(renderer, scale, panel, 172);

    setColor(renderer, 230, 220, 172, 240);
    scaledDebugText(renderer, scale, x + 10.0f * scale, y + 7.0f * scale, "PAUSED");
    setColor(renderer, 166, 184, 210, 230);
    scaledDebugText(renderer, scale, x + 10.0f * scale, y + 25.0f * scale, "START:RESUME");
    scaledDebugText(renderer, scale, x + 122.0f * scale, y + 25.0f * scale, "SEL:OPTIONS");
    scaledDebugText(renderer, scale, x + 10.0f * scale, y + 36.0f * scale, "H/L3/R3:SHOW");
    scaledDebugText(renderer, scale, x + 10.0f * scale, y + 47.0f * scale, "PGUP/DN LB/RB:NEXT");

    setColor(renderer, 224, 190, 82, 164);
    fillScaledRect(renderer, scale, x + 10.0f * scale, y + 61.0f * scale, panelW - 20.0f * scale, std::max(1.0f, scale));
    drawPauseLegendRow(renderer, scale, x + 10.0f * scale, y + 69.0f * scale, "WAITING", "WAITING FOR INPUT", 102, 210, 246);
    drawPauseLegendRow(renderer, scale, x + 10.0f * scale, y + 78.0f * scale, "NOW", "PRESS HIGHLIGHTED", 246, 218, 82);
    drawPauseLegendRow(renderer, scale, x + 10.0f * scale, y + 87.0f * scale, "GOOD", "CORRECT", 104, 244, 172);
    drawPauseLegendRow(renderer, scale, x + 10.0f * scale, y + 96.0f * scale, "MISS", "WRONG INPUT/ORDER", 246, 112, 72);
    drawPauseLegendRow(renderer, scale, x + 10.0f * scale, y + 105.0f * scale, "INCOMPLETE", "SEQUENCE NOT DONE", 248, 170, 58);
}

TrainingCommandHudGeometryReport verifyTrainingCommandHudGeometry(
    const TrainingCommandHudView& view,
    int logicalWidth,
    int logicalHeight) {
    const float widthF = static_cast<float>(logicalWidth);
    const float heightF = static_cast<float>(logicalHeight);
    const CanvasDimensions dimensions{ logicalWidth, logicalHeight };
    const DragonUiMetrics metrics = dragonUiMetricsForCanvas(dimensions, 1.0f);
    const SDL_FRect safe = dragonPixelUiSafeArea(dimensions);
    const float scale = metrics.pixelScale;
    const TrainingCommandHudLayout layout = trainingCommandHudLayout(
        view,
        safe.w / std::max(0.01f, scale),
        safe.h / std::max(0.01f, scale),
        scale,
        safe.x,
        safe.y);
    TrainingCommandHudGeometryReport report;
    report.objectiveVisible = layout.objectiveVisible;
    report.inputVisible = layout.inputVisible;
    report.controllerVisible = layout.guideVisible;
    report.bottomLegendVisible = false;
    report.commandIconsVisible = layout.commandIconsVisible;

    auto inside = [widthF, heightF](const TrainingHudRect& rect) {
        return rect.x >= 0.0f
            && rect.y >= 0.0f
            && rect.x + rect.w <= widthF + 0.5f
            && rect.y + rect.h <= heightF + 0.5f;
    };

    if (layout.objectiveVisible && !inside(layout.objective)) {
        report.detail = "objective outside frame";
        return report;
    }
    if (layout.inputVisible && !inside(layout.input)) {
        report.detail = "input outside frame";
        return report;
    }
    if (layout.guideVisible && !inside(layout.guide)) {
        report.detail = "controller guide outside frame";
        return report;
    }
    if (layout.inputVisible && layout.guideVisible && rectIntersects(layout.input, layout.guide)) {
        report.detail = "input overlaps controller guide";
        return report;
    }
    if (view.commandsVisible && !layout.commandIconsVisible) {
        report.detail = "command icon lane too narrow";
        return report;
    }

    report.ok = true;
    report.detail = "width=" + std::to_string(logicalWidth)
        + " objective=" + std::to_string(report.objectiveVisible ? 1 : 0)
        + " input=" + std::to_string(report.inputVisible ? 1 : 0)
        + " controller=" + std::to_string(report.controllerVisible ? 1 : 0)
        + " bottom_legend=0 icons=" + std::to_string(report.commandIconsVisible ? 1 : 0);
    return report;
}

TrainingPauseHelpGeometryReport verifyTrainingPauseHelpGeometry(
    const TrainingPauseHelpView& view,
    int logicalWidth,
    int logicalHeight) {
    TrainingPauseHelpGeometryReport report;
    report.visible = view.visible;
    report.legendVisible = view.visible;
    if (!view.visible) {
        report.ok = true;
        report.detail = "hidden";
        return report;
    }

    const float widthF = static_cast<float>(logicalWidth);
    const float heightF = static_cast<float>(logicalHeight);
    const DragonUiMetrics metrics = dragonUiMetricsForCanvas(CanvasDimensions{ logicalWidth, logicalHeight }, 1.0f);
    const SDL_FRect safe = dragonPixelUiSafeArea(CanvasDimensions{ logicalWidth, logicalHeight });
    const float scale = metrics.pixelScale;
    const float designWidth = safe.w / std::max(0.01f, scale);
    const float panelDesignW = clampUi(designWidth - 44.0f, 238.0f, 292.0f);
    const float panelW = std::min(panelDesignW * scale, safe.w - 16.0f * scale);
    const float panelH = 121.0f * scale;
    const float x = safe.x + std::max(8.0f * scale, (safe.w - panelW) * 0.5f);
    const float y = safe.y + 61.0f * scale;
    const bool fits = x >= 0.0f
        && y >= 0.0f
        && x + panelW <= widthF + 0.5f
        && y + panelH <= heightF + 0.5f
        && panelW >= 238.0f * scale;
    report.ok = fits;
    report.detail = "width=" + std::to_string(logicalWidth)
        + " panelW=" + std::to_string(static_cast<int>(panelW))
        + " legend=" + std::to_string(report.legendVisible ? 1 : 0);
    if (!fits) {
        report.detail += " overflow";
    }
    return report;
}

} // namespace dragon
