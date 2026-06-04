#include "dragon/MugenText.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace dragon {
namespace {

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::string stripComment(std::string_view line) {
    const auto pos = line.find(';');
    if (pos == std::string_view::npos) {
        return std::string(line);
    }
    return std::string(line.substr(0, pos));
}

} // namespace

std::string trim(std::string_view value) {
    auto begin = value.begin();
    auto end = value.end();
    while (begin != end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (begin != end && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(begin, end);
}

MugenDocument parseMugenTextFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open MUGEN text file: " + path.string());
    }

    MugenDocument doc;
    doc.path = path;

    MugenSection* current = nullptr;
    std::string raw;
    int lineNumber = 0;
    while (std::getline(input, raw)) {
        ++lineNumber;
        const std::string uncommented = stripComment(raw);
        const std::string line = trim(uncommented);
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            MugenSection section;
            section.name = trim(std::string_view(line).substr(1, line.size() - 2));
            section.line = lineNumber;
            doc.sections.push_back(std::move(section));
            current = &doc.sections.back();
            continue;
        }

        if (!current) {
            continue;
        }

        current->body.push_back(line);
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }

        MugenProperty property;
        property.key = trim(std::string_view(line).substr(0, equals));
        property.value = trim(std::string_view(line).substr(equals + 1));
        property.line = lineNumber;
        current->properties.push_back(std::move(property));
    }

    return doc;
}

const MugenSection* findSection(const MugenDocument& doc, std::string_view name) {
    const auto wanted = lower(name);
    for (const auto& section : doc.sections) {
        if (lower(section.name) == wanted) {
            return &section;
        }
    }
    return nullptr;
}

const MugenProperty* findProperty(const MugenSection& section, std::string_view key) {
    const auto wanted = lower(key);
    for (const auto& property : section.properties) {
        if (lower(property.key) == wanted) {
            return &property;
        }
    }
    return nullptr;
}

} // namespace dragon
