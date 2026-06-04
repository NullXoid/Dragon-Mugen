#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace dragon {

struct MugenProperty {
    std::string key;
    std::string value;
    int line = 0;
};

struct MugenSection {
    std::string name;
    int line = 0;
    std::vector<MugenProperty> properties;
    std::vector<std::string> body;
};

struct MugenDocument {
    std::filesystem::path path;
    std::vector<MugenSection> sections;
};

std::string trim(std::string_view value);
MugenDocument parseMugenTextFile(const std::filesystem::path& path);
const MugenSection* findSection(const MugenDocument& doc, std::string_view name);
const MugenProperty* findProperty(const MugenSection& section, std::string_view key);

} // namespace dragon
