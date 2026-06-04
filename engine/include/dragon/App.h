#pragma once

#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace dragon {

int runApp(const std::filesystem::path& gameRoot);
int runVerificationScenario(const std::filesystem::path& gameRoot, std::string_view scenarioName, std::ostream& out);

} // namespace dragon
