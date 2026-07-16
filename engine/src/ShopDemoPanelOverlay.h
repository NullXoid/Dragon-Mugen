#pragma once

// Internal App.cpp implementation shard included from ShopDemoRuntime.h.
// Shop hub item panel, transaction banner, and compact item detail UI.

constexpr float kShopPanelTextScale = 0.76f;
constexpr float kShopPanelMetaTextScale = 0.70f;

float shopDemoPanelTextWidth(const std::string& text, float scale = kShopPanelTextScale) {
    return static_cast<float>(text.size()) * 8.0f * scale;
}

size_t shopDemoPanelFitChars(float availablePx, float scale = kShopPanelTextScale) {
    return static_cast<size_t>(std::max(1.0f, std::floor((availablePx - 2.0f) / (8.0f * scale))));
}

std::string shopDemoPanelFitText(const std::string& text, float availablePx, float scale = kShopPanelTextScale) {
    return fitDebugText(text, shopDemoPanelFitChars(availablePx, scale));
}

void shopDemoPanelText(
    SDL_Renderer* renderer,
    float x,
    float y,
    const std::string& text,
    float scale = kShopPanelTextScale) {
    scaledDebugText(renderer, scale, x, y, text);
}

void shopDemoPanelTextCentered(
    SDL_Renderer* renderer,
    float centerX,
    float y,
    const std::string& text,
    float scale = kShopPanelTextScale) {
    shopDemoPanelText(renderer, centerX - shopDemoPanelTextWidth(text, scale) * 0.5f, y, text, scale);
}

void shopDemoPanelTextRight(
    SDL_Renderer* renderer,
    float rightX,
    float y,
    const std::string& text,
    float scale = kShopPanelTextScale) {
    shopDemoPanelText(renderer, rightX - shopDemoPanelTextWidth(text, scale), y, text, scale);
}

std::array<std::string, 2> shopDemoPanelWrapText(
    const std::string& text,
    float availablePx,
    float scale = kShopPanelMetaTextScale) {
    const size_t maxChars = shopDemoPanelFitChars(availablePx, scale);
    if (text.size() <= maxChars) {
        return { text, "" };
    }

    size_t split = text.rfind(' ', maxChars);
    if (split == std::string::npos || split < maxChars / 2) {
        split = maxChars;
    }

    std::string first = text.substr(0, split);
    std::string second = text.substr(split);
    while (!second.empty() && second.front() == ' ') {
        second.erase(second.begin());
    }

    return {
        shopDemoPanelFitText(first, availablePx, scale),
        shopDemoPanelFitText(second, availablePx, scale),
    };
}

void shopDemoAppendEffectPart(std::string& text, const std::string& part) {
    if (!text.empty()) {
        text += "  ";
    }
    text += part;
}

std::string shopDemoSignedAmount(int value) {
    return value > 0 ? "+" + std::to_string(value) : std::to_string(value);
}

std::string shopDemoSignedPercentFromPermille(int value) {
    const int percent = value >= 0 ? (value + 5) / 10 : (value - 5) / 10;
    return shopDemoSignedAmount(percent) + "%";
}

std::string shopDemoEffectSummary(const ShopCatalogEntry& entry) {
    std::string effect;
    if (entry.attackPermille != 0) {
        shopDemoAppendEffectPart(effect, "STR " + shopDemoSignedPercentFromPermille(entry.attackPermille));
    }
    if (entry.defencePermille != 0) {
        shopDemoAppendEffectPart(effect, "DEF " + shopDemoSignedPercentFromPermille(entry.defencePermille));
    }
    if (entry.lifeBonus != 0) {
        shopDemoAppendEffectPart(effect, "HP " + shopDemoSignedAmount(entry.lifeBonus));
    }
    if (entry.powerBonus != 0) {
        shopDemoAppendEffectPart(effect, "PWR " + shopDemoSignedAmount(entry.powerBonus));
    }
    return effect.empty() ? "EFFECT TRAINING" : effect;
}

const TextureSprite* shopDemoItemIconSprite(const AppState& state, std::string_view itemId) {
    if (itemId == "training_weight") {
        return &state.shopDemo.trainingWeightIcon;
    }
    if (itemId == "guard_charm") {
        return &state.shopDemo.guardCharmIcon;
    }
    if (itemId == "dragon_sash") {
        return &state.shopDemo.dragonSashIcon;
    }
    return nullptr;
}

void drawShopDemoItemIcon(
    SDL_Renderer* renderer,
    const AppState& state,
    float x,
    float y,
    const ShopCatalogEntry& entry,
    bool selected,
    float iconSize = 10.0f) {
    if (const TextureSprite* sprite = shopDemoItemIconSprite(state, entry.itemId);
        sprite && sprite->texture && sprite->width > 0 && sprite->height > 0) {
        const float scale = std::min(iconSize / static_cast<float>(sprite->width), iconSize / static_cast<float>(sprite->height));
        SDL_FRect dst{ x, y - iconSize * 0.25f, static_cast<float>(sprite->width) * scale, static_cast<float>(sprite->height) * scale };
        SDL_RenderTexture(renderer, sprite->texture, nullptr, &dst);
        setColor(renderer, selected ? 8 : 184, selected ? 18 : 194, selected ? 24 : 204, 210);
        drawRect(renderer, x - 1.0f, y - iconSize * 0.30f, iconSize + 1.0f, iconSize + 1.0f);
        return;
    }

    if (entry.slot == "charm") {
        setColor(renderer, selected ? 98 : 72, selected ? 118 : 88, selected ? 220 : 172, selected ? 235 : 210);
    } else if (entry.powerBonus > 0) {
        setColor(renderer, selected ? 124 : 82, selected ? 214 : 142, selected ? 246 : 190, selected ? 235 : 210);
    } else {
        setColor(renderer, selected ? 218 : 170, selected ? 176 : 126, selected ? 84 : 58, selected ? 235 : 210);
    }
    fillRect(renderer, x, y, iconSize * 0.70f, iconSize * 0.70f);
    setColor(renderer, selected ? 12 : 184, selected ? 18 : 194, selected ? 24 : 204, 220);
    drawRect(renderer, x, y, iconSize * 0.70f, iconSize * 0.70f);
}

std::string shopDemoConfirmDetail(const AppState& state, const ShopCatalogEntry& entry) {
    const int balance = dragonProgressionGoldForProfile(state.progression.save, shopDemoProfileId(state));
    switch (state.shopDemo.pendingAction) {
    case ShopPendingAction::Buy:
        return "COST G" + std::to_string(entry.price) + "  BAL G" + std::to_string(balance);
    case ShopPendingAction::Sell:
        return "GET G" + std::to_string(entry.sellPrice) + "  BAL G" + std::to_string(balance);
    case ShopPendingAction::Equip:
    case ShopPendingAction::Unequip:
        return "TARGET " + uppercaseCopy(shopDemoTargetCharacterName(state));
    case ShopPendingAction::None:
    default:
        return "NO ACTION";
    }
}

void drawShopDemoTabs(SDL_Renderer* renderer, float x, float y, float panelW, const AppState& state, float tabH = 12.0f, float textScale = kShopPanelTextScale) {
    constexpr std::array<ShopPanelMode, 3> kModes{ ShopPanelMode::Buy, ShopPanelMode::Sell, ShopPanelMode::Equip };
    const float tabW = (panelW - 24.0f) / 3.0f;
    const auto& tokens = dragonUiTokens();
    for (int i = 0; i < 3; ++i) {
        const float tx = x + 8.0f + static_cast<float>(i) * tabW;
        const bool active = state.shopDemo.panelMode == kModes[static_cast<size_t>(i)];
        setColor(renderer, active ? tokens.primaryTeal : tokens.secondaryPanel, active ? 225 : 210);
        fillRect(renderer, tx, y, tabW - 3.0f, tabH);
        setColor(renderer, active ? tokens.panelBase : tokens.primaryText);
        shopDemoPanelTextCentered(renderer, tx + (tabW - 3.0f) * 0.5f, y + (tabH - 7.0f * textScale) * 0.5f, shopDemoPanelLabel(kModes[static_cast<size_t>(i)]), textScale);
    }
}

enum class ShopPanelLayoutMode {
    FullClassic,
    RightCompact,
    RightWide,
    StandardDefinition,
    HighDefinition,
};

struct DragonCurrencyView {
    std::string label;
    int amount = 0;
};

struct ShopItemDetailView {
    std::string displayName;
    std::string description;
    std::string typeLabel;
    std::string targetName;
    std::string effectSummary;
    int buyPrice = 0;
    int sellPrice = 0;
    int ownedCount = 0;
    int requiredLevel = 0;
    int targetLevel = 0;
    int goldShortfall = 0;
    bool affordable = false;
    bool levelAllowed = false;
    bool equipped = false;
};

struct ShopPanelLayout {
    ShopPanelLayoutMode mode = ShopPanelLayoutMode::RightCompact;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float scale = 1.0f;
    float textScale = kShopPanelTextScale;
    float metaScale = kShopPanelMetaTextScale;
    float pad = 8.0f;
    float rowH = 14.0f;
    float icon = 10.0f;
    float tabH = 12.0f;
    float footerH = 16.0f;
    int visibleRows = 3;
    bool fullClassic = false;
};

ShopPanelLayout shopDemoPanelLayout(const AppState& state) {
    const int width = logicalWidth(state);
    const int height = logicalHeight(state);
    const DragonLayoutClass layoutClass = layoutClassForDimensions(CanvasDimensions{ width, height });
    const DragonUiMetrics metrics = dragonUiMetricsForCanvas(CanvasDimensions{ width, height }, uiScale(state));
    const ShopDemoLayoutRects rects = shopDemoLayoutRects(state);
    ShopPanelLayout layout;
    layout.scale = metrics.pixelScale;
    layout.textScale = kShopPanelTextScale * layout.scale;
    layout.metaScale = kShopPanelMetaTextScale * layout.scale;
    layout.pad = 8.0f * layout.scale;
    layout.rowH = 16.0f * layout.scale;
    layout.icon = metrics.itemIcon;
    layout.tabH = 14.0f * layout.scale;
    layout.footerH = 17.0f * layout.scale;
    layout.y = rects.topBar.h + 8.0f * layout.scale;

    switch (layoutClass) {
    case DragonLayoutClass::Classic:
        layout.mode = ShopPanelLayoutMode::FullClassic;
        layout.fullClassic = true;
        layout.w = std::min(310.0f, static_cast<float>(width) - 12.0f);
        layout.h = static_cast<float>(height) - rects.topBar.h - rects.helpBar.h - 10.0f;
        layout.x = (static_cast<float>(width) - layout.w) * 0.5f;
        layout.y = rects.topBar.h + 5.0f;
        layout.visibleRows = 3;
        break;
    case DragonLayoutClass::ExtraLowRes:
        layout.mode = ShopPanelLayoutMode::RightWide;
        layout.w = std::min(190.0f, static_cast<float>(width) - 16.0f);
        layout.h = std::min(174.0f, static_cast<float>(height) - rects.topBar.h - rects.helpBar.h - 12.0f);
        layout.x = static_cast<float>(width) - layout.w - 8.0f;
        layout.visibleRows = 3;
        break;
    case DragonLayoutClass::StandardDefinition:
        layout.mode = ShopPanelLayoutMode::StandardDefinition;
        layout.w = std::clamp(static_cast<float>(width) * 0.38f, 320.0f, 335.0f);
        layout.h = std::clamp(static_cast<float>(height) * 0.73f, 338.0f, 350.0f);
        layout.x = static_cast<float>(width) - layout.w - static_cast<float>(width) * 0.025f;
        layout.visibleRows = 3;
        break;
    case DragonLayoutClass::HighDefinition:
        layout.mode = ShopPanelLayoutMode::HighDefinition;
        layout.w = std::clamp(static_cast<float>(width) * 0.37f, 470.0f, 486.0f);
        layout.h = std::clamp(static_cast<float>(height) * 0.73f, 506.0f, 526.0f);
        layout.x = static_cast<float>(width) - layout.w - static_cast<float>(width) * 0.015f;
        layout.visibleRows = 3;
        break;
    case DragonLayoutClass::WideLowRes:
    default:
        layout.mode = ShopPanelLayoutMode::RightCompact;
        layout.w = std::min(168.0f, static_cast<float>(width) - 16.0f);
        layout.h = std::min(172.0f, static_cast<float>(height) - rects.topBar.h - rects.helpBar.h - 12.0f);
        layout.x = static_cast<float>(width) - layout.w - 8.0f;
        layout.visibleRows = 3;
        break;
    }
    return layout;
}

DragonCurrencyView shopDemoCurrencyView(const AppState& state) {
    return {
        "GOLD",
        dragonProgressionGoldForProfile(state.progression.save, shopDemoProfileId(state)),
    };
}

ShopItemDetailView shopDemoItemDetailView(const AppState& state, const ShopCatalogEntry& entry) {
    const std::string profileId = shopDemoProfileId(state);
    const std::string targetId = shopDemoTargetCharacterId(state);
    const auto stats = effectiveDragonProgressionStatsForProfile(
        state.progression.data,
        state.progression.save,
        profileId,
        targetId);
    const int gold = dragonProgressionGoldForProfile(state.progression.save, profileId);
    ShopItemDetailView view;
    view.displayName = entry.name;
    view.description = entry.description;
    view.typeLabel = uppercaseCopy(entry.slot);
    view.targetName = uppercaseCopy(shopDemoTargetCharacterName(state));
    view.effectSummary = shopDemoEffectSummary(entry);
    view.buyPrice = entry.price;
    view.sellPrice = entry.sellPrice;
    view.ownedCount = shopDemoOwnedQuantity(state, entry.itemId);
    view.requiredLevel = entry.requiredLevel;
    view.targetLevel = stats.level;
    view.goldShortfall = std::max(0, entry.price - gold);
    view.levelAllowed = stats.level >= entry.requiredLevel;
    view.affordable = view.levelAllowed && view.goldShortfall == 0;
    view.equipped = shopDemoItemEquipped(state, entry.itemId);
    return view;
}

void drawShopDemoItemPanel(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    const auto catalog = shopDemoCatalog(state);
    shopDemoClampSelection(state);

    const auto& tokens = dragonUiTokens();
    const ShopPanelLayout layout = shopDemoPanelLayout(state);
    const DragonCurrencyView currency = shopDemoCurrencyView(state);
    const float x = layout.x;
    const float y = layout.y;
    const float panelW = layout.w;
    const float panelH = layout.h;
    const float panelRight = x + panelW - layout.pad;
    const bool showingEquip = state.shopDemo.panelMode == ShopPanelMode::Equip;
    const bool compact = layout.mode == ShopPanelLayoutMode::RightCompact;
    const int visibleRows = showingEquip && (layout.fullClassic
        || layout.mode == ShopPanelLayoutMode::RightCompact
        || layout.mode == ShopPanelLayoutMode::RightWide)
            ? std::max(2, layout.visibleRows - 1)
            : layout.visibleRows;

    if (layout.fullClassic) {
        const ShopDemoLayoutRects rects = shopDemoLayoutRects(state);
        setColor(renderer, 0, 0, 0, 132);
        fillRect(renderer, rects.world.x, rects.world.y, rects.world.w, rects.world.h);
    }

    setColor(renderer, 0, 0, 0, 118);
    fillRect(renderer, x + 4.0f * layout.scale, y + 5.0f * layout.scale, panelW, panelH);
    setColor(renderer, tokens.panelBase, 246);
    fillRect(renderer, x, y, panelW, panelH);
    setColor(renderer, tokens.mutedGold, 220);
    drawRect(renderer, x, y, panelW, panelH);
    setColor(renderer, tokens.primaryTeal, 82);
    drawRect(renderer, x + 2.0f * layout.scale, y + 2.0f * layout.scale, panelW - 4.0f * layout.scale, panelH - 4.0f * layout.scale);
    setColor(renderer, tokens.secondaryPanel, 245);
    fillRect(renderer, x + layout.scale, y + layout.scale, panelW - 2.0f * layout.scale, 20.0f * layout.scale);
    setColor(renderer, tokens.mutedGold, 210);
    fillRect(renderer, x + layout.pad, y + 21.0f * layout.scale, panelW - 2.0f * layout.pad, std::max(1.0f, layout.scale));
    setColor(renderer, tokens.mutedGold);
    shopDemoPanelText(renderer, x + layout.pad, y + 7.0f * layout.scale, "I.CHIE SHOP", layout.textScale);

    const std::string goldLine = currency.label + " " + std::to_string(currency.amount);
    setColor(renderer, tokens.mutedGold);
    shopDemoPanelTextRight(renderer, panelRight, y + 7.0f * layout.scale, goldLine, layout.textScale);
    setColor(renderer, tokens.mutedText);
    shopDemoPanelText(renderer, x + layout.pad, y + 23.0f * layout.scale, "PROFILE: " + shopDemoPanelFitText(uppercaseCopy(shopDemoProfileName(state)), panelW - 92.0f * layout.scale, layout.metaScale), layout.metaScale);

    const float tabsY = y + 35.0f * layout.scale;
    drawShopDemoTabs(renderer, x, tabsY, panelW, state, layout.tabH, layout.textScale);

    float targetRowY = tabsY + layout.tabH + 5.0f * layout.scale;
    if (showingEquip) {
        setColor(renderer, tokens.secondaryPanel, 218);
        fillRect(renderer, x + layout.pad, targetRowY, panelW - 2.0f * layout.pad, 15.0f * layout.scale);
        setColor(renderer, tokens.primaryTeal);
        shopDemoPanelText(renderer, x + layout.pad + 4.0f * layout.scale, targetRowY + 4.0f * layout.scale, "TARGET", layout.metaScale);
        setColor(renderer, tokens.mutedGold);
        const std::string targetValue = "< " + shopDemoPanelFitText(uppercaseCopy(shopDemoTargetCharacterName(state)), panelRight - (x + 74.0f * layout.scale), layout.metaScale) + " >";
        shopDemoPanelTextRight(renderer, panelRight - 4.0f * layout.scale, targetRowY + 4.0f * layout.scale, targetValue, layout.metaScale);
        targetRowY += 19.0f * layout.scale;
    }

    const float rowStartY = targetRowY + 3.0f * layout.scale;
    const float detailY = rowStartY + static_cast<float>(visibleRows) * layout.rowH + 8.0f * layout.scale;
    const float footerY = y + panelH - layout.footerH - 2.0f * layout.scale;
    const float detailH = std::max(20.0f * layout.scale, footerY - detailY - 5.0f * layout.scale);

    if (catalog.empty()) {
        setColor(renderer, tokens.primaryText);
        shopDemoPanelText(renderer, x + layout.pad, rowStartY + 5.0f * layout.scale, "NO STOCK", layout.textScale);
        return;
    }

    const int first = std::max(0, std::min(state.shopDemo.selectedItem - 1, std::max(0, static_cast<int>(catalog.size()) - visibleRows)));
    for (int visible = 0; visible < visibleRows; ++visible) {
        const int index = first + visible;
        if (index >= static_cast<int>(catalog.size())) break;
        const auto& entry = catalog[static_cast<size_t>(index)];
        const ShopItemDetailView item = shopDemoItemDetailView(state, entry);
        const float rowY = rowStartY + static_cast<float>(visible) * layout.rowH;
        const bool selected = index == state.shopDemo.selectedItem;
        if (selected) {
            setColor(renderer, tokens.primaryTeal, 224);
            fillRect(renderer, x + layout.pad, rowY - 2.0f * layout.scale, panelW - 2.0f * layout.pad, layout.rowH - 1.0f * layout.scale);
            setColor(renderer, tokens.primaryTeal);
            drawRect(renderer, x + layout.pad, rowY - 2.0f * layout.scale, panelW - 2.0f * layout.pad, layout.rowH - 1.0f * layout.scale);
        } else {
            setColor(renderer, tokens.secondaryPanel, state.shopDemo.panelMode == ShopPanelMode::Equip && item.ownedCount <= 0 ? 120 : 190);
            fillRect(renderer, x + layout.pad, rowY - 2.0f * layout.scale, panelW - 2.0f * layout.pad, layout.rowH - 1.0f * layout.scale);
        }
        std::string rowValue;
        if (state.shopDemo.panelMode == ShopPanelMode::Buy) {
            rowValue = item.affordable
                ? "G" + std::to_string(entry.price)
                : (item.levelAllowed ? "NEED G" + std::to_string(item.goldShortfall) : "LV " + std::to_string(item.requiredLevel));
        } else if (state.shopDemo.panelMode == ShopPanelMode::Sell) {
            rowValue = "x" + std::to_string(item.ownedCount) + "  G" + std::to_string(entry.sellPrice);
        } else {
            rowValue = item.equipped ? "EQUIPPED" : "x" + std::to_string(item.ownedCount);
        }
        const float rowValueW = shopDemoPanelTextWidth(rowValue, layout.metaScale);
        drawShopDemoItemIcon(renderer, state, x + layout.pad + 4.0f * layout.scale, rowY + 1.0f * layout.scale, entry, selected, layout.icon);
        setColor(renderer, selected ? tokens.panelBase : tokens.primaryText);
        shopDemoPanelText(
            renderer,
            x + layout.pad + layout.icon + 10.0f * layout.scale,
            rowY + 3.0f * layout.scale,
            shopDemoPanelFitText(entry.name, panelRight - rowValueW - (x + layout.pad + layout.icon + 15.0f * layout.scale), layout.textScale),
            layout.textScale);
        setColor(renderer, selected ? tokens.panelBase : (item.affordable || state.shopDemo.panelMode != ShopPanelMode::Buy ? tokens.mutedGold : tokens.separatorRed));
        shopDemoPanelTextRight(renderer, panelRight - 2.0f * layout.scale, rowY + 3.0f * layout.scale, rowValue, layout.metaScale);
    }

    const auto selectedEntry = shopDemoSelectedEntry(state);
    if (selectedEntry) {
        const ShopItemDetailView item = shopDemoItemDetailView(state, *selectedEntry);
        const float detailRight = panelRight - 2.0f;
        setColor(renderer, tokens.panelBase, 225);
        fillRect(renderer, x + layout.pad, detailY, panelW - 2.0f * layout.pad, detailH);
        setColor(renderer, tokens.primaryTeal, 110);
        drawRect(renderer, x + layout.pad, detailY, panelW - 2.0f * layout.pad, detailH);
        setColor(renderer, tokens.primaryTeal);
        shopDemoPanelText(
            renderer,
            x + layout.pad + 4.0f * layout.scale,
            detailY + 5.0f * layout.scale,
            shopDemoPanelFitText(item.displayName, detailRight - (x + 92.0f * layout.scale), layout.textScale),
            layout.textScale);
        setColor(renderer, tokens.mutedGold);
        std::string modeValue = state.shopDemo.panelMode == ShopPanelMode::Buy
            ? "COST G" + std::to_string(item.buyPrice)
            : state.shopDemo.panelMode == ShopPanelMode::Sell
                ? "VALUE G" + std::to_string(item.sellPrice)
                : (item.equipped ? "EQUIPPED" : "OWNED " + std::to_string(item.ownedCount));
        shopDemoPanelTextRight(renderer, detailRight, detailY + 5.0f * layout.scale, modeValue, layout.metaScale);

        setColor(renderer, tokens.primaryText);
        const auto descriptionLines = shopDemoPanelWrapText(item.description, detailRight - (x + layout.pad + 4.0f * layout.scale), layout.metaScale);
        const float descY = detailY + 16.0f * layout.scale;
        shopDemoPanelText(renderer, x + layout.pad + 4.0f * layout.scale, descY, descriptionLines[0], layout.metaScale);
        if (!descriptionLines[1].empty()) {
            shopDemoPanelText(renderer, x + layout.pad + 4.0f * layout.scale, descY + 8.0f * layout.scale, descriptionLines[1], layout.metaScale);
        }
        const float effectsY = descY + (descriptionLines[1].empty() ? 8.0f : 16.0f) * layout.scale;
        setColor(renderer, tokens.primaryTeal);
        shopDemoPanelText(renderer, x + layout.pad + 4.0f * layout.scale, effectsY, "EFFECTS", layout.metaScale);
        shopDemoPanelText(
            renderer,
            x + (compact ? 56.0f : 72.0f) * layout.scale,
            effectsY,
            shopDemoPanelFitText(item.effectSummary, detailRight - (x + 76.0f * layout.scale), layout.metaScale),
            layout.metaScale);

        const float metaStep = 7.0f * layout.scale;
        const float metaBlockH = metaStep * 3.0f + 7.0f * layout.metaScale;
        float metaY = effectsY + 8.0f * layout.scale;
        const float latestMetaY = detailY + detailH - metaBlockH - 3.0f * layout.scale;
        if (metaY > latestMetaY) {
            metaY = latestMetaY;
        }
        metaY = std::max(metaY, effectsY + 4.0f * layout.scale);
        const float valueX = x + (compact ? 65.0f : 102.0f) * layout.scale;
        auto drawMeta = [&](float yy, const std::string& label, const std::string& value) {
            setColor(renderer, tokens.mutedText);
            shopDemoPanelText(renderer, x + layout.pad + 4.0f * layout.scale, yy, label, layout.metaScale);
            setColor(renderer, tokens.primaryText);
            shopDemoPanelText(renderer, valueX, yy, shopDemoPanelFitText(value, detailRight - valueX, layout.metaScale), layout.metaScale);
        };
        drawMeta(metaY, compact ? "REQ LV" : "REQUIRED LEVEL", std::to_string(item.requiredLevel));
        drawMeta(metaY + metaStep, "TYPE", item.typeLabel);
        drawMeta(metaY + metaStep * 2.0f, compact ? "OWN" : "OWNED", std::to_string(item.ownedCount));
        drawMeta(metaY + metaStep * 3.0f, "TARGET", item.targetName + " LV " + std::to_string(item.targetLevel));
        if (state.shopDemo.panelMode == ShopPanelMode::Buy && !item.affordable) {
            setColor(renderer, tokens.separatorRed);
            const std::string need = item.levelAllowed
                ? "NEED G" + std::to_string(item.goldShortfall) + " MORE"
                : "REQUIRED LEVEL " + std::to_string(item.requiredLevel);
            shopDemoPanelTextRight(renderer, detailRight, effectsY + 8.0f * layout.scale, shopDemoPanelFitText(need, detailRight - (x + layout.pad), layout.metaScale), layout.metaScale);
        }
    }

    if (state.shopDemo.confirmOpen) {
        const float modalH = std::min(58.0f * layout.scale, panelH - 28.0f * layout.scale);
        const float modalY = std::max(y + 46.0f * layout.scale, detailY + 4.0f * layout.scale);
        setColor(renderer, tokens.panelBase, 246);
        fillRect(renderer, x + layout.pad + 4.0f * layout.scale, modalY, panelW - 2.0f * layout.pad - 8.0f * layout.scale, modalH);
        setColor(renderer, tokens.mutedGold);
        drawRect(renderer, x + layout.pad + 4.0f * layout.scale, modalY, panelW - 2.0f * layout.pad - 8.0f * layout.scale, modalH);
        if (selectedEntry) {
            setColor(renderer, tokens.primaryTeal);
            shopDemoPanelTextCentered(
                renderer,
                x + panelW * 0.5f,
                modalY + 8.0f * layout.scale,
                shopDemoPanelFitText(shopDemoActionLabel(state.shopDemo.pendingAction) + " " + selectedEntry->name + "?", panelW - 46.0f * layout.scale, layout.textScale),
                layout.textScale);
            setColor(renderer, tokens.primaryText);
            shopDemoPanelTextCentered(
                renderer,
                x + panelW * 0.5f,
                modalY + 23.0f * layout.scale,
                shopDemoPanelFitText(shopDemoConfirmDetail(state, *selectedEntry), panelW - 46.0f * layout.scale, layout.metaScale),
                layout.metaScale);
        }
        setColor(renderer, tokens.primaryText);
        shopDemoPanelTextCentered(renderer, x + panelW * 0.5f, modalY + 39.0f * layout.scale, "ENTER CONFIRM", layout.metaScale);
        setColor(renderer, tokens.mutedText);
        shopDemoPanelTextCentered(renderer, x + panelW * 0.5f, modalY + 49.0f * layout.scale, "ESC CANCEL", layout.metaScale);
    } else {
        setColor(renderer, tokens.secondaryPanel, 232);
        fillRect(renderer, x + layout.scale, footerY, panelW - 2.0f * layout.scale, layout.footerH);
        setColor(renderer, tokens.mutedText);
        const char* footer = showingEquip
            ? "Q/E LB/RB TAB  L/R TARGET  ENTER ACTION  ESC BACK"
            : "Q/E LB/RB TAB  UP/DOWN ITEM  ENTER ACTION  ESC BACK";
        shopDemoPanelText(renderer, x + layout.pad, footerY + 5.0f * layout.scale, shopDemoPanelFitText(footer, panelRight - (x + layout.pad), layout.metaScale), layout.metaScale);
    }
}

void drawShopDemoTransactionBanner(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.transactionTicks <= 0 || state.shopDemo.transactionTitle.empty()) {
        return;
    }
    const float width = logicalWidthF(state);
    const DragonUiMetrics metrics = dragonUiMetricsForCanvas(CanvasDimensions{ logicalWidth(state), logicalHeight(state) }, uiScale(state));
    const ShopDemoLayoutRects rects = shopDemoLayoutRects(state);
    const auto& tokens = dragonUiTokens();
    const float s = metrics.pixelScale;
    const float bannerW = std::min(240.0f * s, width - 32.0f * s);
    const float x = (width - bannerW) * 0.5f;
    const float bannerH = 32.0f * s;
    const float y = rects.helpBar.y - 40.0f * s;
    const int alpha = std::clamp(state.shopDemo.transactionTicks * 4, 96, 235);
    setColor(renderer, tokens.panelBase, static_cast<Uint8>(alpha));
    fillRect(renderer, x, y, bannerW, bannerH);
    setColor(renderer, tokens.primaryTeal, static_cast<Uint8>(std::min(alpha + 20, 255)));
    drawRect(renderer, x, y, bannerW, bannerH);
    setColor(renderer, tokens.primaryTeal);
    shopDemoPanelTextCentered(renderer, x + bannerW * 0.5f, y + 7.0f * s, shopDemoPanelFitText(state.shopDemo.transactionTitle, bannerW - 18.0f * s, kShopPanelTextScale * s), kShopPanelTextScale * s);
    if (!state.shopDemo.transactionDetail.empty()) {
        setColor(renderer, tokens.mutedGold);
        shopDemoPanelTextCentered(
            renderer,
            x + bannerW * 0.5f,
            y + 19.0f * s,
            shopDemoPanelFitText(state.shopDemo.transactionDetail, bannerW - 18.0f * s, kShopPanelMetaTextScale * s),
            kShopPanelMetaTextScale * s);
    }
}
