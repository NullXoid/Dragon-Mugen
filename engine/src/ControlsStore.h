#pragma once

#include "ControlMapping.h"

#include <filesystem>

namespace dragon {

std::filesystem::path dragonControlsSavePath(const std::filesystem::path& gameRoot);
ControlsSettings loadControlsSettings(const std::filesystem::path& path);
void saveControlsSettings(const std::filesystem::path& path, const ControlsSettings& controls);

} // namespace dragon
