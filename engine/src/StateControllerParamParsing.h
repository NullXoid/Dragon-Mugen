#pragma once

// Internal App.cpp controller-parameter parser implementation header.
// Depends on App.cpp-local CNS/state-controller parse types and helpers.
// Include only through StateControllerParsing.h after trigger parser helpers are defined.
// Do not include from other translation units.

std::optional<float> parseControllerFloatProperty(
    const MugenSection& section,
    std::string_view key,
    const CharacterConstants& constants) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    return parseControllerFloatValue(property->value, constants);
}

std::optional<std::pair<float, float>> parseControllerFloatPairProperty(
    const MugenSection& section,
    std::string_view key,
    const CharacterConstants& constants,
    float fallbackY = 0.0f) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    return parseControllerFloatPairValue(property->value, constants, fallbackY);
}

float parseExplodPositionComponent(const std::string& value, const CharacterConstants& constants, float fallback) {
    if (const auto parsed = parseControllerFloatValue(value, constants)) {
        return *parsed;
    }

    const std::string lowered = lowercaseCopy(value);
    if (findNoCase(lowered, "screenpos y", 0) != std::string::npos) {
        if (const auto minus = lowered.rfind('-'); minus != std::string::npos) {
            return -parseFloatValue(trim(std::string_view(lowered).substr(minus + 1)), -fallback);
        }
        if (const auto plus = lowered.rfind('+'); plus != std::string::npos) {
            return parseFloatValue(trim(std::string_view(lowered).substr(plus + 1)), fallback);
        }
        return 0.0f;
    }
    return fallback;
}

std::pair<float, float> parseExplodPosition(const std::string& value, const CharacterConstants& constants) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { 0.0f, 0.0f };
    }
    const float x = parseExplodPositionComponent(parts[0], constants, 0.0f);
    const float y = parts.size() >= 2 ? parseExplodPositionComponent(parts[1], constants, 0.0f) : 0.0f;
    return { x, y };
}

EnvShakeSpec parseEnvShakeSpec(const MugenSection& section) {
    EnvShakeSpec shake;
    if (const auto* time = findProperty(section, "time")) {
        shake.time = std::max(0, parseIntValue(time->value, shake.time));
        shake.timeExpression = trim(time->value);
    }
    if (const auto* frequency = findProperty(section, "freq")) {
        shake.frequency = std::max(1, parseIntValue(frequency->value, shake.frequency));
        shake.frequencyExpression = trim(frequency->value);
    }
    if (const auto* amplitude = findProperty(section, "ampl")) {
        shake.amplitude = parseFloatValue(amplitude->value, shake.amplitude);
        shake.amplitudeExpression = trim(amplitude->value);
    }
    if (const auto* phase = findProperty(section, "phase")) {
        shake.phase = parseIntValue(phase->value, shake.phase);
        shake.phaseExpression = trim(phase->value);
    }
    shake.enabled = shake.time > 0 && std::abs(shake.amplitude) > 0.001f;
    return shake;
}

std::array<int, 3> parseIntTripleValue(const std::string& value, int fallbackR, int fallbackG, int fallbackB) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB };
    }
    return {
        parseIntValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseIntValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseIntValue(parts[2], fallbackB) : fallbackB,
    };
}

std::array<float, 3> parseFloatTripleValue(const std::string& value, float fallbackR, float fallbackG, float fallbackB) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB };
    }
    return {
        parseFloatValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseFloatValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseFloatValue(parts[2], fallbackB) : fallbackB,
    };
}

std::array<std::string, 3> parseExpressionTripleValue(
    const std::string& value,
    std::array<std::string, 3> fallback = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return fallback;
    }
    std::array<std::string, 3> out = fallback;
    for (size_t i = 0; i < std::min<size_t>(parts.size(), out.size()); ++i) {
        out[i] = trim(parts[i]);
    }
    return out;
}

std::array<int, 4> parseIntQuadValue(const std::string& value, int fallbackR, int fallbackG, int fallbackB, int fallbackPeriod) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackR, fallbackG, fallbackB, fallbackPeriod };
    }
    return {
        parseIntValue(parts[0], fallbackR),
        parts.size() >= 2 ? parseIntValue(parts[1], fallbackG) : fallbackG,
        parts.size() >= 3 ? parseIntValue(parts[2], fallbackB) : fallbackB,
        parts.size() >= 4 ? parseIntValue(parts[3], fallbackPeriod) : fallbackPeriod,
    };
}

std::array<std::string, 4> parseExpressionQuadValue(
    const std::string& value,
    std::array<std::string, 4> fallback = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return fallback;
    }
    std::array<std::string, 4> out = fallback;
    for (size_t i = 0; i < std::min<size_t>(parts.size(), out.size()); ++i) {
        out[i] = trim(parts[i]);
    }
    return out;
}

PaletteEffectSpec parsePaletteEffectSpec(const MugenSection& section, std::string_view prefix = "") {
    PaletteEffectSpec effect;
    const std::string keyPrefix(prefix);
    auto prefixed = [&keyPrefix](std::string_view key) {
        return keyPrefix.empty() ? std::string(key) : keyPrefix + "." + std::string(key);
    };

    if (const auto* time = findProperty(section, prefixed("time"))) {
        effect.time = parseIntValue(time->value, effect.time);
        effect.timeExpression = trim(time->value);
    }
    if (const auto* add = findProperty(section, prefixed("add"))) {
        const auto values = parseIntTripleValue(add->value, 0, 0, 0);
        effect.addR = values[0];
        effect.addG = values[1];
        effect.addB = values[2];
        effect.addExpressions = parseExpressionTripleValue(add->value);
    }
    if (const auto* mul = findProperty(section, prefixed("mul"))) {
        const auto values = parseIntTripleValue(mul->value, 256, 256, 256);
        effect.mulR = values[0];
        effect.mulG = values[1];
        effect.mulB = values[2];
        effect.mulExpressions = parseExpressionTripleValue(mul->value, { "256", "256", "256" });
    }
    if (const auto* sinAdd = findProperty(section, prefixed("sinadd"))) {
        const auto values = parseIntQuadValue(sinAdd->value, 0, 0, 0, 0);
        effect.sinAddR = values[0];
        effect.sinAddG = values[1];
        effect.sinAddB = values[2];
        effect.sinPeriod = values[3];
        effect.sinAddExpressions = parseExpressionQuadValue(sinAdd->value);
    }
    if (const auto* color = findProperty(section, prefixed("color"))) {
        effect.color = parseIntValue(color->value, effect.color);
        effect.colorExpression = trim(color->value);
    }
    if (const auto* invertAll = findProperty(section, prefixed("invertall"))) {
        effect.invertAll = parseIntValue(invertAll->value, 0) != 0;
        effect.invertAllExpression = trim(invertAll->value);
    }

    effect.enabled = effect.time != 0
        && (effect.addR != 0 || effect.addG != 0 || effect.addB != 0
            || effect.mulR != 256 || effect.mulG != 256 || effect.mulB != 256
            || effect.sinAddR != 0 || effect.sinAddG != 0 || effect.sinAddB != 0
            || effect.color != 256 || effect.invertAll);
    return effect;
}

std::optional<char> parseControllerCharProperty(const MugenSection& section, std::string_view key) {
    const auto* property = findProperty(section, key);
    if (!property) {
        return std::nullopt;
    }
    const std::string value = trim(property->value);
    if (value.empty()) {
        return std::nullopt;
    }
    return static_cast<char>(SDL_toupper(static_cast<unsigned char>(value.front())));
}

std::string normalizeAssertSpecialFlag(std::string_view value) {
    std::string normalized = lowercaseCopy(trim(value));
    normalized.erase(
        std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0 || ch == '_' || ch == '-';
        }),
        normalized.end());
    return normalized;
}

std::vector<std::string> parseAssertSpecialFlags(const MugenSection& section) {
    std::vector<std::string> flags;
    for (const auto& property : section.properties) {
        if (!startsWithNoCase(property.key, "flag")) {
            continue;
        }

        const std::string flag = normalizeAssertSpecialFlag(property.value);
        if (!flag.empty()
            && std::find(flags.begin(), flags.end(), flag) == flags.end()) {
            flags.push_back(flag);
        }
    }
    return flags;
}

std::optional<std::pair<MugenVariableRef, std::string>> parseDirectVariableAssignment(const MugenSection& section) {
    for (const auto& property : section.properties) {
        if (const auto target = parseMugenVariableRef(property.key)) {
            return std::pair<MugenVariableRef, std::string>{ *target, trim(property.value) };
        }
    }
    return std::nullopt;
}

std::optional<MugenVariableRef> parseVariableControllerTarget(const MugenSection& section) {
    if (const auto direct = parseDirectVariableAssignment(section)) {
        return direct->first;
    }
    if (const auto* fv = findProperty(section, "fv")) {
        const auto index = parsePlainIntValue(fv->value);
        if (index && *index >= 0) {
            return MugenVariableRef{ MugenVariableBank::FVar, *index };
        }
    }
    if (const auto* v = findProperty(section, "v")) {
        const auto index = parsePlainIntValue(v->value);
        if (index && *index >= 0) {
            return MugenVariableRef{ MugenVariableBank::Var, *index };
        }
    }
    return std::nullopt;
}

bool isSupportedMoveType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'I' || value == 'A' || value == 'H';
}

bool isSupportedPhysicsType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'S' || value == 'C' || value == 'A' || value == 'N';
}

std::vector<MugenDocument> loadCharacterStateDocuments(const CharacterFiles& files) {
    std::vector<MugenDocument> documents;
    documents.reserve(files.stateFiles.size());
    for (const auto& path : files.stateFiles) {
        documents.push_back(parseMugenTextFile(path));
    }
    return documents;
}

std::vector<std::string> loadVictoryQuotes(const CharacterFiles& files) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<std::string> quotes;
    for (const auto& document : documents) {
        for (const auto& section : document.sections) {
            if (!equalsNoCase(section.name, "Quotes")) {
                continue;
            }
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "victory")) {
                    continue;
                }
                const int index = parseIntValue(std::string(std::string_view(property.key).substr(7)), 0);
                if (index <= 0 || index > 99) {
                    continue;
                }
                if (quotes.size() < static_cast<size_t>(index)) {
                    quotes.resize(static_cast<size_t>(index));
                }
                quotes[static_cast<size_t>(index - 1)] = unquote(trim(property.value));
            }
        }
    }
    return quotes;
}

