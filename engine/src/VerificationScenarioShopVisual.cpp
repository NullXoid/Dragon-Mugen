#include "VerificationScenarioCommon.h"

#include "DragonUi.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace dragon::verification {
namespace {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int runDragonUiThemeTokenConsistency(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY dragon-ui-theme-token-consistency\n";
    const auto& tokens = dragonUiTokens();
    record(out, counts,
        tokens.panelBase.r == 0x07 && tokens.panelBase.g == 0x10 && tokens.panelBase.b == 0x19
            && tokens.secondaryPanel.r == 0x10 && tokens.secondaryPanel.g == 0x1A && tokens.secondaryPanel.b == 0x27
            && tokens.primaryTeal.r == 0x51 && tokens.primaryTeal.g == 0xD2 && tokens.primaryTeal.b == 0xC6
            && tokens.mutedGold.r == 0xE7 && tokens.mutedGold.g == 0xC3 && tokens.mutedGold.b == 0x5A
            && tokens.characterPurple.r == 0x7A && tokens.characterPurple.g == 0x4D && tokens.characterPurple.b == 0xD8
            && tokens.separatorRed.r == 0xC6 && tokens.separatorRed.g == 0x4F && tokens.separatorRed.b == 0x55
            && tokens.primaryText.r == 0xE9 && tokens.primaryText.g == 0xED && tokens.primaryText.b == 0xF3
            && tokens.mutedText.r == 0x89 && tokens.mutedText.g == 0x96 && tokens.mutedText.b == 0xA7
            ? Status::Pass : Status::Fail,
        "locked_palette_values",
        "Dragon UI tokens match art-direction lock");
    record(out, counts,
        dragonTextColor(DragonTypographyRole::PanelTitle).r == tokens.mutedGold.r
            && dragonTextColor(DragonTypographyRole::MetadataValue).g == tokens.primaryTeal.g
            && dragonTextColor(DragonTypographyRole::HelpText).b == tokens.mutedText.b
            ? Status::Pass : Status::Fail,
        "typography_roles_mapped",
        "display/panel/currency, metadata, and help roles use shared colors");
    summary(out, counts);
    return exitCode(counts);
}

int runShopOverlayResponsiveLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-overlay-responsive-layout\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string panelText = readTextFile(root / "engine" / "src" / "ShopDemoPanelOverlay.h");
    record(out, counts,
        panelText.find("ShopPanelLayoutMode") != std::string::npos
            && panelText.find("RightCompact") != std::string::npos
            && panelText.find("RightWide") != std::string::npos
            && panelText.find("StandardDefinition") != std::string::npos
            && panelText.find("HighDefinition") != std::string::npos
            ? Status::Pass : Status::Fail,
        "responsive_layout_modes_declared",
        "shop overlay has low-res, SD, and HD modes");
    record(out, counts,
        dimensionsForPreset(CanvasPreset::Classic320x240).width == 320
            && dimensionsForPreset(CanvasPreset::Wide426x240).width == 426
            && dimensionsForPreset(CanvasPreset::Extra480x240).width == 480
            && dimensionsForPreset(CanvasPreset::Sd854x480).height == 480
            && dimensionsForPreset(CanvasPreset::Hd1280x720).height == 720
            ? Status::Pass : Status::Fail,
        "canvas_dimensions_available",
        "all five canvas presets resolve to width/height pairs");
    summary(out, counts);
    return exitCode(counts);
}

int runShopOverlayClassicFullLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-overlay-classic-full-layout\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string panelText = readTextFile(root / "engine" / "src" / "ShopDemoPanelOverlay.h");
    record(out, counts,
        panelText.find("FullClassic") != std::string::npos
            && panelText.find("layout.w = std::min(310.0f") != std::string::npos
            && panelText.find("fillRect(renderer, rects.world.x") != std::string::npos
            ? Status::Pass : Status::Fail,
        "classic_full_panel_with_world_dim",
        "320x240 uses near-full panel and dims the world");
    summary(out, counts);
    return exitCode(counts);
}

int runShopOverlaySdLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-overlay-sd-layout\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string panelText = readTextFile(root / "engine" / "src" / "ShopDemoPanelOverlay.h");
    const DragonUiMetrics metrics = dragonUiMetricsForPreset(CanvasPreset::Sd854x480);
    record(out, counts,
        metrics.pixelScale == 2.0f
            && metrics.topBarH == 48.0f
            && panelText.find("320.0f, 335.0f") != std::string::npos
            ? Status::Pass : Status::Fail,
        "sd_panel_and_two_x_density",
        "854x480 uses the stable composition with crisp 2x pixel UI density");
    summary(out, counts);
    return exitCode(counts);
}

int runShopOverlayHdLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-overlay-hd-layout\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string panelText = readTextFile(root / "engine" / "src" / "ShopDemoPanelOverlay.h");
    const DragonUiMetrics metrics = dragonUiMetricsForPreset(CanvasPreset::Hd1280x720);
    record(out, counts,
        metrics.pixelScale == 3.0f
            && metrics.topBarH == 72.0f
            && panelText.find("470.0f, 486.0f") != std::string::npos
            ? Status::Pass : Status::Fail,
        "hd_panel_and_three_x_density",
        "1280x720 uses the stable composition with crisp 3x pixel UI density");
    summary(out, counts);
    return exitCode(counts);
}

int runShopCharacterDepthOrder(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-character-depth-order\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string runtimeText = readTextFile(root / "engine" / "src" / "ShopDemoRuntime.h");
    const std::string sceneText = readTextFile(root / "engine" / "src" / "ShopHubScene.h");
    const std::string shopText = runtimeText + "\n" + sceneText;
    const size_t shopkeeperShadow = runtimeText.find("drawShopDemoShopkeeperShadow(renderer, state);");
    const size_t shopkeeperBody = runtimeText.find("drawShopDemoShopkeeper(renderer, state);");
    const size_t behindBranch = runtimeText.find("if (playerBehindCounter)");
    const size_t playerBehindShadow = runtimeText.find("drawShopDemoPlayerShadow(renderer, state);", behindBranch);
    const size_t playerBehindBody = runtimeText.find("drawShopDemoPlayer(renderer, state);", playerBehindShadow);
    const size_t counterFront = runtimeText.find("drawShopDemoCounterFront(renderer, state);", playerBehindBody);
    const size_t frontBranch = runtimeText.find("if (!playerBehindCounter)", counterFront);
    const size_t playerFrontShadow = runtimeText.find("drawShopDemoPlayerShadow(renderer, state);", frontBranch);
    const size_t playerFrontBody = runtimeText.find("drawShopDemoPlayer(renderer, state);", playerFrontShadow);
    const bool found = shopkeeperShadow != std::string::npos
        && shopkeeperBody != std::string::npos
        && behindBranch != std::string::npos
        && playerBehindShadow != std::string::npos
        && playerBehindBody != std::string::npos
        && counterFront != std::string::npos
        && frontBranch != std::string::npos
        && playerFrontShadow != std::string::npos
        && playerFrontBody != std::string::npos
        && shopkeeperShadow < shopkeeperBody
        && shopkeeperBody < behindBranch
        && behindBranch < playerBehindShadow
        && playerBehindBody < counterFront
        && counterFront < frontBranch
        && frontBranch < playerFrontShadow
        && playerFrontShadow < playerFrontBody;
    record(out, counts, found ? Status::Pass : Status::Fail,
        "depth_aware_shop_world_render_order",
        "back-lane player draws before the counter face; front-floor player draws after it");
    record(out, counts,
        shopText.find("drawShopDemoContactShadow") != std::string::npos
            && shopText.find("shopDemoPlayerTargetHeight") != std::string::npos
            && shopText.find("shopDemoShopkeeperTargetHeight") != std::string::npos
            && shopText.find("shopDemoShopkeeperVisualY") != std::string::npos
            && shopText.find("shopDemoPlayerBehindCounter") != std::string::npos
            && shopText.find("shopDemoWorldZoomTarget") != std::string::npos
            && shopText.find("SDL_SetRenderClipRect(renderer, &worldClip)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "character_scale_shadows_and_depth",
        "characters scale from world viewport, render shadows, switch occlusion by player depth, and stay clipped to the world camera");
    summary(out, counts);
    return exitCode(counts);
}

int runShopPresentationDebugLabelVisibility(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-presentation-debug-label-visibility\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string runtimeText = readTextFile(root / "engine" / "src" / "ShopDemoRuntime.h");
    const std::string appText = readTextFile(root / "engine" / "src" / "App.cpp");
    const std::string mainText = readTextFile(root / "engine" / "src" / "main.cpp");
    const std::string stateText = readTextFile(root / "engine" / "src" / "ShopDemoState.h");
    const std::string flowText = readTextFile(root / "engine" / "src" / "FrontendFlow.h");
    record(out, counts,
        runtimeText.find("\"P1\"") == std::string::npos
            && runtimeText.find("interactionPromptTicks") == std::string::npos
            && runtimeText.find("ENTER  TALK / SHOP") != std::string::npos
            && runtimeText.find("ENTER  BUY / SELL") != std::string::npos
            && runtimeText.find("shopDemoOpenShopPrompt(state)") != std::string::npos
            && runtimeText.find("ShopInteractionKind::ShopkeeperTalk") != std::string::npos
            && runtimeText.find("shopDemoCounterServiceVolume") != std::string::npos
            && runtimeText.find("shopDemoShopkeeperTalkVolume") != std::string::npos
            && runtimeText.find("shopDemoSetTransaction(state, \"I.CHIE\", shopDemoGreetingText(state)") != std::string::npos
            && runtimeText.find("shopDemoOpenServicePanelAfterGreeting") != std::string::npos
            && runtimeText.find("shopDemoShopActionKeyPressed") != std::string::npos
            && runtimeText.find("InputAction::LK") != std::string::npos
            && runtimeText.find("if (state.shopDemo.shopkeeperGreetingReady) {\n                shopDemoOpenServicePanel(state);") == std::string::npos
            && runtimeText.find("state.shopDemo.shopOpen = true") > runtimeText.find("ShopInteractionKind::CounterService")
            && stateText.find("shopkeeperGreetingReady") != std::string::npos
            && flowText.find("handleShopDemoShopActionButton") != std::string::npos
            && flowText.find("gamepadButtonMatchesControlAction(state, shopPlayerIndex, button, InputAction::LK)") != std::string::npos
            && appText.find("options.hasShopPlayerDepth") != std::string::npos
            && mainText.find("--shop-player-depth") != std::string::npos
            ? Status::Pass : Status::Fail,
        "normal_shop_hides_p1_marker",
        "shop uses one footer prompt; I.Chie greeting continues to shop through LK / PlayStation X");
    summary(out, counts);
    return exitCode(counts);
}

int runShopLiveProfileCurrencyView(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-live-profile-currency-view\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string panelText = readTextFile(root / "engine" / "src" / "ShopDemoPanelOverlay.h");
    const std::string runtimeText = readTextFile(root / "engine" / "src" / "ShopDemoRuntime.h");
    record(out, counts,
        panelText.find("DragonCurrencyView") != std::string::npos
            && panelText.find("ShopItemDetailView") != std::string::npos
            && panelText.find("shopDemoCurrencyView") != std::string::npos
            && panelText.find("shopDemoItemDetailView") != std::string::npos
            && runtimeText.find("dragonProgressionGoldForProfile") != std::string::npos
            ? Status::Pass : Status::Fail,
        "live_view_data_structs",
        "profile, currency, target, owned count, and equipment data are prepared before rendering");
    record(out, counts,
        panelText.find("KASOM") == std::string::npos
            && panelText.find("KUNG FU MAN") == std::string::npos
            && runtimeText.find("KASOM") == std::string::npos
            ? Status::Pass : Status::Fail,
        "example_values_not_hardcoded",
        "renderer uses live values, not spec examples");
    summary(out, counts);
    return exitCode(counts);
}

int runWorldTextureFilterSelection(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY world-texture-filter-selection\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string runtimeText = readTextFile(root / "engine" / "src" / "ShopDemoRuntime.h");
    const std::string hubText = readTextFile(root / "engine" / "src" / "ShopHubScene.h");
    record(out, counts,
        hubText.find("TextureFilter filter = TextureFilter::Linear") != std::string::npos
            && runtimeText.find("TextureFilter::Nearest") != std::string::npos
            && runtimeText.find("training_weight.png\", TextureFilter::Nearest") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_filter_intent",
        "HD shop cutouts default linear while item icons are nearest");
    summary(out, counts);
    return exitCode(counts);
}

int runUiNearestFilterSelection(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY ui-nearest-filter-selection\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string appText = readTextFile(root / "engine" / "src" / "AppUiProjectionAssembly.h");
    const std::string loadingText = readTextFile(root / "engine" / "src" / "RuntimeLoading.h");
    record(out, counts,
        appText.find("TextureFilter::Nearest") != std::string::npos
            && appText.find("SDL_SCALEMODE_NEAREST") != std::string::npos
            && loadingText.find("TextureFilter filter = TextureFilter::Nearest") != std::string::npos
            && loadingText.find("setTextureSpriteFilterIntent(sprite, filter)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "default_decoded_and_ui_sprites_nearest",
        "SFF/MUGEN and pixel UI assets are not globally blurred");
    summary(out, counts);
    return exitCode(counts);
}

int runVideoCanvasSd854x480(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY video-canvas-sd-854x480\n";
    MainSettings settings;
    settings.canvasPreset = CanvasPreset::Sd854x480;
    const CanvasDimensions dims = dimensionsForPreset(settings.canvasPreset);
    record(out, counts,
        dims.width == 854 && dims.height == 480
            && canvasSizeSettingText(settings) == "854x480 SD 480P"
            && layoutClassForPreset(settings.canvasPreset) == DragonLayoutClass::StandardDefinition
            ? Status::Pass : Status::Fail,
        "sd_canvas_preset",
        canvasSizeSettingText(settings));
    summary(out, counts);
    return exitCode(counts);
}

int runVideoCanvasHd1280x720(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY video-canvas-hd-1280x720\n";
    MainSettings settings;
    settings.canvasPreset = CanvasPreset::Hd1280x720;
    const CanvasDimensions dims = dimensionsForPreset(settings.canvasPreset);
    record(out, counts,
        dims.width == 1280 && dims.height == 720
            && canvasSizeSettingText(settings) == "1280x720 HD 720P"
            && layoutClassForPreset(settings.canvasPreset) == DragonLayoutClass::HighDefinition
            ? Status::Pass : Status::Fail,
        "hd_canvas_preset",
        canvasSizeSettingText(settings));
    summary(out, counts);
    return exitCode(counts);
}

int runDragonUiSdTwoXScaling(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY dragon-ui-sd-two-x-scaling\n";
    const DragonUiMetrics metrics = dragonUiMetricsForPreset(CanvasPreset::Sd854x480);
    const SDL_FRect safe = dragonPixelUiSafeArea(dimensionsForPreset(CanvasPreset::Sd854x480));
    record(out, counts,
        metrics.pixelScale == 2.0f
            && metrics.rowH == 36.0f
            && safe.x == 0.0f && safe.w == 854.0f && safe.h == 480.0f
            ? Status::Pass : Status::Fail,
        "sd_output_preset_uses_two_x_pixel_ui_density",
        "safe=" + std::to_string(static_cast<int>(safe.w)) + "x" + std::to_string(static_cast<int>(safe.h)));
    summary(out, counts);
    return exitCode(counts);
}

int runDragonUiHdThreeXScaling(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY dragon-ui-hd-three-x-scaling\n";
    const DragonUiMetrics metrics = dragonUiMetricsForPreset(CanvasPreset::Hd1280x720);
    const SDL_FRect safe = dragonPixelUiSafeArea(dimensionsForPreset(CanvasPreset::Hd1280x720));
    record(out, counts,
        metrics.pixelScale == 3.0f
            && metrics.rowH == 54.0f
            && safe.x == 0.0f && safe.w == 1280.0f && safe.h == 720.0f
            ? Status::Pass : Status::Fail,
        "hd_output_preset_uses_three_x_pixel_ui_density",
        "safe=" + std::to_string(static_cast<int>(safe.w)) + "x" + std::to_string(static_cast<int>(safe.h)));
    summary(out, counts);
    return exitCode(counts);
}

int runWorldViewportSdHdLayout(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY world-viewport-sd-hd-layout\n";
    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string hubText = readTextFile(root / "engine" / "src" / "ShopHubScene.h");
    record(out, counts,
        hubText.find("ShopDemoLayoutRects") != std::string::npos
            && hubText.find("topBar") != std::string::npos
            && hubText.find("world") != std::string::npos
            && hubText.find("helpBar") != std::string::npos
            && hubText.find("shopDemoSceneY(state, 124.8f)") != std::string::npos
            && hubText.find("shopDemoSceneY(state, 182.4f)") != std::string::npos
            && hubText.find("shopDemoWorldFocusY240") != std::string::npos
            ? Status::Pass : Status::Fail,
        "world_viewport_rects",
        "counter placement is relative to the zoomable world viewport with a thick concept-style front face");
    record(out, counts,
        hubText.find("shopDemoLayoutRects(state).world.h * 0.33f") != std::string::npos
            && hubText.find("shopDemoLayoutRects(state).world.h * 0.27f") != std::string::npos
            ? Status::Pass : Status::Fail,
        "character_percent_heights",
        "A.Ben and I.Chie scale from world viewport height");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
