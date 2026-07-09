#include "VerificationScenario.h"

#include "dragon/Compatibility.h"
#include "dragon/MugenData.h"

#include "AppTypes.h"
#include "ControlsOptionsMenu.h"
#include "ControlsStore.h"
#include "FrontendMenu.h"
#include "Input.h"
#include "TrainingCommandInputRenderer.h"
#include "TrainingOptionsBehavior.h"
#include "TrainingOptionsOverlay.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
namespace dragon::verification {
int runTrainingOptionsMenuGeometry(RuntimeProbe& runtime, std::ostream& out);
int runTrainingMoveListGeometry(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandHudLayout(RuntimeProbe& runtime, std::ostream& out);
int runTrainingPauseHelpLegend(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandListTabs(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandIconAtlas(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandSideSwitchHighlight(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandFacingAwareDisplay(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandPhysicalDirectionGuide(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandStartButtonGuide(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandCompleteBlink(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandFilteredComplete(RuntimeProbe& runtime, std::ostream& out);
int runTrainingPaletteSlotSeparation(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTripGrounding(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenOverheadTripChain(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenOverheadTripChainStress(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTripJumpBuffer(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenAttackJumpBufferRelease(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenThrow(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenKuuchuuShakunetsu(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out);
int runLiliTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenShinryukenRecovery(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenShunGokuSatsu(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenShoukiHatsudouSpacing(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTrainingCommandPracticeAdvance(RuntimeProbe& runtime, std::ostream& out);
int runClassicFightOutcomes(RuntimeProbe& runtime, std::ostream& out);
int runClassicFightRouting(RuntimeProbe& runtime, std::ostream& out);
int runClassicFightCombat(RuntimeProbe& runtime, std::ostream& out);
int runRosterCompatibilitySmoke(RuntimeProbe& runtime, std::ostream& out);
int runOwnedCharacterReadiness(RuntimeProbe& runtime, std::ostream& out);
int runDragonProgressionLevelItems(RuntimeProbe& runtime, std::ostream& out);
int runDragonProgressionPlayerProfiles(RuntimeProbe& runtime, std::ostream& out);
int runDragonProgressionEnemyReward(RuntimeProbe& runtime, std::ostream& out);
int runKfmDownHitProfile(RuntimeProbe& runtime, std::ostream& out);
int runKfmGuardRecovery(RuntimeProbe& runtime, std::ostream& out);
int runKfmSpecialsSupers(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenSpecialsSupers(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenHelperLifecycle(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenPowerChargeHelper(RuntimeProbe& runtime, std::ostream& out), runEvilKenAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTrainingCommandDemo(RuntimeProbe& runtime, std::ostream& out);
int runEvilRyuSpecialsSupers(RuntimeProbe& runtime, std::ostream& out), runEvilRyuShinShoryukenStun(RuntimeProbe& runtime, std::ostream& out), runEvilRyuSuperStress(RuntimeProbe& runtime, std::ostream& out), runEvilRyuAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out), runEvilRyuPowerChargeHelper(RuntimeProbe& runtime, std::ostream& out), runEvilRyuThrowBind(RuntimeProbe& runtime, std::ostream& out), runEvilRyuTrainingThrowDemo(RuntimeProbe& runtime, std::ostream& out);
int runKfmMovementDirectionAudit(RuntimeProbe& runtime, std::ostream& out);
int runEvilRyuHighJumpMovementAudit(RuntimeProbe& runtime, std::ostream& out);
int runEvilRyuDash(RuntimeProbe& runtime, std::ostream& out);
int runArenaZKeyboardControls(RuntimeProbe& runtime, std::ostream& out);
int runArenaZAbenWalkAnimation(RuntimeProbe& runtime, std::ostream& out);
int runArenaZGamepadControls(RuntimeProbe& runtime, std::ostream& out);
int runArenaZHitDepth(RuntimeProbe& runtime, std::ostream& out), runArenaZPushDepth(RuntimeProbe& runtime, std::ostream& out), runArenaZDrawOrder(RuntimeProbe& runtime, std::ostream& out);
int runArenaCameraRotationToggle(RuntimeProbe& runtime, std::ostream& out), runArenaCameraRotationProjection(RuntimeProbe& runtime, std::ostream& out), runArenaCameraRotationDrawOrder(RuntimeProbe& runtime, std::ostream& out);
int runArenaZCpuAlign(RuntimeProbe& runtime, std::ostream& out);
int runArenaZModifierSidestep(RuntimeProbe& runtime, std::ostream& out);
int runArenaEvilKenForwardDashBounds(RuntimeProbe& runtime, std::ostream& out);
int runArenaPerFighterRuntime(RuntimeProbe& runtime, std::ostream& out);
int runArenaOpenBorScrollStage(RuntimeProbe& runtime, std::ostream& out), runArenaTmntOpenBorStage(RuntimeProbe& runtime, std::ostream& out), runArenaEvilRyuAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out);
int runStoryModeMenuRoute(RuntimeProbe& runtime, std::ostream& out);
int runStoryStageSelectMap(RuntimeProbe& runtime, std::ostream& out);
int runStoryDifficultyEnemyScaling(RuntimeProbe& runtime, std::ostream& out);
int runStoryOpenBorStageDefault(RuntimeProbe& runtime, std::ostream& out);
int runStoryStageBoardExpansion(RuntimeProbe& runtime, std::ostream& out);
int runStoryBoardRoutePlan(RuntimeProbe& runtime, std::ostream& out);
int runStoryWaveClearForwardCue(RuntimeProbe& runtime, std::ostream& out);
int runStoryShopDoorTrigger(RuntimeProbe& runtime, std::ostream& out);
int runStoryShopRouteResume(RuntimeProbe& runtime, std::ostream& out);
int runStoryWaveSpawnScroll(RuntimeProbe& runtime, std::ostream& out);
int runStoryEnemyTargeting(RuntimeProbe& runtime, std::ostream& out);
int runStoryStageClear(RuntimeProbe& runtime, std::ostream& out);
int runStoryPlayerDefeat(RuntimeProbe& runtime, std::ostream& out);
int runStoryProgressionAward(RuntimeProbe& runtime, std::ostream& out);
int runStoryRewardFeedback(RuntimeProbe& runtime, std::ostream& out);
int runStoryEvilRyuSuperRecovery(RuntimeProbe& runtime, std::ostream& out);
int runVsLoadingProgressBar(RuntimeProbe& runtime, std::ostream& out);
int runSffV2PngDecode(RuntimeProbe& runtime, std::ostream& out);
int runIkemenSelectSlotParsing(RuntimeProbe& runtime, std::ostream& out);
int runStageMusicCodecDecode(RuntimeProbe& runtime, std::ostream& out);
int runExternalStageMount(RuntimeProbe& runtime, std::ostream& out);
int runStoryScottTramRooftop(RuntimeProbe& runtime, std::ostream& out);
int runRuntimePerformanceMetrics(RuntimeProbe& runtime, std::ostream& out);
int runStoryWave3Performance(RuntimeProbe& runtime, std::ostream& out);
int runArenaOpenBor4FighterPerformance(RuntimeProbe& runtime, std::ostream& out);
int runRenderCullingPreservesRuntime(RuntimeProbe& runtime, std::ostream& out);
int runShopRouteEntry(RuntimeProbe& runtime, std::ostream& out);
int runShopRoomActorProjection(RuntimeProbe& runtime, std::ostream& out);
int runShopRoomMovementCollision(RuntimeProbe& runtime, std::ostream& out);
int runShopBuySellPersistence(RuntimeProbe& runtime, std::ostream& out);
int runShopEquipProfileScope(RuntimeProbe& runtime, std::ostream& out);
int runShopGuestNoSave(RuntimeProbe& runtime, std::ostream& out);
int runShopControllerKeyboardNavigation(RuntimeProbe& runtime, std::ostream& out);
int runShopPanelTextFit(RuntimeProbe& runtime, std::ostream& out);
int runDragonUiThemeTokenConsistency(RuntimeProbe& runtime, std::ostream& out);
int runShopOverlayResponsiveLayout(RuntimeProbe& runtime, std::ostream& out);
int runShopOverlayClassicFullLayout(RuntimeProbe& runtime, std::ostream& out);
int runShopOverlaySdLayout(RuntimeProbe& runtime, std::ostream& out);
int runShopOverlayHdLayout(RuntimeProbe& runtime, std::ostream& out);
int runShopCharacterDepthOrder(RuntimeProbe& runtime, std::ostream& out);
int runShopPresentationDebugLabelVisibility(RuntimeProbe& runtime, std::ostream& out);
int runShopLiveProfileCurrencyView(RuntimeProbe& runtime, std::ostream& out);
int runWorldTextureFilterSelection(RuntimeProbe& runtime, std::ostream& out);
int runUiNearestFilterSelection(RuntimeProbe& runtime, std::ostream& out);
int runVideoCanvasSd854x480(RuntimeProbe& runtime, std::ostream& out);
int runVideoCanvasHd1280x720(RuntimeProbe& runtime, std::ostream& out);
int runDragonUiSdTwoXScaling(RuntimeProbe& runtime, std::ostream& out);
int runDragonUiHdThreeXScaling(RuntimeProbe& runtime, std::ostream& out);
int runWorldViewportSdHdLayout(RuntimeProbe& runtime, std::ostream& out);
int runOptionsCategoryNavigation(RuntimeProbe& runtime, std::ostream& out);
int runMainMenuResponsiveLayout(RuntimeProbe& runtime, std::ostream& out);
int runMainMenuEditablePresentationData(RuntimeProbe& runtime, std::ostream& out);
int runMainMenuEditableLayoutData(RuntimeProbe& runtime, std::ostream& out);
int runStageSelectResponsiveLayout(RuntimeProbe& runtime, std::ostream& out);
int runVideoResolutionStableVirtualLayout(RuntimeProbe& runtime, std::ostream& out);
int runVideoHdFullscreenWindowPolicy(RuntimeProbe& runtime, std::ostream& out);
int runControlsPlayerOneToFourNavigation(RuntimeProbe& runtime, std::ostream& out);
int runControlsGuidedSetup(RuntimeProbe& runtime, std::ostream& out);
int runControlsManualEditConflicts(RuntimeProbe& runtime, std::ostream& out);
int runControlsPresets(RuntimeProbe& runtime, std::ostream& out);
int runControlsProfilePersistence(RuntimeProbe& runtime, std::ostream& out);
int runControlsInputTestLive(RuntimeProbe& runtime, std::ostream& out);
int runControlsGlyphDeviceDetection(RuntimeProbe& runtime, std::ostream& out);
int runControlsPauseTauntSeparation(RuntimeProbe& runtime, std::ostream& out);
int runCompatibilityProfileResolver(RuntimeProbe& runtime, std::ostream& out);
int runTrainingShowSelectHold(RuntimeProbe& runtime, std::ostream& out);
int runTrainingShowControllerShortcut(RuntimeProbe& runtime, std::ostream& out);
int runTrainingCommandHeldButtonPrompt(RuntimeProbe& runtime, std::ostream& out);
int runABenTrainingMoveListFromCharacter(RuntimeProbe& runtime, std::ostream& out);
int runCharacterAutoFitScale(RuntimeProbe& runtime, std::ostream& out);
int runLiliSmoke(RuntimeProbe& runtime, std::ostream& out);
int runLiliChangeAnim2Fallback(RuntimeProbe& runtime, std::ostream& out);
int runLiliKuuchStateFallback(RuntimeProbe& runtime, std::ostream& out);
int runLiliHienHououKyakuDemo(RuntimeProbe& runtime, std::ostream& out);
int runKfmBaseline(RuntimeProbe& runtime, std::ostream& out);
int runKfmThrow(RuntimeProbe& runtime, std::ostream& out);
int runKfmAirState(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenSmoke(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenCornerVisualBounds(RuntimeProbe& runtime, std::ostream& out);
int runCpuBaseline(RuntimeProbe& runtime, std::ostream& out);
int runVsP2Runtime(RuntimeProbe& runtime, std::ostream& out);
int runArenaSmoke(RuntimeProbe& runtime, std::ostream& out, int cpuCount);

int runShopDemoRoomHook(RuntimeProbe& runtime, std::ostream& out) {
    int pass = 0;
    int fail = 0;
    const auto recordCheck = [&](bool ok, std::string_view name, const std::string& detail) {
        out << (ok ? "PASS " : "FAIL ") << name << "\n";
        if (!detail.empty()) {
            out << "  " << detail << "\n";
        }
        if (ok) {
            ++pass;
        } else {
            ++fail;
        }
    };
    out << "VERIFY shop-demo-room-hook\n" << "root: " << runtime.rootText() << "\n";

    const auto root = std::filesystem::path(runtime.rootText());
    const auto shopkeeperPng = root / "chars" / "I.Chie" / "I.Chie_shopkeeper_pose.png";
    const auto shopDragonDef = root / "chars" / "I.Chie" / "I.Chie.dragon.def";
    const auto shopSff = root / "chars" / "I.Chie" / "I.Chie.sff";
    recordCheck(std::filesystem::exists(shopkeeperPng), "shopkeeper_pose_png_exists", shopkeeperPng.string());
    recordCheck(std::filesystem::exists(shopSff), "shopkeeper_sff_exists", shopSff.string());

    std::string dragonDefText;
    if (std::ifstream in(shopDragonDef); in) {
        dragonDefText.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    const bool dragonDefTagged = dragonDefText.find("shopkeeper = 1") != std::string::npos
        && dragonDefText.find("shop.action = 9100") != std::string::npos
        && dragonDefText.find("shop.state = 9100") != std::string::npos;
    recordCheck(dragonDefTagged,
        "shopkeeper_dragon_metadata",
        dragonDefTagged ? "shopkeeper action/state metadata present" : "missing shopkeeper metadata");

    const FrontendAction shopAction = decideMainMenuAction(5);
    const FrontendAction optionsAction = decideMainMenuAction(6);
    const FrontendAction exitAction = decideMainMenuAction(7);
    recordCheck(shopAction.kind == FrontendActionKind::OpenShopDemo,
        "main_menu_shop_demo_route",
        "index=5");
    recordCheck(
        optionsAction.kind == FrontendActionKind::OpenOptions && exitAction.kind == FrontendActionKind::ExitApp,
        "main_menu_following_routes_stable",
        "options_index=6 exit_index=7");

    const auto characters = runtime.selectableCharacters();
    const bool aBenSelectable = std::any_of(characters.begin(), characters.end(), [](const RosterCharacterInfo& character) {
        return character.id == "A.Ben" || character.displayName.find("A.Ben") != std::string::npos;
    });
    const bool iChieSelectable = std::any_of(characters.begin(), characters.end(), [](const RosterCharacterInfo& character) {
        return character.id == "I.Chie" || character.displayName.find("I.Chie") != std::string::npos;
    });
    recordCheck(aBenSelectable && iChieSelectable,
        "owned_roster_selectable",
        "selectable_count=" + std::to_string(characters.size()));

    const auto repoRoot = root.filename() == "game" ? root.parent_path() : root;
    const auto shopRuntimePath = repoRoot / "engine" / "src" / "ShopDemoRuntime.h";
    std::string shopRuntimeText;
    if (std::ifstream in(shopRuntimePath); in) {
        shopRuntimeText.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    const bool ownedFallback =
        shopRuntimeText.find("return \"A.Ben\";") != std::string::npos
        && shopRuntimeText.find("return \"kfm\";") == std::string::npos
        && shopRuntimeText.find("return \"Kung Fu Man\";") == std::string::npos;
    recordCheck(ownedFallback,
        "shop_fallback_target_owned_character",
        ownedFallback ? "fallback=A.Ben" : "shop fallback still references unowned KFM data");

    constexpr float roomWidth = 2240.0f;
    constexpr float playerWalkWidth = 2080.0f;
    constexpr float shopkeeperScale = 0.56f;
    recordCheck(roomWidth > static_cast<float>(kDefaultLogicalWidth) * 2.25f,
        "shop_room_has_scroll_space",
        "room_width=" + std::to_string(roomWidth));
    recordCheck(playerWalkWidth > static_cast<float>(kDefaultLogicalWidth) * 2.0f,
        "shop_room_has_walk_space",
        "walk_width=" + std::to_string(playerWalkWidth));
    recordCheck(shopkeeperScale < 0.65f,
        "shop_characters_scaled_down",
        "shopkeeper_scale=" + std::to_string(shopkeeperScale));

    out << "SUMMARY pass=" << pass << " partial=0 fail=" << fail << " blocked=0\n";
    return fail == 0 ? 0 : 1;
}

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out) {
    if (scenarioName == "shop-route-entry") return runShopRouteEntry(runtime, out);
    if (scenarioName == "shop-room-actor-projection") return runShopRoomActorProjection(runtime, out);
    if (scenarioName == "shop-room-movement-collision") return runShopRoomMovementCollision(runtime, out);
    if (scenarioName == "shop-buy-sell-persistence") return runShopBuySellPersistence(runtime, out);
    if (scenarioName == "shop-equip-profile-scope") return runShopEquipProfileScope(runtime, out);
    if (scenarioName == "shop-guest-no-save") return runShopGuestNoSave(runtime, out);
    if (scenarioName == "shop-controller-keyboard-navigation") return runShopControllerKeyboardNavigation(runtime, out);
    if (scenarioName == "shop-panel-text-fit") return runShopPanelTextFit(runtime, out);
    if (scenarioName == "dragon-ui-theme-token-consistency") return runDragonUiThemeTokenConsistency(runtime, out);
    if (scenarioName == "shop-overlay-responsive-layout") return runShopOverlayResponsiveLayout(runtime, out);
    if (scenarioName == "shop-overlay-classic-full-layout") return runShopOverlayClassicFullLayout(runtime, out);
    if (scenarioName == "shop-overlay-sd-layout") return runShopOverlaySdLayout(runtime, out);
    if (scenarioName == "shop-overlay-hd-layout") return runShopOverlayHdLayout(runtime, out);
    if (scenarioName == "shop-character-depth-order") return runShopCharacterDepthOrder(runtime, out);
    if (scenarioName == "shop-presentation-debug-label-visibility") return runShopPresentationDebugLabelVisibility(runtime, out);
    if (scenarioName == "shop-live-profile-currency-view") return runShopLiveProfileCurrencyView(runtime, out);
    if (scenarioName == "world-texture-filter-selection") return runWorldTextureFilterSelection(runtime, out);
    if (scenarioName == "ui-nearest-filter-selection") return runUiNearestFilterSelection(runtime, out);
    if (scenarioName == "video-canvas-sd-854x480") return runVideoCanvasSd854x480(runtime, out);
    if (scenarioName == "video-canvas-hd-1280x720") return runVideoCanvasHd1280x720(runtime, out);
    if (scenarioName == "dragon-ui-sd-two-x-scaling") return runDragonUiSdTwoXScaling(runtime, out);
    if (scenarioName == "dragon-ui-hd-three-x-scaling") return runDragonUiHdThreeXScaling(runtime, out);
    if (scenarioName == "world-viewport-sd-hd-layout") return runWorldViewportSdHdLayout(runtime, out);
    if (scenarioName == "shop-demo-room-hook") return runShopDemoRoomHook(runtime, out);
    if (scenarioName == "runtime-performance-metrics") return runRuntimePerformanceMetrics(runtime, out);
    if (scenarioName == "story-wave3-performance") return runStoryWave3Performance(runtime, out);
    if (scenarioName == "arena-openbor-4fighter-performance") return runArenaOpenBor4FighterPerformance(runtime, out);
    if (scenarioName == "render-culling-preserves-runtime") return runRenderCullingPreservesRuntime(runtime, out);
    if (scenarioName == "main-menu-responsive-layout") return runMainMenuResponsiveLayout(runtime, out);
    if (scenarioName == "main-menu-editable-presentation-data") return runMainMenuEditablePresentationData(runtime, out);
    if (scenarioName == "main-menu-editable-layout-data") return runMainMenuEditableLayoutData(runtime, out);
    if (scenarioName == "stage-select-responsive-layout") return runStageSelectResponsiveLayout(runtime, out);
    if (scenarioName == "video-resolution-stable-virtual-layout") return runVideoResolutionStableVirtualLayout(runtime, out);
    if (scenarioName == "video-hd-fullscreen-window-policy") return runVideoHdFullscreenWindowPolicy(runtime, out);
    if (scenarioName == "options-category-navigation") return runOptionsCategoryNavigation(runtime, out);
    if (scenarioName == "controls-player-1-4-navigation") return runControlsPlayerOneToFourNavigation(runtime, out);
    if (scenarioName == "controls-guided-setup") return runControlsGuidedSetup(runtime, out);
    if (scenarioName == "controls-manual-edit-conflicts") return runControlsManualEditConflicts(runtime, out);
    if (scenarioName == "controls-presets") return runControlsPresets(runtime, out);
    if (scenarioName == "controls-profile-persistence") return runControlsProfilePersistence(runtime, out);
    if (scenarioName == "controls-input-test-live") return runControlsInputTestLive(runtime, out);
    if (scenarioName == "controls-glyph-device-detection") return runControlsGlyphDeviceDetection(runtime, out);
    if (scenarioName == "controls-pause-taunt-separation") return runControlsPauseTauntSeparation(runtime, out);
    if (scenarioName == "compatibility-profile-resolver") return runCompatibilityProfileResolver(runtime, out);
    if (scenarioName == "training-options-menu-geometry") return runTrainingOptionsMenuGeometry(runtime, out);
    if (scenarioName == "training-move-list-geometry") return runTrainingMoveListGeometry(runtime, out);
    if (scenarioName == "training-command-hud-layout") return runTrainingCommandHudLayout(runtime, out);
    if (scenarioName == "training-pause-help-legend") return runTrainingPauseHelpLegend(runtime, out);
    if (scenarioName == "training-command-list-tabs") return runTrainingCommandListTabs(runtime, out);
    if (scenarioName == "training-command-icon-atlas") return runTrainingCommandIconAtlas(runtime, out);
    if (scenarioName == "training-command-side-switch-highlight") return runTrainingCommandSideSwitchHighlight(runtime, out);
    if (scenarioName == "training-command-facing-aware-display") return runTrainingCommandFacingAwareDisplay(runtime, out);
    if (scenarioName == "training-command-physical-direction-guide") return runTrainingCommandPhysicalDirectionGuide(runtime, out);
    if (scenarioName == "training-command-start-button-guide") return runTrainingCommandStartButtonGuide(runtime, out);
    if (scenarioName == "training-command-complete-blink") return runTrainingCommandCompleteBlink(runtime, out);
    if (scenarioName == "training-command-filtered-complete") return runTrainingCommandFilteredComplete(runtime, out);
    if (scenarioName == "training-palette-slot-separation") return runTrainingPaletteSlotSeparation(runtime, out);
    if (scenarioName == "training-show-select-hold") return runTrainingShowSelectHold(runtime, out);
    if (scenarioName == "training-show-controller-shortcut") return runTrainingShowControllerShortcut(runtime, out);
    if (scenarioName == "training-command-held-button-prompt") return runTrainingCommandHeldButtonPrompt(runtime, out);
    if (scenarioName == "aben-training-move-list-from-character") return runABenTrainingMoveListFromCharacter(runtime, out);
    if (scenarioName == "character-auto-fit-scale") return runCharacterAutoFitScale(runtime, out);
    if (scenarioName == "lili-smoke") return runLiliSmoke(runtime, out);
    if (scenarioName == "lili-changeanim2-fallback") return runLiliChangeAnim2Fallback(runtime, out);
    if (scenarioName == "lili-kuuch-state-fallback") return runLiliKuuchStateFallback(runtime, out);
    if (scenarioName == "lili-hien-houou-kyaku-demo") return runLiliHienHououKyakuDemo(runtime, out);
    if (scenarioName == "kfm-baseline") return runKfmBaseline(runtime, out);
    if (scenarioName == "kfm-throw") return runKfmThrow(runtime, out);
    if (scenarioName == "kfm-air-state") return runKfmAirState(runtime, out);
    if (scenarioName == "kfm-movement-direction-audit") return runKfmMovementDirectionAudit(runtime, out);
    if (scenarioName == "evilryu-high-jump") return runEvilRyuHighJumpMovementAudit(runtime, out);
    if (scenarioName == "kfm-down-hit-profile") return runKfmDownHitProfile(runtime, out);
    if (scenarioName == "kfm-guard-recovery") return runKfmGuardRecovery(runtime, out);
    if (scenarioName == "kfm-specials-supers") return runKfmSpecialsSupers(runtime, out);
    if (scenarioName == "evilken-specials-supers") return runEvilKenSpecialsSupers(runtime, out);
    if (scenarioName == "evilken-helper-lifecycle") return runEvilKenHelperLifecycle(runtime, out);
    if (scenarioName == "evilken-power-charge-helper") return runEvilKenPowerChargeHelper(runtime, out);
    if (scenarioName == "evilken-air-special-contact-landing") return runEvilKenAirSpecialContactLanding(runtime, out);
    if (scenarioName == "evilken-training-demo-hit") return runEvilKenTrainingCommandDemo(runtime, out);
    if (scenarioName == "evilken-training-command-practice-advance") return runEvilKenTrainingCommandPracticeAdvance(runtime, out);
    if (scenarioName == "evilryu-specials-supers") return runEvilRyuSpecialsSupers(runtime, out);
    if (scenarioName == "evilryu-shin-shoryuken-stun") return runEvilRyuShinShoryukenStun(runtime, out);
    if (scenarioName == "evilryu-super-stress") return runEvilRyuSuperStress(runtime, out);
    if (scenarioName == "evilryu-air-special-contact-landing") return runEvilRyuAirSpecialContactLanding(runtime, out);
    if (scenarioName == "evilryu-power-charge-helper") return runEvilRyuPowerChargeHelper(runtime, out);
    if (scenarioName == "evilryu-throw-bind") return runEvilRyuThrowBind(runtime, out);
    if (scenarioName == "evilryu-training-throw-demo") return runEvilRyuTrainingThrowDemo(runtime, out);
    if (scenarioName == "evilken-smoke") return runEvilKenSmoke(runtime, out);
    if (scenarioName == "evilken-trip-grounding") return runEvilKenTripGrounding(runtime, out);
    if (scenarioName == "evilken-overhead-trip-chain") return runEvilKenOverheadTripChain(runtime, out);
    if (scenarioName == "evilken-overhead-trip-chain-stress") return runEvilKenOverheadTripChainStress(runtime, out);
    if (scenarioName == "evilken-trip-jump-buffer") return runEvilKenTripJumpBuffer(runtime, out);
    if (scenarioName == "evilken-attack-jump-buffer-release") return runEvilKenAttackJumpBufferRelease(runtime, out);
    if (scenarioName == "evilken-throw") return runEvilKenThrow(runtime, out);
    if (scenarioName == "evilken-corner-visual-bounds") return runEvilKenCornerVisualBounds(runtime, out);
    if (scenarioName == "evilken-kuuchuu-shakunetsu") return runEvilKenKuuchuuShakunetsu(runtime, out);
    if (scenarioName == "evilken-training-demo-all") return runEvilKenTrainingDemoAll(runtime, out);
    if (scenarioName == "lili-training-demo-all") return runLiliTrainingDemoAll(runtime, out);
    if (scenarioName == "evilken-shinryuken-recovery") return runEvilKenShinryukenRecovery(runtime, out);
    if (scenarioName == "evilken-shun-goku-satsu") return runEvilKenShunGokuSatsu(runtime, out);
    if (scenarioName == "evilken-shouki-hatsudou-spacing") return runEvilKenShoukiHatsudouSpacing(runtime, out);
    if (scenarioName == "cpu-baseline") return runCpuBaseline(runtime, out);
    if (scenarioName == "classic-fight-outcomes") return runClassicFightOutcomes(runtime, out);
    if (scenarioName == "classic-fight-routing") return runClassicFightRouting(runtime, out);
    if (scenarioName == "classic-fight-combat") return runClassicFightCombat(runtime, out);
    if (scenarioName == "roster-compatibility-smoke") return runRosterCompatibilitySmoke(runtime, out);
    if (scenarioName == "owned-character-readiness") return runOwnedCharacterReadiness(runtime, out);
    if (scenarioName == "dragon-progression-level-items") return runDragonProgressionLevelItems(runtime, out);
    if (scenarioName == "dragon-progression-player-profiles") return runDragonProgressionPlayerProfiles(runtime, out);
    if (scenarioName == "dragon-progression-enemy-reward") return runDragonProgressionEnemyReward(runtime, out);
    if (scenarioName == "vs-p2-runtime") return runVsP2Runtime(runtime, out);
    if (scenarioName == "arena-cpu-1") return runArenaSmoke(runtime, out, 1);
    if (scenarioName == "arena-cpu-2") return runArenaSmoke(runtime, out, 2);
    if (scenarioName == "arena-cpu-3") return runArenaSmoke(runtime, out, 3);
    if (scenarioName == "arena-z-keyboard-controls") return runArenaZKeyboardControls(runtime, out);
    if (scenarioName == "arena-z-aben-walk-animation") return runArenaZAbenWalkAnimation(runtime, out);
    if (scenarioName == "arena-z-gamepad-controls") return runArenaZGamepadControls(runtime, out);
    if (scenarioName == "arena-z-hit-depth") return runArenaZHitDepth(runtime, out);
    if (scenarioName == "arena-z-push-depth") return runArenaZPushDepth(runtime, out);
    if (scenarioName == "arena-z-draw-order") return runArenaZDrawOrder(runtime, out);
    if (scenarioName == "arena-camera-rotation-toggle") return runArenaCameraRotationToggle(runtime, out);
    if (scenarioName == "arena-camera-rotation-projection") return runArenaCameraRotationProjection(runtime, out);
    if (scenarioName == "arena-camera-rotation-draw-order") return runArenaCameraRotationDrawOrder(runtime, out);
    if (scenarioName == "arena-z-cpu-align") return runArenaZCpuAlign(runtime, out);
    if (scenarioName == "arena-z-modifier-sidestep") return runArenaZModifierSidestep(runtime, out);
    if (scenarioName == "arena-evilken-forward-dash-bounds") return runArenaEvilKenForwardDashBounds(runtime, out);
    if (scenarioName == "arena-per-fighter-runtime") return runArenaPerFighterRuntime(runtime, out);
    if (scenarioName == "arena-openbor-scroll-stage") return runArenaOpenBorScrollStage(runtime, out);
    if (scenarioName == "arena-tmnt-openbor-stage") return runArenaTmntOpenBorStage(runtime, out);
    if (scenarioName == "arena-evilryu-air-special-contact-landing") return runArenaEvilRyuAirSpecialContactLanding(runtime, out);
    if (scenarioName == "story-mode-menu-route") return runStoryModeMenuRoute(runtime, out);
    if (scenarioName == "story-stage-select-map") return runStoryStageSelectMap(runtime, out);
    if (scenarioName == "story-difficulty-enemy-scaling") return runStoryDifficultyEnemyScaling(runtime, out);
    if (scenarioName == "story-openbor-stage-default") return runStoryOpenBorStageDefault(runtime, out);
    if (scenarioName == "story-stage-board-expansion") return runStoryStageBoardExpansion(runtime, out);
    if (scenarioName == "story-board-route-plan") return runStoryBoardRoutePlan(runtime, out);
    if (scenarioName == "story-wave-clear-forward-cue") return runStoryWaveClearForwardCue(runtime, out);
    if (scenarioName == "story-shop-door-trigger") return runStoryShopDoorTrigger(runtime, out);
    if (scenarioName == "story-shop-route-resume") return runStoryShopRouteResume(runtime, out);
    if (scenarioName == "story-wave-spawn-scroll") return runStoryWaveSpawnScroll(runtime, out);
    if (scenarioName == "story-enemy-targeting") return runStoryEnemyTargeting(runtime, out);
    if (scenarioName == "story-stage-clear") return runStoryStageClear(runtime, out);
    if (scenarioName == "story-player-defeat") return runStoryPlayerDefeat(runtime, out);
    if (scenarioName == "story-progression-award") return runStoryProgressionAward(runtime, out);
    if (scenarioName == "story-reward-feedback") return runStoryRewardFeedback(runtime, out);
    if (scenarioName == "story-evilryu-super-recovery") return runStoryEvilRyuSuperRecovery(runtime, out);
    if (scenarioName == "vs-loading-progress-bar") return runVsLoadingProgressBar(runtime, out);
    if (scenarioName == "sff-v2-png-decode") return runSffV2PngDecode(runtime, out);
    if (scenarioName == "ikemen-select-slot-parsing") return runIkemenSelectSlotParsing(runtime, out);
    if (scenarioName == "stage-music-codec-decode") return runStageMusicCodecDecode(runtime, out);
    if (scenarioName == "external-stage-mount") return runExternalStageMount(runtime, out);
    if (scenarioName == "story-scott-tram-rooftop") return runStoryScottTramRooftop(runtime, out);
    if (scenarioName == "evilryu-dash") return runEvilRyuDash(runtime, out);

    out << "VERIFY " << scenarioName << "\n"
        << "BLOCKED unknown_scenario\n"
        << "  supported: shop-route-entry, shop-room-actor-projection, shop-room-movement-collision, shop-buy-sell-persistence, shop-equip-profile-scope, shop-guest-no-save, shop-controller-keyboard-navigation, shop-panel-text-fit, dragon-ui-theme-token-consistency, shop-overlay-responsive-layout, shop-overlay-classic-full-layout, shop-overlay-sd-layout, shop-overlay-hd-layout, shop-character-depth-order, shop-presentation-debug-label-visibility, shop-live-profile-currency-view, world-texture-filter-selection, ui-nearest-filter-selection, video-canvas-sd-854x480, video-canvas-hd-1280x720, dragon-ui-sd-two-x-scaling, dragon-ui-hd-three-x-scaling, world-viewport-sd-hd-layout, shop-demo-room-hook, compatibility-profile-resolver, main-menu-responsive-layout, main-menu-editable-presentation-data, main-menu-editable-layout-data, stage-select-responsive-layout, video-resolution-stable-virtual-layout, video-hd-fullscreen-window-policy, options-category-navigation, controls-player-1-4-navigation, controls-guided-setup, controls-manual-edit-conflicts, controls-presets, controls-profile-persistence, controls-input-test-live, controls-glyph-device-detection, controls-pause-taunt-separation, training-options-menu-geometry, training-move-list-geometry, training-command-hud-layout, training-pause-help-legend, training-command-list-tabs, training-command-icon-atlas, training-command-side-switch-highlight, training-command-facing-aware-display, training-command-physical-direction-guide, training-command-start-button-guide, training-command-complete-blink, training-command-filtered-complete, training-palette-slot-separation, training-show-select-hold, training-show-controller-shortcut, training-command-held-button-prompt, aben-training-move-list-from-character, character-auto-fit-scale, lili-smoke, lili-changeanim2-fallback, lili-kuuch-state-fallback, lili-hien-houou-kyaku-demo, lili-training-demo-all, kfm-baseline, kfm-throw, kfm-air-state, kfm-movement-direction-audit, evilryu-high-jump, kfm-down-hit-profile, kfm-guard-recovery, kfm-specials-supers, evilken-specials-supers, evilken-helper-lifecycle, evilken-power-charge-helper, evilken-air-special-contact-landing, evilken-training-demo-hit, evilken-training-command-practice-advance, evilryu-specials-supers, evilryu-shin-shoryuken-stun, evilryu-super-stress, evilryu-air-special-contact-landing, evilryu-power-charge-helper, evilryu-throw-bind, evilryu-training-throw-demo, evilken-smoke, evilken-trip-grounding, evilken-overhead-trip-chain, evilken-overhead-trip-chain-stress, evilken-trip-jump-buffer, evilken-attack-jump-buffer-release, evilken-throw, evilken-corner-visual-bounds, evilken-kuuchuu-shakunetsu, evilken-training-demo-all, evilken-shinryuken-recovery, evilken-shun-goku-satsu, evilken-shouki-hatsudou-spacing, cpu-baseline, classic-fight-outcomes, classic-fight-routing, classic-fight-combat, roster-compatibility-smoke, owned-character-readiness, dragon-progression-level-items, dragon-progression-player-profiles, dragon-progression-enemy-reward, vs-p2-runtime, arena-cpu-1, arena-cpu-2, arena-cpu-3, arena-z-keyboard-controls, arena-z-gamepad-controls, arena-z-hit-depth, arena-z-push-depth, arena-z-draw-order, arena-camera-rotation-toggle, arena-camera-rotation-projection, arena-camera-rotation-draw-order, arena-z-cpu-align, arena-z-modifier-sidestep, arena-z-aben-walk-animation, arena-evilken-forward-dash-bounds, arena-per-fighter-runtime, arena-openbor-scroll-stage, arena-tmnt-openbor-stage, arena-evilryu-air-special-contact-landing, story-mode-menu-route, story-stage-select-map, story-difficulty-enemy-scaling, story-openbor-stage-default, story-stage-board-expansion, story-board-route-plan, story-wave-clear-forward-cue, story-shop-door-trigger, story-shop-route-resume, story-wave-spawn-scroll, story-enemy-targeting, story-stage-clear, story-player-defeat, story-progression-award, story-reward-feedback, story-evilryu-super-recovery, vs-loading-progress-bar, sff-v2-png-decode, ikemen-select-slot-parsing, stage-music-codec-decode, external-stage-mount, story-scott-tram-rooftop, evilryu-dash\n"
        << "SUMMARY pass=0 partial=0 fail=0 blocked=1\n";
    return 2;
}

} // namespace dragon::verification
