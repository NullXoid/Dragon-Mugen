#pragma once

#include "dragon/DragonProgression.h"

#include <algorithm>
#include <string>
#include <vector>

namespace dragon {

struct ShopCatalogEntry {
    std::string itemId;
    std::string name;
    std::string slot;
    std::string description;
    int requiredLevel = 1;
    int price = 0;
    int sellPrice = 0;
    int lifeBonus = 0;
    int powerBonus = 0;
    int attackPermille = 0;
    int defencePermille = 0;
};

inline ShopCatalogEntry shopCatalogEntryFromItem(const DragonItemDefinition& item) {
    ShopCatalogEntry entry;
    entry.itemId = item.id;
    entry.name = item.displayName.empty() ? item.id : item.displayName;
    entry.slot = item.slot;
    entry.description = item.description;
    entry.requiredLevel = std::max(1, item.requiredLevel);
    entry.price = std::max(0, item.price);
    entry.sellPrice = std::max(0, item.sellPrice);
    entry.lifeBonus = item.lifeBonus;
    entry.powerBonus = item.powerBonus;
    entry.attackPermille = item.attackPermille;
    entry.defencePermille = item.defencePermille;
    return entry;
}

inline std::vector<ShopCatalogEntry> buildDefaultShopCatalog(const DragonProgressionData& data) {
    std::vector<ShopCatalogEntry> entries;
    entries.reserve(data.items.size());
    for (const auto& item : data.items) {
        if (item.id.empty()) {
            continue;
        }
        auto entry = shopCatalogEntryFromItem(item);
        if (entry.price <= 0 && entry.sellPrice <= 0) {
            continue;
        }
        entries.push_back(std::move(entry));
    }
    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.requiredLevel != rhs.requiredLevel) return lhs.requiredLevel < rhs.requiredLevel;
        return lhs.name < rhs.name;
    });
    return entries;
}

} // namespace dragon
