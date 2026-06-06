#include "TrainingDebugOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace dragon {
namespace {

void setDebugColor(SDL_Renderer* renderer, TrainingDebugColorView color) {
    setColor(renderer, color.r, color.g, color.b, color.a);
}

void drawDebugLine(SDL_Renderer* renderer, const TrainingDebugLineView& line) {
    setDebugColor(renderer, line.color);
    if (std::abs(line.y2 - line.y1) < 0.001f) {
        fillRect(renderer, std::min(line.x1, line.x2), line.y1, std::max(1.0f, std::abs(line.x2 - line.x1)), 1.0f);
        return;
    }
    if (std::abs(line.x2 - line.x1) < 0.001f) {
        fillRect(renderer, line.x1, std::min(line.y1, line.y2), 1.0f, std::max(1.0f, std::abs(line.y2 - line.y1)));
        return;
    }
    SDL_RenderLine(renderer, line.x1, line.y1, line.x2, line.y2);
}

void drawDebugRect(SDL_Renderer* renderer, const TrainingDebugRectView& box) {
    setDebugColor(renderer, box.color);
    drawRect(renderer, box.x, box.y, box.w, box.h);
    if (box.doubleOutline && box.w > 3.0f && box.h > 3.0f) {
        drawRect(renderer, box.x + 1.0f, box.y + 1.0f, box.w - 2.0f, box.h - 2.0f);
    }
}

void drawDebugReadout(SDL_Renderer* renderer, const TrainingDebugReadoutView& readout) {
    if (!readout.visible) {
        return;
    }

    setColor(renderer, 8, 10, 12);
    fillRect(renderer, 4, 42, 250, 130);
    setColor(renderer, 78, 90, 112);
    drawRect(renderer, 4, 42, 250, 130);

    for (std::size_t i = 0; i < readout.lines.size(); ++i) {
        if (i == 0) {
            setColor(renderer, 210, 224, 238);
        } else {
            setColor(renderer, 155, 164, 174);
        }
        debugText(renderer, 8.0f, 46.0f + static_cast<float>(i) * 12.0f, readout.lines[i]);
    }
}

} // namespace

void drawTrainingDebugOverlay(const UiRenderContext& ui, const TrainingDebugView& view) {
    SDL_Renderer* renderer = ui.renderer;
    for (const auto& line : view.lines) {
        drawDebugLine(renderer, line);
    }
    for (const auto& box : view.boxes) {
        drawDebugRect(renderer, box);
    }
    drawDebugReadout(renderer, view.readout);
}

} // namespace dragon
