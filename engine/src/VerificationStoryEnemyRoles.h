#pragma once

#include "StoryBoardPlan.h"

#include <string>
#include <string_view>
#include <vector>

namespace dragon::verification {

inline std::string firstRoleRef(const std::vector<std::string>& refs) {
    return refs.empty() ? std::string{} : refs.front();
}

inline bool storyVerifierContainsNoCase(const std::string& value, std::string_view needle) {
    return storyBoardLowercase(value).find(storyBoardLowercase(needle)) != std::string::npos;
}

inline bool characterRefMatchesSnapshot(const std::string& characterId, const std::string& characterName, std::string_view ref) {
    return !ref.empty()
        && (storyBoardEqualsNoCase(characterId, ref)
            || storyVerifierContainsNoCase(characterId, ref)
            || storyVerifierContainsNoCase(characterName, ref));
}

inline bool storyRouteEnemySetupConfigured(const StoryBoardRoute& route, const StoryBoardNode* roleNode) {
    const std::string gruntRef = firstRoleRef(route.enemySetup.grunts);
    const std::string miniBossRef = firstRoleRef(route.enemySetup.miniBosses);
    const std::string bossRef = firstRoleRef(route.enemySetup.bosses);
    return roleNode
        && !gruntRef.empty()
        && !miniBossRef.empty()
        && !bossRef.empty()
        && roleNode->regularEnemyRef == gruntRef
        && roleNode->midBossEnemyRef == miniBossRef
        && roleNode->bossEnemyRef == bossRef;
}

inline std::string storyRouteEnemySetupDetail(const StoryBoardRoute& route, const StoryBoardNode* roleNode) {
    return "grunts=" + firstRoleRef(route.enemySetup.grunts)
        + " mini_bosses=" + firstRoleRef(route.enemySetup.miniBosses)
        + " bosses=" + firstRoleRef(route.enemySetup.bosses)
        + (roleNode
                ? " node_regular=" + roleNode->regularEnemyRef
                    + " node_mini=" + roleNode->midBossEnemyRef
                    + " node_boss=" + roleNode->bossEnemyRef
                : " missing side-scroller board");
}

} // namespace dragon::verification
