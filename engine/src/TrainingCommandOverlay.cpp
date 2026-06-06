#include "TrainingCommandOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace dragon {

namespace {

void drawCommandStepChip(
    SDL_Renderer* renderer,
    float scale,
    float x,
    float y,
    float w,
    const TrainingCommandStepView& step) {
    switch (step.status) {
    case TrainingCommandStepStatus::Matched:
        setColor(renderer, 74, 170, 134, 238);
        break;
    case TrainingCommandStepStatus::Current:
        setColor(renderer, 230, 190, 105, 244);
        break;
    case TrainingCommandStepStatus::Pending:
    default:
        setColor(renderer, 22, 28, 38, 232);
        break;
    }
    fillScaledRect(renderer, scale, x, y, w, 11.0f);

    setColor(renderer, 78, 94, 124);
    drawScaledRect(renderer, scale, x, y, w, 11.0f);

    if (step.status == TrainingCommandStepStatus::Pending) {
        setColor(renderer, 174, 184, 196);
    } else {
        setColor(renderer, 8, 12, 16);
    }
    scaledDebugText(renderer, scale, x + 4.0f, y + 2.0f, step.label);
}

} // namespace

void drawTrainingCommandOverlay(const UiRenderContext& ui, const TrainingCommandHudView& view) {
    if (!view.input.visible && !view.commandsVisible) {
        return;
    }

    SDL_Renderer* renderer = ui.renderer;
    const float scale = ui.scale;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float panelW = view.commandsVisible ? std::min(188.0f, widthF - 16.0f) : std::min(154.0f, widthF - 16.0f);
    const float panelX = view.commandsVisible ? 8.0f : widthF - panelW - 8.0f;
    const float panelY = view.commandsVisible ? 40.0f : 42.0f;
    const float panelH = view.commandsVisible ? 88.0f : 48.0f;

    setColor(renderer, 5, 7, 12, view.commandsVisible ? 138 : 212);
    fillScaledRect(renderer, scale, panelX, panelY, panelW, panelH);
    setColor(renderer, 54, 70, 98);
    drawScaledRect(renderer, scale, panelX, panelY, panelW, panelH);

    setColor(renderer, 20, 30, 48, 220);
    fillScaledRect(renderer, scale, panelX + 2.0f, panelY + 2.0f, panelW - 4.0f, 14.0f);
    setColor(renderer, 230, 220, 172);
    scaledDebugText(renderer, scale, panelX + 7.0f, panelY + 5.0f, view.commandsVisible ? "COMMAND TRAINING" : "INPUT");

    if (view.commandsVisible) {
        setColor(renderer, 128, 171, 225);
        scaledDebugText(renderer, scale, panelX + panelW - 66.0f, panelY + 5.0f, view.categoryLabel);
        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, panelX + panelW - 32.0f, panelY + 5.0f, view.pageLabel);

        if (view.completeFlash) {
            setColor(renderer, 74, 170, 134, 230);
            fillScaledRect(renderer, scale, panelX + 7.0f, panelY + 21.0f, panelW - 14.0f, 14.0f);
            setColor(renderer, 8, 12, 16);
        } else if (view.demoActive) {
            setColor(renderer, 96, 134, 214, 214);
            fillScaledRect(renderer, scale, panelX + 7.0f, panelY + 21.0f, panelW - 14.0f, 14.0f);
            setColor(renderer, 236, 240, 246);
        } else {
            setColor(renderer, 222, 226, 232);
        }
        scaledDebugText(renderer, scale, panelX + 10.0f, panelY + 23.0f, view.currentMoveName);

        setColor(renderer, 230, 190, 105);
        scaledDebugText(renderer, scale, panelX + 10.0f, panelY + 38.0f, view.currentMoveInput);

        float chipX = panelX + 10.0f;
        const float chipY = panelY + 52.0f;
        const float chipRight = panelX + panelW - 10.0f;
        int chipsDrawn = 0;
        for (const auto& step : view.practiceSteps) {
            if (chipsDrawn >= 6) {
                break;
            }
            const float chipW = std::clamp(12.0f + static_cast<float>(step.label.size()) * 5.5f, 24.0f, 50.0f);
            if (chipX + chipW > chipRight) {
                break;
            }
            drawCommandStepChip(renderer, scale, chipX, chipY, chipW, step);
            chipX += chipW + 4.0f;
            ++chipsDrawn;
        }

        if (view.input.visible) {
            setColor(renderer, 130, 142, 156);
            scaledDebugText(renderer, scale, panelX + 10.0f, panelY + 66.0f, "NOW");
            setColor(renderer, 222, 226, 232);
            scaledDebugText(renderer, scale, panelX + 38.0f, panelY + 66.0f, view.input.currentInput);
        }

        setColor(renderer, view.demoActive ? 116 : 230, view.demoActive ? 190 : 220, view.demoActive ? 154 : 172);
        scaledDebugText(renderer, scale, panelX + 10.0f, panelY + 78.0f, view.demoActive ? "CPU DEMO" : view.showMeLabel);
        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, panelX + panelW - 54.0f, panelY + 78.0f, "F2 LIST");
        return;
    }

    if (view.input.visible) {
        float y = panelY + 23.0f;
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, "INPUT");
        setColor(renderer, 222, 226, 232);
        scaledDebugText(renderer, scale, panelX + 58.0f, y, view.input.currentInput);
        y += 12.0f;

        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, view.input.recentInputs);
    }
}

} // namespace dragon
