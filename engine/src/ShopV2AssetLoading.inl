// Included inside ensureShopDemoAssets after the optional sprite loaders exist.
state.shopDemo.shopkeeperPoseV2 = loadOptionalShopSprite("data/shop/v2/ichie_welcome.png");
state.shopDemo.shopkeeperPoseV2.axisX = state.shopDemo.shopkeeperPoseV2.width / 2;
state.shopDemo.shopkeeperPoseV2.axisY = std::max(0, state.shopDemo.shopkeeperPoseV2.height - 2);
constexpr std::array<const char*, 10> kShopV2PropPaths{
    "data/shop/v2/shelf_cabinet.png",
    "data/shop/v2/ichie_banner.png",
    "data/shop/v2/dragon_emblem.png",
    "data/shop/v2/hologram_terminal.png",
    "data/shop/v2/neon_wall_panel.png",
    "data/shop/v2/large_crate.png",
    "data/shop/v2/medium_crate.png",
    "data/shop/v2/small_crate.png",
    "data/shop/v2/crystal_pedestal.png",
    "data/shop/v2/sword_stand.png",
};
for (std::size_t i = 0; i < state.shopDemo.shopV2Props.size(); ++i) {
    TextureSprite& prop = state.shopDemo.shopV2Props[i];
    prop = loadOptionalShopSprite(kShopV2PropPaths[i]);
    prop.axisX = prop.width / 2;
    prop.axisY = std::max(0, prop.height - 2);
}
