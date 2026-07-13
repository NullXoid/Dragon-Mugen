#include "dragon/DragonProgression.h"

#include "dragon/MugenText.h"

#include <SDL3/SDL_stdinc.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace dragon {
namespace {

std::string lowercase(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    return lowercase(lhs) == lowercase(rhs);
}

std::string firstEnvironmentValue(std::initializer_list<const char*> names) {
    for (const char* name : names) {
        const char* value = SDL_getenv(name);
        if (value && !trim(value).empty()) {
            return trim(value);
        }
    }
    return {};
}

std::vector<std::string> selectableProfileIds(const DragonProgressionSave& save, int playerIndex) {
    const int safePlayer = std::clamp(playerIndex, 0, kDragonProgressionPlayerCount - 1);
    const int otherPlayer = safePlayer == 0 ? 1 : 0;
    const std::string otherProfile = dragonProgressionPlayerProfileId(save, otherPlayer);

    std::vector<std::string> ids;
    if (safePlayer == 1) {
        ids.push_back(dragonProgressionGuestProfileId());
    }
    for (const auto& profile : save.profiles) {
        const std::string id = normalizeDragonProgressionProfileId(profile.id.empty() ? profile.displayName : profile.id);
        if (id.empty() || isDragonProgressionGuestProfile(id)) {
            continue;
        }
        if (!isDragonProgressionGuestProfile(otherProfile) && equalsNoCase(id, otherProfile)) {
            continue;
        }
        if (std::find_if(ids.begin(), ids.end(), [&](const auto& existing) {
                return equalsNoCase(existing, id);
            }) == ids.end()) {
            ids.push_back(id);
        }
    }
    return ids;
}

} // namespace

std::string dragonProgressionGuestProfileId() {
    return "guest";
}

std::string dragonProgressionGuestProfileName() {
    return "Guest";
}

bool isDragonProgressionGuestProfile(std::string_view profileId) {
    return equalsNoCase(normalizeDragonProgressionProfileId(profileId), dragonProgressionGuestProfileId());
}

std::string defaultDragonProgressionProfileName() {
    std::string name = firstEnvironmentValue({ "DRAGON_MUGEN_PROFILE", "USERNAME", "USER" });
    if (name.empty()) {
        name = "Player 1";
    }
    return name;
}

std::string normalizeDragonProgressionProfileId(std::string_view profileName) {
    std::string source = trim(profileName);
    if (source.empty()) {
        source = defaultDragonProgressionProfileName();
    }

    std::string out;
    bool previousSeparator = false;
    for (unsigned char ch : source) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
            previousSeparator = false;
        } else if (ch == '_' || ch == '-' || std::isspace(ch)) {
            if (!previousSeparator && !out.empty()) {
                out.push_back('_');
                previousSeparator = true;
            }
        }
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out.empty() ? "player1" : out;
}

const DragonProgressionProfile* findDragonProgressionProfile(
    const DragonProgressionSave& save,
    std::string_view profileId) {
    const std::string id = normalizeDragonProgressionProfileId(profileId);
    for (const auto& profile : save.profiles) {
        if (equalsNoCase(profile.id, id)) {
            return &profile;
        }
    }
    return nullptr;
}

DragonProgressionProfile& ensureDragonProgressionProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view displayName) {
    std::string resolvedDisplayName = trim(displayName);
    if (resolvedDisplayName.empty()) {
        resolvedDisplayName = trim(profileId);
    }
    if (resolvedDisplayName.empty()) {
        resolvedDisplayName = defaultDragonProgressionProfileName();
    }
    const std::string id = normalizeDragonProgressionProfileId(profileId.empty() ? resolvedDisplayName : profileId);
    if (save.activeProfileId.empty()) {
        save.activeProfileId = id;
    }
    for (auto& profile : save.profiles) {
        if (equalsNoCase(profile.id, id)) {
            profile.id = id;
            if (trim(profile.displayName).empty()) {
                profile.displayName = resolvedDisplayName;
            }
            return profile;
        }
    }
    DragonProgressionProfile profile;
    profile.id = id;
    profile.displayName = resolvedDisplayName;
    save.profiles.push_back(std::move(profile));
    return save.profiles.back();
}

std::string createNextDragonProgressionProfile(
    DragonProgressionSave& save,
    std::string_view preferredBaseName) {
    std::string base = trim(preferredBaseName);
    if (base.empty() || equalsNoCase(base, dragonProgressionGuestProfileName())) {
        base = "Player";
    }

    for (int suffix = 1; suffix < 1000; ++suffix) {
        const std::string name = base + " " + std::to_string(suffix);
        const std::string id = normalizeDragonProgressionProfileId(name);
        if (!findDragonProgressionProfile(save, id) && !isDragonProgressionGuestProfile(id)) {
            auto& profile = ensureDragonProgressionProfile(save, id, name);
            return profile.id;
        }
    }

    auto& profile = ensureDragonProgressionProfile(save, base + " Extra", base + " Extra");
    return profile.id;
}

const DragonProgressionProfile* activeDragonProgressionProfile(const DragonProgressionSave& save) {
    if (!save.activeProfileId.empty()) {
        for (const auto& profile : save.profiles) {
            if (equalsNoCase(profile.id, save.activeProfileId)) {
                return &profile;
            }
        }
    }
    return save.profiles.empty() ? nullptr : &save.profiles.front();
}

DragonProgressionProfile& activeDragonProgressionProfile(DragonProgressionSave& save) {
    if (save.activeProfileId.empty()) {
        return ensureDragonProgressionProfile(save);
    }
    for (auto& profile : save.profiles) {
        if (equalsNoCase(profile.id, save.activeProfileId)) {
            return profile;
        }
    }
    return ensureDragonProgressionProfile(save, save.activeProfileId, save.activeProfileId);
}

void setActiveDragonProgressionProfile(DragonProgressionSave& save, std::string_view profileName) {
    auto& profile = ensureDragonProgressionProfile(save, profileName, profileName);
    save.activeProfileId = profile.id;
    save.playerProfileIds[0] = profile.id;
    if (equalsNoCase(save.playerProfileIds[1], profile.id)) {
        save.playerProfileIds[1] = dragonProgressionGuestProfileId();
    }
}

std::string dragonProgressionProfileDisplayName(const DragonProgressionSave& save) {
    if (const auto* profile = activeDragonProgressionProfile(save)) {
        if (!trim(profile->displayName).empty()) {
            return trim(profile->displayName);
        }
        if (!profile->id.empty()) {
            return profile->id;
        }
    }
    return defaultDragonProgressionProfileName();
}

std::string dragonProgressionProfileDisplayName(
    const DragonProgressionSave& save,
    std::string_view profileId) {
    if (isDragonProgressionGuestProfile(profileId)) {
        return dragonProgressionGuestProfileName();
    }
    if (const auto* profile = findDragonProgressionProfile(save, profileId)) {
        if (!trim(profile->displayName).empty()) {
            return trim(profile->displayName);
        }
        return profile->id;
    }
    const std::string id = normalizeDragonProgressionProfileId(profileId);
    return id.empty() ? defaultDragonProgressionProfileName() : id;
}

std::string dragonProgressionPlayerProfileId(
    const DragonProgressionSave& save,
    int playerIndex) {
    const int safePlayer = std::clamp(playerIndex, 0, kDragonProgressionPlayerCount - 1);
    std::string id = normalizeDragonProgressionProfileId(save.playerProfileIds[static_cast<size_t>(safePlayer)]);
    if (safePlayer == 1 && id.empty()) {
        return dragonProgressionGuestProfileId();
    }
    if (safePlayer == 1 && isDragonProgressionGuestProfile(id)) {
        return dragonProgressionGuestProfileId();
    }
    if (!id.empty() && findDragonProgressionProfile(save, id)) {
        return id;
    }
    if (safePlayer == 1) {
        return dragonProgressionGuestProfileId();
    }
    if (!save.activeProfileId.empty() && findDragonProgressionProfile(save, save.activeProfileId)) {
        return normalizeDragonProgressionProfileId(save.activeProfileId);
    }
    if (const auto* profile = activeDragonProgressionProfile(save)) {
        return normalizeDragonProgressionProfileId(profile->id);
    }
    return safePlayer == 0
        ? normalizeDragonProgressionProfileId(defaultDragonProgressionProfileName())
        : dragonProgressionGuestProfileId();
}

std::string dragonProgressionPlayerProfileDisplayName(
    const DragonProgressionSave& save,
    int playerIndex) {
    return dragonProgressionProfileDisplayName(save, dragonProgressionPlayerProfileId(save, playerIndex));
}

void setDragonProgressionPlayerProfile(
    DragonProgressionSave& save,
    int playerIndex,
    std::string_view profileName) {
    const int safePlayer = std::clamp(playerIndex, 0, kDragonProgressionPlayerCount - 1);
    if (safePlayer == 1 && isDragonProgressionGuestProfile(profileName)) {
        save.playerProfileIds[1] = dragonProgressionGuestProfileId();
        return;
    }

    std::string displayName = trim(profileName);
    if (displayName.empty()) {
        displayName = safePlayer == 0 ? defaultDragonProgressionProfileName() : dragonProgressionGuestProfileName();
    }
    if (safePlayer == 0 && isDragonProgressionGuestProfile(displayName)) {
        displayName = defaultDragonProgressionProfileName();
    }

    auto& profile = ensureDragonProgressionProfile(save, displayName, displayName);
    if (safePlayer == 0) {
        save.playerProfileIds[0] = profile.id;
        save.activeProfileId = profile.id;
        if (equalsNoCase(save.playerProfileIds[1], profile.id)) {
            save.playerProfileIds[1] = dragonProgressionGuestProfileId();
        }
        return;
    }

    const std::string p1Profile = dragonProgressionPlayerProfileId(save, 0);
    save.playerProfileIds[1] = equalsNoCase(profile.id, p1Profile)
        ? dragonProgressionGuestProfileId()
        : profile.id;
}

void cycleDragonProgressionPlayerProfile(
    DragonProgressionSave& save,
    int playerIndex,
    int direction) {
    if (direction == 0) {
        return;
    }
    const int safePlayer = std::clamp(playerIndex, 0, kDragonProgressionPlayerCount - 1);
    ensureDragonProgressionProfile(save);
    auto ids = selectableProfileIds(save, safePlayer);
    if (ids.empty()) {
        ids.push_back(ensureDragonProgressionProfile(save).id);
    }

    const std::string current = dragonProgressionPlayerProfileId(save, safePlayer);
    auto currentIt = std::find_if(ids.begin(), ids.end(), [&](const auto& id) {
        return equalsNoCase(id, current);
    });
    int index = currentIt == ids.end() ? 0 : static_cast<int>(std::distance(ids.begin(), currentIt));
    index = (index + direction + static_cast<int>(ids.size())) % static_cast<int>(ids.size());
    setDragonProgressionPlayerProfile(save, safePlayer, ids[static_cast<size_t>(index)]);
}

} // namespace dragon
