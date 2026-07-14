#include "UiMenuList.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace dragon {

namespace {

float debugTextWidth(const std::string& text, float scale = 1.0f) {
    return static_cast<float>(text.size()) * 8.0f * scale;
}

std::size_t charsThatFit(float width, float scale = 1.0f) {
    return static_cast<std::size_t>(std::max(1.0f, std::floor(width / (8.0f * scale))));
}

std::string fitted(const std::string& text, float width, float scale = 1.0f) {
    return fitDebugText(text, charsThatFit(width, scale));
}

std::string displayValue(const UiMenuListRowView& row) {
    if (row.value.empty()) {
        return "";
    }
    return row.adjustable ? "< " + row.value + " >" : row.value;
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
    float textScale = 1.0f;
};

MenuListLayout menuListLayout(const UiMenuListView& view, float frameW, float frameH, const UiMenuListStyle& style) {
    const float textScale = std::clamp(style.textScale, 0.60f, 3.0f);
    const float originX = style.frameX;
    const float originY = style.frameY;
    const float layoutFrameW = style.frameW > 0.0f ? style.frameW : frameW;
    const float layoutFrameH = style.frameH > 0.0f ? style.frameH : frameH;
    float maxLabelW = debugTextWidth(view.labelHeader.empty() ? "SETTING" : view.labelHeader, textScale);
    for (const auto& row : view.rows) {
        maxLabelW = std::max(maxLabelW, debugTextWidth(row.label, textScale));
    }

    MenuListLayout layout;
    layout.textScale = textScale;
    layout.rowTextOffsetY = 3.0f * textScale;
    frameW = std::max(280.0f * textScale, layoutFrameW);
    frameH = std::max(160.0f * textScale, layoutFrameH);
    const float edgePad = 10.0f * textScale;
    const float availableMaxW = std::clamp(frameW - edgePad * 2.0f, 260.0f * textScale, style.maxPanelW);
    const float minW = std::min(style.minPanelW, availableMaxW);
    const float valueCellW = std::min(190.0f * textScale, std::max(76.0f * textScale, availableMaxW * 0.48f));
    const float headerW = 10.0f * textScale + debugTextWidth(view.title, textScale) + 18.0f * textScale + debugTextWidth(view.pageLabel, textScale) + 10.0f * textScale;
    const float footerW = view.footer.empty() ? 0.0f : 20.0f * textScale + debugTextWidth(view.footer, textScale) + 8.0f * textScale;
    const float statusW = view.statusLine.empty() ? 0.0f : 20.0f * textScale + debugTextWidth(view.statusLine, textScale) + 8.0f * textScale;
    const float contentW = 16.0f * textScale + 18.0f * textScale + maxLabelW + 22.0f * textScale + valueCellW + 18.0f * textScale;
    const float desiredPanelW = std::max({ contentW, headerW, footerW, statusW, minW });

    layout.panelW = std::clamp(std::ceil(desiredPanelW), minW, availableMaxW);
    layout.panelX = originX + std::floor((frameW - layout.panelW) * 0.5f);
    layout.panelY = originY + style.panelY;
    layout.listX = layout.panelX + 16.0f * textScale;
    layout.listW = layout.panelW - 32.0f * textScale;
    layout.listY = layout.panelY + 48.0f * textScale;
    layout.columnHeaderY = layout.listY - 10.0f * textScale;
    layout.listH = static_cast<float>(std::max<std::size_t>(1, view.rows.size())) * style.rowH + 8.0f * textScale;
    layout.statusY = layout.listY + layout.listH + 5.0f * textScale;
    layout.footerY = layout.statusY + (view.statusLine.empty() ? 0.0f : 14.0f * textScale);
    layout.panelH = layout.footerY - layout.panelY + (view.footer.empty() ? 4.0f * textScale : 17.0f * textScale);
    if (layout.panelY + layout.panelH > originY + frameH - 4.0f) {
        layout.panelY = std::max(originY + 4.0f, originY + frameH - layout.panelH - 4.0f);
        layout.listY = layout.panelY + 48.0f * textScale;
        layout.columnHeaderY = layout.listY - 10.0f * textScale;
        layout.statusY = layout.listY + layout.listH + 5.0f * textScale;
        layout.footerY = layout.statusY + (view.statusLine.empty() ? 0.0f : 14.0f * textScale);
    }

    layout.labelX = layout.listX + 18.0f * textScale;
    layout.valueCellW = std::min(valueCellW, std::max(76.0f * textScale, layout.panelW * 0.46f));
    layout.valueCellX = layout.listX + layout.listW - layout.valueCellW - 14.0f * textScale;
    return layout;
}

void drawSoftPanel(SDL_Renderer* renderer, const MenuListLayout& layout) {
    const auto& tokens = dragonUiTokens();
    setColor(renderer, tokens.panelBase, 232);
    fillRect(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH);
    setColor(renderer, tokens.secondaryPanel, 238);
    fillRect(renderer, layout.panelX + layout.textScale, layout.panelY + layout.textScale, layout.panelW - 2.0f * layout.textScale, 18.0f * layout.textScale);
    setColor(renderer, tokens.primaryTeal, 170);
    drawRect(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH);
}

void drawValueCell(SDL_Renderer* renderer, const MenuListLayout& layout, float rowTop, float rowH, const std::string& value, bool selected) {
    if (value.empty()) {
        return;
    }

    const auto& tokens = dragonUiTokens();
    const float cellY = rowTop + layout.textScale;
    const float cellH = std::max(9.0f * layout.textScale, rowH - 3.0f * layout.textScale);
    setColor(renderer, selected ? tokens.mutedGold : tokens.primaryTeal, selected ? 235 : 140);
    drawRect(renderer, layout.valueCellX, cellY, layout.valueCellW, cellH);
    setColor(renderer, selected ? tokens.mutedGold : tokens.secondaryPanel, selected ? 42 : 228);
    fillRect(renderer, layout.valueCellX + layout.textScale, cellY + layout.textScale, layout.valueCellW - 2.0f * layout.textScale, cellH - 2.0f * layout.textScale);

    const std::string text = fitted(value, layout.valueCellW - 8.0f * layout.textScale, layout.textScale);
    setColor(renderer, selected ? tokens.mutedGold : tokens.primaryText);
    const float textX = layout.valueCellX + std::max(2.0f * layout.textScale, (layout.valueCellW - debugTextWidth(text, layout.textScale)) * 0.5f);
    scaledDebugText(renderer, layout.textScale, textX, rowTop + layout.rowTextOffsetY, text);
}

} // namespace

void drawUiMenuList(const UiRenderContext& ui, const UiMenuListView& view, const UiMenuListStyle& style) {
    SDL_Renderer* renderer = ui.renderer;
    const auto& tokens = dragonUiTokens();
    const MenuListLayout layout = menuListLayout(view, static_cast<float>(ui.logicalWidth), static_cast<float>(ui.logicalHeight), style);
    const float s = layout.textScale;
    drawSoftPanel(renderer, layout);

    setColor(renderer, tokens.secondaryPanel);
    fillRect(renderer, layout.panelX + 2.0f * s, layout.panelY + 2.0f * s, layout.panelW - 4.0f * s, 18.0f * s);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, layout.panelX + 2.0f * s, layout.panelY + 20.0f * s, layout.panelW - 4.0f * s, 2.0f * s);
    setColor(renderer, tokens.mutedGold);
    scaledDebugText(renderer, s, layout.panelX + 10.0f * s, layout.panelY + 8.0f * s, view.title);
    if (!view.pageLabel.empty()) {
        setColor(renderer, tokens.primaryTeal);
        scaledDebugText(renderer, s, layout.panelX + layout.panelW - 10.0f * s - debugTextWidth(view.pageLabel, s), layout.panelY + 8.0f * s, view.pageLabel);
    }

    setColor(renderer, tokens.mutedText);
    scaledDebugText(renderer, s, layout.labelX, layout.columnHeaderY, view.labelHeader);
    if (!view.valueHeader.empty()) {
        scaledDebugText(renderer, s, layout.valueCellX + (layout.valueCellW - debugTextWidth(view.valueHeader, s)) * 0.5f,
            layout.columnHeaderY,
            view.valueHeader);
    }

    setColor(renderer, tokens.panelBase, 220);
    fillRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);
    setColor(renderer, tokens.primaryTeal, 130);
    drawRect(renderer, layout.listX, layout.listY, layout.listW, layout.listH);

    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& row = view.rows[static_cast<std::size_t>(i)];
        const float rowTop = layout.listY + 4.0f * s + static_cast<float>(i) * style.rowH;
        const float textY = rowTop + layout.rowTextOffsetY;
        const std::string label = fitted(row.label, layout.valueCellX - layout.labelX - 10.0f * s, s);
        const std::string value = displayValue(row);
        if (row.selected) {
            if (style.redSelection) {
                setColor(renderer, tokens.mutedGold, 235);
                fillRect(renderer, layout.listX + 2.0f * s, rowTop, layout.listW - 4.0f * s, style.rowH - 1.0f * s);
                setColor(renderer, tokens.primaryTeal, 210);
                fillRect(renderer, layout.listX + 3.0f * s, rowTop + 1.0f * s, layout.listW - 6.0f * s, 1.0f * s);
                setColor(renderer, tokens.panelBase);
            } else {
                setColor(renderer, tokens.primaryTeal, 238);
                fillRect(renderer, layout.listX + 2.0f * s, rowTop, layout.listW - 4.0f * s, style.rowH - 1.0f * s);
                setColor(renderer, tokens.panelBase);
            }
        } else {
            setColor(renderer, i % 2 == 0 ? tokens.secondaryPanel : tokens.panelBase, 190);
            fillRect(renderer, layout.listX + 2.0f * s, rowTop, layout.listW - 4.0f * s, style.rowH - 1.0f * s);
            if (row.disabled) {
                setColor(renderer, tokens.mutedText, 160);
            } else {
                setColor(renderer, tokens.primaryText);
            }
        }
        scaledDebugText(renderer, s, layout.labelX, textY, label);
        drawValueCell(renderer, layout, rowTop, style.rowH, value, row.selected);
    }

    if (!view.statusLine.empty()) {
        setColor(renderer, tokens.secondaryPanel, 224);
        fillRect(renderer, layout.listX, layout.statusY, layout.listW, 13.0f * s);
        setColor(renderer, tokens.primaryTeal, 150);
        drawRect(renderer, layout.listX, layout.statusY, layout.listW, 13.0f * s);
        setColor(renderer, tokens.mutedText);
        scaledDebugText(renderer, s, layout.listX + 8.0f * s, layout.statusY + 4.0f * s,
            fitted(view.statusLine, layout.listW - 16.0f * s, s));
    }

    if (!view.footer.empty()) {
        setColor(renderer, tokens.secondaryPanel, 230);
        fillRect(renderer, layout.panelX + 1.0f * s, layout.footerY, layout.panelW - 2.0f * s, 16.0f * s);
        setColor(renderer, tokens.mutedText);
        scaledDebugText(renderer, s, layout.panelX + 20.0f * s, layout.footerY + 5.0f * s,
            fitted(view.footer, layout.panelW - 28.0f * s, s));
    }
}

UiMenuListGeometrySnapshot uiMenuListGeometrySnapshot(
    const UiMenuListView& view,
    float frameW,
    float frameH,
    const UiMenuListStyle& style) {
    if (view.rows.empty()) {
        return {};
    }

    const MenuListLayout layout = menuListLayout(view, frameW, frameH, style);
    const float firstRowTop = layout.listY + 4.0f * layout.textScale;
    const float valueCellY = firstRowTop + layout.textScale;
    const float valueCellH = std::max(9.0f * layout.textScale, style.rowH - 3.0f * layout.textScale);
    return {
        true,
        SDL_FRect{ layout.panelX, layout.panelY, layout.panelW, layout.panelH },
        SDL_FRect{ layout.valueCellX, valueCellY, layout.valueCellW, valueCellH },
    };
}

UiMenuListGeometryReport verifyUiMenuListGeometry(
    const UiMenuListView& view,
    float frameW,
    float frameH,
    const UiMenuListStyle& style) {
    const MenuListLayout layout = menuListLayout(view, frameW, frameH, style);
    if (view.rows.empty()) {
        return { false, "no rows" };
    }
    const float boundsX = style.frameX;
    const float boundsY = style.frameY;
    const float boundsW = style.frameW > 0.0f ? style.frameW : frameW;
    const float boundsH = style.frameH > 0.0f ? style.frameH : frameH;
    if (layout.panelX < boundsX || layout.panelY < boundsY
        || layout.panelX + layout.panelW > boundsX + boundsW
        || layout.panelY + layout.panelH > boundsY + boundsH) {
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
        const float rowTop = layout.listY + 4.0f * layout.textScale + static_cast<float>(i) * style.rowH;
        const float rowBottom = rowTop + style.rowH;
        if (rowTop < layout.listY || rowBottom > layout.listY + layout.listH) {
            return { false, "row outside list: " + std::to_string(i) };
        }
        const std::string label = fitted(row.label, layout.valueCellX - layout.labelX - 10.0f * layout.textScale, layout.textScale);
        if (layout.labelX + debugTextWidth(label, layout.textScale) + 6.0f * layout.textScale > layout.valueCellX) {
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

UiMenuListGeometryReport verifyUiMenuListGeometry(
    const UiMenuListView& view,
    float frameW,
    const UiMenuListStyle& style) {
    return verifyUiMenuListGeometry(view, frameW, 240.0f, style);
}

} // namespace dragon
