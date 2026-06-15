#include "UiMenuList.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace dragon {

namespace {

constexpr float kVirtualH = 240.0f;

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size()) * 8.0f;
}

float debugTextWidth(const char* text) {
    return text ? debugTextWidth(std::string(text)) : 0.0f;
}

std::size_t charsThatFit(float width) {
    return static_cast<std::size_t>(std::max(1.0f, std::floor(width / 8.0f)));
}

std::string fitted(const std::string& text, float width) {
    return fitDebugText(text, charsThatFit(width));
}

std::string displayValue(const UiMenuListRowView& row) {
    if (row.value.empty()) {
        return "";
    }
    return row.adjustable ? "< " + row.value + " >" : row.value;
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

struct MenuListLayout {
    float panelX = 0.0f;
    float panelY = 0.0f;
    float panelW = 0.0f;
    float panelH = 0.0f;
    float listX = 0.0f;
    float listY = 0.0f;
    float listW = 0.0f;
    float listH = 0.0f;
    float labelX = 0.0f;
    float valueCellX = 0.0f;
    float valueCellW = 0.0f;
    float columnHeaderY = 0.0f;
    float statusY = 0.0f;
    float footerY = 0.0f;
    float rowTextOffsetY = 3.0f;
};

MenuListLayout menuListLayout(const UiMenuListView& view, float frameW, const UiMenuListStyle& style) {
    float maxLabelW = debugTextWidth(view.labelHeader.empty() ? "SETTING" : view.labelHeader);
    float maxValueW = debugTextWidth(view.valueHeader.empty() ? "VALUE" : view.valueHeader);
    for (const auto& row : view.rows) {
        maxLabelW = std::max(maxLabelW, debugTextWidth(row.label));
        maxValueW = std::max(maxValueW, debugTextWidth(displayValue(row)));
    }

    MenuListLayout layout;
    frameW = std::max(280.0f, frameW);
    const float availableMaxW = std::clamp(frameW - 20.0f, 260.0f, style.maxPanelW);
    const float minW = std::min(style.minPanelW, availableMaxW);
    const float valueCellW = std::clamp(maxValueW + 18.0f, 76.0f, std::min(190.0f, availableMaxW * 0.48f));
    const float headerW = 10.0f + debugTextWidth(view.title) + 18.0f + debugTextWidth(view.pageLabel) + 10.0f;
    const float footerW = view.footer.empty() ? 0.0f : 20.0f + debugTextWidth(view.footer) + 8.0f;
    const float statusW = view.statusLine.empty() ? 0.0f : 20.0f + debugTextWidth(view.statusLine) + 8.0f;
    const float contentW = 16.0f + 18.0f + maxLabelW + 22.0f + valueCellW + 18.0f;
    const float desiredPanelW = std::max({ contentW, headerW, footerW, statusW, minW });

    layout.panelW = std::clamp(std::ceil(desiredPanelW), minW, availableMaxW);
    layout.panelX = std::floor((frameW - layout.panelW) * 0.5f);
    layout.panelY = style.panelY;
    layout.listX = layout.panelX + 16.0f;
    layout.listW = layout.panelW - 32.0f;
    layout.listY = layout.panelY + 48.0f;
    layout.columnHeaderY = layout.listY - 10.0f;
    layout.listH = static_cast<float>(std::max<std::size_t>(1, view.rows.size())) * style.rowH + 8.0f;
    layout.statusY = layout.listY + layout.listH + 5.0f;
    layout.footerY = layout.statusY + (view.statusLine.empty() ? 0.0f : 14.0f);
    layout.panelH = layout.footerY - layout.panelY + (view.footer.empty() ? 4.0f : 17.0f);
    if (layout.panelY + layout.panelH > kVirtualH - 4.0f) {
        layout.panelY = std::max(4.0f, kVirtualH - layout.panelH - 4.0f);
        layout.listY = layout.panelY + 48.0f;
        layout.columnHeaderY = layout.listY - 10.0f;
        layout.statusY = layout.listY + layout.listH + 5.0f;
        layout.footerY = layout.statusY + (view.statusLine.empty() ? 0.0f : 14.0f);
    }

    layout.labelX = layout.listX + 18.0f;
    layout.valueCellW = std::min(valueCellW, std::max(76.0f, layout.panelW * 0.46f));
    layout.valueCellX = layout.listX + layout.listW - layout.valueCellW - 14.0f;
    return layout;
}

void drawSoftPanel(SDL_Renderer* renderer, const MenuListLayout& layout) {
    setColor(renderer, 8, 12, 18, 226);
    fillRect(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH);
    setColor(renderer, 24, 34, 50, 238);
    fillRect(renderer, layout.panelX + 1.0f, layout.panelY + 1.0f, layout.panelW - 2.0f, 18.0f);
    setColor(renderer, 66, 84, 112);
    drawRect(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH);
}

void drawValueCell(SDL_Renderer* renderer, const MenuListLayout& layout, float rowTop, const std::string& value, bool selected) {
    if (value.empty()) {
        return;
    }

    const float pillY = rowTop + 1.0f;
    const float pillH = 9.0f;
    setColor(renderer, selected ? 255 : 82, selected ? 238 : 96, selected ? 160 : 124, 220);
    fillPill(renderer, layout.valueCellX, pillY, layout.valueCellW, pillH);
    setColor(renderer, selected ? 230 : 18, selected ? 220 : 24, selected ? 172 : 34, selected ? 238 : 232);
    fillPill(renderer, layout.valueCellX + 1.0f, pillY + 1.0f, layout.valueCellW - 2.0f, pillH - 2.0f);

    const std::string text = fitted(value, layout.valueCellW - 8.0f);
    setColor(renderer, selected ? 8 : 214, selected ? 12 : 224, selected ? 16 : 234);
    const float textX = layout.valueCellX + std::max(2.0f, (layout.valueCellW - debugTextWidth(text)) * 0.5f);
    debugText(renderer, textX, rowTop + layout.rowTextOffsetY, text);
}

} // namespace

void drawUiMenuList(const UiRenderContext& ui, const UiMenuListView& view, const UiMenuListStyle& style) {
    SDL_Renderer* renderer = ui.renderer;
    const MenuListLayout layout = menuListLayout(view, static_cast<float>(ui.logicalWidth), style);
    drawSoftPanel(renderer, layout);

    setColor(renderer, 28, 42, 74);
    fillRect(renderer, layout.panelX + 2.0f, layout.panelY + 2.0f, layout.panelW - 4.0f, 18.0f);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, layout.panelX + 2.0f, layout.panelY + 20.0f, layout.panelW - 4.0f, 2.0f);
    setColor(renderer, 230, 220, 172);
    debugText(renderer, layout.panelX + 10.0f, layout.panelY + 8.0f, view.title);
    if (!view.pageLabel.empty()) {
        setColor(renderer, 126, 164, 214);
        debugText(renderer, layout.panelX + layout.panelW - 10.0f - debugTextWidth(view.pageLabel), layout.panelY + 8.0f, view.pageLabel);
    }

    setColor(renderer, 142, 154, 168);
    debugText(renderer, layout.labelX, layout.columnHeaderY, view.labelHeader);
    if (!view.valueHeader.empty()) {
        debugText(renderer, layout.valueCellX + (layout.valueCellW - debugTextWidth(view.valueHeader)) * 0.5f,
            layout.columnHeaderY,
            view.valueHeader);
    }

    setColor(renderer, 12, 17, 26, 210);
    fillRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);
    setColor(renderer, 58, 72, 96);
    drawRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);

    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& row = view.rows[static_cast<std::size_t>(i)];
        const float rowTop = layout.listY + 4.0f + static_cast<float>(i) * style.rowH;
        const float textY = rowTop + layout.rowTextOffsetY;
        const std::string label = fitted(row.label, layout.valueCellX - layout.labelX - 10.0f);
        const std::string value = displayValue(row);
        if (row.selected) {
            if (style.redSelection) {
                setColor(renderer, 130, 22, 54, 238);
                fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, style.rowH - 1.0f);
                setColor(renderer, 218, 42, 84, 232);
                fillRect(renderer, layout.listX + 3.0f, rowTop + 1.0f, layout.listW - 6.0f, 1.0f);
                setColor(renderer, 250, 238, 214);
            } else {
                setColor(renderer, 74, 170, 134, 238);
                fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, style.rowH - 1.0f);
                setColor(renderer, 8, 12, 16);
            }
        } else {
            setColor(renderer, i % 2 == 0 ? 16 : 10, i % 2 == 0 ? 22 : 16, i % 2 == 0 ? 32 : 24, 170);
            fillRect(renderer, layout.listX + 2.0f, rowTop, layout.listW - 4.0f, style.rowH - 1.0f);
            if (row.disabled) {
                setColor(renderer, 112, 120, 134);
            } else {
                setColor(renderer, 186, 196, 208);
            }
        }
        debugText(renderer, layout.labelX, textY, label);
        drawValueCell(renderer, layout, rowTop, value, row.selected);
    }

    if (!view.statusLine.empty()) {
        setColor(renderer, 16, 22, 32, 224);
        fillRect(renderer, layout.listX, layout.statusY, layout.listW, 13.0f);
        setColor(renderer, 58, 72, 96, 220);
        drawRect(renderer, layout.listX, layout.statusY, layout.listW, 13.0f);
        setColor(renderer, 130, 142, 156);
        debugText(renderer, layout.listX + 8.0f, layout.statusY + 4.0f,
            fitted(view.statusLine, layout.listW - 16.0f));
    }

    if (!view.footer.empty()) {
        setColor(renderer, 32, 42, 58, 230);
        fillRect(renderer, layout.panelX + 1.0f, layout.footerY, layout.panelW - 2.0f, 16.0f);
        setColor(renderer, 142, 154, 168);
        debugText(renderer, layout.panelX + 20.0f, layout.footerY + 5.0f,
            fitted(view.footer, layout.panelW - 28.0f));
    }
}

UiMenuListGeometryReport verifyUiMenuListGeometry(
    const UiMenuListView& view,
    float frameW,
    const UiMenuListStyle& style) {
    const MenuListLayout layout = menuListLayout(view, frameW, style);
    if (view.rows.empty()) {
        return { false, "no rows" };
    }
    if (layout.panelX < 0.0f || layout.panelY < 0.0f
        || layout.panelX + layout.panelW > frameW
        || layout.panelY + layout.panelH > kVirtualH) {
        return { false, "panel outside frame" };
    }
    if (layout.valueCellX <= layout.labelX) {
        return { false, "value column overlaps labels" };
    }
    if (layout.valueCellX + layout.valueCellW > layout.listX + layout.listW - 4.0f) {
        return { false, "value column outside list" };
    }
    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& row = view.rows[static_cast<std::size_t>(i)];
        const float rowTop = layout.listY + 4.0f + static_cast<float>(i) * style.rowH;
        const float rowBottom = rowTop + style.rowH;
        if (rowTop < layout.listY || rowBottom > layout.listY + layout.listH) {
            return { false, "row outside list: " + std::to_string(i) };
        }
        const std::string label = fitted(row.label, layout.valueCellX - layout.labelX - 10.0f);
        if (layout.labelX + debugTextWidth(label) + 6.0f > layout.valueCellX) {
            return { false, "label/value collision: " + label };
        }
    }
    return {
        true,
        "frame=" + std::to_string(static_cast<int>(frameW))
            + " panelW=" + std::to_string(static_cast<int>(layout.panelW))
            + " rows=" + std::to_string(view.rows.size()),
    };
}

} // namespace dragon
