#pragma once

// Internal App.cpp implementation shard.
// This file depends on App.cpp-local M.U.G.E.N text helpers, primitive parsing
// helpers, and MainMenuPresentationConfig. Include only from App.cpp after those
// dependencies are available.

bool parseDragonBoolValue(const std::string& value, bool fallback) {
    const std::string normalized = lowercaseCopy(trim(value));
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return fallback;
}

std::string dragonSectionValue(const MugenSection& section, std::string_view key, std::string fallback = {}) {
    if (const auto* property = findProperty(section, key)) {
        return unquote(trim(property->value));
    }
    return fallback;
}

std::array<std::string, kMainMenuOptionCount> defaultMainMenuLabels() {
    return {
        "TRAINING",
        "SINGLE PLAYER",
        "VS MODE",
        "ARENA MODE",
        "STORY MODE",
        "SHOP DEMO",
        "OPTIONS",
        "EXIT",
    };
}

std::string sectionStringValue(const MugenSection& section, std::string_view key) {
    if (const auto* property = findProperty(section, key)) {
        return unquote(trim(property->value));
    }
    return {};
}

void loadMotifMainMenuLabels(const std::filesystem::path& gameRoot, MainMenuPresentationConfig& config) {
    const auto systemDef = gameRoot / "data" / "system.def";
    if (!std::filesystem::exists(systemDef)) {
        return;
    }

    constexpr std::array<std::string_view, kMainMenuOptionCount> motifKeys{ {
        "menu.itemname.training",
        "menu.itemname.arcade",
        "menu.itemname.versus",
        "",
        "",
        "",
        "menu.itemname.options",
        "menu.itemname.exit",
    } };

    try {
        const MugenDocument doc = parseMugenTextFile(systemDef);
        const MugenSection* section = findSection(doc, "Title Info");
        if (!section) {
            return;
        }
        for (std::size_t i = 0; i < motifKeys.size(); ++i) {
            if (motifKeys[i].empty()) {
                continue;
            }
            const std::string label = sectionStringValue(*section, motifKeys[i]);
            if (!label.empty()) {
                config.labels[i] = label;
            }
        }
    } catch (const std::exception& ex) {
        SDL_Log("M.U.G.E.N title menu label load failed: %s", ex.what());
    }
}

void applyDragonMainMenuLabels(const MugenSection& section, MainMenuPresentationConfig& config) {
    constexpr std::array<std::string_view, kMainMenuOptionCount> dragonKeys{ {
        "label.training",
        "label.single_player",
        "label.vs_mode",
        "label.arena_mode",
        "label.story_mode",
        "label.shop_demo",
        "label.options",
        "label.exit",
    } };

    for (std::size_t i = 0; i < dragonKeys.size(); ++i) {
        const std::string label = dragonSectionValue(section, dragonKeys[i]);
        if (!label.empty()) {
            config.labels[i] = label;
        }
    }
}

MainMenuLogoMode parseMainMenuLogoMode(std::string value, MainMenuPresentationConfig& config) {
    value = trim(value);
    const std::string normalized = lowercaseCopy(value);
    if (normalized.empty() || normalized == "none" || normalized == "off") {
        return MainMenuLogoMode::None;
    }
    if (normalized == "motif" || normalized == "system") {
        return MainMenuLogoMode::Motif;
    }
    config.logoPath = value;
    return MainMenuLogoMode::Image;
}

void applyDragonMainMenuLayout(const MugenSection& section, MainMenuPresentationConfig& config) {
    config.titleLeft = dragonSectionValue(section, "title.left", config.titleLeft);
    config.titleCenter = dragonSectionValue(section, "title.center", config.titleCenter);
    config.titleBarVisible = parseDragonBoolValue(
        dragonSectionValue(section, "title.visible", config.titleBarVisible ? "1" : "0"),
        config.titleBarVisible);
    config.panelLeftText = dragonSectionValue(section, "panel.left.text", config.panelLeftText);
    config.panelRightText = dragonSectionValue(section, "panel.right.text", config.panelRightText);

    config.logoMode = parseMainMenuLogoMode(dragonSectionValue(section, "logo", "none"), config);
    config.logoX = parseFloatValue(dragonSectionValue(section, "logo.x", std::to_string(config.logoX)), config.logoX);
    config.logoY = parseFloatValue(dragonSectionValue(section, "logo.y", std::to_string(config.logoY)), config.logoY);
    config.logoScale = std::clamp(
        parseFloatValue(dragonSectionValue(section, "logo.scale", std::to_string(config.logoScale)), config.logoScale),
        0.05f,
        4.0f);
    config.logoAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "logo.alpha", std::to_string(config.logoAlpha)), config.logoAlpha),
        0,
        255);

    config.panel.x = parseFloatValue(dragonSectionValue(section, "panel.x", std::to_string(config.panel.x)), config.panel.x);
    config.panel.y = parseFloatValue(dragonSectionValue(section, "panel.y", std::to_string(config.panel.y)), config.panel.y);
    config.panel.w = std::clamp(
        parseFloatValue(dragonSectionValue(section, "panel.w", std::to_string(config.panel.w)), config.panel.w),
        80.0f,
        426.0f);
    config.panel.h = std::clamp(
        parseFloatValue(dragonSectionValue(section, "panel.h", std::to_string(config.panel.h)), config.panel.h),
        60.0f,
        240.0f);
    config.panel.headerH = std::clamp(
        parseFloatValue(dragonSectionValue(section, "panel.header.h", std::to_string(config.panel.headerH)), config.panel.headerH),
        12.0f,
        60.0f);
    config.panel.rowH = std::clamp(
        parseFloatValue(dragonSectionValue(section, "menu.row.h", std::to_string(config.panel.rowH)), config.panel.rowH),
        6.0f,
        28.0f);
    config.panel.selectedInsetX = std::clamp(
        parseFloatValue(dragonSectionValue(section, "selection.inset.x", std::to_string(config.panel.selectedInsetX)), config.panel.selectedInsetX),
        0.0f,
        60.0f);
    config.panel.selectedInsetY = std::clamp(
        parseFloatValue(dragonSectionValue(section, "selection.inset.y", std::to_string(config.panel.selectedInsetY)), config.panel.selectedInsetY),
        0.0f,
        18.0f);
    config.panel.panelFillAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "panel.fill.alpha", std::to_string(config.panel.panelFillAlpha)), config.panel.panelFillAlpha),
        0,
        255);
    config.panel.panelBorderAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "panel.border.alpha", std::to_string(config.panel.panelBorderAlpha)), config.panel.panelBorderAlpha),
        0,
        255);
    config.panel.headerFillAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "panel.header.alpha", std::to_string(config.panel.headerFillAlpha)), config.panel.headerFillAlpha),
        0,
        255);
    config.panel.selectionBorderAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "selection.border.alpha", std::to_string(config.panel.selectionBorderAlpha)), config.panel.selectionBorderAlpha),
        0,
        255);
    config.panel.selectionFillAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "selection.fill.alpha", std::to_string(config.panel.selectionFillAlpha)), config.panel.selectionFillAlpha),
        0,
        255);
    config.panel.selectionUnderlineAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "selection.underline.alpha", std::to_string(config.panel.selectionUnderlineAlpha)), config.panel.selectionUnderlineAlpha),
        0,
        255);
    config.panel.shadowAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "panel.shadow.alpha", std::to_string(config.panel.shadowAlpha)), config.panel.shadowAlpha),
        0,
        255);
    config.panel.shadowOffsetX = parseFloatValue(
        dragonSectionValue(section, "panel.shadow.offset.x", std::to_string(config.panel.shadowOffsetX)),
        config.panel.shadowOffsetX);
    config.panel.shadowOffsetY = parseFloatValue(
        dragonSectionValue(section, "panel.shadow.offset.y", std::to_string(config.panel.shadowOffsetY)),
        config.panel.shadowOffsetY);
    config.motifShadowAlpha = std::clamp(
        parseIntValue(dragonSectionValue(section, "motif.shadow.alpha", std::to_string(config.motifShadowAlpha)), config.motifShadowAlpha),
        0,
        255);
}

MainMenuPresentationConfig loadMainMenuPresentationConfig(const std::filesystem::path& gameRoot) {
    MainMenuPresentationConfig config;
    config.labels = defaultMainMenuLabels();
    loadMotifMainMenuLabels(gameRoot, config);

    const auto dragonDef = gameRoot / "data" / "dragon.def";
    if (!std::filesystem::exists(dragonDef)) {
        return config;
    }

    try {
        const MugenDocument doc = parseMugenTextFile(dragonDef);
        const MugenSection* section = findSection(doc, "Dragon.MainMenu");
        if (!section) {
            return config;
        }

        applyDragonMainMenuLabels(*section, config);
        applyDragonMainMenuLayout(*section, config);

        const std::string background = dragonSectionValue(*section, "background", "motif");
        const std::string normalized = lowercaseCopy(background);
        if (background.empty() || normalized == "motif" || normalized == "system") {
            config.backgroundMode = MainMenuBackgroundMode::Motif;
        } else if (normalized == "fallback" || normalized == "grid") {
            config.backgroundMode = MainMenuBackgroundMode::Fallback;
        } else if (normalized == "none" || normalized == "off") {
            config.backgroundMode = MainMenuBackgroundMode::None;
        } else {
            config.backgroundMode = MainMenuBackgroundMode::Image;
            config.backgroundPath = background;
        }

        config.fallbackGrid = parseDragonBoolValue(
            dragonSectionValue(*section, "fallback.grid", config.fallbackGrid ? "1" : "0"),
            config.fallbackGrid);
        config.backgroundPanX = std::clamp(
            parseFloatValue(dragonSectionValue(*section, "background.pan.x", "0.5"), 0.5f),
            0.0f,
            1.0f);
        config.backgroundDimAlpha = std::clamp(
            parseIntValue(dragonSectionValue(*section, "background.dim", "0"), 0),
            0,
            255);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon main menu config load failed: %s", ex.what());
    }
    return config;
}
