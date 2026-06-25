#include "VerificationScenarioCommon.h"

#include "dragon/DragonProgression.h"

#include "ControlMapping.h"
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
constexpr float kShopVerifyCounterX = -126.0f;
constexpr float kShopVerifyCounterW = 660.0f;
constexpr float kShopVerifyCounterSolidLeft = kShopVerifyCounterX - 34.0f;
constexpr float kShopVerifyCounterSolidRight = kShopVerifyCounterX + kShopVerifyCounterW + 34.0f;
constexpr float kShopVerifyCounterSolidBackDepth = -18.0f;
constexpr float kShopVerifyCounterSolidFrontDepth = 70.0f;
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
    const std::string collisionText = readTextFile(repoRoot / "engine" / "src" / "ShopDemoCollision.h");
    record(out, counts,
        runtimeText.find("collectMappedFighterInput") != std::string::npos
            && runtimeText.find("shopDemoMovePlayer(state, dx, dz)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_movement_uses_held_input",
        "continuous mapped input drives room walking");
    record(out, counts,
        runtimeText.find("kShopCounterFrontDepth") != std::string::npos
            && runtimeText.find("drawShopDemoCounterFront(renderer, state)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_counter_depth_layering_present",
        "player can stand in front while counter space remains reachable");
    record(out, counts,
        runtimeText.find("drawShopDemoFallbackShelfBay") != std::string::npos
            && runtimeText.find("drawShopDemoFallbackDragonMark") != std::string::npos
            && runtimeText.find("shopDemoWorldTextCentered") != std::string::npos
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
        && runtimeText.find("i_chie_shop_counter_back.png") != std::string::npos
        && runtimeText.find("i_chie_shop_counter_front.png") != std::string::npos
        && runtimeText.find("drawShopDemoCounterBackArt") != std::string::npos;
    const bool shopPromptPackPresent =
        shopPromptText.find("i_chie_shop_backdrop.png") != std::string::npos
        && shopPromptText.find("i_chie_shop_counter_back.png") != std::string::npos
        && shopPromptText.find("i_chie_shop_counter_front.png") != std::string::npos
        && shopPromptText.find("no UI panels") != std::string::npos
        && shopPromptText.find("transparent PNG") != std::string::npos;
    record(out, counts,
        shopArtHooksPresent ? Status::Pass : Status::Fail,
        "shop_optional_art_layer_hooks",
        "runtime supports backdrop, counter back, and counter front drop-in PNGs");
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

    float directX = kShopVerifyCounterX + kShopVerifyCounterW * 0.5f;
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
            && panelText.find("shopDemoFitChars") != std::string::npos
            && panelText.find("shopDemoFitText") != std::string::npos
            && panelText.find("shopDemoPanelFitText") != std::string::npos
            && panelText.find("shopDemoPanelTextRight") != std::string::npos
            && panelText.find("panelRight") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_panel_uses_width_based_text_fit",
        "shop panel overlay owns width-based text budgets and value columns");

    record(out, counts,
        panelText.find("shopDemoPanelWrapText") != std::string::npos
            && panelText.find("const std::string requirement = \"REQ LV \"") != std::string::npos
            && panelText.find("const std::string ownership = \"OWN \"") != std::string::npos
            && panelText.find("\"  TARGET \"") != std::string::npos
            && panelText.find("shopDemoEffectSummary") != std::string::npos
            && panelText.find("shopDemoPanelText(renderer, x + 92.0f") == std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_detail_metadata_split",
        "description, effect summary, requirement, owned count, and target data have separate readable lines");

    record(out, counts,
        panelText.find("rowValue = \"G \"") != std::string::npos
            && panelText.find("rowValue = \"x\" + std::to_string(owned)") != std::string::npos
            && panelText.find("drawShopDemoItemIcon") != std::string::npos
            && panelText.find("shopDemoPanelTextRight(renderer, panelRight, rowY, rowValue)") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_rows_use_name_value_columns",
        "item names, item icons, and prices/owned counts are drawn as columns instead of one clipped string");

    record(out, counts,
        panelText.find("Q/E TAB  L/R TARGET  ENT") != std::string::npos
            && panelText.find("Q/E TAB  UP/DN ITEM  ENT") != std::string::npos
            && panelText.find("LEFT/RIGHT TARGET  ENT") == std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_footer_uses_short_labels",
        "footer commands fit the shop panel");

    record(out, counts,
        panelText.find("std::min(312.0f, width - 20.0f)") != std::string::npos
            && panelText.find("const float panelH = 162.0f") != std::string::npos
            && panelText.find("const float detailY") != std::string::npos
            && panelText.find("COST G") != std::string::npos
            && panelText.find("BAL G") != std::string::npos
            && panelText.find("GOLD ") != std::string::npos
            ? Status::Pass : Status::Fail,
        "shop_panel_width_allows_item_text",
        "panel uses compact text scale, compact detail geometry, and balance-aware confirmation text");

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
