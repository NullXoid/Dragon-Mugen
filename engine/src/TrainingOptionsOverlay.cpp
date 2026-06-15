#include "TrainingOptionsOverlay.h"

#include "TrainingCommandInputRenderer.h"
#include "UiMenuList.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace dragon {

namespace {

constexpr float kMenuTextScale = 0.76f;
constexpr float kMoveListTextScale = 0.74f;
constexpr float kOptionsClassicVirtualW = 320.0f;
constexpr float kOptionsDefaultVirtualW = 426.0f;
constexpr float kOptionsExtraWideVirtualW = 480.0f;
constexpr float kOptionsVirtualH = 240.0f;
constexpr float kOptionsPanelMinW = 320.0f;
constexpr float kOptionsPanelMaxW = 380.0f;
constexpr float kMoveListPanelMinW = 316.0f;
constexpr float kMoveListPanelMaxW = 462.0f;
constexpr int kMoveListVisibleRows = 10;
constexpr const char* kOptionsTitle = "TRAINING OPTIONS";
constexpr const char* kOptionsFooter = "UP/DN SEL  ENT TOG  ESC/F2";
constexpr const char* kMoveListTitle = "COMMAND LIST";
constexpr const char* kMoveListFooter = "H DEMO  ENT BACK  L/R TAB";

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
    setColor(renderer, 8, 12, 18, 214);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 23, 34, 52, 226);
    fillRect(renderer, x + 1.0f, y + 1.0f, w - 2.0f, 17.0f);
    setColor(renderer, 56, 72, 100, 226);
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

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size()) * 8.0f;
}

float debugTextWidth(const char* text) {
    return text ? debugTextWidth(std::string(text)) : 0.0f;
}

struct OptionsMenuLayout {
    float panelX = 0.0f;
    float panelY = 30.0f;
    float panelW = kOptionsPanelMinW;
    float panelH = 184.0f;
    float columnHeaderY = 0.0f;
    float listX = 0.0f;
    float listY = 58.0f;
    float listW = 0.0f;
    float listH = 126.0f;
    float footerY = 0.0f;
    float footerH = 16.0f;
    float labelX = 0.0f;
    float statusCellX = 0.0f;
    float statusCellW = 0.0f;
    float rowH = 11.0f;
    float rowTextOffsetY = 2.0f;
};

OptionsMenuLayout optionsMenuLayout(const TrainingOptionsMenuView& view, float frameW) {
    float maxLabelW = debugTextWidth("SETTING");
    float maxStatusW = debugTextWidth("VALUE");
    for (const auto& row : view.rows) {
        maxLabelW = std::max(maxLabelW, debugTextWidth(optionsMenuLabel(row.label)));
        maxStatusW = std::max(maxStatusW, debugTextWidth(optionsMenuStatus(row.status)));
    }

    OptionsMenuLayout layout;
    frameW = std::max(kOptionsClassicVirtualW, frameW);
    const float availableMaxW = std::clamp(frameW - 20.0f, 240.0f, kOptionsPanelMaxW);
    const float minW = std::min(kOptionsPanelMinW, availableMaxW);
    const float statusCellW = std::clamp(maxStatusW + 28.0f, 64.0f, 96.0f);
    const float contentW = 16.0f + 18.0f + maxLabelW + 28.0f + statusCellW + 26.0f + 16.0f;
    const float footerW = 20.0f + debugTextWidth(kOptionsFooter) + 8.0f;
    const float headerW = 10.0f + debugTextWidth(kOptionsTitle) + 16.0f + debugTextWidth(view.pageLabel) + 10.0f;
    const float desiredPanelW = std::max(std::max(contentW + 24.0f, footerW), headerW);
    layout.panelW = std::clamp(std::ceil(desiredPanelW), minW, availableMaxW);
    layout.panelX = std::floor((frameW - layout.panelW) * 0.5f);

    const int rowCount = std::max(1, static_cast<int>(view.rows.size()));
    layout.listX = layout.panelX + 16.0f;
    layout.listY = layout.panelY + 34.0f;
    layout.listW = layout.panelW - 32.0f;
    layout.listH = static_cast<float>(rowCount) * layout.rowH + 8.0f;
    layout.columnHeaderY = layout.listY - 10.0f;
    layout.footerY = layout.listY + layout.listH + 5.0f;
    layout.panelH = layout.footerY - layout.panelY + layout.footerH + 1.0f;
    if (layout.panelY + layout.panelH > kOptionsVirtualH - 4.0f) {
        layout.panelY = std::max(4.0f, kOptionsVirtualH - layout.panelH - 4.0f);
        layout.listY = layout.panelY + 34.0f;
        layout.columnHeaderY = layout.listY - 10.0f;
        layout.footerY = layout.listY + layout.listH + 5.0f;
    }

    layout.labelX = layout.listX + 18.0f;
    layout.statusCellW = statusCellW;
    layout.statusCellX = layout.listX + layout.listW - layout.statusCellW - 26.0f;
    return layout;
}

float selectedLabelPillX(const OptionsMenuLayout& layout) {
    return layout.listX + 7.0f;
}

float selectedLabelPillW(const OptionsMenuLayout& layout) {
    return std::max(24.0f, layout.statusCellX - selectedLabelPillX(layout) - 14.0f);
}

void fillPill(SDL_Renderer* renderer, float x, float y, float w, float h) {
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }
    const float radius = h * 0.5f;
    fillRect(renderer, x + radius, y, std::max(0.0f, w - radius * 2.0f), h);

    const int rowCount = std::max(1, static_cast<int>(std::ceil(h)));
    const float centerY = y + radius;
    for (int row = 0; row < rowCount; ++row) {
        const float sampleY = y + static_cast<float>(row) + 0.5f;
        const float dy = sampleY - centerY;
        const float span = std::sqrt(std::max(0.0f, radius * radius - dy * dy));
        fillRect(renderer, x + radius - span, y + static_cast<float>(row), span, 1.0f);
        fillRect(renderer, x + w - radius, y + static_cast<float>(row), span, 1.0f);
    }
}

void drawValuePill(SDL_Renderer* renderer, const OptionsMenuLayout& layout, float rowTop, const std::string& status, bool selected) {
    const float pillY = rowTop + 1.0f;
    const float pillH = layout.rowH - 3.0f;

    setColor(renderer, selected ? 255 : 82, selected ? 238 : 96, selected ? 160 : 124, 220);
    fillPill(renderer, layout.statusCellX, pillY, layout.statusCellW, pillH);
    setColor(renderer, selected ? 230 : 18, selected ? 220 : 24, selected ? 172 : 34, selected ? 238 : 232);
    fillPill(renderer, layout.statusCellX + 1.0f, pillY + 1.0f, layout.statusCellW - 2.0f, pillH - 2.0f);

    setColor(renderer, selected ? 8 : 214, selected ? 12 : 224, selected ? 16 : 234);
    const float statusX = layout.statusCellX + std::max(2.0f, (layout.statusCellW - debugTextWidth(status)) * 0.5f);
    debugText(renderer, statusX, rowTop + layout.rowTextOffsetY, status);
}

struct MoveListLayout {
    float panelX = 0.0f;
    float panelY = 8.0f;
    float panelW = kMoveListPanelMinW;
    float panelH = 222.0f;
    float tabsY = 0.0f;
    float tabW = 0.0f;
    float sectionY = 0.0f;
    float columnHeaderY = 0.0f;
    float listX = 0.0f;
    float listY = 0.0f;
    float listW = 0.0f;
    float listH = 0.0f;
    float scrollX = 0.0f;
    float detailY = 0.0f;
    float footerY = 0.0f;
    float footerH = 16.0f;
    float numberX = 0.0f;
    float labelX = 0.0f;
    float inputCellX = 0.0f;
    float inputCellW = 0.0f;
    float rowH = 12.0f;
    float rowTextOffsetY = 3.0f;
};

std::string moveListInputForLayout(const TrainingMoveRowView& row) {
    return row.sectionHeader ? "" : (row.input.empty() ? "-" : row.input);
}

CommandInputRenderOptions moveListCommandOptions(const TrainingMoveListView& view, CommandInputChipTone tone = CommandInputChipTone::Normal) {
    CommandInputRenderOptions options;
    options.tone = tone;
    options.iconAtlas = view.commandIcons;
    if (view.physicalDirections) {
        options.directionPresentation = CommandInputDirectionPresentation::Physical;
        options.facing = view.facing;
    }
    return options;
}

MoveListLayout moveListLayout(const TrainingMoveListView& view, float frameW) {
    float maxInputW = debugTextWidth("INPUT");
    const CommandInputRenderOptions inputOptions = moveListCommandOptions(view);
    for (const auto& row : view.rows) {
        if (row.sectionHeader) {
            continue;
        }
        maxInputW = std::max(maxInputW, commandInputWidth(moveListInputForLayout(row), inputOptions));
    }
    if (view.detail.visible) {
        maxInputW = std::max(maxInputW, commandInputWidth(view.detail.performInput, inputOptions));
    }

    MoveListLayout layout;
    frameW = std::max(kOptionsClassicVirtualW, frameW);
    const float availableMaxW = std::clamp(frameW - 12.0f, 292.0f, kMoveListPanelMaxW);
    const float minW = std::min(kMoveListPanelMinW, availableMaxW);
    const float inputCellW = std::clamp(maxInputW + 12.0f, 104.0f, 194.0f);
    const float headerW = 10.0f + debugTextWidth(kMoveListTitle) + 18.0f + debugTextWidth(view.pageLabel) + 10.0f;
    const float footerW = 20.0f + debugTextWidth(kMoveListFooter) + 8.0f;
    const float contentW = 16.0f + 28.0f + 16.0f + 132.0f + 20.0f + inputCellW + 28.0f;
    const float desiredPanelW = std::max(std::max(headerW, footerW), contentW);

    layout.panelW = std::clamp(std::ceil(desiredPanelW), minW, availableMaxW);
    layout.panelX = std::floor((frameW - layout.panelW) * 0.5f);
    layout.inputCellW = std::min(inputCellW, std::max(86.0f, layout.panelW * 0.42f));
    layout.listX = layout.panelX + 16.0f;
    layout.tabsY = layout.panelY + 26.0f;
    layout.tabW = (layout.panelW - 44.0f) * 0.5f;
    layout.sectionY = 0.0f;
    layout.listY = layout.panelY + 55.0f;
    layout.listW = layout.panelW - 32.0f;
    layout.listH = static_cast<float>(kMoveListVisibleRows) * layout.rowH + 8.0f;
    layout.columnHeaderY = layout.listY - 10.0f;
    layout.detailY = layout.listY + layout.listH + 4.0f;
    layout.footerY = layout.detailY + 16.0f;
    layout.panelH = layout.footerY - layout.panelY + layout.footerH + 2.0f;
    if (layout.panelY + layout.panelH > kOptionsVirtualH - 4.0f) {
        layout.panelY = std::max(4.0f, kOptionsVirtualH - layout.panelH - 4.0f);
        layout.tabsY = layout.panelY + 26.0f;
        layout.sectionY = 0.0f;
        layout.listY = layout.panelY + 55.0f;
        layout.columnHeaderY = layout.listY - 10.0f;
        layout.detailY = layout.listY + layout.listH + 4.0f;
        layout.footerY = layout.detailY + 16.0f;
    }
    layout.numberX = layout.listX + 10.0f;
    layout.labelX = layout.listX + 40.0f;
    layout.scrollX = layout.listX + layout.listW - 9.0f;
    layout.inputCellX = layout.scrollX - layout.inputCellW - 14.0f;
    return layout;
}

std::size_t charsThatFit(float width) {
    return static_cast<std::size_t>(std::max(1.0f, std::floor(width / 8.0f)));
}

std::string normalizedMoveSectionLabel(const std::string& category);

std::string moveRowDisplayLabel(const MoveListLayout& layout, const TrainingMoveRowView& row) {
    if (row.sectionHeader) {
        return fitted(normalizedMoveSectionLabel(row.category.empty() ? row.label : row.category), charsThatFit(layout.listW - 22.0f));
    }
    const float availableW = std::max(8.0f, layout.inputCellX - layout.labelX - 18.0f);
    return fitted(row.label, charsThatFit(availableW));
}

std::string moveRowDisplayInput(const MoveListLayout& layout, const TrainingMoveRowView& row) {
    return fitted(moveListInputForLayout(row), charsThatFit(layout.inputCellW - 8.0f));
}

void drawMoveInputChips(
    SDL_Renderer* renderer,
    const MoveListLayout& layout,
    const TrainingMoveListView& view,
    float rowTop,
    const std::string& input,
    bool selected) {
    drawCommandInputChips(
        renderer,
        layout.inputCellX + 4.0f,
        rowTop + 1.0f,
        layout.inputCellW - 8.0f,
        input,
        moveListCommandOptions(view, selected ? CommandInputChipTone::Selected : CommandInputChipTone::Normal));
}

std::string normalizedMoveSectionLabel(const std::string& category) {
    if (category == "NORMAL" || category == "NORMALS") {
        return "Normal Move";
    }
    if (category == "SPECIAL" || category == "SPECIALS") {
        return "Special Move";
    }
    if (category == "SUPER" || category == "SUPERS") {
        return "Super Move";
    }
    return category.empty() ? "All Techniques" : category;
}

std::string moveListSectionLabel(const TrainingMoveListView& view) {
    if (view.detail.visible && !view.detail.type.empty()) {
        return normalizedMoveSectionLabel(view.detail.type);
    }
    return normalizedMoveSectionLabel(view.categoryLabel);
}

void drawMiniButton(SDL_Renderer* renderer, float x, float y, const std::string& label, bool active = false) {
    const float w = std::max(13.0f, debugTextWidth(label) + 6.0f);
    setColor(renderer, active ? 230 : 58, active ? 220 : 68, active ? 172 : 86, active ? 230 : 220);
    fillPill(renderer, x, y, w, 9.0f);
    setColor(renderer, active ? 18 : 10, active ? 24 : 16, active ? 34 : 24, 238);
    fillPill(renderer, x + 1.0f, y + 1.0f, w - 2.0f, 7.0f);
    setColor(renderer, active ? 255 : 186, active ? 242 : 196, active ? 210 : 208);
    debugText(renderer, x + std::max(2.0f, (w - debugTextWidth(label)) * 0.5f), y + 2.0f, label);
}

void drawMoveListTab(
    SDL_Renderer* renderer,
    float x,
    float y,
    float w,
    const std::string& label,
    const std::string& shoulder,
    bool selected) {
    setColor(renderer, selected ? 21 : 13, selected ? 31 : 20, selected ? 48 : 32, selected ? 220 : 186);
    fillRect(renderer, x, y, w, 13.0f);
    setColor(renderer, selected ? 66 : 34, selected ? 82 : 48, selected ? 112 : 68, 208);
    drawRect(renderer, x, y, w, 13.0f);
    drawMiniButton(renderer, x + 5.0f, y + 2.0f, shoulder, selected);
    setColor(renderer, selected ? 230 : 142, selected ? 220 : 154, selected ? 172 : 168);
    debugText(renderer, x + 27.0f, y + 4.0f, fitted(label, charsThatFit(std::max(8.0f, w - 35.0f))));
    if (selected) {
        setColor(renderer, 224, 38, 96, 238);
        fillRect(renderer, x + 2.0f, y + 12.0f, w - 4.0f, 2.0f);
    }
}

void drawMoveListInlineSection(SDL_Renderer* renderer, const MoveListLayout& layout, float rowTop, const TrainingMoveRowView& row) {
    setColor(renderer, 16, 22, 34, 226);
    fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, layout.rowH - 1.0f);
    setColor(renderer, 224, 38, 96, 236);
    fillRect(renderer, layout.listX + 6.0f, rowTop + 2.0f, 2.0f, layout.rowH - 4.0f);
    setColor(renderer, 214, 218, 224);
    debugText(renderer, layout.listX + 12.0f, rowTop + layout.rowTextOffsetY, moveRowDisplayLabel(layout, row));
}

void drawMoveListScrollbar(SDL_Renderer* renderer, const MoveListLayout& layout, const TrainingMoveListView& view) {
    const int total = std::max(view.totalCount, static_cast<int>(view.rows.size()));
    const int visible = std::max(1, view.visibleCapacity > 0 ? view.visibleCapacity : static_cast<int>(view.rows.size()));
    setColor(renderer, 30, 38, 52, 220);
    fillRect(renderer, layout.scrollX, layout.listY + 4.0f, 4.0f, layout.listH - 8.0f);
    if (total <= visible) {
        setColor(renderer, 104, 124, 156, 230);
        fillRect(renderer, layout.scrollX, layout.listY + 4.0f, 4.0f, layout.listH - 8.0f);
        return;
    }

    const float trackH = layout.listH - 8.0f;
    const float thumbH = std::clamp(trackH * (static_cast<float>(visible) / static_cast<float>(total)), 12.0f, trackH);
    const int maxScroll = std::max(1, total - visible);
    const float scrollT = std::clamp(static_cast<float>(view.firstVisibleIndex) / static_cast<float>(maxScroll), 0.0f, 1.0f);
    const float thumbY = layout.listY + 4.0f + (trackH - thumbH) * scrollT;
    setColor(renderer, 206, 214, 226, 232);
    fillRect(renderer, layout.scrollX, thumbY, 4.0f, thumbH);
}

void drawMoveFooterLegendItem(SDL_Renderer* renderer, float& x, float y, const std::string& key, const std::string& label) {
    drawMiniButton(renderer, x, y, key);
    x += std::max(13.0f, debugTextWidth(key) + 6.0f) + 4.0f;
    setColor(renderer, 142, 154, 168);
    debugText(renderer, x, y + 2.0f, label);
    x += debugTextWidth(label) + 10.0f;
}

float moveFooterLegendItemWidth(const std::string& key, const std::string& label) {
    return std::max(13.0f, debugTextWidth(key) + 6.0f) + 4.0f + debugTextWidth(label) + 10.0f;
}

void drawMoveFooterLegendItemIfFits(
    SDL_Renderer* renderer,
    float& x,
    float y,
    float maxX,
    const std::string& key,
    const std::string& label) {
    if (x + moveFooterLegendItemWidth(key, label) <= maxX) {
        drawMoveFooterLegendItem(renderer, x, y, key, label);
    }
}

void drawMoveListFooterLegend(SDL_Renderer* renderer, const MoveListLayout& layout) {
    setColor(renderer, 32, 42, 58, 230);
    fillRect(renderer, layout.panelX + 1.0f, layout.footerY, layout.panelW - 2.0f, layout.footerH);
    float x = layout.panelX + 10.0f;
    const float maxX = layout.panelX + layout.panelW - 8.0f;
    const float y = layout.footerY + 4.0f;
    drawMoveFooterLegendItem(renderer, x, y, "H", "Demo");
    drawMoveFooterLegendItem(renderer, x, y, "ENT", "Back");
    drawMoveFooterLegendItemIfFits(renderer, x, y, maxX, "L/R", "Tabs");
    drawMoveFooterLegendItemIfFits(renderer, x, y, maxX, "UP/DN", "Select");
}

} // namespace

void drawTrainingMoveListPage(const UiRenderContext& ui, const TrainingMoveListView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const MoveListLayout layout = moveListLayout(view, static_cast<float>(ui.logicalWidth));
    drawSoftPanel(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH);

    setColor(renderer, 28, 42, 74);
    fillRect(renderer, layout.panelX + 2.0f, layout.panelY + 2.0f, layout.panelW - 4.0f, 18.0f);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, layout.panelX + 2.0f, layout.panelY + 20.0f, layout.panelW - 4.0f, 2.0f);
    setColor(renderer, 230, 220, 172);
    debugText(renderer, layout.panelX + 10.0f, layout.panelY + 8.0f, kMoveListTitle);
    if (!view.selectedCharacterLabel.empty()) {
        setColor(renderer, 142, 154, 168);
        debugText(renderer, layout.panelX + 118.0f, layout.panelY + 8.0f,
            fitted("P1 " + view.selectedCharacterLabel, charsThatFit(std::max(8.0f, layout.panelW - 206.0f))));
    }
    setColor(renderer, 126, 164, 214);
    debugText(renderer, layout.panelX + layout.panelW - 10.0f - debugTextWidth(view.pageLabel), layout.panelY + 8.0f, view.pageLabel);

    drawMoveListTab(
        renderer,
        layout.panelX + 10.0f,
        layout.tabsY,
        layout.tabW,
        "Main Techniques",
        "L1",
        view.activeTab == TrainingMoveListTab::Main);
    drawMoveListTab(
        renderer,
        layout.panelX + 22.0f + layout.tabW,
        layout.tabsY,
        layout.tabW,
        "All Techniques",
        "R1",
        view.activeTab == TrainingMoveListTab::All);

    setColor(renderer, 142, 154, 168);
    debugText(renderer, layout.labelX, layout.columnHeaderY, "MOVE");
    debugText(renderer, layout.inputCellX + (layout.inputCellW - debugTextWidth("INPUT")) * 0.5f, layout.columnHeaderY, "INPUT");
    setColor(renderer, 208, 168, 92);
    debugText(renderer, layout.numberX, layout.columnHeaderY, "#");

    setColor(renderer, 12, 17, 26, 210);
    fillRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);
    setColor(renderer, 58, 72, 96);
    drawRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);

    if (view.empty) {
        setColor(renderer, 184, 178, 168);
        debugText(renderer, layout.labelX, layout.listY + 20.0f, "No loaded moves");
    } else {
        for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
            const auto& row = view.rows[static_cast<std::size_t>(i)];
            const float rowTop = layout.listY + 4.0f + static_cast<float>(i) * layout.rowH;
            const float textY = rowTop + layout.rowTextOffsetY;
            if (row.sectionHeader) {
                drawMoveListInlineSection(renderer, layout, rowTop, row);
                continue;
            }
            const std::string label = moveRowDisplayLabel(layout, row);
            const std::string input = moveListInputForLayout(row);
            if (row.selected) {
                setColor(renderer, 130, 22, 54, 238);
                fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, layout.rowH - 1.0f);
                setColor(renderer, 218, 42, 84, 232);
                fillRect(renderer, layout.listX + 3.0f, rowTop + 1.0f, layout.listW - 6.0f, 1.0f);
                setColor(renderer, 250, 238, 214);
            } else {
                setColor(renderer, i % 2 == 0 ? 16 : 10, i % 2 == 0 ? 22 : 16, i % 2 == 0 ? 32 : 24, 170);
                fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, layout.rowH - 1.0f);
                setColor(renderer, 186, 196, 208);
            }
            if (row.sectionStart && !row.category.empty()) {
                setColor(renderer, row.selected ? 255 : 224, row.selected ? 206 : 38, row.selected ? 154 : 96, 238);
                fillRect(renderer, layout.listX + 5.0f, rowTop + 2.0f, 2.0f, layout.rowH - 4.0f);
                setColor(renderer, row.selected ? 250 : 186, row.selected ? 238 : 196, row.selected ? 214 : 208);
            }
            debugText(renderer, layout.numberX, textY, row.number);
            debugText(renderer, layout.labelX, textY, label);
            drawMoveInputChips(renderer, layout, view, rowTop, input, row.selected);
        }
    }
    drawMoveListScrollbar(renderer, layout, view);

    setColor(renderer, 16, 22, 32, 224);
    fillRect(renderer, layout.listX, layout.detailY, layout.listW, 13.0f);
    setColor(renderer, 58, 72, 96, 220);
    drawRect(renderer, layout.listX, layout.detailY, layout.listW, 13.0f);
    if (view.detail.visible) {
        const std::string category = view.categoryLabel.empty() ? "ALL" : view.categoryLabel;
        const std::string detailLeft = category + "  STATE " + view.detail.state + "  " + view.detail.type;
        const std::string detailRight = view.detail.powerStatus;
        setColor(renderer, 130, 142, 156);
        debugText(renderer, layout.listX + 8.0f, layout.detailY + 4.0f,
            fitted(detailLeft, charsThatFit(std::max(8.0f, layout.inputCellX - layout.listX - 18.0f))));
        setColor(renderer, 116, 190, 154);
        debugText(renderer, layout.listX + layout.listW - 8.0f - debugTextWidth(fitted(detailRight, charsThatFit(layout.inputCellW))),
            layout.detailY + 4.0f,
            fitted(detailRight, charsThatFit(layout.inputCellW)));
    } else {
        setColor(renderer, 130, 142, 156);
        debugText(renderer, layout.listX + 8.0f, layout.detailY + 4.0f, "No executable commands loaded");
    }

    drawMoveListFooterLegend(renderer, layout);
}

void drawTrainingOptionsMenu(const UiRenderContext& ui, const TrainingOptionsMenuView& view) {
    std::vector<UiMenuListRowView> rows;
    rows.reserve(view.rows.size());
    for (const auto& row : view.rows) {
        rows.push_back(UiMenuListRowView{
            optionsMenuLabel(row.label),
            optionsMenuStatus(row.status),
            row.selected,
            false,
            false,
        });
    }

    drawUiMenuList(
        ui,
        UiMenuListView{
            rows,
            kOptionsTitle,
            view.pageLabel,
            "SETTING",
            "VALUE",
            "",
            kOptionsFooter,
        },
        UiMenuListStyle{
            30.0f,
            320.0f,
            380.0f,
            12.0f,
            true,
        });
}

namespace {

TrainingOptionsMenuGeometryReport verifyTrainingOptionsMenuGeometryFrame(const TrainingOptionsMenuView& view, float frameW) {
    std::vector<UiMenuListRowView> rows;
    rows.reserve(view.rows.size());
    for (const auto& row : view.rows) {
        rows.push_back(UiMenuListRowView{
            optionsMenuLabel(row.label),
            optionsMenuStatus(row.status),
            row.selected,
            false,
            false,
        });
    }

    const auto report = verifyUiMenuListGeometry(
        UiMenuListView{
            rows,
            kOptionsTitle,
            view.pageLabel,
            "SETTING",
            "VALUE",
            "",
            kOptionsFooter,
        },
        frameW,
        UiMenuListStyle{
            30.0f,
            320.0f,
            380.0f,
            12.0f,
            true,
        });
    return { report.ok, report.detail };
}

TrainingOptionsMenuGeometryReport verifyTrainingMoveListGeometryFrame(const TrainingMoveListView& view, float frameW) {
    const MoveListLayout layout = moveListLayout(view, frameW);
    if (!view.empty && view.rows.empty()) {
        return { false, "non-empty command list has no rows" };
    }
    if (view.rows.size() > kMoveListVisibleRows) {
        return { false, "too many rows: " + std::to_string(view.rows.size()) };
    }
    if (layout.panelX < 0.0f || layout.panelY < 0.0f
        || layout.panelX + layout.panelW > frameW
        || layout.panelY + layout.panelH > kOptionsVirtualH) {
        return { false, "panel outside frame " + std::to_string(static_cast<int>(frameW)) + "x240" };
    }
    if (layout.listX < layout.panelX + 2.0f
        || layout.listX + layout.listW > layout.panelX + layout.panelW - 2.0f) {
        return { false, "list outside panel" };
    }
    if (layout.panelX + 10.0f + debugTextWidth(kMoveListTitle) > layout.panelX + layout.panelW - 10.0f - debugTextWidth(view.pageLabel)) {
        return { false, "title/page label collision" };
    }
    if (layout.columnHeaderY < layout.panelY + 22.0f || layout.columnHeaderY + 7.0f > layout.listY) {
        return { false, "column headers outside header band" };
    }
    if (layout.inputCellX <= layout.labelX) {
        return { false, "input column overlaps move labels" };
    }
    if (layout.inputCellX + layout.inputCellW > layout.listX + layout.listW - 4.0f) {
        return { false, "input cell outside list panel" };
    }
    if (layout.detailY < layout.listY + layout.listH
        || layout.detailY + 13.0f > layout.footerY) {
        return { false, "detail strip outside panel" };
    }
    if (layout.footerY + layout.footerH > layout.panelY + layout.panelH
        || layout.panelX + 20.0f + debugTextWidth(kMoveListFooter) > layout.panelX + layout.panelW - 8.0f) {
        return { false, "footer outside panel" };
    }

    const auto selectedRows = std::count_if(view.rows.begin(), view.rows.end(), [](const TrainingMoveRowView& row) {
        return row.selected;
    });
    if (!view.empty && selectedRows != 1) {
        return { false, "expected one selected row, got " + std::to_string(selectedRows) };
    }

    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& row = view.rows[static_cast<std::size_t>(i)];
        const float rowTop = layout.listY + 4.0f + static_cast<float>(i) * layout.rowH;
        const float rowBottom = rowTop + layout.rowH;
        if (rowTop < layout.listY || rowBottom > layout.listY + layout.listH) {
            return { false, "row outside list panel: " + std::to_string(i) };
        }
        if (row.sectionHeader) {
            const std::string label = moveRowDisplayLabel(layout, row);
            if (layout.listX + 12.0f + debugTextWidth(label) + 6.0f > layout.scrollX) {
                return { false, "section header too wide: " + label };
            }
            continue;
        }
        if (layout.numberX + debugTextWidth(row.number) + 4.0f > layout.labelX) {
            return { false, "row number overlaps label: " + row.number };
        }

        const std::string label = moveRowDisplayLabel(layout, row);
        const std::string input = moveRowDisplayInput(layout, row);
        if (layout.labelX + debugTextWidth(label) + 6.0f > layout.inputCellX) {
            return { false, "label/input collision: " + label + " / " + input };
        }
        const CommandInputToken ellipsis{ CommandInputTokenKind::Chip, ".." };
        if (layout.inputCellW < commandInputTokenWidth(ellipsis, moveListCommandOptions(view)) + 8.0f) {
            return { false, "input column too narrow for chips" };
        }
        if (row.selected) {
            if (layout.listX + 2.0f < layout.listX
                || layout.listX + layout.listW - 2.0f > layout.listX + layout.listW) {
                return { false, "selected row band outside list" };
            }
        }
    }

    return {
        true,
        "frame=" + std::to_string(static_cast<int>(frameW))
            + " panelW=" + std::to_string(static_cast<int>(layout.panelW))
            + " rows=" + std::to_string(view.rows.size())
            + " page=" + view.pageLabel,
    };
}

} // namespace

TrainingOptionsMenuGeometryReport verifyTrainingOptionsMenuGeometry(const TrainingOptionsMenuView& view) {
    constexpr float frames[] = {
        kOptionsClassicVirtualW,
        kOptionsDefaultVirtualW,
        kOptionsExtraWideVirtualW,
    };
    std::string detail;
    for (float frameW : frames) {
        const auto report = verifyTrainingOptionsMenuGeometryFrame(view, frameW);
        if (!report.ok) {
            return report;
        }
        if (!detail.empty()) {
            detail += "; ";
        }
        detail += report.detail;
    }
    return { true, detail };
}

TrainingOptionsMenuGeometryReport verifyTrainingMoveListGeometry(const TrainingMoveListView& view) {
    constexpr float frames[] = {
        kOptionsClassicVirtualW,
        kOptionsDefaultVirtualW,
        kOptionsExtraWideVirtualW,
    };
    std::string detail;
    for (float frameW : frames) {
        const auto report = verifyTrainingMoveListGeometryFrame(view, frameW);
        if (!report.ok) {
            return report;
        }
        if (!detail.empty()) {
            detail += "; ";
        }
        detail += report.detail;
    }
    return { true, detail };
}

} // namespace dragon
