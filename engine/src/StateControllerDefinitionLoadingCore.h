#pragma once

// Internal App.cpp state-controller loading fragment: core state/movement/audio controller handlers.
// This file is included inside loadStateDefinitions and depends on its local variables.
// Include only from StateControllerDefinitionLoading.h.
// Do not include from other translation units.

            if (startsWithNoCase(controllerType, "ChangeState") || startsWithNoCase(controllerType, "SelfState")) {
                const bool selfState = startsWithNoCase(controllerType, "SelfState");
                const auto* value = findProperty(section, "value");
                if (!value) {
                    continue;
                }

                const auto targetState = parsePlainIntValue(value->value);
                const std::string targetStateExpression = trim(value->value);
                const int parsedTargetState = targetState.value_or(0);

                bool animEndTrigger = false;
                for (const auto& property : section.properties) {
                    if (!startsWithNoCase(property.key, "trigger")) {
                        continue;
                    }
                    const auto trigger = trim(property.value);
                    if (equalsNoCase(stripOuterParens(trigger), "!animtime")) {
                        animEndTrigger = true;
                        continue;
                    }
                    if (!startsWithNoCase(trigger, "AnimTime")) {
                        continue;
                    }
                    if (const auto equals = trigger.find('='); equals != std::string::npos) {
                        animEndTrigger = parseIntValue(trim(std::string_view(trigger).substr(equals + 1)), 1) == 0;
                    }
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                if (animEndTrigger) {
                    state.hasAnimEndChangeState = true;
                    state.animEndChangeState = parsedTargetState;
                    state.animEndChangeStateExpression = targetStateExpression;
                    state.animEndSelfState = selfState;
                    if (const auto* ctrl = findProperty(section, "ctrl")) {
                        state.hasAnimEndCtrl = true;
                        state.animEndCtrl = parseIntValue(ctrl->value, 0) != 0;
                    }
                    continue;
                }

                if (!controllerTrigger) {
                    continue;
                }

                StateChangeStateController changeState;
                changeState.id = nextRuntimeControllerId++;
                changeState.trigger = *controllerTrigger;
                changeState.targetState = parsedTargetState;
                changeState.targetStateExpression = targetStateExpression;
                changeState.selfState = selfState;
                if (const auto* ctrl = findProperty(section, "ctrl")) {
                    changeState.hasCtrl = true;
                    changeState.ctrl = parseIntValue(ctrl->value, 0) != 0;
                }
                state.changeStates.push_back(std::move(changeState));
                continue;
            }

        if (startsWithNoCase(controllerType, "PlaySnd")) {
            const auto* value = findProperty(section, "value");
            if (!value
                || (hasTriggerProperties && !controllerTrigger)
                || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }
            const auto sound = parseSoundValue(value->value);
            if (!sound) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateSoundController soundController;
            soundController.id = nextSoundControllerId++;
            soundController.trigger = controllerTrigger ? *controllerTrigger : controllerOptions;
            soundController.triggerTime = triggerTime;
            soundController.triggerAnimElem = triggerAnimElem;
            soundController.group = sound->group;
            soundController.index = sound->index;
            soundController.forceCommon = sound->forceCommon;
            if (const auto* channel = findProperty(section, "channel")) {
                soundController.channel = parseIntValue(channel->value, soundController.channel);
            }
            if (const auto* lowPriority = findProperty(section, "lowpriority")) {
                soundController.lowPriority = parseIntValue(lowPriority->value, 0) != 0;
            }
            if (const auto* volume = findProperty(section, "volume")) {
                soundController.gain = mugenVolumeToGain(volume->value);
            }
            if (const auto* loop = findProperty(section, "loop")) {
                soundController.loop = parseIntValue(loop->value, 0) != 0;
            }
            state.sounds.push_back(std::move(soundController));
            state.audioControllers.push_back(StateAudioControllerRef{
                StateAudioControllerKind::PlaySnd,
                state.sounds.size() - 1,
            });
            continue;
        }

        if (startsWithNoCase(controllerType, "StopSnd")) {
            const auto* channel = findProperty(section, "channel");
            if (!channel
                || (hasTriggerProperties && !controllerTrigger)
                || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }

            const int parsedChannel = parseIntValue(channel->value, -1);
            if (parsedChannel < 0) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateStopSoundController stopSound;
            stopSound.id = nextRuntimeControllerId++;
            stopSound.trigger = controllerTrigger ? *controllerTrigger : controllerOptions;
            stopSound.triggerTime = triggerTime;
            stopSound.triggerAnimElem = triggerAnimElem;
            stopSound.channel = parsedChannel;
            state.stopSounds.push_back(std::move(stopSound));
            state.audioControllers.push_back(StateAudioControllerRef{
                StateAudioControllerKind::StopSnd,
                state.stopSounds.size() - 1,
            });
            continue;
        }

        if (startsWithNoCase(controllerType, "CtrlSet")) {
            const auto* value = findProperty(section, "value");
            if (!value || (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0)) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateCtrlController ctrlSet;
            ctrlSet.id = nextCtrlControllerId++;
            ctrlSet.trigger = controllerTrigger ? *controllerTrigger : controllerOptions;
            ctrlSet.triggerTime = triggerTime;
            ctrlSet.triggerAnimElem = triggerAnimElem;
            ctrlSet.value = parseIntValue(value->value, 0) != 0;
            state.ctrlSets.push_back(std::move(ctrlSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "VarSet")
            || startsWithNoCase(controllerType, "VarAdd")
            || startsWithNoCase(controllerType, "VarRandom")) {
            if (!controllerTrigger) {
                continue;
            }

            const auto target = parseVariableControllerTarget(section);
            if (!target) {
                continue;
            }

            StateVariableController variable;
            variable.id = nextRuntimeControllerId++;
            variable.trigger = *controllerTrigger;
            variable.target = *target;
            if (startsWithNoCase(controllerType, "VarAdd")) {
                variable.operation = StateVariableOperation::Add;
            } else if (startsWithNoCase(controllerType, "VarRandom")) {
                variable.operation = StateVariableOperation::Random;
            } else {
                variable.operation = StateVariableOperation::Set;
            }

            if (variable.operation == StateVariableOperation::Random) {
                const auto* range = findProperty(section, "range");
                if (!range) {
                    continue;
                }
                const auto parts = splitCsv(range->value);
                if (parts.empty()) {
                    continue;
                }
                variable.rangeMinExpression = trim(parts[0]);
                variable.rangeMaxExpression = parts.size() >= 2 ? trim(parts[1]) : variable.rangeMinExpression;
            } else if (const auto direct = parseDirectVariableAssignment(section)) {
                variable.valueExpression = direct->second;
            } else if (const auto* value = findProperty(section, "value")) {
                variable.valueExpression = trim(value->value);
            } else {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.variableControllers.push_back(std::move(variable));
            continue;
        }

        if (startsWithNoCase(controllerType, "VelSet")
            || startsWithNoCase(controllerType, "VelAdd")
            || startsWithNoCase(controllerType, "VelMul")) {
            if (!controllerTrigger) {
                continue;
            }

            StateVelocityController velocity;
            velocity.id = nextRuntimeControllerId++;
            velocity.trigger = *controllerTrigger;
            if (startsWithNoCase(controllerType, "VelAdd")) {
                velocity.operation = StateVelocityOperation::Add;
            } else if (startsWithNoCase(controllerType, "VelMul")) {
                velocity.operation = StateVelocityOperation::Mul;
            } else {
                velocity.operation = StateVelocityOperation::Set;
            }
            if (const auto* xProperty = findProperty(section, "x")) {
                velocity.hasX = true;
                velocity.xExpression = trim(xProperty->value);
                if (const auto x = parseControllerFloatValue(velocity.xExpression, constants)) {
                    velocity.x = *x;
                }
            }
            if (const auto* yProperty = findProperty(section, "y")) {
                velocity.hasY = true;
                velocity.yExpression = trim(yProperty->value);
                if (const auto y = parseControllerFloatValue(velocity.yExpression, constants)) {
                    velocity.y = *y;
                }
            }
            if (!velocity.hasX && !velocity.hasY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.velocityControllers.push_back(std::move(velocity));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePosSetController posSet;
            posSet.id = nextRuntimeControllerId++;
            posSet.trigger = *controllerTrigger;
            if (const auto x = parseControllerFloatProperty(section, "x", constants)) {
                posSet.hasX = true;
                posSet.x = *x;
            }
            if (const auto y = parseControllerFloatProperty(section, "y", constants)) {
                posSet.hasY = true;
                posSet.y = *y;
            }
            if (!posSet.hasX && !posSet.hasY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.posSets.push_back(std::move(posSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "StateTypeSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StateTypeSetController typeSet;
            typeSet.id = nextRuntimeControllerId++;
            typeSet.trigger = *controllerTrigger;
            if (const auto stateType = parseControllerCharProperty(section, "statetype")) {
                if (isSupportedStateType(*stateType)) {
                    typeSet.hasStateType = true;
                    typeSet.stateType = *stateType;
                }
            }
            if (const auto moveType = parseControllerCharProperty(section, "movetype")) {
                if (isSupportedMoveType(*moveType)) {
                    typeSet.hasMoveType = true;
                    typeSet.moveType = *moveType;
                }
            }
            if (const auto physics = parseControllerCharProperty(section, "physics")) {
                if (isSupportedPhysicsType(*physics)) {
                    typeSet.hasPhysics = true;
                    typeSet.physics = *physics;
                }
            }
            if (!typeSet.hasStateType && !typeSet.hasMoveType && !typeSet.hasPhysics) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.stateTypeSets.push_back(std::move(typeSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "ScreenBound")) {
            if (!controllerTrigger) {
                continue;
            }

            StateScreenBoundController screenBound;
            screenBound.id = nextRuntimeControllerId++;
            screenBound.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                screenBound.value = parseIntValue(value->value, screenBound.value ? 1 : 0) != 0;
            }
            if (const auto* moveCamera = findProperty(section, "movecamera")) {
                const auto values = parseIntPairValue(moveCamera->value);
                screenBound.moveCameraX = values.first != 0;
                screenBound.moveCameraY = values.second != 0;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.screenBounds.push_back(std::move(screenBound));
            continue;
        }

        if (startsWithNoCase(controllerType, "Width")) {
            if (!controllerTrigger) {
                continue;
            }

            StateWidthController width;
            width.id = nextRuntimeControllerId++;
            width.trigger = *controllerTrigger;
            const bool hasExplicitEdge = findProperty(section, "edge") != nullptr;
            const bool hasExplicitPlayer = findProperty(section, "player") != nullptr;

            if (const auto edge = parseControllerFloatPairProperty(section, "edge", constants)) {
                width.hasEdge = true;
                width.edgeFront = edge->first;
                width.edgeBack = edge->second;
            }
            if (const auto player = parseControllerFloatPairProperty(section, "player", constants)) {
                width.hasPlayer = true;
                width.playerFront = player->first;
                width.playerBack = player->second;
            }
            if (!hasExplicitEdge && !hasExplicitPlayer) {
                if (const auto value = parseControllerFloatPairProperty(section, "value", constants)) {
                    width.hasEdge = true;
                    width.edgeFront = value->first;
                    width.edgeBack = value->second;
                    width.hasPlayer = true;
                    width.playerFront = value->first;
                    width.playerBack = value->second;
                }
            }
            if (!width.hasEdge && !width.hasPlayer) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.widths.push_back(std::move(width));
            continue;
        }

        if (startsWithNoCase(controllerType, "PlayerPush")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePlayerPushController playerPush;
            playerPush.id = nextRuntimeControllerId++;
            playerPush.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                playerPush.value = parseIntValue(value->value, playerPush.value ? 1 : 0) != 0;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.playerPushes.push_back(std::move(playerPush));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallDamage")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallDamageController hitFallDamage;
            hitFallDamage.id = nextRuntimeControllerId++;
            hitFallDamage.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallDamages.push_back(std::move(hitFallDamage));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallVel")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallVelController hitFallVel;
            hitFallVel.id = nextRuntimeControllerId++;
            hitFallVel.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallVels.push_back(std::move(hitFallVel));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitFallSet")) {
            if (!controllerTrigger) {
                continue;
            }

            StateHitFallSetController hitFallSet;
            hitFallSet.id = nextRuntimeControllerId++;
            hitFallSet.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                hitFallSet.value = parseIntValue(value->value, hitFallSet.value);
            }
            if (const auto xvel = parseControllerFloatProperty(section, "xvel", constants)) {
                hitFallSet.hasXVelocity = true;
                hitFallSet.xVelocity = *xvel;
            }
            if (const auto yvel = parseControllerFloatProperty(section, "yvel", constants)) {
                hitFallSet.hasYVelocity = true;
                hitFallSet.yVelocity = *yvel;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.hitFallSets.push_back(std::move(hitFallSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "SprPriority")) {
            if (!controllerTrigger) {
                continue;
            }

            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }

            StateSprPriorityController sprPriority;
            sprPriority.id = nextRuntimeControllerId++;
            sprPriority.trigger = *controllerTrigger;
            sprPriority.value = parseIntValue(value->value, 0);

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.sprPriorities.push_back(std::move(sprPriority));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosFreeze")) {
            if (!controllerTrigger) {
                continue;
            }

            StatePosFreezeController posFreeze;
            posFreeze.id = nextRuntimeControllerId++;
            posFreeze.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                posFreeze.freezeX = parseIntValue(x->value, 0) != 0;
            }
            if (const auto* y = findProperty(section, "y")) {
                posFreeze.freezeY = parseIntValue(y->value, 0) != 0;
            }
            if (!posFreeze.freezeX && !posFreeze.freezeY) {
                posFreeze.freezeX = true;
                posFreeze.freezeY = true;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.posFreezes.push_back(std::move(posFreeze));
            continue;
        }

            if (startsWithNoCase(controllerType, "HitVelSet")) {
                if (!controllerTrigger) {
                    continue;
                }

            StateHitVelSetController hitVelSet;
            hitVelSet.id = nextRuntimeControllerId++;
            hitVelSet.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                hitVelSet.applyX = parseIntValue(x->value, 0) != 0;
            }
            if (const auto* y = findProperty(section, "y")) {
                hitVelSet.applyY = parseIntValue(y->value, 0) != 0;
            }
            if (!hitVelSet.applyX && !hitVelSet.applyY) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
                state.hitVelSets.push_back(std::move(hitVelSet));
                continue;
            }

            if (startsWithNoCase(controllerType, "NotHitBy") || startsWithNoCase(controllerType, "HitBy")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* value = findProperty(section, "value");
                if (!value) {
                    value = findProperty(section, "value2");
                }
                if (!value) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateHitProtectionController protection;
                protection.id = nextRuntimeControllerId++;
                protection.trigger = *controllerTrigger;
                protection.notHitBy = startsWithNoCase(controllerType, "NotHitBy");
                protection.value = trim(value->value);
                if (const auto* time = findProperty(section, "time")) {
                    protection.time = std::max(1, parseIntValue(time->value, protection.time));
                }
                state.hitProtections.push_back(std::move(protection));
                continue;
            }

            if (startsWithNoCase(controllerType, "HitOverride")) {
                if (!controllerTrigger) {
                    continue;
                }
                const auto* attr = findProperty(section, "attr");
                const auto* stateNo = findProperty(section, "stateno");
                if (!attr || !stateNo) {
                    continue;
                }
                const auto parsedState = parsePlainIntValue(stateNo->value);
                if (!parsedState) {
                    continue;
                }

                auto& state = states[static_cast<size_t>(currentStateIndex)];
                StateHitOverrideController overrideController;
                overrideController.id = nextRuntimeControllerId++;
                overrideController.trigger = *controllerTrigger;
                overrideController.attr = trim(attr->value);
                overrideController.stateNo = *parsedState;
                if (const auto* time = findProperty(section, "time")) {
                    overrideController.time = parseIntValue(time->value, overrideController.time);
                }
                state.hitOverrides.push_back(std::move(overrideController));
                continue;
            }
