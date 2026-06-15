#include "TrainingCommandOverlay.h"

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
    const TrainingCommandHudView& view) {
    CommandInputRenderOptions options;
    options.scale = scale;
    options.tone = tone;
    options.iconAtlas = view.commandIcons;
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
    return options;
}

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size()) * 8.0f;
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

    const float sweepW = 34.0f;
    const float sweepX = x - sweepW + (w + sweepW * 1.6f) * std::clamp(progress, 0.0f, 1.0f);
    setColor(renderer, 255, 236, 142, 80);
    fillScaledRect(renderer, scale, sweepX, y, sweepW, h);
    setColor(renderer, 136, 255, 192, 112);
    fillScaledRect(renderer, scale, sweepX + sweepW * 0.35f, y, sweepW * 0.32f, h);
    setColor(renderer, 238, 255, 246, 120);
    fillScaledRect(renderer, scale, sweepX + sweepW * 0.50f, y + 1.0f, 2.0f, h - 2.0f);
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
    const float glowRadius = 12.0f * pulse;
    setColor(renderer, 34, 245, 146, flashOn ? 94 : 54);
    fillScaledCircle(renderer, scale, centerX, centerY, glowRadius);
    setColor(renderer, 255, 224, 132, flashOn ? 120 : 72);
    fillScaledCircle(renderer, scale, centerX, centerY, glowRadius * 0.72f);

    const float size = 18.0f * pulse;
    if (hasTexture(check)) {
        drawScaledUiSprite(renderer, scale, check, centerX - size * 0.5f, centerY - size * 0.5f, size, size);
        return;
    }

    setColor(renderer, 130, 255, 190, 255);
    fillScaledRect(renderer, scale, centerX - 6.0f, centerY + 1.0f, 4.0f, 4.0f);
    fillScaledRect(renderer, scale, centerX - 2.0f, centerY + 3.0f, 4.0f, 4.0f);
    fillScaledRect(renderer, scale, centerX + 2.0f, centerY - 1.0f, 4.0f, 4.0f);
    fillScaledRect(renderer, scale, centerX + 6.0f, centerY - 5.0f, 4.0f, 4.0f);
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
    return drawCommandInputIconGlyph(renderer, centerX - w * 0.5f, centerY - h * 0.5f, w, h, token, options);
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
    setGuideButtonColors(renderer, button, true, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, radius);
    setGuideButtonColors(renderer, button, false, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, std::max(1.0f, radius - 1.0f));

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
    scaledDebugText(renderer, scale, centerX - debugTextWidth(label) * 0.5f, centerY - 3.0f, label);
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
        fillScaledRect(renderer, scale, centerX - 4.0f, centerY - 4.0f, 9.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 3.0f, centerY - 3.0f, 7.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 2.0f, centerY - 2.0f, 5.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 1.0f, centerY - 1.0f, 3.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX, centerY, 1.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 1.0f, centerY + 1.0f, 3.0f, 4.0f);
        return;
    }
    if (label == "v") {
        fillScaledRect(renderer, scale, centerX - 1.0f, centerY - 5.0f, 3.0f, 4.0f);
        fillScaledRect(renderer, scale, centerX, centerY, 1.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 1.0f, centerY + 1.0f, 3.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 2.0f, centerY + 2.0f, 5.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 3.0f, centerY + 3.0f, 7.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 4.0f, centerY + 4.0f, 9.0f, 1.0f);
        return;
    }
    if (label == "<") {
        fillScaledRect(renderer, scale, centerX - 4.0f, centerY, 1.0f, 1.0f);
        fillScaledRect(renderer, scale, centerX - 3.0f, centerY - 1.0f, 1.0f, 3.0f);
        fillScaledRect(renderer, scale, centerX - 2.0f, centerY - 2.0f, 1.0f, 5.0f);
        fillScaledRect(renderer, scale, centerX - 1.0f, centerY - 3.0f, 1.0f, 7.0f);
        fillScaledRect(renderer, scale, centerX, centerY - 4.0f, 1.0f, 9.0f);
        fillScaledRect(renderer, scale, centerX + 1.0f, centerY - 1.0f, 4.0f, 3.0f);
        return;
    }
    if (label == ">") {
        fillScaledRect(renderer, scale, centerX - 5.0f, centerY - 1.0f, 4.0f, 3.0f);
        fillScaledRect(renderer, scale, centerX, centerY - 4.0f, 1.0f, 9.0f);
        fillScaledRect(renderer, scale, centerX + 1.0f, centerY - 3.0f, 1.0f, 7.0f);
        fillScaledRect(renderer, scale, centerX + 2.0f, centerY - 2.0f, 1.0f, 5.0f);
        fillScaledRect(renderer, scale, centerX + 3.0f, centerY - 1.0f, 1.0f, 3.0f);
        fillScaledRect(renderer, scale, centerX + 4.0f, centerY, 1.0f, 1.0f);
        return;
    }

    scaledDebugText(renderer, scale, centerX - debugTextWidth(label) * 0.5f, centerY - 3.0f, label);
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
    constexpr float radius = 5.6f;
    setGuideButtonColors(renderer, button, true, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, radius);
    setGuideButtonColors(renderer, button, false, flash);
    fillScaledCircle(renderer, scale, centerX, centerY, radius - 1.0f);

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
        std::pair<float, float>{ x + 9.0f, y + 22.0f },
        std::pair<float, float>{ x + 19.0f, y + 12.0f },
        std::pair<float, float>{ x + 19.0f, y + 32.0f },
        std::pair<float, float>{ x + 29.0f, y + 22.0f },
    };
    for (std::size_t i = 0; i < centers.size(); ++i) {
        if constexpr (std::is_same_v<ButtonView, TrainingCommandDirectionGuideButtonView>) {
            drawDirectionGuideButton(renderer, scale, centers[i].first, centers[i].second, buttons[i], commandIcons, flash);
        } else {
            drawGuideButton(renderer, scale, centers[i].first, centers[i].second, buttons[i], commandIcons, flash);
        }
    }
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

    constexpr float dockW = 86.0f;
    constexpr float dockH = 44.0f;
    setColor(renderer, 5, 8, 14, 118);
    fillScaledRect(renderer, scale, x, y, dockW, dockH);
    setColor(renderer, flash ? 96 : 48, flash ? 170 : 62, flash ? 132 : 88, flash ? 190 : 132);
    drawScaledRect(renderer, scale, x, y, dockW, dockH);
    setColor(renderer, 42, 58, 82, 92);
    fillScaledRect(renderer, scale, x + 42.0f, y + 6.0f, 1.0f, dockH - 12.0f);

    if (directionGuide.visible) {
        drawGuideCluster(renderer, scale, x + 2.0f, y + 2.0f, directionGuide.directions, commandIcons, flash);
    }
    if (buttonGuide.visible) {
        drawGuideCluster(renderer, scale, x + 45.0f, y + 2.0f, buttonGuide.buttons, commandIcons, flash);
    }
}

} // namespace

void drawTrainingCommandOverlay(const UiRenderContext& ui, const TrainingCommandHudView& view) {
    if (!view.input.visible && !view.commandsVisible) {
        return;
    }

    SDL_Renderer* renderer = ui.renderer;
    const float scale = ui.scale;
    const float widthF = static_cast<float>(ui.logicalWidth);

    if (view.commandsVisible) {
        const float promptX = 34.0f;
        const float promptY = 142.0f;
        const bool showAnyGuide = view.buttonGuide.visible || view.directionGuide.visible;
        const float guidePanelW = showAnyGuide ? 90.0f : 0.0f;
        const float promptW = std::clamp(widthF - promptX - 14.0f - guidePanelW, 164.0f, 226.0f);
        const float guideX = promptX + promptW + 4.0f;
        const bool guideFits = showAnyGuide && guideX + guidePanelW <= widthF - 8.0f;
        const bool flashOn = view.completeFlash && ((SDL_GetTicks() / 120) % 2 == 0);
        const float completeProgress = completionProgress(view);

        setColor(renderer, 5, 7, 12, 206);
        fillScaledRect(renderer, scale, promptX - 8.0f, promptY - 8.0f, promptW + 4.0f, 60.0f);
        setColor(renderer, view.completeFlash ? 76 : 54, view.completeFlash ? 152 : 70, view.completeFlash ? 118 : 98, 220);
        drawScaledRect(renderer, scale, promptX - 8.0f, promptY - 8.0f, promptW + 4.0f, 60.0f);

        if (view.completeFlash) {
            setColor(renderer, flashOn ? 96 : 44, flashOn ? 220 : 156, flashOn ? 160 : 116, flashOn ? 188 : 128);
            fillScaledRect(renderer, scale, promptX - 5.0f, promptY - 5.0f, promptW - 2.0f, 17.0f);
            setColor(renderer, 8, 12, 16);
        } else if (view.demoActive) {
            setColor(renderer, 96, 134, 214, 132);
            fillScaledRect(renderer, scale, promptX - 5.0f, promptY - 5.0f, promptW - 2.0f, 17.0f);
            setColor(renderer, 236, 240, 246);
        } else {
            setColor(renderer, 24, 32, 48, 220);
            fillScaledRect(renderer, scale, promptX - 5.0f, promptY - 5.0f, promptW - 2.0f, 17.0f);
            setColor(renderer, 222, 226, 232);
        }
        scaledDebugText(renderer, scale, promptX, promptY, view.currentMoveName);
        setColor(renderer, 230, 190, 105, 180);
        fillScaledRect(renderer, scale, promptX - 5.0f, promptY + 12.0f, promptW - 2.0f, 1.0f);
        if (view.completionVisible) {
            drawCompletionCheckBadge(
                renderer,
                scale,
                view.completionCheck,
                promptX + promptW - 12.0f,
                promptY + 5.0f,
                completeProgress,
                flashOn);
        }

        float stepX = promptX;
        const float stepY = promptY + 17.0f;
        const float stepRight = promptX + promptW - 12.0f;
        int stepsDrawn = 0;
        if (view.completionVisible) {
            drawCompletionSweep(
                renderer,
                scale,
                promptX - 4.0f,
                stepY - 4.0f,
                promptW - 10.0f,
                15.0f,
                completeProgress,
                flashOn);
        }
        for (const auto& step : view.practiceSteps) {
            if (stepsDrawn >= 8) {
                break;
            }
            const auto stepOptions = commandInputOptions(scale, commandStepTone(step.status), view);
            const float stepW = std::max(10.0f, commandInputWidth(step.label, stepOptions));
            if (stepX + stepW > stepRight) {
                break;
            }
            drawCommandInputChips(
                renderer,
                stepX,
                stepY - 2.0f,
                stepRight - stepX,
                step.label,
                stepOptions);
            stepX += stepW + 6.0f;
            ++stepsDrawn;
        }
        if (stepsDrawn == 0) {
            drawCommandInputChips(
                renderer,
                promptX,
                stepY - 2.0f,
                promptW - 18.0f,
                view.currentMoveInput,
                commandInputOptions(scale, CommandInputChipTone::Current, view));
        }

        if (view.input.visible) {
            setColor(renderer, 130, 142, 156);
            scaledDebugText(renderer, scale, promptX, promptY + 34.0f, "NOW");
            setColor(renderer, 18, 24, 34, 220);
            fillScaledRect(renderer, scale, promptX + 27.0f, promptY + 31.0f, promptW - 38.0f, 11.0f);
            drawCommandInputChips(
                renderer,
                promptX + 30.0f,
                promptY + 32.0f,
                promptW - 44.0f,
                view.input.currentInput,
                liveInputOptions(scale, CommandInputChipTone::Normal, view));
        }

        if (view.completionVisible) {
            drawCompletionCheckBadge(
                renderer,
                scale,
                view.completionCheck,
                promptX + 8.0f,
                promptY + 50.0f,
                completeProgress,
                flashOn);
            setColor(renderer, 130, 142, 156);
            scaledDebugText(renderer, scale, promptX + 25.0f, promptY + 47.0f, view.nextMoveLabel);
        } else {
            setColor(renderer, view.demoActive ? 116 : 230, view.demoActive ? 190 : 220, view.demoActive ? 154 : 172);
            scaledDebugText(renderer, scale, promptX, promptY + 47.0f, view.demoActive ? "CPU DEMO" : view.showMeLabel);
            setColor(renderer, 130, 142, 156);
            scaledDebugText(renderer, scale, promptX + 96.0f, promptY + 47.0f, view.nextMoveLabel);
        }
        if (guideFits) {
            drawTrainingGuideDock(
                renderer,
                scale,
                guideX,
                promptY + 4.0f,
                view.directionGuide,
                view.buttonGuide,
                view.commandIcons,
                flashOn);
        }
        return;
    }

    if (view.input.visible) {
        const float panelW = std::min(178.0f, widthF - 16.0f);
        const float panelX = widthF - panelW - 8.0f;
        const float panelY = 42.0f;

        setColor(renderer, 5, 7, 12, 224);
        fillScaledRect(renderer, scale, panelX, panelY, panelW, 50.0f);
        setColor(renderer, 54, 70, 98);
        drawScaledRect(renderer, scale, panelX, panelY, panelW, 50.0f);
        setColor(renderer, 20, 30, 48, 220);
        fillScaledRect(renderer, scale, panelX + 2.0f, panelY + 2.0f, panelW - 4.0f, 14.0f);
        setColor(renderer, 158, 64, 58, 200);
        fillScaledRect(renderer, scale, panelX + 2.0f, panelY + 16.0f, panelW - 4.0f, 1.0f);
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 7.0f, panelY + 5.0f, "INPUT");

        float y = panelY + 23.0f;
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, "INPUT");
        setColor(renderer, 18, 24, 34, 230);
        fillScaledRect(renderer, scale, panelX + 54.0f, y - 3.0f, panelW - 64.0f, 11.0f);
        drawCommandInputChips(
            renderer,
            panelX + 58.0f,
            y - 2.0f,
            panelW - 70.0f,
            view.input.currentInput,
            liveInputOptions(scale, CommandInputChipTone::Normal, view));
        y += 12.0f;

        drawCommandInputChips(
            renderer,
            panelX + 8.0f,
            y - 2.0f,
            panelW - 16.0f,
            view.input.recentInputs,
            liveInputOptions(scale, CommandInputChipTone::Pending, view));
    }
}

} // namespace dragon
