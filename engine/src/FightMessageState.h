#pragma once

#include <string>

namespace dragon {

struct FightMessageState {
    std::string lastHitText;
    int lastHitTextTicks = 0;
};

} // namespace dragon
