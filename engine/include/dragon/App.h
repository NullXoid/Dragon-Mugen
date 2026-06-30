#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>

namespace dragon {

struct AppStartupOptions {
    std::string screen;
    std::string canvas;
    std::string optionsScreen;
    std::string performanceHud;
    int uiScalePercent = 0;
    bool hasShopPlayerX = false;
    float shopPlayerX = 0.0f;
    bool hasShopPlayerDepth = false;
    float shopPlayerDepth = 0.0f;
    bool shopOpen = false;
};

int runApp(const std::filesystem::path& gameRoot, const AppStartupOptions& startupOptions = {});
int runVerificationScenario(const std::filesystem::path& gameRoot, std::string_view scenarioName, std::ostream& out);

} // namespace dragon
