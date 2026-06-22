#include "FightDebugLog.h"

#include <SDL3/SDL.h>

#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>

namespace dragon {
namespace {

std::string lowerEnvValue(const char* value) {
    std::string out;
    if (!value) {
        return out;
    }
    for (const char ch : std::string_view(value)) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool debugFlagEnabled(const char* name) {
    const std::string value = lowerEnvValue(std::getenv(name));
    return !value.empty()
        && value != "0"
        && value != "false"
        && value != "off"
        && value != "no";
}

bool hitConsoleLogEnabled() {
    static const bool enabled = debugFlagEnabled("DRAGON_DEBUG_HIT_LOG") || debugFlagEnabled("DRAGON_HIT_LOG");
    return enabled;
}

} // namespace

void logFightHitEvent(std::string_view text) {
    if (!hitConsoleLogEnabled() || text.empty()) {
        return;
    }
    SDL_Log("%.*s", static_cast<int>(text.size()), text.data());
}

} // namespace dragon
