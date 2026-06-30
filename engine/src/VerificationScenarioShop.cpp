#include "VerificationScenarioCommon.h"

#include "dragon/DragonProgression.h"
#include "ControlMapping.h"
#include "DragonUi.h"
#include "FrontendMenu.h"
#include "ShopCatalog.h"
#include "ShopDemoCollision.h"

#include <algorithm>
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

const ShopCatalogEntry* findShopEntry(
    const std::vector<ShopCatalogEntry>& catalog,
    std::string_view itemId) {
    const auto it = std::find_if(catalog.begin(), catalog.end(), [&](const auto& entry) {
        return lowercaseAsciiCopy(entry.itemId) == lowercaseAsciiCopy(itemId);
    });
    return it == catalog.end() ? nullptr : &*it;
}

bool hasBinding(const ControlProfileBinding& profile, InputAction action, PhysicalInputBinding binding) {
    const auto* actionBinding = findActionBinding(profile, action);
    if (!actionBinding) {
        return false;
    }
    return std::any_of(actionBinding->bindings.begin(), actionBinding->bindings.end(), [&](const auto& existing) {
        return samePhysicalInput(existing, binding);
    });
}

constexpr float kShopVerifyRoomLeft = -1120.0f;
constexpr float kShopVerifyRoomRight = 1120.0f;
constexpr float kShopVerifyPlayerMinX = -1040.0f;
constexpr float kShopVerifyPlayerMaxX = 1040.0f;
constexpr float kShopVerifyPlayerMinDepth = -82.0f;
constexpr float kShopVerifyPlayerMaxDepth = 118.0f;
constexpr float kShopVerifyCounterVisualCenterX = 238.0f;
constexpr float kShopVerifyCounterSolidHalfWidth = 112.0f;
constexpr float kShopVerifyCounterServiceHalfWidth = 126.0f;
constexpr float kShopVerifyShopkeeperX = 238.0f;
constexpr float kShopVerifyShopkeeperTalkHalfWidth = 64.0f;
constexpr float kShopVerifyCounterSolidLeft = kShopVerifyCounterVisualCenterX - kShopVerifyCounterSolidHalfWidth;
constexpr float kShopVerifyCounterSolidRight = kShopVerifyCounterVisualCenterX + kShopVerifyCounterSolidHalfWidth;
constexpr float kShopVerifyCounterSolidBackDepth = -18.0f;
constexpr float kShopVerifyCounterSolidFrontDepth = 70.0f;
constexpr float kShopVerifyCounterInteractMinDepth = kShopVerifyCounterSolidFrontDepth + 0.5f;
constexpr float kShopVerifyCounterInteractMaxDepth = kShopVerifyCounterSolidFrontDepth + 36.0f;
constexpr float kShopVerifyShopkeeperTalkMinDepth = kShopVerifyPlayerMinDepth;
constexpr float kShopVerifyShopkeeperTalkMaxDepth = kShopVerifyCounterInteractMaxDepth;
constexpr float kShopVerifyCollisionEpsilon = 0.5f;
constexpr float kShopVerifyWalkSpeed = 3.105f;
constexpr float kShopVerifyDepthSpeed = 2.115f;
constexpr float kShopVerifyRunMultiplier = 1.7f;
constexpr float kShopVerifyShopkeeperScale = 0.56f;

dragon::shop_demo::ShopCounterCollisionBounds shopVerifierCounterBounds() {
    return {
        kShopVerifyPlayerMinX,
        kShopVerifyPlayerMaxX,
        kShopVerifyPlayerMinDepth,
        kShopVerifyPlayerMaxDepth,
        kShopVerifyCounterSolidLeft,
        kShopVerifyCounterSolidRight,
        kShopVerifyCounterSolidBackDepth,
        kShopVerifyCounterSolidFrontDepth,
        kShopVerifyCollisionEpsilon,
    };
}

dragon::shop_demo::ShopInteractionVolume shopVerifierCounterServiceVolume() {
    return {
        kShopVerifyCounterVisualCenterX - kShopVerifyCounterServiceHalfWidth,
        kShopVerifyCounterVisualCenterX + kShopVerifyCounterServiceHalfWidth,
        kShopVerifyCounterInteractMinDepth,
        kShopVerifyCounterInteractMaxDepth,
    };
}

dragon::shop_demo::ShopInteractionVolume shopVerifierShopkeeperTalkVolume() {
    return {
        kShopVerifyShopkeeperX - kShopVerifyShopkeeperTalkHalfWidth,
        kShopVerifyShopkeeperX + kShopVerifyShopkeeperTalkHalfWidth,
        kShopVerifyShopkeeperTalkMinDepth,
        kShopVerifyShopkeeperTalkMaxDepth,
    };
}

void stepShopVerifierPlayer(float& x, float& depthZ, float dx, float dz) {
    const auto bounds = shopVerifierCounterBounds();
    float oldX = std::clamp(x, bounds.playerMinX, bounds.playerMaxX);
    float oldDepth = std::clamp(depthZ, bounds.playerMinDepth, bounds.playerMaxDepth);
    dragon::shop_demo::shopDemoSnapOutOfCounterSolid(bounds, oldX, oldDepth);
    float nextX = std::clamp(oldX + dx, bounds.playerMinX, bounds.playerMaxX);
    float nextDepth = std::clamp(oldDepth + dz, bounds.playerMinDepth, bounds.playerMaxDepth);
    dragon::shop_demo::shopDemoResolveCounterCollision(bounds, oldX, oldDepth, dx, dz, nextX, nextDepth);
    x = nextX;
    depthZ = nextDepth;
}

} // namespace

int runShopRouteEntry(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-route-entry\n";
    out << "root: " << runtime.rootText() << "\n";

    record(out, counts,
        decideMainMenuAction(5).kind == FrontendActionKind::OpenShopDemo ? Status::Pass : Status::Fail,
        "main_menu_index_opens_shop_hub",
        "index=5");
    record(out, counts,
        decideMainMenuAction(6).kind == FrontendActionKind::OpenOptions
            && decideMainMenuAction(7).kind == FrontendActionKind::ExitApp ? Status::Pass : Status::Fail,
        "following_main_menu_routes_stable",
        "options=6 exit=7");

    const auto gameRoot = std::filesystem::path(runtime.rootText());
    const auto repoRoot = gameRoot.filename() == "game" ? gameRoot.parent_path() : gameRoot;
    const auto spec = repoRoot / "docs" / "FEATURE_SPECS" / "0011_arena_shop_hub.md";
    const std::string specText = readTextFile(spec);
    record(out, counts,
        specText.find("Shop Hub") != std::string::npos
            && specText.find("buy, sell, equip, unequip") != std::string::npos ? Status::Pass : Status::Fail,
        "feature_spec_present",
        spec.string());

    summary(out, counts);
    return exitCode(counts);
}

int runShopRoomActorProjection(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-room-actor-projection\n";
    out << "root: " << runtime.rootText() << "\n";

    const auto root = std::filesystem::path(runtime.rootText());
    const auto shopkeeperPng = root / "chars" / "I.Chie" / "I.Chie_shopkeeper_pose.png";
    const auto shopDragonDef = root / "chars" / "I.Chie" / "I.Chie.dragon.def";
    const auto shopSff = root / "chars" / "I.Chie" / "I.Chie.sff";
    record(out, counts, std::filesystem::exists(shopkeeperPng) ? Status::Pass : Status::Fail,
        "shopkeeper_pose_png_exists",
        shopkeeperPng.string());
    record(out, counts, std::filesystem::exists(shopSff) ? Status::Pass : Status::Fail,
        "shopkeeper_sff_exists",
        shopSff.string());

    const std::string dragonDefText = readTextFile(shopDragonDef);
    const bool dragonDefTagged = dragonDefText.find("shopkeeper = 1") != std::string::npos
        && dragonDefText.find("shop.action = 9100") != std::string::npos
        && dragonDefText.find("shop.state = 9100") != std::string::npos;
    record(out, counts, dragonDefTagged ? Status::Pass : Status::Fail,
        "shopkeeper_dragon_metadata",
        dragonDefTagged ? "shopkeeper action/state metadata present" : "missing shopkeeper metadata");

    const auto characters = runtime.selectableCharacters();
    const bool iChieSelectable = std::any_of(characters.begin(), characters.end(), [](const RosterCharacterInfo& character) {
        return character.id == "I.Chie" || character.displayName.find("I.Chie") != std::string::npos;
    });
    record(out, counts, !iChieSelectable ? Status::Pass : Status::Fail,
        "shop_npc_not_selectable_roster",
        "selectable_count=" + std::to_string(characters.size()));

    const float roomWidth = kShopVerifyRoomRight - kShopVerifyRoomLeft;
    const float playerWalkWidth = kShopVerifyPlayerMaxX - kShopVerifyPlayerMinX;
    record(out, counts, roomWidth > static_cast<float>(kDefaultLogicalWidth) * 2.25f ? Status::Pass : Status::Fail,
        "shop_room_has_scroll_space",
        "room_width=" + std::to_string(roomWidth));
    record(out, counts,
        playerWalkWidth > static_cast<float>(kDefaultLogicalWidth) * 2.0f
            && kShopVerifyPlayerMaxDepth > kShopVerifyPlayerMinDepth ? Status::Pass : Status::Fail,
        "shop_room_has_walk_depth_space",
        "walk_width=" + std::to_string(playerWalkWidth)
            + " depth=" + std::to_string(kShopVerifyPlayerMinDepth)
            + ".." + std::to_string(kShopVerifyPlayerMaxDepth));
    record(out, counts, kShopVerifyShopkeeperScale < 0.65f ? Status::Pass : Status::Fail,
        "shop_characters_scaled_down",
        "shopkeeper_scale=" + std::to_string(kShopVerifyShopkeeperScale));

    const auto repoRoot = root.filename() == "game" ? root.parent_path() : root;
    const std::string runtimeText = readTextFile(repoRoot / "engine" / "src" / "ShopDemoRuntime.h");
    const std::string sceneText = readTextFile(repoRoot / "engine" / "src" / "ShopHubScene.h");
    const std::string collisionText = readTextFile(repoRoot / "engine" / "src" / "ShopDemoCollision.h");
    record(out, counts,
        runtimeText.find("collectMappedFighterInput") != std::string::npos
            && runtimeText.find("shopDemoMovePlayer(state, dx, dz)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_movement_uses_held_input",
        "continuous mapped input drives room walking");
    record(out, counts,
        runtimeText.find("shopDemoCounterCameraX") != std::string::npos
            && runtimeText.find("shopDemoVisiblePlayerMinX") != std::string::npos
            && runtimeText.find("shopDemoVisiblePlayerMaxX") != std::string::npos
            && runtimeText.find("shopDemoWorldZoomTarget") != std::string::npos
            && runtimeText.find("shopDemoWorldFocusTargetX") != std::string::npos
            && runtimeText.find("const float targetCamera = shopDemoClampCamera(state, shopDemoWorldFocusTargetX(state))") != std::string::npos
            && runtimeText.find("shopDemoUpdateCamera(state)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_camera_composes_counter_and_shop_open_zoom",
        "closed shop camera keeps counter composition; greeting/open states focus and zoom the world shot");
    record(out, counts,
        runtimeText.find("kShopCounterFrontDepth") != std::string::npos
            && runtimeText.find("drawShopDemoCounterFront(renderer, state)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_counter_depth_layering_present",
        "player can stand in front while counter space remains reachable");
    record(out, counts,
        sceneText.find("drawShopDemoFallbackShelfBay") != std::string::npos
            && sceneText.find("drawShopDemoFallbackDragonMark") != std::string::npos
            && sceneText.find("shopDemoWorldTextCentered") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_fallback_room_art_pass",
        "code-drawn fallback shop has shelf bays, neon branding, and layered room props");
    record(out, counts,
        collisionText.find("shopDemoInsideCounterSolid") != std::string::npos
            && runtimeText.find("shopDemoResolveCounterCollision") != std::string::npos
            && runtimeText.find("kShopCounterSolidBackDepth") != std::string::npos
            && runtimeText.find("kShopCounterSolidFrontDepth") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_counter_has_solid_collision",
        "counter blocks direct walk-through while preserving front/back lanes");
    record(out, counts,
        collisionText.find("shopDemoSnapOutOfCounterSolid") != std::string::npos
            && runtimeText.find("kShopPlayerMinDepth") != std::string::npos
            && runtimeText.find("kShopPlayerMaxDepth") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_back_route_around_counter",
        "top/back depth lane stays navigable around the counter");
    record(out, counts,
        runtimeText.find("kShopRunMultiplier") != std::string::npos
            && runtimeText.find("input.depthModifier") != std::string::npos
            && runtimeText.find("SHIFT/LT RUN") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_run_speed_modifier",
        "depth modifier acts as a non-combat shop run button");
    const auto shopPromptReadme = root / "data" / "shop" / "README.md";
    const std::string shopPromptText = readTextFile(shopPromptReadme);
    const bool shopArtHooksPresent =
        runtimeText.find("i_chie_shop_backdrop.png") != std::string::npos
        && runtimeText.find("i_chie_shop_counter_back.png") == std::string::npos
        && runtimeText.find("i_chie_shop_counter_front.png") != std::string::npos
        && runtimeText.find("drawShopDemoCounterBackArt") == std::string::npos
        && runtimeText.find("kShopCounterVisualCenterX") != std::string::npos
        && runtimeText.find("kShopCounterVisualDefaultAspect") != std::string::npos
        && sceneText.find("frontH * counterAspect()") != std::string::npos
        && sceneText.find("shopDemoCounterVisualBottomY(state)") != std::string::npos
        && sceneText.find("frontBottomY - frontH") != std::string::npos
        && runtimeText.find("kShopCounterW + 80.0f") == std::string::npos
        && sceneText.find("void drawShopTextureCoverVerticalAligned") != std::string::npos;
    const bool shopPromptPackPresent =
        shopPromptText.find("i_chie_shop_backdrop.png") != std::string::npos
        && shopPromptText.find("i_chie_shop_counter_back.png") == std::string::npos
        && shopPromptText.find("i_chie_shop_counter_front.png") != std::string::npos
        && shopPromptText.find("no UI panels") != std::string::npos
        && shopPromptText.find("transparent PNG") != std::string::npos;
    record(out, counts,
        shopArtHooksPresent ? Status::Pass : Status::Fail,
        "shop_optional_art_layer_hooks",
        "runtime supports backdrop and one placed front counter prop without drawing a second rear desk");
    record(out, counts,
        shopPromptPackPresent ? Status::Pass : Status::Fail,
        "shop_art_prompt_pack",
        shopPromptReadme.string());

    summary(out, counts);
    return exitCode(counts);
}

int runShopRoomMovementCollision(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-room-movement-collision\n";

    const auto bounds = shopVerifierCounterBounds();
    const float roomWidth = kShopVerifyRoomRight - kShopVerifyRoomLeft;
    const float walkDepth = kShopVerifyPlayerMaxDepth - kShopVerifyPlayerMinDepth;
    record(out, counts,
        roomWidth >= 2200.0f && walkDepth >= 190.0f ? Status::Pass : Status::Fail,
        "expanded_shop_floor_space",
        "room_width=" + std::to_string(roomWidth)
            + " depth=" + std::to_string(kShopVerifyPlayerMinDepth)
            + ".." + std::to_string(kShopVerifyPlayerMaxDepth));

    float directX = kShopVerifyCounterVisualCenterX;
    float directDepth = kShopVerifyCounterSolidFrontDepth + 18.0f;
    for (int i = 0; i < 90; ++i) {
        stepShopVerifierPlayer(directX, directDepth, 0.0f, -kShopVerifyDepthSpeed * kShopVerifyRunMultiplier);
    }
    record(out, counts,
        directDepth >= kShopVerifyCounterSolidFrontDepth
            && !dragon::shop_demo::shopDemoInsideCounterSolid(bounds, directX, directDepth)
            ? Status::Pass : Status::Fail,
        "direct_walk_through_counter_blocked",
        "x=" + std::to_string(directX) + " depth=" + std::to_string(directDepth));

    float frontX = kShopVerifyCounterSolidLeft - 110.0f;
    float frontDepth = kShopVerifyCounterSolidFrontDepth + 20.0f;
    for (int i = 0; i < 320; ++i) {
        stepShopVerifierPlayer(frontX, frontDepth, kShopVerifyWalkSpeed, 0.0f);
    }
    record(out, counts,
        frontX > kShopVerifyCounterSolidRight + 80.0f
            && frontDepth > kShopVerifyCounterSolidFrontDepth
            && !dragon::shop_demo::shopDemoInsideCounterSolid(bounds, frontX, frontDepth)
            ? Status::Pass : Status::Fail,
        "front_aisle_walks_past_counter",
        "x=" + std::to_string(frontX) + " depth=" + std::to_string(frontDepth));

    float backX = kShopVerifyCounterSolidLeft - 120.0f;
    float backDepth = kShopVerifyCounterSolidBackDepth - 34.0f;
    for (int i = 0; i < 320; ++i) {
        stepShopVerifierPlayer(backX, backDepth, kShopVerifyWalkSpeed, 0.0f);
    }
    record(out, counts,
        backX > kShopVerifyCounterSolidRight + 100.0f
            && backDepth < kShopVerifyCounterSolidBackDepth
            && !dragon::shop_demo::shopDemoInsideCounterSolid(bounds, backX, backDepth)
            ? Status::Pass : Status::Fail,
        "top_back_route_goes_around_counter",
        "x=" + std::to_string(backX) + " depth=" + std::to_string(backDepth));

    for (int i = 0; i < 90; ++i) {
        stepShopVerifierPlayer(backX, backDepth, 0.0f, kShopVerifyDepthSpeed * kShopVerifyRunMultiplier);
    }
    record(out, counts,
        backDepth > kShopVerifyCounterSolidFrontDepth
            && backX > kShopVerifyCounterSolidRight
            && !dragon::shop_demo::shopDemoInsideCounterSolid(bounds, backX, backDepth)
            ? Status::Pass : Status::Fail,
        "side_lane_connects_back_and_front",
        "x=" + std::to_string(backX) + " depth=" + std::to_string(backDepth));

    const float walkStep = kShopVerifyWalkSpeed;
    const float runStep = kShopVerifyWalkSpeed * kShopVerifyRunMultiplier;
    record(out, counts,
        runStep >= walkStep * 1.6f ? Status::Pass : Status::Fail,
        "run_speed_is_faster_than_walk",
        "walk=" + std::to_string(walkStep) + " run=" + std::to_string(runStep));

    const auto serviceVolume = shopVerifierCounterServiceVolume();
    const auto talkVolume = shopVerifierShopkeeperTalkVolume();
    const bool serviceContainsFrontCounter = dragon::shop_demo::shopDemoInsideInteractionVolume(
        serviceVolume,
        kShopVerifyCounterVisualCenterX,
        kShopVerifyCounterSolidFrontDepth + 18.0f);
    const bool serviceSkipsBehindCounter = !dragon::shop_demo::shopDemoInsideInteractionVolume(
        serviceVolume,
        kShopVerifyCounterVisualCenterX,
        kShopVerifyCounterSolidBackDepth - 24.0f);
    record(out, counts,
        serviceContainsFrontCounter
            && serviceSkipsBehindCounter
            ? Status::Pass : Status::Fail,
        "counter_service_hitbox_available",
        "counter x=" + std::to_string(kShopVerifyCounterVisualCenterX) + " wins on the customer-side overlap");
    record(out, counts,
        dragon::shop_demo::shopDemoInsideInteractionVolume(
            talkVolume,
            kShopVerifyShopkeeperX,
            kShopVerifyCounterSolidFrontDepth + 18.0f)
            && dragon::shop_demo::shopDemoInsideInteractionVolume(
                talkVolume,
                kShopVerifyShopkeeperX,
                kShopVerifyCounterSolidBackDepth - 24.0f)
            ? Status::Pass : Status::Fail,
        "shopkeeper_talk_hitbox_available",
        "shopkeeper x=" + std::to_string(kShopVerifyShopkeeperX) + " supports front and behind-counter talk");

    summary(out, counts);
    return exitCode(counts);
}

int runShopBuySellPersistence(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-buy-sell-persistence\n";
    out << "root: " << runtime.rootText() << "\n";

    DragonProgressionData data;
    try {
        data = loadDragonProgressionData(std::filesystem::path(runtime.rootText()));
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "load_progression_data", ex.what());
        summary(out, counts);
        return exitCode(counts);
    }
    const auto catalog = buildDefaultShopCatalog(data);
    const auto* entry = findShopEntry(catalog, "training_weight");
    record(out, counts, entry && entry->price > 0 && entry->sellPrice > 0 ? Status::Pass : Status::Fail,
        "priced_shop_catalog_entry",
        entry ? ("price=" + std::to_string(entry->price) + " sell=" + std::to_string(entry->sellPrice)) : "missing training_weight");
    if (!entry) {
        summary(out, counts);
        return exitCode(counts);
    }

    DragonProgressionSave save;
    setDragonProgressionPlayerProfile(save, 0, "Shop Verifier");
    const std::string profileId = dragonProgressionPlayerProfileId(save, 0);
    const int startingGold = dragonProgressionGoldForProfile(save, profileId);
    const bool spent = spendDragonProgressionGoldForProfile(save, profileId, entry->price);
    if (spent) {
        grantDragonProgressionItemForProfile(save, profileId, entry->itemId, 1);
    }
    record(out, counts,
        spent
            && dragonProgressionGoldForProfile(save, profileId) == startingGold - entry->price
            && inventoryEntryForProfile(save, profileId, entry->itemId).has_value()
            ? Status::Pass : Status::Fail,
        "buy_spends_gold_and_grants_item",
        "gold=" + std::to_string(dragonProgressionGoldForProfile(save, profileId))
            + " item=" + entry->itemId);

    const auto path = std::filesystem::temp_directory_path() / "dragon_mugen_shop_buy_sell_verify.def";
    try {
        saveDragonProgressionSave(path, save);
        DragonProgressionSave loaded = loadDragonProgressionSave(path);
        record(out, counts,
            dragonProgressionGoldForProfile(loaded, profileId) == startingGold - entry->price
                && inventoryEntryForProfile(loaded, profileId, entry->itemId).has_value()
                ? Status::Pass : Status::Fail,
            "buy_round_trip_persists",
            path.string());

        const bool removed = removeDragonProgressionItemForProfile(loaded, profileId, entry->itemId, 1);
        if (removed) {
            addDragonProgressionGoldForProfile(loaded, profileId, entry->sellPrice);
        }
        saveDragonProgressionSave(path, loaded);
        const DragonProgressionSave sold = loadDragonProgressionSave(path);
        record(out, counts,
            removed
                && !inventoryEntryForProfile(sold, profileId, entry->itemId).has_value()
                && dragonProgressionGoldForProfile(sold, profileId) == startingGold - entry->price + entry->sellPrice
                ? Status::Pass : Status::Fail,
            "sell_removes_item_and_persists_gold",
            "gold=" + std::to_string(dragonProgressionGoldForProfile(sold, profileId)));
        std::error_code ec;
        std::filesystem::remove(path, ec);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Fail, "buy_sell_round_trip", ex.what());
    }

    summary(out, counts);
    return exitCode(counts);
}

int runShopEquipProfileScope(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-equip-profile-scope\n";
    out << "root: " << runtime.rootText() << "\n";

    DragonProgressionData data;
    try {
        data = loadDragonProgressionData(std::filesystem::path(runtime.rootText()));
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "load_progression_data", ex.what());
        summary(out, counts);
        return exitCode(counts);
    }

    DragonProgressionSave save;
    setDragonProgressionPlayerProfile(save, 0, "Shop Equip One");
    setDragonProgressionPlayerProfile(save, 1, "Shop Equip Two");
    const std::string p1 = dragonProgressionPlayerProfileId(save, 0);
    const std::string p2 = dragonProgressionPlayerProfileId(save, 1);
    grantDragonProgressionItemForProfile(save, p1, "training_weight", 1);
    std::string reason;
    const bool p1Equipped = equipDragonProgressionItemForProfile(data, save, p1, "kfm", "training_weight", &reason);
    record(out, counts,
        p1Equipped && isDragonProgressionItemEquippedForProfile(save, p1, "kfm", "training_weight")
            ? Status::Pass : Status::Fail,
        "p1_can_equip_owned_item",
        reason);

    std::string p2Reason;
    const bool p2Equipped = equipDragonProgressionItemForProfile(data, save, p2, "kfm", "training_weight", &p2Reason);
    record(out, counts,
        !p2Equipped && !isDragonProgressionItemEquippedForProfile(save, p2, "kfm", "training_weight")
            ? Status::Pass : Status::Fail,
        "p2_cannot_equip_unowned_p1_item",
        p2Reason);

    const bool unequipped = unequipDragonProgressionItemForProfile(save, p1, "kfm", "training_weight");
    record(out, counts,
        unequipped && !isDragonProgressionItemEquippedForProfile(save, p1, "kfm", "training_weight")
            ? Status::Pass : Status::Fail,
        "unequip_removes_profile_character_item",
        "p1=" + p1 + " p2=" + p2);

    summary(out, counts);
    return exitCode(counts);
}

int runShopGuestNoSave(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-guest-no-save\n";
    out << "root: " << runtime.rootText() << "\n";

    DragonProgressionData data;
    try {
        data = loadDragonProgressionData(std::filesystem::path(runtime.rootText()));
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "load_progression_data", ex.what());
        summary(out, counts);
        return exitCode(counts);
    }

    DragonProgressionSave save;
    const std::string guest = dragonProgressionGuestProfileId();
    addDragonProgressionGoldForProfile(save, guest, 500);
    const bool spent = spendDragonProgressionGoldForProfile(save, guest, 10);
    grantDragonProgressionItemForProfile(save, guest, "training_weight", 1);
    std::string reason;
    const bool equipped = equipDragonProgressionItemForProfile(data, save, guest, "kfm", "training_weight", &reason);
    const bool removed = removeDragonProgressionItemForProfile(save, guest, "training_weight", 1);
    const bool guestCreated = findDragonProgressionProfile(save, guest) != nullptr;
    record(out, counts,
        !spent
            && !equipped
            && !removed
            && !guestCreated
            && dragonProgressionGoldForProfile(save, guest) == 0
            && !inventoryEntryForProfile(save, guest, "training_weight").has_value()
            ? Status::Pass : Status::Fail,
        "guest_transactions_do_not_persist",
        reason);

    summary(out, counts);
    return exitCode(counts);
}

int runShopControllerKeyboardNavigation(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-controller-keyboard-navigation\n";

    const ControlProfileBinding profile = makeDefaultControlProfile("shop-player", 0);
    const bool directionsAvailable =
        hasBinding(profile, InputAction::MoveLeft, keyBinding(SDL_SCANCODE_LEFT))
        && hasBinding(profile, InputAction::MoveRight, keyBinding(SDL_SCANCODE_RIGHT))
        && hasBinding(profile, InputAction::MoveUp, keyBinding(SDL_SCANCODE_UP))
        && hasBinding(profile, InputAction::MoveDown, keyBinding(SDL_SCANCODE_DOWN))
        && hasBinding(profile, InputAction::MoveLeft, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        && hasBinding(profile, InputAction::MoveRight, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        && hasBinding(profile, InputAction::MoveUp, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_UP))
        && hasBinding(profile, InputAction::MoveDown, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_DOWN));
    record(out, counts, directionsAvailable ? Status::Pass : Status::Fail,
        "keyboard_controller_directions_available",
        "shop uses the same menu/fighting movement directions");

    const bool pauseTauntSeparate =
        hasBinding(profile, InputAction::Pause, keyBinding(SDL_SCANCODE_RETURN))
        && hasBinding(profile, InputAction::Pause, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_START))
        && hasBinding(profile, InputAction::Taunt, gamepadButtonBinding(SDL_GAMEPAD_BUTTON_TOUCHPAD));
    record(out, counts, pauseTauntSeparate ? Status::Pass : Status::Fail,
        "pause_and_taunt_remain_distinct_for_shop",
        "start/options is pause/menu confirm, touchpad remains taunt-capable elsewhere");

    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string runtimeText = readTextFile(root / "engine" / "src" / "ShopDemoRuntime.h");
    const std::string flowText = readTextFile(root / "engine" / "src" / "FrontendFlow.h");
    record(out, counts,
        runtimeText.find("SDL_GAMEPAD_AXIS_LEFT_TRIGGER") != std::string::npos
            && runtimeText.find("SDL_GAMEPAD_AXIS_RIGHT_TRIGGER") != std::string::npos
            && flowText.find("SDL_GAMEPAD_BUTTON_LEFT_SHOULDER") != std::string::npos
            && flowText.find("SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_tabs_support_shoulders_and_triggers",
        "L1/R1 and L2/R2 switch buy/sell/equip");
    record(out, counts,
        runtimeText.find("shopDemoCycleEquipCharacter") != std::string::npos
            && runtimeText.find("selectedEquipCharacter") != std::string::npos
            && runtimeText.find("drawShopDemoTransactionBanner") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_equip_target_and_transaction_feedback",
        "equip target selection and large transaction banner are present");

    record(out, counts,
        decideMainMenuAction(5).kind == FrontendActionKind::OpenShopDemo ? Status::Pass : Status::Fail,
        "enter_action_reaches_shop_route",
        "main menu index 5");

    summary(out, counts);
    return exitCode(counts);
}

int runShopPanelTextFit(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY shop-panel-text-fit\n";

    const auto root = std::filesystem::path(".").lexically_normal();
    const std::string runtimeText = readTextFile(root / "engine" / "src" / "ShopDemoRuntime.h");
    const std::string panelText = readTextFile(root / "engine" / "src" / "ShopDemoPanelOverlay.h");
    const std::string shopText = runtimeText + "\n" + panelText;
    record(out, counts,
        shopText.find("ShopDemoPanelOverlay.h") != std::string::npos
            && panelText.find("shopDemoPanelFitText") != std::string::npos
            && panelText.find("shopDemoPanelFitChars") != std::string::npos
            && panelText.find("shopDemoPanelWrapText") != std::string::npos
            && panelText.find("shopDemoPanelTextRight") != std::string::npos
            && panelText.find("panelRight") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_panel_uses_width_based_text_fit",
        "shop panel overlay owns width-based text budgets and value columns");

    record(out, counts,
        panelText.find("shopDemoPanelWrapText") != std::string::npos
            && panelText.find("\"REQUIRED LEVEL\"") != std::string::npos
            && panelText.find("\"OWNED\"") != std::string::npos
            && panelText.find("\"TARGET\"") != std::string::npos
            && panelText.find("drawMeta") != std::string::npos
            && panelText.find("shopDemoEffectSummary") != std::string::npos
            && panelText.find("shopDemoPanelText(renderer, x + 92.0f") == std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_detail_metadata_split",
        "description, effect summary, requirement, owned count, and target data have separate readable lines");

    record(out, counts,
        panelText.find("rowValue = item.affordable") != std::string::npos
            && panelText.find("\"x\" + std::to_string(item.ownedCount)") != std::string::npos
            && panelText.find("drawShopDemoItemIcon") != std::string::npos
            && panelText.find("shopDemoPanelTextRight(renderer, panelRight - 2.0f * layout.scale, rowY + 3.0f * layout.scale, rowValue") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_rows_use_name_value_columns",
        "item names, item icons, and prices/owned counts are drawn as columns instead of one clipped string");

    record(out, counts,
        panelText.find("Q/E LB/RB TAB  L/R TARGET") != std::string::npos
            && panelText.find("Q/E LB/RB TAB  UP/DOWN ITEM") != std::string::npos
            && panelText.find("LEFT/RIGHT TARGET  ENT") == std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_footer_uses_short_labels",
        "footer commands fit the shop panel");

    record(out, counts,
        panelText.find("ShopPanelLayoutMode") != std::string::npos
            && panelText.find("FullClassic") != std::string::npos
            && panelText.find("RightCompact") != std::string::npos
            && panelText.find("StandardDefinition") != std::string::npos
            && panelText.find("HighDefinition") != std::string::npos
            && panelText.find("const float detailY") != std::string::npos
            && panelText.find("COST G") != std::string::npos
            && panelText.find("BAL G") != std::string::npos
            && panelText.find("DragonCurrencyView") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_panel_responsive_layouts",
        "panel has classic/right/SD/HD layouts, compact detail geometry, and balance-aware confirmation text");

    summary(out, counts);
    return exitCode(counts);
}

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
        metrics.pixelScale == 1.0f
            && metrics.topBarH == 24.0f
            && panelText.find("320.0f, 335.0f") != std::string::npos
            ? Status::Pass : Status::Fail,
        "sd_panel_and_stable_density",
        "854x480 is an output preset; Dragon UI keeps the stable presentation grid");
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
        metrics.pixelScale == 1.0f
            && metrics.topBarH == 24.0f
            && panelText.find("470.0f, 486.0f") != std::string::npos
            ? Status::Pass : Status::Fail,
        "hd_panel_and_stable_density",
        "1280x720 is an output preset; Dragon UI keeps the stable presentation grid");
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
            && loadingText.find("TextureFilter filter = TextureFilter::Nearest") != std::string::npos && loadingText.find("setTextureSpriteFilterIntent(sprite, filter)") != std::string::npos
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
        metrics.pixelScale == 1.0f
            && metrics.rowH == 18.0f
            && safe.x == 0.0f && safe.w == 854.0f && safe.h == 480.0f
            ? Status::Pass : Status::Fail,
        "sd_output_preset_keeps_stable_ui_density",
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
        metrics.pixelScale == 1.0f
            && metrics.rowH == 18.0f
            && safe.x == 0.0f && safe.w == 1280.0f && safe.h == 720.0f
            ? Status::Pass : Status::Fail,
        "hd_output_preset_keeps_stable_ui_density",
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
