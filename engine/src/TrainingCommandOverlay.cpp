#include "TrainingCommandOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace dragon {

namespace {

void drawCommandStepText(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    const TrainingCommandStepView& step) {
    const float chipW = std::max(12.0f, static_cast<float>(step.label.size()) * 6.0f + 6.0f);
    switch (step.status) {
    case TrainingCommandStepStatus::Matched:
        setColor(renderer, 28, 86, 68, 210);
        fillScaledRect(renderer, scale, x - 3.0f, y - 3.0f, chipW, 10.0f);
        setColor(renderer, 116, 190, 154);
        break;
    case TrainingCommandStepStatus::Current:
        setColor(renderer, 92, 68, 28, 222);
        fillScaledRect(renderer, scale, x - 3.0f, y - 3.0f, chipW, 10.0f);
        setColor(renderer, 238, 213, 130);
        break;
    case TrainingCommandStepStatus::Pending:
    default:
        setColor(renderer, 18, 24, 34, 210);
        fillScaledRect(renderer, scale, x - 3.0f, y - 3.0f, chipW, 10.0f);
        setColor(renderer, 174, 184, 196);
        break;
    }
    scaledDebugText(renderer, scale, x, y, step.label);
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
        const float promptW = std::min(178.0f, widthF - promptX - 10.0f);

        setColor(renderer, 5, 7, 12, 206);
        fillScaledRect(renderer, scale, promptX - 8.0f, promptY - 8.0f, promptW + 4.0f, 60.0f);
        setColor(renderer, 54, 70, 98, 220);
        drawScaledRect(renderer, scale, promptX - 8.0f, promptY - 8.0f, promptW + 4.0f, 60.0f);

        if (view.completeFlash) {
            setColor(renderer, 74, 170, 134, 148);
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

        float stepX = promptX;
        const float stepY = promptY + 17.0f;
        const float stepRight = promptX + promptW - 12.0f;
        int stepsDrawn = 0;
        for (const auto& step : view.practiceSteps) {
            if (stepsDrawn >= 8) {
                break;
            }
            const float stepW = std::max(10.0f, static_cast<float>(step.label.size()) * 6.0f);
            if (stepX + stepW > stepRight) {
                break;
            }
            drawCommandStepText(renderer, scale, stepX, stepY, step);
            stepX += stepW + 8.0f;
            ++stepsDrawn;
        }
        if (stepsDrawn == 0) {
            setColor(renderer, 230, 190, 105);
            scaledDebugText(renderer, scale, promptX, stepY, view.currentMoveInput);
        }

        if (view.input.visible) {
            setColor(renderer, 130, 142, 156);
            scaledDebugText(renderer, scale, promptX, promptY + 34.0f, "NOW");
            setColor(renderer, 18, 24, 34, 220);
            fillScaledRect(renderer, scale, promptX + 27.0f, promptY + 31.0f, promptW - 38.0f, 11.0f);
            setColor(renderer, 222, 226, 232);
            scaledDebugText(renderer, scale, promptX + 30.0f, promptY + 34.0f, view.input.currentInput);
        }

        setColor(renderer, view.demoActive ? 116 : 230, view.demoActive ? 190 : 220, view.demoActive ? 154 : 172);
        scaledDebugText(renderer, scale, promptX, promptY + 47.0f, view.demoActive ? "CPU DEMO" : view.showMeLabel);
        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, promptX + 86.0f, promptY + 47.0f, "F2 LIST");
        return;
    }

    if (view.input.visible) {
        const float panelW = std::min(154.0f, widthF - 16.0f);
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
        setColor(renderer, 222, 226, 232);
        scaledDebugText(renderer, scale, panelX + 58.0f, y, view.input.currentInput);
        y += 12.0f;

        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, view.input.recentInputs);
    }
}

} // namespace dragon
