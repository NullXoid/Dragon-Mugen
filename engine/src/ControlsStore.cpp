#include "ControlsStore.h"

#include "dragon/MugenText.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace dragon {
namespace {

std::string lowercaseAscii(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    return lowercaseAscii(lhs) == lowercaseAscii(rhs);
}

bool startsWithNoCase(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size()
        && equalsNoCase(value.substr(0, prefix.size()), prefix);
}

int parseInt(std::string_view value, int fallback) {
    try {
        size_t consumed = 0;
        const int parsed = std::stoi(trim(value), &consumed, 10);
        return consumed == 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

std::string propertyString(const MugenSection& section, std::string_view key, std::string fallback = {}) {
    if (const auto* property = findProperty(section, key)) {
        return trim(property->value);
    }
    return fallback;
}

int propertyInt(const MugenSection& section, std::string_view key, int fallback) {
    if (const auto* property = findProperty(section, key)) {
        return parseInt(property->value, fallback);
    }
    return fallback;
}

InputActionSet parseActionSet(std::string_view value, InputActionSet fallback) {
    const std::string normalized = lowercaseAscii(trim(value));
    if (normalized == "arena" || normalized == "flying dragon" || normalized == "arena / flying dragon") {
        return InputActionSet::Arena;
    }
    if (normalized == "beat'emup" || normalized == "beat em up" || normalized == "beat 'em up" || normalized == "openbor") {
        return InputActionSet::BeatEmUp;
    }
    if (normalized == "fighting") {
        return InputActionSet::Fighting;
    }
    return fallback;
}

std::vector<std::string> splitCsv(std::string_view value) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t comma = value.find(',', start);
        const size_t end = comma == std::string_view::npos ? value.size() : comma;
        std::string item = trim(value.substr(start, end - start));
        if (!item.empty()) {
            out.push_back(std::move(item));
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return out;
}

std::string joinBindings(const ControlActionBinding& binding) {
    std::string out;
    for (const auto& input : binding.bindings) {
        if (!out.empty()) {
            out += ", ";
        }
        out += physicalInputToken(input);
    }
    return out;
}

void parseActionBindings(ControlProfileBinding& profile, const MugenSection& section) {
    for (InputAction action : fightingInputActions()) {
        const std::string key = std::string("bind.") + std::string(inputActionId(action));
        const std::string value = propertyString(section, key);
        if (value.empty()) {
            continue;
        }
        auto& actionBinding = ensureActionBinding(profile, action);
        actionBinding.bindings.clear();
        for (const auto& token : splitCsv(value)) {
            if (auto parsed = parsePhysicalInputToken(token)) {
                actionBinding.bindings.push_back(*parsed);
            }
        }
    }
    for (InputAction action : beatEmUpInputActions()) {
        const std::string key = std::string("bind.") + std::string(inputActionId(action));
        const std::string value = propertyString(section, key);
        if (value.empty()) {
            continue;
        }
        auto& actionBinding = ensureActionBinding(profile, action);
        actionBinding.bindings.clear();
        for (const auto& token : splitCsv(value)) {
            if (auto parsed = parsePhysicalInputToken(token)) {
                actionBinding.bindings.push_back(*parsed);
            }
        }
    }
}

} // namespace

std::filesystem::path dragonControlsSavePath(const std::filesystem::path& gameRoot) {
    return gameRoot / "save" / "controls.def";
}

ControlsSettings loadControlsSettings(const std::filesystem::path& path) {
    ControlsSettings controls;
    if (!std::filesystem::exists(path)) {
        return controls;
    }

    const MugenDocument doc = parseMugenTextFile(path);
    for (const auto& section : doc.sections) {
        if (equalsNoCase(section.name, "Dragon.Controls")) {
            controls.schemaVersion = propertyInt(section, "version", controls.schemaVersion);
            for (int i = 0; i < kControlPlayerCount; ++i) {
                const std::string key = "p" + std::to_string(i + 1) + ".gamepad";
                controls.gamepadAssignments[static_cast<size_t>(i)] =
                    propertyInt(section, key, controls.gamepadAssignments[static_cast<size_t>(i)]);
            }
        } else if (startsWithNoCase(section.name, "Profile ") && section.name.find(".Controls") != std::string::npos) {
            const std::string rest = trim(std::string_view(section.name).substr(8));
            const size_t dot = rest.find('.');
            const std::string profileId = dot == std::string::npos ? rest : trim(std::string_view(rest).substr(0, dot));
            ControlProfileBinding profile = makeDefaultControlProfile(profileId, 0);
            profile.profileId = lowercaseAscii(profileId);
            profile.presetName = propertyString(section, "preset", profile.presetName);
            profile.actionSet = parseActionSet(propertyString(section, "actionset"), profile.actionSet);
            profile.deadzone = std::clamp(propertyInt(section, "deadzone", profile.deadzone), 1000, 32000);
            profile.triggerThreshold = std::clamp(propertyInt(section, "trigger.threshold", profile.triggerThreshold), 1000, 32000);
            parseActionBindings(profile, section);
            controls.profiles.push_back(std::move(profile));
        }
    }
    return controls;
}

void saveControlsSettings(const std::filesystem::path& path, const ControlsSettings& controls) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Could not write Dragon controls save: " + path.string());
    }

    output << "; Dragon local controls save. This is local player data, not M.U.G.E.N content.\n";
    output << "[Dragon.Controls]\n";
    output << "version = 1\n";
    for (int i = 0; i < kControlPlayerCount; ++i) {
        output << "p" << (i + 1) << ".gamepad = " << controls.gamepadAssignments[static_cast<size_t>(i)] << "\n";
    }
    output << "\n";

    for (const auto& profile : controls.profiles) {
        if (profile.profileId.empty() || equalsNoCase(profile.profileId, "guest")) {
            continue;
        }
        output << "[Profile " << profile.profileId << ".Controls]\n";
        output << "preset = " << profile.presetName << "\n";
        output << "actionset = " << inputActionSetLabel(profile.actionSet) << "\n";
        output << "deadzone = " << profile.deadzone << "\n";
        output << "trigger.threshold = " << profile.triggerThreshold << "\n";
        for (const auto& actionBinding : profile.actionBindings) {
            if (actionBinding.bindings.empty()) {
                continue;
            }
            output << "bind." << inputActionId(actionBinding.action) << " = " << joinBindings(actionBinding) << "\n";
        }
        output << "\n";
    }
}

} // namespace dragon
