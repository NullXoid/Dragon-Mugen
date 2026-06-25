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
    bool selected) {
    if (const TextureSprite* sprite = shopDemoItemIconSprite(state, entry.itemId);
        sprite && sprite->texture && sprite->width > 0 && sprite->height > 0) {
        const float scale = std::min(10.0f / static_cast<float>(sprite->width), 10.0f / static_cast<float>(sprite->height));
        SDL_FRect dst{ x, y - 3.0f, static_cast<float>(sprite->width) * scale, static_cast<float>(sprite->height) * scale };
        SDL_RenderTexture(renderer, sprite->texture, nullptr, &dst);
        setColor(renderer, selected ? 8 : 184, selected ? 18 : 194, selected ? 24 : 204, 210);
        drawRect(renderer, x - 1.0f, y - 4.0f, 11.0f, 11.0f);
        return;
    }

    if (entry.slot == "charm") {
        setColor(renderer, selected ? 98 : 72, selected ? 118 : 88, selected ? 220 : 172, selected ? 235 : 210);
    } else if (entry.powerBonus > 0) {
        setColor(renderer, selected ? 124 : 82, selected ? 214 : 142, selected ? 246 : 190, selected ? 235 : 210);
    } else {
        setColor(renderer, selected ? 218 : 170, selected ? 176 : 126, selected ? 84 : 58, selected ? 235 : 210);
    }
    fillRect(renderer, x, y, 7.0f, 7.0f);
    setColor(renderer, selected ? 12 : 184, selected ? 18 : 194, selected ? 24 : 204, 220);
    drawRect(renderer, x, y, 7.0f, 7.0f);
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

void drawShopDemoTabs(SDL_Renderer* renderer, float x, float y, float panelW, const AppState& state) {
    constexpr std::array<ShopPanelMode, 3> kModes{ ShopPanelMode::Buy, ShopPanelMode::Sell, ShopPanelMode::Equip };
    const float tabW = (panelW - 24.0f) / 3.0f;
    for (int i = 0; i < 3; ++i) {
        const float tx = x + 8.0f + static_cast<float>(i) * tabW;
        const bool active = state.shopDemo.panelMode == kModes[static_cast<size_t>(i)];
        setColor(renderer, active ? 78 : 28, active ? 180 : 42, active ? 142 : 56, active ? 220 : 190);
        fillRect(renderer, tx, y, tabW - 3.0f, 12.0f);
        setColor(renderer, active ? 8 : 184, active ? 13 : 194, active ? 18 : 204);
        shopDemoPanelTextCentered(renderer, tx + (tabW - 3.0f) * 0.5f, y + 3.0f, shopDemoPanelLabel(kModes[static_cast<size_t>(i)]));
    }
}

size_t shopDemoFitChars(float availablePx) {
    return static_cast<size_t>(std::max(1.0f, std::floor((availablePx - 2.0f) / 8.0f)));
}

std::string shopDemoFitText(const std::string& text, float availablePx) {
    return fitDebugText(text, shopDemoFitChars(availablePx));
}

void drawShopDemoItemPanel(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    const auto catalog = shopDemoCatalog(state);
    shopDemoClampSelection(state);

    const float width = logicalWidthF(state);
    const float panelW = std::min(312.0f, width - 20.0f);
    const float x = width - panelW - 10.0f;
    const float y = 39.0f;
    const float panelH = 162.0f;
    const float panelRight = x + panelW - 12.0f;
    setColor(renderer, 5, 8, 13, 228);
    fillRect(renderer, x, y, panelW, panelH);
    setColor(renderer, 76, 98, 128);
    drawRect(renderer, x, y, panelW, panelH);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, x + 8, y + 18, panelW - 16, 1);
    setColor(renderer, 230, 220, 172);
    shopDemoPanelText(renderer, x + 12, y + 7, "I.CHIE SHOP");

    const std::string profileLine = shopDemoPanelFitText(shopDemoProfileName(state), 70.0f);
    const std::string goldLine = "GOLD " + std::to_string(dragonProgressionGoldForProfile(state.progression.save, shopDemoProfileId(state)));
    setColor(renderer, 108, 244, 156);
    shopDemoPanelText(renderer, x + 104.0f, y + 7, profileLine);
    setColor(renderer, 255, 238, 120);
    shopDemoPanelTextRight(renderer, panelRight, y + 7, goldLine);

    drawShopDemoTabs(renderer, x, y + 24.0f, panelW, state);

    const bool showingEquip = state.shopDemo.panelMode == ShopPanelMode::Equip;
    const float rowStartY = showingEquip ? y + 57.0f : y + 44.0f;
    const float detailY = showingEquip ? y + 95.0f : y + 83.0f;
    const float detailH = showingEquip ? 54.0f : 64.0f;
    if (showingEquip) {
        setColor(renderer, 14, 20, 28, 205);
        fillRect(renderer, x + 10.0f, y + 39.0f, panelW - 20.0f, 13.0f);
        setColor(renderer, 124, 208, 246);
        shopDemoPanelText(renderer, x + 16.0f, y + 42.0f, "TARGET");
        setColor(renderer, 255, 238, 120);
        shopDemoPanelText(
            renderer,
            x + 72.0f,
            y + 42.0f,
            "< " + shopDemoPanelFitText(uppercaseCopy(shopDemoTargetCharacterName(state)), panelRight - (x + 92.0f)) + " >");
    }

    if (catalog.empty()) {
        setColor(renderer, 196, 204, 214);
        shopDemoPanelText(renderer, x + 16.0f, rowStartY + 5.0f, "NO STOCK");
        return;
    }

    const int first = std::max(0, std::min(state.shopDemo.selectedItem - 1, std::max(0, static_cast<int>(catalog.size()) - 3)));
    for (int visible = 0; visible < 3; ++visible) {
        const int index = first + visible;
        if (index >= static_cast<int>(catalog.size())) break;
        const auto& entry = catalog[static_cast<size_t>(index)];
        const float rowY = rowStartY + static_cast<float>(visible) * 14.0f;
        const bool selected = index == state.shopDemo.selectedItem;
        if (selected) {
            setColor(renderer, 72, 176, 138, 215);
            fillRect(renderer, x + 10.0f, rowY - 3.0f, panelW - 20.0f, 13.0f);
        }
        setColor(renderer, selected ? 8 : 196, selected ? 12 : 204, selected ? 16 : 214);
        const int owned = shopDemoOwnedQuantity(state, entry.itemId);
        std::string rowValue;
        if (state.shopDemo.panelMode == ShopPanelMode::Buy) {
            rowValue = "G " + std::to_string(entry.price);
        } else if (state.shopDemo.panelMode == ShopPanelMode::Sell) {
            rowValue = "x" + std::to_string(owned) + "  G " + std::to_string(entry.sellPrice);
        } else {
            rowValue = shopDemoItemEquipped(state, entry.itemId) ? "*EQ" : "x" + std::to_string(owned);
        }
        const float rowValueW = shopDemoPanelTextWidth(rowValue);
        drawShopDemoItemIcon(renderer, state, x + 14.0f, rowY - 1.0f, entry, selected);
        shopDemoPanelText(
            renderer,
            x + 30.0f,
            rowY,
            shopDemoPanelFitText(entry.name, panelRight - rowValueW - (x + 38.0f)));
        shopDemoPanelTextRight(renderer, panelRight, rowY, rowValue);
    }

    const auto selectedEntry = shopDemoSelectedEntry(state);
    if (selectedEntry) {
        const auto stats = effectiveDragonProgressionStatsForProfile(
            state.progression.data,
            state.progression.save,
            shopDemoProfileId(state),
            shopDemoTargetCharacterId(state));
        const float detailRight = panelRight - 2.0f;
        setColor(renderer, 2, 5, 9, 178);
        fillRect(renderer, x + 10.0f, detailY, panelW - 20.0f, detailH);
        setColor(renderer, 34, 42, 50, 150);
        fillRect(renderer, x + 10.0f, detailY, panelW - 20.0f, 1.0f);
        setColor(renderer, 124, 208, 246);
        shopDemoPanelText(
            renderer,
            x + 12.0f,
            detailY + 5.0f,
            shopDemoPanelFitText(selectedEntry->name, detailRight - (x + 92.0f), kShopPanelMetaTextScale),
            kShopPanelMetaTextScale);
        setColor(renderer, 255, 238, 120);
        std::string modeValue = state.shopDemo.panelMode == ShopPanelMode::Buy
            ? "COST G" + std::to_string(selectedEntry->price)
            : state.shopDemo.panelMode == ShopPanelMode::Sell
                ? "VALUE G" + std::to_string(selectedEntry->sellPrice)
                : (shopDemoItemEquipped(state, selectedEntry->itemId) ? "EQUIPPED" : "OWN " + std::to_string(shopDemoOwnedQuantity(state, selectedEntry->itemId)));
        shopDemoPanelTextRight(renderer, detailRight, detailY + 5.0f, modeValue, kShopPanelMetaTextScale);

        setColor(renderer, 150, 156, 166);
        const auto descriptionLines = shopDemoPanelWrapText(selectedEntry->description, detailRight - (x + 12.0f));
        shopDemoPanelText(renderer, x + 12.0f, detailY + 17.0f, descriptionLines[0], kShopPanelMetaTextScale);
        if (!descriptionLines[1].empty()) {
            shopDemoPanelText(renderer, x + 12.0f, detailY + 27.0f, descriptionLines[1], kShopPanelMetaTextScale);
        }
        setColor(renderer, 108, 244, 156);
        shopDemoPanelText(
            renderer,
            x + 12.0f,
            detailY + (descriptionLines[1].empty() ? 29.0f : 38.0f),
            shopDemoPanelFitText(shopDemoEffectSummary(*selectedEntry), detailRight - (x + 12.0f), kShopPanelMetaTextScale),
            kShopPanelMetaTextScale);
        const std::string requirement = "REQ LV " + std::to_string(selectedEntry->requiredLevel)
            + "  TYPE " + uppercaseCopy(selectedEntry->slot);
        const std::string ownership = "OWN " + std::to_string(shopDemoOwnedQuantity(state, selectedEntry->itemId))
            + "  TARGET " + uppercaseCopy(shopDemoTargetCharacterName(state))
            + " LV " + std::to_string(stats.level);
        setColor(renderer, 255, 218, 106);
        shopDemoPanelText(renderer, x + 12.0f, detailY + detailH - 10.0f, shopDemoPanelFitText(requirement, (detailRight - (x + 12.0f)) * 0.48f, kShopPanelMetaTextScale), kShopPanelMetaTextScale);
        shopDemoPanelTextRight(renderer, detailRight, detailY + detailH - 10.0f, shopDemoPanelFitText(ownership, (detailRight - (x + 12.0f)) * 0.52f, kShopPanelMetaTextScale), kShopPanelMetaTextScale);
    }

    if (state.shopDemo.confirmOpen) {
        setColor(renderer, 14, 18, 24, 238);
        fillRect(renderer, x + 14.0f, detailY - 4.0f, panelW - 28.0f, 52.0f);
        setColor(renderer, 225, 202, 112);
        drawRect(renderer, x + 14.0f, detailY - 4.0f, panelW - 28.0f, 52.0f);
        if (selectedEntry) {
            setColor(renderer, 255, 238, 120);
            shopDemoPanelTextCentered(
                renderer,
                x + panelW * 0.5f,
                detailY + 2.0f,
                shopDemoPanelFitText(shopDemoActionLabel(state.shopDemo.pendingAction) + " " + selectedEntry->name, panelW - 46.0f));
            setColor(renderer, 108, 244, 156);
            shopDemoPanelTextCentered(
                renderer,
                x + panelW * 0.5f,
                detailY + 15.0f,
                shopDemoPanelFitText(shopDemoConfirmDetail(state, *selectedEntry), panelW - 46.0f));
        }
        setColor(renderer, 196, 204, 214);
        shopDemoPanelTextCentered(renderer, x + panelW * 0.5f, detailY + 31.0f, "ENTER CONFIRM");
        setColor(renderer, 178, 188, 202);
        shopDemoPanelTextCentered(renderer, x + panelW * 0.5f, detailY + 42.0f, "ESC CANCEL", kShopPanelMetaTextScale);
    } else {
        setColor(renderer, 150, 156, 166);
        const char* footer = showingEquip
            ? "Q/E TAB  L/R TARGET  ENT"
            : "Q/E TAB  UP/DN ITEM  ENT";
        shopDemoPanelText(renderer, x + 12.0f, y + 151.0f, shopDemoPanelFitText(footer, panelRight - (x + 12.0f)));
    }
}

void drawShopDemoTransactionBanner(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.transactionTicks <= 0 || state.shopDemo.transactionTitle.empty()) {
        return;
    }
    const float width = logicalWidthF(state);
    const float bannerW = std::min(300.0f, width - 32.0f);
    const float x = (width - bannerW) * 0.5f;
    const float y = 171.0f;
    const int alpha = std::clamp(state.shopDemo.transactionTicks * 4, 96, 235);
    setColor(renderer, 4, 10, 8, static_cast<Uint8>(alpha));
    fillRect(renderer, x, y, bannerW, 35.0f);
    setColor(renderer, 74, 214, 154, static_cast<Uint8>(std::min(alpha + 20, 255)));
    drawRect(renderer, x, y, bannerW, 35.0f);
    setColor(renderer, 108, 244, 156);
    shopDemoPanelTextCentered(renderer, x + bannerW * 0.5f, y + 7.0f, shopDemoPanelFitText(state.shopDemo.transactionTitle, bannerW - 18.0f));
    if (!state.shopDemo.transactionDetail.empty()) {
        setColor(renderer, 255, 238, 120);
        shopDemoPanelTextCentered(
            renderer,
            x + bannerW * 0.5f,
            y + 21.0f,
            shopDemoPanelFitText(state.shopDemo.transactionDetail, bannerW - 18.0f),
            kShopPanelMetaTextScale);
    }
}
