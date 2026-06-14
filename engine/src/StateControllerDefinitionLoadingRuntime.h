#pragma once

// Internal App.cpp state-controller loading fragment: runtime presentation and scalar controller handlers.
// This file is included inside loadStateDefinitions and depends on its local variables.
// Include only from StateControllerDefinitionLoading.h.
// Do not include from other translation units.


        if (startsWithNoCase(controllerType, "AssertSpecial")) {
            if (!controllerTrigger) {
                continue;
            }

            StateAssertSpecialController assertSpecial;
            assertSpecial.id = nextRuntimeControllerId++;
            assertSpecial.trigger = *controllerTrigger;
            assertSpecial.flags = parseAssertSpecialFlags(section);
            if (assertSpecial.flags.empty()) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.assertSpecials.push_back(std::move(assertSpecial));
            continue;
        }

        if (startsWithNoCase(controllerType, "PosAdd")) {
            if (!controllerTrigger && triggerTime < 0 && triggerAnimElem < 0) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StatePosAddController posAdd;
            posAdd.id = controllerTrigger ? nextRuntimeControllerId++ : nextPosAddControllerId++;
            posAdd.trigger = controllerTrigger ? *controllerTrigger : controllerOptions;
            posAdd.triggerTime = triggerTime;
            posAdd.triggerAnimElem = triggerAnimElem;
            posAdd.x = findProperty(section, "x") ? parseFloatValue(findProperty(section, "x")->value, 0.0f) : 0.0f;
            posAdd.y = findProperty(section, "y") ? parseFloatValue(findProperty(section, "y")->value, 0.0f) : 0.0f;
            state.posAdds.push_back(std::move(posAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "ChangeAnim")) {
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateChangeAnimController changeAnim;
            changeAnim.id = controllerTrigger ? nextRuntimeControllerId++ : nextChangeAnimControllerId++;
            changeAnim.trigger = controllerTrigger ? *controllerTrigger : controllerOptions;
            changeAnim.triggerTime = triggerTime;
            changeAnim.triggerAnimElem = triggerAnimElem;
            changeAnim.value = parseIntValue(value->value, state.anim);
            changeAnim.valueExpression = trim(value->value);
            changeAnim.useCustomStateOwnerAnimation = startsWithNoCase(controllerType, "ChangeAnim2");
            if (const auto* elem = findProperty(section, "elem")) {
                changeAnim.elem = parseIntValue(elem->value, 1);
                changeAnim.elemExpression = trim(elem->value);
            }
            changeAnim.requiresMoveContact = requiresMoveContact;
            changeAnim.animElemTimeConditions = std::move(animElemTimeConditions);
            state.changeAnims.push_back(std::move(changeAnim));
        }

        if (startsWithNoCase(controllerType, "Helper")) {
            const auto* stateNo = findProperty(section, "stateno");
            if (!controllerTrigger || !stateNo) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateHelperController helper;
            helper.id = nextRuntimeControllerId++;
            helper.trigger = *controllerTrigger;
            helper.stateNo = parseIntValue(stateNo->value, 0);
            helper.stateNoExpression = trim(stateNo->value);
            helper.helperId = findProperty(section, "id") ? parseIntValue(findProperty(section, "id")->value, helper.stateNo) : helper.stateNo;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                const auto expressions = parseExpressionPairValue(pos->value);
                helper.x = values.first;
                helper.y = values.second;
                helper.xExpression = expressions.first;
                helper.yExpression = expressions.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                helper.postype = lowercaseCopy(trim(postype->value));
            }
            if (const auto* sprPriority = findProperty(section, "sprpriority")) {
                helper.sprPriority = parseIntValue(sprPriority->value, helper.sprPriority);
            }
            if (const auto* facing = findProperty(section, "facing")) {
                helper.facing = parseIntValue(facing->value, helper.facing);
            }
            if (const auto* pauseMoveTime = findProperty(section, "pausemovetime")) {
                helper.pauseMoveTime = parseIntValue(pauseMoveTime->value, helper.pauseMoveTime);
            }
            if (const auto* superMoveTime = findProperty(section, "supermovetime")) {
                helper.superMoveTime = parseIntValue(superMoveTime->value, helper.pauseMoveTime);
            } else {
                helper.superMoveTime = helper.pauseMoveTime;
            }
            if (const auto* scaleX = findProperty(section, "size.xscale")) {
                helper.scaleX = parseFloatValue(scaleX->value, helper.scaleX);
                helper.scaleXExpression = trim(scaleX->value);
            }
            if (const auto* scaleY = findProperty(section, "size.yscale")) {
                helper.scaleY = parseFloatValue(scaleY->value, helper.scaleY);
                helper.scaleYExpression = trim(scaleY->value);
            }
            state.helpers.push_back(std::move(helper));
            continue;
        }

        if (startsWithNoCase(controllerType, "DestroySelf")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateDestroySelfController destroySelf;
            destroySelf.id = nextRuntimeControllerId++;
            destroySelf.trigger = *controllerTrigger;
            state.destroySelfs.push_back(std::move(destroySelf));
            continue;
        }

        if (startsWithNoCase(controllerType, "BindToParent")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateBindToParentController bind;
            bind.id = nextRuntimeControllerId++;
            bind.trigger = *controllerTrigger;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                bind.x = values.first;
                bind.y = values.second;
            }
            state.bindToParents.push_back(std::move(bind));
            continue;
        }

        if (startsWithNoCase(controllerType, "BindToRoot")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateBindToRootController bind;
            bind.id = nextRuntimeControllerId++;
            bind.trigger = *controllerTrigger;
            if (const auto* time = findProperty(section, "time")) {
                bind.time = std::max(1, parseIntValue(time->value, bind.time));
            }
            if (const auto* facing = findProperty(section, "facing")) {
                bind.facing = std::clamp(parseIntValue(facing->value, bind.facing), -1, 1);
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                bind.x = values.first;
                bind.y = values.second;
            }
            state.bindToRoots.push_back(std::move(bind));
            continue;
        }

        if (startsWithNoCase(controllerType, "ParentVarAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto assignment = parseDirectVariableAssignment(section);
            if (!assignment) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateParentVarAddController parentVarAdd;
            parentVarAdd.id = nextRuntimeControllerId++;
            parentVarAdd.trigger = *controllerTrigger;
            parentVarAdd.target = assignment->first;
            parentVarAdd.valueExpression = assignment->second;
            state.parentVarAdds.push_back(std::move(parentVarAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "VarRangeSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            const auto* fvalue = findProperty(section, "fvalue");
            if (!value && !fvalue) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateVarRangeSetController rangeSet;
            rangeSet.id = nextRuntimeControllerId++;
            rangeSet.trigger = *controllerTrigger;
            rangeSet.floatBank = fvalue != nullptr;
            rangeSet.valueExpression = trim(rangeSet.floatBank ? fvalue->value : value->value);
            rangeSet.first = findProperty(section, "first") ? parseIntValue(findProperty(section, "first")->value, 0) : 0;
            rangeSet.last = findProperty(section, "last")
                ? parseIntValue(findProperty(section, "last")->value, rangeSet.floatBank ? 39 : 59)
                : (rangeSet.floatBank ? 39 : 59);
            state.varRangeSets.push_back(std::move(rangeSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "PowerAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StatePowerAddController powerAdd;
            powerAdd.id = nextRuntimeControllerId++;
            powerAdd.trigger = *controllerTrigger;
            powerAdd.valueExpression = trim(value->value);
            state.powerAdds.push_back(std::move(powerAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "LifeAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateLifeAddController lifeAdd;
            lifeAdd.id = nextRuntimeControllerId++;
            lifeAdd.trigger = *controllerTrigger;
            lifeAdd.valueExpression = trim(value->value);
            if (const auto* kill = findProperty(section, "kill")) {
                lifeAdd.kill = parseIntValue(kill->value, 1) != 0;
            }
            state.lifeAdds.push_back(std::move(lifeAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "HitAdd")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateHitAddController hitAdd;
            hitAdd.id = nextRuntimeControllerId++;
            hitAdd.trigger = *controllerTrigger;
            hitAdd.valueExpression = trim(value->value);
            state.hitAdds.push_back(std::move(hitAdd));
            continue;
        }

        if (startsWithNoCase(controllerType, "AttackDist")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAttackDistController attackDist;
            attackDist.id = nextRuntimeControllerId++;
            attackDist.trigger = *controllerTrigger;
            attackDist.valueExpression = trim(value->value);
            state.attackDists.push_back(std::move(attackDist));
            continue;
        }

        if (startsWithNoCase(controllerType, "DefenceMulSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateDefenceMulSetController defenceMulSet;
            defenceMulSet.id = nextRuntimeControllerId++;
            defenceMulSet.trigger = *controllerTrigger;
            defenceMulSet.valueExpression = trim(value->value);
            state.defenceMulSets.push_back(std::move(defenceMulSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "AttackMulSet")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAttackMulSetController attackMulSet;
            attackMulSet.id = nextRuntimeControllerId++;
            attackMulSet.trigger = *controllerTrigger;
            attackMulSet.valueExpression = trim(value->value);
            state.attackMulSets.push_back(std::move(attackMulSet));
            continue;
        }

        if (startsWithNoCase(controllerType, "AngleSet")
            || startsWithNoCase(controllerType, "AngleAdd")
            || startsWithNoCase(controllerType, "AngleMul")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* value = findProperty(section, "value");
            if (!value) {
                continue;
            }
            StateAngleController angle;
            angle.id = nextRuntimeControllerId++;
            angle.trigger = *controllerTrigger;
            angle.valueExpression = trim(value->value);
            if (startsWithNoCase(controllerType, "AngleAdd")) {
                angle.operation = StateAngleOperation::Add;
            } else if (startsWithNoCase(controllerType, "AngleMul")) {
                angle.operation = StateAngleOperation::Mul;
            } else {
                angle.operation = StateAngleOperation::Set;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.angleControllers.push_back(std::move(angle));
            continue;
        }

        if (startsWithNoCase(controllerType, "AngleDraw")) {
            if (!controllerTrigger) {
                continue;
            }
            StateAngleDrawController angleDraw;
            angleDraw.id = nextRuntimeControllerId++;
            angleDraw.trigger = *controllerTrigger;
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.angleDraws.push_back(std::move(angleDraw));
            continue;
        }

        if (startsWithNoCase(controllerType, "Offset")) {
            if (!controllerTrigger) {
                continue;
            }
            StateOffsetController offset;
            offset.id = nextRuntimeControllerId++;
            offset.trigger = *controllerTrigger;
            if (const auto* x = findProperty(section, "x")) {
                offset.hasX = true;
                offset.xExpression = trim(x->value);
            }
            if (const auto* y = findProperty(section, "y")) {
                offset.hasY = true;
                offset.yExpression = trim(y->value);
            }
            if (!offset.hasX && !offset.hasY) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.offsets.push_back(std::move(offset));
            continue;
        }

        if (startsWithNoCase(controllerType, "ForceFeedback")) {
            if (!controllerTrigger) {
                continue;
            }
            StateForceFeedbackController forceFeedback;
            forceFeedback.id = nextRuntimeControllerId++;
            forceFeedback.trigger = *controllerTrigger;
            if (const auto* waveform = findProperty(section, "waveform")) {
                forceFeedback.waveform = lowercaseCopy(trim(waveform->value));
            }
            if (const auto* time = findProperty(section, "time")) {
                forceFeedback.time = std::max(0, parseIntValue(time->value, forceFeedback.time));
            }
            if (const auto* ampl = findProperty(section, "ampl")) {
                const auto values = splitCsv(ampl->value);
                if (!values.empty()) {
                    forceFeedback.amplitude = std::clamp(parseIntValue(values.front(), forceFeedback.amplitude), 0, 255);
                }
            }
            if (const auto* self = findProperty(section, "self")) {
                forceFeedback.self = parseIntValue(self->value, forceFeedback.self ? 1 : 0) != 0;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.forceFeedbacks.push_back(std::move(forceFeedback));
            continue;
        }

        if (startsWithNoCase(controllerType, "GameMakeAnim")) {
            if (!controllerTrigger) {
                continue;
            }
            StateGameMakeAnimController gameMakeAnim;
            gameMakeAnim.id = nextRuntimeControllerId++;
            gameMakeAnim.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                gameMakeAnim.valueExpression = trim(value->value);
            }
            if (const auto* under = findProperty(section, "under")) {
                gameMakeAnim.under = parseIntValue(under->value, 0) != 0;
            }
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseFloatPairValue(pos->value);
                gameMakeAnim.x = values.first;
                gameMakeAnim.y = values.second;
            }
            if (const auto* random = findProperty(section, "random")) {
                gameMakeAnim.random = std::max(0, parseIntValue(random->value, 0));
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.gameMakeAnims.push_back(std::move(gameMakeAnim));
            continue;
        }

        if (startsWithNoCase(controllerType, "DisplayToClipboard")
            || startsWithNoCase(controllerType, "AppendToClipboard")) {
            if (!controllerTrigger) {
                continue;
            }
            StateClipboardController clipboard;
            clipboard.id = nextRuntimeControllerId++;
            clipboard.trigger = *controllerTrigger;
            clipboard.append = startsWithNoCase(controllerType, "AppendToClipboard");
            if (const auto* text = findProperty(section, "text")) {
                clipboard.text = unquote(trim(text->value));
            }
            if (const auto* params = findProperty(section, "params")) {
                clipboard.params = splitCsv(params->value);
                for (auto& param : clipboard.params) {
                    param = trim(param);
                }
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.clipboards.push_back(std::move(clipboard));
            continue;
        }

