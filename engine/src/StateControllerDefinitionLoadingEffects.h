#pragma once

// Internal App.cpp state-controller loading fragment: palette, environment, and explod controller handlers.
// This file is included inside loadStateDefinitions and depends on its local variables.
// Include only from StateControllerDefinitionLoading.h.
// Do not include from other translation units.


            if (startsWithNoCase(controllerType, "Null")) {
                continue;
            }

            if (startsWithNoCase(controllerType, "PalFX") || startsWithNoCase(controllerType, "BGPalFX")) {
                if (!controllerTrigger) {
                    continue;
                }

                StatePaletteEffectController paletteEffect;
                paletteEffect.id = nextRuntimeControllerId++;
                paletteEffect.trigger = *controllerTrigger;
                paletteEffect.background = startsWithNoCase(controllerType, "BGPalFX");
                paletteEffect.effect = parsePaletteEffectSpec(section);
                if (!paletteEffect.effect.enabled) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.paletteEffects.push_back(std::move(paletteEffect));
                continue;
            }

            if (startsWithNoCase(controllerType, "EnvColor")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateEnvColorController envColor;
                envColor.id = nextRuntimeControllerId++;
                envColor.trigger = *controllerTrigger;
                if (const auto* value = findProperty(section, "value")) {
                    const auto values = parseIntTripleValue(value->value, 255, 255, 255);
                    envColor.r = values[0];
                    envColor.g = values[1];
                    envColor.b = values[2];
                }
                if (const auto* time = findProperty(section, "time")) {
                    envColor.time = std::max(0, parseIntValue(time->value, envColor.time));
                }
                if (envColor.time <= 0) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.envColors.push_back(std::move(envColor));
                continue;
            }

            if (startsWithNoCase(controllerType, "EnvShake")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateEnvShakeController envShake;
                envShake.id = nextRuntimeControllerId++;
                envShake.trigger = *controllerTrigger;
                envShake.shake = parseEnvShakeSpec(section);
                if (!envShake.shake.enabled) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.envShakes.push_back(std::move(envShake));
                continue;
            }

            if (startsWithNoCase(controllerType, "FallEnvShake")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateFallEnvShakeController fallEnvShake;
                fallEnvShake.id = nextRuntimeControllerId++;
                fallEnvShake.trigger = *controllerTrigger;

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.fallEnvShakes.push_back(std::move(fallEnvShake));
                continue;
            }

            if (startsWithNoCase(controllerType, "ModifyExplod")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateModifyExplodController modifyExplod;
                modifyExplod.id = nextRuntimeControllerId++;
                modifyExplod.trigger = *controllerTrigger;
                if (const auto* explodId = findProperty(section, "id")) {
                    modifyExplod.explodId = parseIntValue(explodId->value, -1);
                }
                if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                    modifyExplod.hasSprPriority = true;
                    modifyExplod.sprPriority = parseIntValue(sprPriority->value, 0);
                }
                if (const auto* scale = findProperty(section, "scale")) {
                    const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                    modifyExplod.hasScale = true;
                    modifyExplod.scaleX = values.first;
                    modifyExplod.scaleY = values.second;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.modifyExplods.push_back(std::move(modifyExplod));
                continue;
            }

            if (startsWithNoCase(controllerType, "RemoveExplod")) {
                if (!controllerTrigger) {
                    continue;
                }

                StateRemoveExplodController removeExplod;
                removeExplod.id = nextRuntimeControllerId++;
                removeExplod.trigger = *controllerTrigger;
                if (const auto* explodId = findProperty(section, "id")) {
                    removeExplod.explodId = parseIntValue(explodId->value, -1);
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.removeExplods.push_back(std::move(removeExplod));
                continue;
            }

            if (startsWithNoCase(controllerType, "Explod")) {
                if (!controllerTrigger) {
                    continue;
                }

            const auto* anim = findProperty(section, "anim");
            if (!anim) {
                continue;
            }

            std::string animValue = trim(anim->value);
            StateExplodController explod;
            explod.id = nextRuntimeControllerId++;
            explod.trigger = *controllerTrigger;
            if (const auto* explodId = findProperty(section, "id")) {
                explod.explodId = parseIntValue(explodId->value, -1);
            }
            if (!animValue.empty() && (animValue.front() == 'F' || animValue.front() == 'f')) {
                explod.fromFightFx = true;
                animValue = trim(std::string_view(animValue).substr(1));
            }
            explod.anim = parseIntValue(animValue, -1);
            if (explod.anim < 0) {
                continue;
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                explod.x = values.first;
                explod.y = values.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                explod.postype = lowercaseCopy(trim(postype->value));
            }
            if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                explod.sprPriority = parseIntValue(sprPriority->value, 0);
            }
            if (const auto* bindTime = findProperty(section, "bindtime")) {
                explod.bindTime = parseIntValue(bindTime->value, explod.bindTime);
            }
            if (const auto* removeTime = findProperty(section, "removetime")) {
                explod.removeTime = parseIntValue(removeTime->value, explod.removeTime);
            }
            if (const auto* scale = findProperty(section, "scale")) {
                const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                explod.scaleX = values.first;
                explod.scaleY = values.second;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.explods.push_back(std::move(explod));
            continue;
        }
