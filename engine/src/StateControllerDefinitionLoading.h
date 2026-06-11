#pragma once

// Internal App.cpp state-controller definition loading header.
// Depends on App.cpp-local CNS/state-controller parse types and helpers.
// Include only through StateControllerParsing.h after trigger and parameter helpers are defined.
// Do not include from other translation units.

std::optional<int> parseStateNumber(std::string_view sectionName, std::string_view prefix) {
    if (!startsWithNoCase(sectionName, prefix)) {
        return std::nullopt;
    }

    try {
        return std::stoi(trim(sectionName.substr(prefix.size())));
    } catch (...) {
        return std::nullopt;
    }
}

char parseStateChar(const MugenSection* section, std::string_view key, char fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    const auto value = trim(property->value);
    if (value.empty()) {
        return fallback;
    }
    return static_cast<char>(SDL_toupper(static_cast<unsigned char>(value.front())));
}

std::vector<StateDefinition> loadStateDefinitions(const CharacterFiles& files, const CharacterConstants& constants) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<StateDefinition> states;
    int nextSoundControllerId = 1;
    int nextCtrlControllerId = 1;
    int nextPosAddControllerId = 1;
    int nextChangeAnimControllerId = 1;
    int nextRuntimeControllerId = 1;

    for (const auto& cns : documents) {
        int currentStateIndex = -1;
        for (const auto& section : cns.sections) {
            const auto stateNo = parseStateNumber(section.name, "Statedef ");
            if (stateNo) {
                StateDefinition state;
                state.stateNo = *stateNo;
                state.stateType = parseStateChar(&section, "type", state.stateType);
                state.moveType = parseStateChar(&section, "movetype", state.moveType);
                state.physics = parseStateChar(&section, "physics", state.physics);
                if (const auto* anim = findProperty(section, "anim")) {
                    state.hasAnim = true;
                    state.anim = parseIntValue(anim->value, state.anim);
                } else {
                    state.anim = state.stateNo;
                }
                if (const auto* ctrl = findProperty(section, "ctrl")) {
                    state.ctrl = parseIntValue(ctrl->value, state.ctrl ? 1 : 0) != 0;
                }
                if (const auto* powerAdd = findProperty(section, "poweradd")) {
                    state.powerAddExpression = trim(powerAdd->value);
                }
                if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                    state.sprPriority = parseIntValue(sprPriority->value, state.sprPriority);
                }
                if (const auto* velset = findProperty(section, "velset")) {
                    if (const auto values = parseControllerFloatPairValue(velset->value, constants)) {
                        state.hasVelset = true;
                        state.velsetX = values->first;
                        state.velsetY = values->second;
                    }
                }
                states.push_back(std::move(state));
                currentStateIndex = static_cast<int>(states.size()) - 1;
                continue;
            }

            if (currentStateIndex < 0 || !startsWithNoCase(section.name, "State ")) {
                continue;
            }

            const auto* type = findProperty(section, "type");
            if (!type) {
                continue;
            }

            const std::string controllerType = trim(type->value);
            bool hasTriggerProperties = false;
            int triggerTime = -1;
            int triggerAnimElem = -1;
            bool requiresMoveContact = false;
            std::vector<AnimElemTimeCondition> animElemTimeConditions;
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "trigger")) {
                    continue;
                }
                hasTriggerProperties = true;
                const auto trigger = trim(property.value);
                for (const auto& clause : splitAndClauses(trigger)) {
                    if (const auto time = parseTimeTrigger(clause)) {
                        triggerTime = *time;
                    } else if (const auto elem = parseAnimElemTrigger(clause)) {
                        triggerAnimElem = *elem;
                    } else if (const auto condition = parseAnimElemTimeCondition(clause)) {
                        animElemTimeConditions.push_back(*condition);
                    } else if (triggerIsMoveContact(clause)) {
                        requiresMoveContact = true;
                    }
                }
            }
            const StateControllerTrigger controllerOptions = parseStateControllerTriggerOptions(section);
            const auto controllerTrigger = parseStateControllerTrigger(section);

// Controller handler bodies are split into internal fragments to keep parser debt bounded.
#include "StateControllerDefinitionLoadingCore.h"
#include "StateControllerDefinitionLoadingTarget.h"
#include "StateControllerDefinitionLoadingEffects.h"
#include "StateControllerDefinitionLoadingRuntime.h"
#include "StateControllerDefinitionLoadingProjectile.h"

        }
    }

    return states;
}
