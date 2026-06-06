#include "TrainingOptionsOverlay.h"

#include "UiRenderPrimitives.h"

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
        return "INVINCIBLE";
    }
    if (label == "DUMMY FREEZE") {
        return "FREEZE";
    }
    if (label == "DUMMY GUARD") {
        return "GUARD";
    }
    if (label == "GUARD DMG") {
        return "DAMAGE";
    }
    if (label == "P2 CONTROL") {
        return "P2 CTRL";
    }
    if (label == "COMMAND HUD") {
        return "COMMAND";
    }
    if (label == "INPUT HUD") {
        return "INPUT";
    }
    if (label == "MOVE TYPE") {
        return "TYPE";
    }
    if (label == "MOVE LIST") {
        return "LIST";
    }
    if (label == "RESET POS") {
        return "RESET";
    }
    return label;
}

std::string optionsMenuStatus(const std::string& status) {
    if (status == "NORMAL") {
        return "NORM";
    }
    if (status == "SPECIAL") {
        return "SPEC";
    }
    if (status == "STAND") {
        return "STND";
    }
    if (status == "CROUCH") {
        return "CRCH";
    }
    return status;
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

    constexpr float panelX = 16.0f;
    constexpr float panelY = 28.0f;
    constexpr float panelW = 292.0f;
    constexpr float panelH = 198.0f;
    drawSoftPanel(renderer, panelX, panelY, panelW, panelH);

    setColor(renderer, 230, 220, 172);
    debugText(renderer, panelX + 10.0f, panelY + 8.0f, "TRAINING OPTIONS");
    setColor(renderer, 126, 164, 214);
    debugText(renderer, panelX + 254.0f, panelY + 8.0f, "F2");

    constexpr int rowsPerColumn = 10;
    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const int column = i / rowsPerColumn;
        const int row = i % rowsPerColumn;
        const float x = panelX + (column == 0 ? 12.0f : 140.0f);
        const float statusX = column == 0 ? panelX + 100.0f : panelX + 204.0f;
        const float y = panelY + 34.0f + static_cast<float>(row * 13);
        const auto& option = view.rows[static_cast<std::size_t>(i)];
        if (option.selected) {
            setColor(renderer, 72, 164, 134, 238);
            fillRect(renderer, x - 6.0f, y - 3.0f, column == 0 ? 116.0f : 140.0f, 12.0f);
            setColor(renderer, 8, 12, 18);
        } else {
            setColor(renderer, 186, 196, 208);
        }
        debugText(renderer, x, y, optionsMenuLabel(option.label));
        setColor(renderer, option.selected ? 8 : 162, option.selected ? 12 : 188, option.selected ? 18 : 222);
        debugText(renderer, statusX, y, fitted(optionsMenuStatus(option.status), 5));
    }

    setColor(renderer, 32, 42, 58, 230);
    fillRect(renderer, panelX + 1.0f, panelY + panelH - 18.0f, panelW - 2.0f, 17.0f);
    setColor(renderer, 142, 154, 168);
    compactText(renderer, panelX + 10.0f, panelY + panelH - 12.0f, "ENTER toggle   ARROWS select   ESC/F2 close", 0.68f);
}

} // namespace dragon
