#pragma once

// Internal App.cpp state-controller loading fragment: target and pause controller handlers.
// This file is included inside loadStateDefinitions and depends on its local variables.
// Include only from StateControllerDefinitionLoading.h.
// Do not include from other translation units.


            if (startsWithNoCase(controllerType, "TargetState")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                const auto parsedValue = parsePlainIntValue(value->value);
                if (!parsedValue) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetStateController targetState;
                targetState.id = nextRuntimeControllerId++;
                targetState.trigger = *controllerTrigger;
                targetState.value = *parsedValue;
                if (const auto* id = findProperty(section, "id")) {
                    targetState.targetId = parseIntValue(id->value, targetState.targetId);
                }
                state.targetStates.push_back(std::move(targetState));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetBind")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetBindController targetBind;
                targetBind.id = nextRuntimeControllerId++;
                targetBind.trigger = *controllerTrigger;
                if (const auto pos = parseControllerFloatPairProperty(section, "pos", constants)) {
                    targetBind.x = pos->first;
                    targetBind.y = pos->second;
                }
                if (const auto* time = findProperty(section, "time")) {
                    targetBind.time = parseIntValue(time->value, targetBind.time);
                }
                if (const auto* id = findProperty(section, "id")) {
                    targetBind.targetId = parseIntValue(id->value, targetBind.targetId);
                }
                state.targetBinds.push_back(std::move(targetBind));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetDrop")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetDropController targetDrop;
                targetDrop.id = nextRuntimeControllerId++;
                targetDrop.trigger = *controllerTrigger;
                if (const auto* excludeId = findProperty(section, "excludeID")) {
                    targetDrop.excludeId = parseIntValue(excludeId->value, targetDrop.excludeId);
                }
                state.targetDrops.push_back(std::move(targetDrop));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetFacing")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetFacingController targetFacing;
                targetFacing.id = nextRuntimeControllerId++;
                targetFacing.trigger = *controllerTrigger;
                targetFacing.value = parseIntValue(value->value, targetFacing.value);
                if (const auto* id = findProperty(section, "id")) {
                    targetFacing.targetId = parseIntValue(id->value, targetFacing.targetId);
                }
                state.targetFacings.push_back(std::move(targetFacing));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetLifeAdd")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetLifeAddController targetLifeAdd;
                targetLifeAdd.id = nextRuntimeControllerId++;
                targetLifeAdd.trigger = *controllerTrigger;
                targetLifeAdd.valueExpression = trim(value->value);
                if (const auto* id = findProperty(section, "id")) {
                    targetLifeAdd.targetId = parseIntValue(id->value, targetLifeAdd.targetId);
                }
                if (const auto* kill = findProperty(section, "kill")) {
                    targetLifeAdd.kill = parseIntValue(kill->value, targetLifeAdd.kill ? 1 : 0) != 0;
                }
                if (const auto* absolute = findProperty(section, "absolute")) {
                    targetLifeAdd.absolute = parseIntValue(absolute->value, targetLifeAdd.absolute ? 1 : 0) != 0;
                }
                state.targetLifeAdds.push_back(std::move(targetLifeAdd));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetPowerAdd")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetPowerAddController targetPowerAdd;
                targetPowerAdd.id = nextRuntimeControllerId++;
                targetPowerAdd.trigger = *controllerTrigger;
                targetPowerAdd.valueExpression = trim(value->value);
                if (const auto* id = findProperty(section, "id")) {
                    targetPowerAdd.targetId = parseIntValue(id->value, targetPowerAdd.targetId);
                }
                state.targetPowerAdds.push_back(std::move(targetPowerAdd));
                continue;
            }

            if (startsWithNoCase(controllerType, "TargetVelAdd") || startsWithNoCase(controllerType, "TargetVelSet")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTargetVelocityController targetVelocity;
                targetVelocity.id = nextRuntimeControllerId++;
                targetVelocity.trigger = *controllerTrigger;
                targetVelocity.add = startsWithNoCase(controllerType, "TargetVelAdd");
                if (const auto* x = findProperty(section, "x")) {
                    targetVelocity.xExpression = trim(x->value);
                    targetVelocity.hasX = true;
                }
                if (const auto* y = findProperty(section, "y")) {
                    targetVelocity.yExpression = trim(y->value);
                    targetVelocity.hasY = true;
                }
                if (const auto* id = findProperty(section, "id")) {
                    targetVelocity.targetId = parseIntValue(id->value, targetVelocity.targetId);
                }
                if (!targetVelocity.hasX && !targetVelocity.hasY) {
                    continue;
                }
                state.targetVelocities.push_back(std::move(targetVelocity));
                continue;
            }

            if (startsWithNoCase(controllerType, "Turn")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateTurnController turn;
                turn.id = nextRuntimeControllerId++;
                turn.trigger = *controllerTrigger;
                state.turns.push_back(std::move(turn));
                continue;
            }

            if (equalsNoCase(controllerType, "Pause") || equalsNoCase(controllerType, "SuperPause")) {
                if (!controllerTrigger) {
                    continue;
                }
                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StatePauseController pause;
                pause.id = nextRuntimeControllerId++;
                pause.trigger = *controllerTrigger;
                pause.superPause = equalsNoCase(controllerType, "SuperPause");
                pause.time = pause.superPause ? 30 : 1;
                if (const auto* time = findProperty(section, "time")) {
                    pause.time = std::max(1, parseIntValue(time->value, pause.time));
                }
                if (const auto* moveTime = findProperty(section, "movetime")) {
                    pause.moveTime = std::max(0, parseIntValue(moveTime->value, pause.moveTime));
                }
                if (const auto* powerAdd = findProperty(section, "poweradd")) {
                    pause.powerAdd = parseIntValue(powerAdd->value, pause.powerAdd);
                }
                if (const auto* sound = findProperty(section, "sound")) {
                    if (const auto parsed = parseSoundValue(sound->value)) {
                        pause.soundGroup = parsed->group;
                        pause.soundIndex = parsed->index;
                        pause.soundForceCommon = parsed->forceCommon;
                    }
                }
                state.pauses.push_back(std::move(pause));
                continue;
            }
