#include "TrainingOptionsOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace dragon {

namespace {

constexpr float kMenuTextScale = 0.76f;
constexpr float kMoveListTextScale = 0.74f;

void compactText(SDL_Renderer* renderer, float x, float y, const std::string& text, float scale = kMenuTextScale) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, oldX * scale, oldY * scale);
    debugText(renderer, x / scale, y / scale, text);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

void compactText(SDL_Renderer* renderer, float x, float y, const char* text, float scale = kMenuTextScale) {
    compactText(renderer, x, y, std::string(text), scale);
}

std::string fitted(const std::string& value, std::size_t maxChars) {
    return fitDebugText(value, maxChars);
}

void drawSoftPanel(SDL_Renderer* renderer, float x, float y, float w, float h) {
    setColor(renderer, 8, 12, 18, 226);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 24, 34, 50, 238);
    fillRect(renderer, x + 1.0f, y + 1.0f, w - 2.0f, 17.0f);
    setColor(renderer, 66, 84, 112);
    drawRect(renderer, x, y, w, h);
}

std::string optionsMenuLabel(const std::string& label) {
    if (label == "DUMMY INV") {
        return "DUMMY INVINCIBLE";
    }
    if (label == "GUARD DMG") {
        return "GUARD DAMAGE";
    }
    return label;
}

std::string optionsMenuStatus(const std::string& status) {
    return status;
}

struct OptionsMenuLayout {
    float panelX = 10.0f;
    float panelY = 30.0f;
    float panelW = 300.0f;
    float panelH = 184.0f;
    float listX = 22.0f;
    float listY = 58.0f;
    float listW = 276.0f;
    float listH = 126.0f;
    float labelX = 34.0f;
    float statusCellX = 196.0f;
    float statusCellW = 70.0f;
    float rowH = 12.0f;
    float rowTextOffsetY = 3.0f;
};

OptionsMenuLayout optionsMenuLayout() {
    return OptionsMenuLayout{};
}

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size()) * 8.0f;
}

} // namespace

void drawTrainingMoveListPage(const UiRenderContext& ui, const TrainingMoveListView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    constexpr float panelX = 4.0f;
    constexpr float panelY = 37.0f;
    constexpr float panelW = 310.0f;
    constexpr float panelH = 171.0f;
    drawSoftPanel(renderer, panelX, panelY, panelW, panelH);

    setColor(renderer, 28, 42, 74);
    fillRect(renderer, panelX + 2.0f, panelY + 2.0f, panelW - 4.0f, 18.0f);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, panelX + 2.0f, panelY + 20.0f, panelW - 4.0f, 2.0f);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, panelX + 10.0f, panelY + 26.0f, panelW - 20.0f, 1.0f);

    setColor(renderer, 230, 220, 172);
    debugText(renderer, panelX + 10.0f, panelY + 8.0f, "COMMAND LIST");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, panelX + 256.0f, panelY + 8.0f, view.pageLabel);
    setColor(renderer, 174, 184, 196);
    compactText(renderer, panelX + 10.0f, panelY + 21.0f, view.categoryLabel, 0.68f);

    setColor(renderer, 16, 20, 28, 232);
    fillRect(renderer, panelX + 10.0f, panelY + 34.0f, 150.0f, 101.0f);
    setColor(renderer, 78, 94, 124);
    drawRect(renderer, panelX + 10.0f, panelY + 34.0f, 150.0f, 101.0f);
    setColor(renderer, 208, 168, 92);
    compactText(renderer, panelX + 16.0f, panelY + 41.0f, "MOVES", kMoveListTextScale);

    if (view.empty) {
        setColor(renderer, 184, 178, 168);
        debugText(renderer, panelX + 18.0f, panelY + 64.0f, "No loaded moves");
    } else {
        for (int row = 0; row < static_cast<int>(view.rows.size()); ++row) {
            const auto& entry = view.rows[static_cast<std::size_t>(row)];
            const float y = panelY + 55.0f + static_cast<float>(row * 11);
            if (entry.selected) {
                setColor(renderer, 74, 170, 134);
                fillRect(renderer, panelX + 16.0f, y - 3.0f, 138.0f, 10.0f);
                setColor(renderer, 8, 12, 16);
            } else {
                setColor(renderer, row % 2 == 0 ? 174 : 138, row % 2 == 0 ? 184 : 150, row % 2 == 0 ? 196 : 164);
            }
            compactText(renderer, panelX + 18.0f, y, entry.number, 0.74f);
            compactText(renderer, panelX + 38.0f, y, fitted(entry.label, 16), 0.74f);
        }
    }

    setColor(renderer, 16, 20, 28, 232);
    fillRect(renderer, panelX + 168.0f, panelY + 34.0f, 126.0f, 101.0f);
    setColor(renderer, 78, 94, 124);
    drawRect(renderer, panelX + 168.0f, panelY + 34.0f, 126.0f, 101.0f);
    setColor(renderer, 208, 168, 92);
    compactText(renderer, panelX + 176.0f, panelY + 41.0f, "SELECTED", 0.68f);

    if (view.detail.visible) {
        setColor(renderer, 222, 226, 232);
        compactText(renderer, panelX + 176.0f, panelY + 56.0f, fitted(view.detail.name, 17), 0.74f);
        setColor(renderer, 128, 171, 225);
        compactText(renderer, panelX + 176.0f, panelY + 73.0f, "STATE", 0.62f);
        setColor(renderer, 230, 190, 105);
        compactText(renderer, panelX + 222.0f, panelY + 73.0f, view.detail.state, 0.62f);
        setColor(renderer, 128, 171, 225);
        compactText(renderer, panelX + 176.0f, panelY + 87.0f, "INPUT", 0.62f);
        setColor(renderer, 222, 226, 232);
        compactText(renderer, panelX + 176.0f, panelY + 101.0f, fitted(view.detail.input, 16), 0.68f);
        setColor(renderer, 128, 171, 225);
        compactText(renderer, panelX + 176.0f, panelY + 117.0f, "TYPE", 0.62f);
        setColor(renderer, 222, 226, 232);
        compactText(renderer, panelX + 214.0f, panelY + 117.0f, view.detail.type, 0.62f);
        setColor(renderer, 116, 190, 154);
        compactText(renderer, panelX + 176.0f, panelY + 128.0f, view.detail.powerStatus, 0.62f);
    }

    setColor(renderer, 34, 26, 30, 238);
    fillRect(renderer, panelX + 10.0f, panelY + 140.0f, 284.0f, 21.0f);
    setColor(renderer, 158, 64, 58);
    drawRect(renderer, panelX + 10.0f, panelY + 140.0f, 284.0f, 21.0f);
    setColor(renderer, 230, 220, 172);
    if (view.detail.visible) {
        compactText(renderer, panelX + 18.0f, panelY + 147.0f, "INPUT", 0.72f);
        setColor(renderer, 222, 226, 232);
        compactText(renderer, panelX + 58.0f, panelY + 147.0f, fitted(view.detail.performInput, 34), 0.72f);
    } else {
        compactText(renderer, panelX + 18.0f, panelY + 147.0f, "No executable commands loaded", 0.72f);
    }

    setColor(renderer, 130, 142, 156);
    compactText(renderer, panelX + 14.0f, panelY + 164.0f, "UP/DOWN choose   ENTER show   ESC back", 0.62f);
}

void drawTrainingOptionsMenu(const UiRenderContext& ui, const TrainingOptionsMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    const OptionsMenuLayout layout = optionsMenuLayout();
    drawSoftPanel(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH);

    setColor(renderer, 28, 42, 74);
    fillRect(renderer, layout.panelX + 2.0f, layout.panelY + 2.0f, layout.panelW - 4.0f, 18.0f);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, layout.panelX + 2.0f, layout.panelY + 20.0f, layout.panelW - 4.0f, 2.0f);
    setColor(renderer, 230, 220, 172);
    debugText(renderer, layout.panelX + 10.0f, layout.panelY + 8.0f, "TRAINING OPTIONS");
    setColor(renderer, 126, 164, 214);
    debugText(renderer, layout.panelX + layout.panelW - 10.0f - debugTextWidth(view.pageLabel), layout.panelY + 8.0f, view.pageLabel);
    setColor(renderer, 142, 154, 168);
    debugText(renderer, layout.labelX, layout.listY - 9.0f, "SETTING");
    debugText(renderer, layout.statusCellX + (layout.statusCellW - debugTextWidth("VALUE")) * 0.5f, layout.listY - 9.0f, "VALUE");

    setColor(renderer, 12, 17, 26, 210);
    fillRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);
    setColor(renderer, 58, 72, 96);
    drawRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);

    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& option = view.rows[static_cast<std::size_t>(i)];
        const std::string label = optionsMenuLabel(option.label);
        const std::string status = optionsMenuStatus(option.status);
        const float rowTop = layout.listY + 4.0f + static_cast<float>(i) * layout.rowH;
        const float textY = rowTop + layout.rowTextOffsetY;
        if (option.selected) {
            setColor(renderer, 74, 170, 134, 230);
            fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, layout.rowH - 1.0f);
            setColor(renderer, 230, 220, 172, 220);
            fillRect(renderer, layout.listX + 4.0f, rowTop + layout.rowH - 1.0f, layout.listW - 8.0f, 1.0f);
            setColor(renderer, 8, 12, 18);
        } else {
            setColor(renderer, i % 2 == 0 ? 16 : 10, i % 2 == 0 ? 22 : 16, i % 2 == 0 ? 32 : 24, 170);
            fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, layout.rowH - 1.0f);
            setColor(renderer, 186, 196, 208);
        }
        debugText(renderer, layout.labelX, textY, label);

        setColor(renderer, option.selected ? 230 : 18, option.selected ? 220 : 24, option.selected ? 172 : 34, option.selected ? 238 : 232);
        fillRect(renderer, layout.statusCellX, rowTop + 1.0f, layout.statusCellW, layout.rowH - 3.0f);
        setColor(renderer, option.selected ? 255 : 82, option.selected ? 238 : 96, option.selected ? 160 : 124, 210);
        drawRect(renderer, layout.statusCellX, rowTop + 1.0f, layout.statusCellW, layout.rowH - 3.0f);
        setColor(renderer, option.selected ? 8 : 214, option.selected ? 12 : 224, option.selected ? 16 : 234);
        const float statusX = layout.statusCellX + std::max(2.0f, (layout.statusCellW - debugTextWidth(status)) * 0.5f);
        debugText(renderer, statusX, textY, status);
    }

    setColor(renderer, 32, 42, 58, 230);
    fillRect(renderer, layout.panelX + 1.0f, layout.panelY + layout.panelH - 20.0f, layout.panelW - 2.0f, 19.0f);
    setColor(renderer, 142, 154, 168);
    debugText(renderer, layout.panelX + 20.0f, layout.panelY + layout.panelH - 14.0f, "UP/DN SEL  ENT TOG  ESC/F2");
}

TrainingOptionsMenuGeometryReport verifyTrainingOptionsMenuGeometry(const TrainingOptionsMenuView& view) {
    const OptionsMenuLayout layout = optionsMenuLayout();
    if (view.rows.empty()) {
        return { false, "no rows" };
    }
    if (view.rows.size() > 10) {
        return { false, "too many rows: " + std::to_string(view.rows.size()) };
    }
    if (layout.panelX < 0.0f || layout.panelY < 0.0f
        || layout.panelX + layout.panelW > 320.0f
        || layout.panelY + layout.panelH > 240.0f) {
        return { false, "panel outside 320x240 virtual frame" };
    }
    if (layout.panelX + 20.0f + debugTextWidth("UP/DN SEL  ENT TOG  ESC/F2") > layout.panelX + layout.panelW - 8.0f) {
        return { false, "footer outside panel" };
    }

    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& option = view.rows[static_cast<std::size_t>(i)];
        const std::string label = optionsMenuLabel(option.label);
        const std::string status = optionsMenuStatus(option.status);
        const float rowTop = layout.listY + 4.0f + static_cast<float>(i) * layout.rowH;
        const float rowBottom = rowTop + layout.rowH;
        if (rowTop < layout.listY || rowBottom > layout.listY + layout.listH) {
            return { false, "row outside list panel: " + std::to_string(i) };
        }

        const float statusTextX = layout.statusCellX + std::max(2.0f, (layout.statusCellW - debugTextWidth(status)) * 0.5f);
        const float labelRight = layout.labelX + debugTextWidth(label);
        if (layout.statusCellX + layout.statusCellW > layout.listX + layout.listW - 4.0f) {
            return { false, "status cell outside list panel" };
        }
        if (layout.statusCellX < layout.labelX) {
            return { false, "status overlaps label column: " + option.status };
        }
        if (labelRight + 8.0f > layout.statusCellX) {
            return { false, "label/status collision: " + label + " / " + status };
        }
        if (statusTextX < layout.statusCellX
            || statusTextX + debugTextWidth(status) > layout.statusCellX + layout.statusCellW) {
            return { false, "status outside list panel: " + status };
        }
    }

    return { true, "rows=" + std::to_string(view.rows.size()) + " page=" + view.pageLabel };
}

} // namespace dragon
