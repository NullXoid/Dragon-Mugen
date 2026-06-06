#pragma once

// Internal App.cpp state-controller loading fragment: projectile, palette remap, afterimage, and dust controller handlers.
// This file is included inside loadStateDefinitions and depends on its local variables.
// Include only from StateControllerDefinitionLoading.h.
// Do not include from other translation units.

        if (startsWithNoCase(controllerType, "VictoryQuote")) {
            if (!controllerTrigger) {
                continue;
            }
            StateVictoryQuoteController victoryQuote;
            victoryQuote.id = nextRuntimeControllerId++;
            victoryQuote.trigger = *controllerTrigger;
            if (const auto* value = findProperty(section, "value")) {
                victoryQuote.valueExpression = trim(value->value);
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.victoryQuotes.push_back(std::move(victoryQuote));
            continue;
        }

        if (startsWithNoCase(controllerType, "RemapPal")) {
            if (!controllerTrigger) {
                continue;
            }
            StateRemapPalController remapPal;
            remapPal.id = nextRuntimeControllerId++;
            remapPal.trigger = *controllerTrigger;
            if (const auto* source = findProperty(section, "source")) {
                const auto values = splitCsv(source->value);
                if (!values.empty()) {
                    remapPal.remap.sourceGroupExpression = trim(values[0]);
                }
                if (values.size() >= 2) {
                    remapPal.remap.sourceItemExpression = trim(values[1]);
                }
            }
            if (const auto* dest = findProperty(section, "dest")) {
                const auto values = splitCsv(dest->value);
                if (!values.empty()) {
                    remapPal.remap.destGroupExpression = trim(values[0]);
                }
                if (values.size() >= 2) {
                    remapPal.remap.destItemExpression = trim(values[1]);
                }
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            state.remapPals.push_back(std::move(remapPal));
            continue;
        }

        if (startsWithNoCase(controllerType, "Trans")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateTransController trans;
            trans.id = nextRuntimeControllerId++;
            trans.trigger = *controllerTrigger;
            if (const auto* transValue = findProperty(section, "trans")) {
                trans.trans = lowercaseCopy(trim(transValue->value));
            }
            if (const auto* alpha = findProperty(section, "alpha")) {
                const auto values = splitCsv(alpha->value);
                if (!values.empty()) {
                    trans.alphaSourceExpression = values[0];
                }
                if (values.size() >= 2) {
                    trans.alphaDestExpression = values[1];
                }
            }
            state.transControllers.push_back(std::move(trans));
            continue;
        }

        if (startsWithNoCase(controllerType, "AfterImageTime")) {
            if (!controllerTrigger) {
                continue;
            }
            const auto* time = findProperty(section, "time");
            if (!time) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAfterImageTimeController afterImageTime;
            afterImageTime.id = nextRuntimeControllerId++;
            afterImageTime.trigger = *controllerTrigger;
            afterImageTime.timeExpression = trim(time->value);
            state.afterImageTimes.push_back(std::move(afterImageTime));
            continue;
        }

        if (startsWithNoCase(controllerType, "AfterImage")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateAfterImageController afterImage;
            afterImage.id = nextRuntimeControllerId++;
            afterImage.trigger = *controllerTrigger;
            if (const auto* time = findProperty(section, "time")) {
                afterImage.time = parseIntValue(time->value, afterImage.time);
            }
            if (const auto* length = findProperty(section, "length")) {
                afterImage.length = std::max(1, parseIntValue(length->value, afterImage.length));
            }
            if (const auto* timeGap = findProperty(section, "timegap")) {
                afterImage.timeGap = std::max(1, parseIntValue(timeGap->value, afterImage.timeGap));
            }
            if (const auto* frameGap = findProperty(section, "framegap")) {
                afterImage.frameGap = std::max(1, parseIntValue(frameGap->value, afterImage.frameGap));
            }
            if (const auto* trans = findProperty(section, "trans")) {
                const std::string mode = lowercaseCopy(trim(trans->value));
                afterImage.additive = mode.find("add") != std::string::npos;
            }
            state.afterImages.push_back(std::move(afterImage));
            continue;
        }

        if (startsWithNoCase(controllerType, "Projectile")) {
            if (!controllerTrigger) {
                continue;
            }

            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateProjectileController projectile;
            projectile.id = nextRuntimeControllerId++;
            projectile.trigger = *controllerTrigger;
            projectile.projectileId = findProperty(section, "projid") ? parseIntValue(findProperty(section, "projid")->value, 0) : projectile.id;
            projectile.anim = findProperty(section, "projanim") ? parseIntValue(findProperty(section, "projanim")->value, -1) : -1;
            projectile.hitAnim = findProperty(section, "projhitanim") ? parseIntValue(findProperty(section, "projhitanim")->value, projectile.anim) : projectile.anim;
            projectile.removeAnim = findProperty(section, "projremanim") ? parseIntValue(findProperty(section, "projremanim")->value, projectile.hitAnim) : projectile.hitAnim;
            projectile.cancelAnim = findProperty(section, "projcancelanim") ? parseIntValue(findProperty(section, "projcancelanim")->value, projectile.removeAnim) : projectile.removeAnim;
            projectile.hits = findProperty(section, "projhits") ? std::max(1, parseIntValue(findProperty(section, "projhits")->value, 1)) : 1;
            projectile.missTime = findProperty(section, "projmisstime") ? std::max(0, parseIntValue(findProperty(section, "projmisstime")->value, 0)) : 0;
            projectile.removeTime = findProperty(section, "projremovetime") ? parseIntValue(findProperty(section, "projremovetime")->value, -1) : -1;
            projectile.removeWhenHit = findProperty(section, "projremove") ? parseIntValue(findProperty(section, "projremove")->value, projectile.removeWhenHit) : projectile.removeWhenHit;
            projectile.pauseMoveTime = findProperty(section, "pausemovetime") ? std::max(0, parseIntValue(findProperty(section, "pausemovetime")->value, 0)) : 0;
            projectile.superMoveTime = findProperty(section, "supermovetime") ? std::max(0, parseIntValue(findProperty(section, "supermovetime")->value, projectile.pauseMoveTime)) : projectile.pauseMoveTime;
            if (const auto* priority = findProperty(section, "projpriority")) {
                const auto values = parseIntPairValue(priority->value, projectile.priority, projectile.cancelPriority);
                projectile.priority = values.first;
                projectile.cancelPriority = values.second;
            } else if (const auto* priority = findProperty(section, "priority")) {
                const auto values = parseIntPairValue(priority->value, projectile.priority, projectile.cancelPriority);
                projectile.priority = values.first;
                projectile.cancelPriority = values.second;
            }
            if (const auto* edgeBound = findProperty(section, "projedgebound")) {
                projectile.projEdgeBound = parseFloatValue(edgeBound->value, projectile.projEdgeBound);
            }
            if (const auto* screenBound = findProperty(section, "projscreenbound")) {
                projectile.projEdgeBound = parseFloatValue(screenBound->value, projectile.projEdgeBound);
            }
            if (const auto* stageBound = findProperty(section, "projstagebound")) {
                projectile.projStageBound = parseFloatValue(stageBound->value, projectile.projStageBound);
            }
            if (const auto* heightBound = findProperty(section, "projheightbound")) {
                const auto values = parseFloatPairValue(heightBound->value, projectile.projHeightBoundLow, projectile.projHeightBoundHigh);
                projectile.projHeightBoundLow = values.first;
                projectile.projHeightBoundHigh = values.second;
            }
            if (const auto* scale = findProperty(section, "projscale")) {
                const auto values = parseFloatPairValue(scale->value, 1.0f, 1.0f);
                projectile.scaleX = values.first;
                projectile.scaleY = values.second;
                const auto expressions = parseExpressionPairValue(scale->value, "1", "1");
                projectile.scaleXExpression = expressions.first;
                projectile.scaleYExpression = expressions.second;
            }
            if (const auto* shadow = findProperty(section, "projshadow")) {
                const auto parts = splitCsv(shadow->value);
                if (!parts.empty() && parseIntValue(parts[0], 0) == -1) {
                    projectile.shadowEnabled = true;
                    projectile.shadowR = 96;
                    projectile.shadowG = 96;
                    projectile.shadowB = 96;
                } else {
                    const auto values = parseIntTripleValue(shadow->value, 0, 0, 0);
                    projectile.shadowEnabled = values[0] != 0 || values[1] != 0 || values[2] != 0;
                    projectile.shadowR = values[0];
                    projectile.shadowG = values[1];
                    projectile.shadowB = values[2];
                }
            }
            if (const auto* velocity = findProperty(section, "velocity")) {
                const auto values = parseFloatPairValue(velocity->value);
                projectile.vx = values.first;
                projectile.vy = values.second;
                const auto expressions = parseExpressionPairValue(velocity->value);
                projectile.vxExpression = expressions.first;
                projectile.vyExpression = expressions.second;
            }
            if (const auto* removeVelocity = findProperty(section, "remvelocity")) {
                const auto values = parseFloatPairValue(removeVelocity->value);
                projectile.removeVx = values.first;
                projectile.removeVy = values.second;
                const auto expressions = parseExpressionPairValue(removeVelocity->value);
                projectile.removeVxExpression = expressions.first;
                projectile.removeVyExpression = expressions.second;
            }
            if (const auto* accel = findProperty(section, "accel")) {
                const auto values = parseFloatPairValue(accel->value);
                projectile.ax = values.first;
                projectile.ay = values.second;
                const auto expressions = parseExpressionPairValue(accel->value);
                projectile.axExpression = expressions.first;
                projectile.ayExpression = expressions.second;
            }
            if (const auto* velMul = findProperty(section, "velmul")) {
                const auto values = parseFloatPairValue(velMul->value, 1.0f, 1.0f);
                projectile.velMulX = values.first;
                projectile.velMulY = values.second;
                const auto expressions = parseExpressionPairValue(velMul->value, "1", "1");
                projectile.velMulXExpression = expressions.first;
                projectile.velMulYExpression = expressions.second;
            }
            if (const auto* offset = findProperty(section, "offset")) {
                const auto values = parseFloatPairValue(offset->value);
                projectile.x = values.first;
                projectile.y = values.second;
                const auto expressions = parseExpressionPairValue(offset->value);
                projectile.xExpression = expressions.first;
                projectile.yExpression = expressions.second;
            }
            if (const auto* postype = findProperty(section, "postype")) {
                projectile.postype = lowercaseCopy(trim(postype->value));
            }

            projectile.hitDef.id = projectile.id;
            projectile.hitDef.targetId = projectile.projectileId;
            projectile.hitDef.stateNo = state.stateNo;
            if (const auto* attr = findProperty(section, "attr")) {
                projectile.hitDef.attr = trim(attr->value);
            }
            if (const auto* damage = findProperty(section, "damage")) {
                const auto values = parseIntPairValue(damage->value);
                projectile.hitDef.damage = values.first;
                projectile.hitDef.guardDamage = values.second;
                const auto expressions = parseExpressionPairValue(damage->value);
                projectile.hitDef.damageExpression = expressions.first;
                projectile.hitDef.guardDamageExpression = expressions.second;
            }
            if (const auto* animtype = findProperty(section, "animtype")) {
                projectile.hitDef.animtype = trim(animtype->value);
            }
            if (const auto* hitflag = findProperty(section, "hitflag")) {
                projectile.hitDef.hitflag = trim(hitflag->value);
            }
            if (const auto* guardflag = findProperty(section, "guardflag")) {
                projectile.hitDef.guardflag = trim(guardflag->value);
            }
            if (const auto* guardDistance = findProperty(section, "guard.dist")) {
                projectile.hitDef.guardDistance = parseIntValue(guardDistance->value, projectile.hitDef.guardDistance);
            }
            if (const auto* pausetime = findProperty(section, "pausetime")) {
                const auto values = parseIntPairValue(pausetime->value);
                projectile.hitDef.pausetimeP1 = values.first;
                projectile.hitDef.pausetimeP2 = values.second;
                const auto expressions = parseExpressionPairValue(pausetime->value);
                projectile.hitDef.pausetimeP1Expression = expressions.first;
                projectile.hitDef.pausetimeP2Expression = expressions.second;
            }
            if (const auto* sparkNo = findProperty(section, "sparkno")) {
                projectile.hitDef.sparkNo = parseIntValue(sparkNo->value, projectile.hitDef.sparkNo);
                projectile.hitDef.sparkNoExpression = trim(sparkNo->value);
            }
            if (const auto* guardSparkNo = findProperty(section, "guard.sparkno")) {
                projectile.hitDef.guardSparkNo = parseIntValue(guardSparkNo->value, projectile.hitDef.guardSparkNo);
                projectile.hitDef.guardSparkNoExpression = trim(guardSparkNo->value);
            }
            if (const auto* sparkxy = findProperty(section, "sparkxy")) {
                const auto values = parseFloatPairValue(sparkxy->value);
                projectile.hitDef.sparkX = values.first;
                projectile.hitDef.sparkY = values.second;
                const auto expressions = parseExpressionPairValue(sparkxy->value);
                projectile.hitDef.sparkXExpression = expressions.first;
                projectile.hitDef.sparkYExpression = expressions.second;
            }
            if (const auto* hitSound = findProperty(section, "hitsound")) {
                if (const auto values = parseSoundValue(hitSound->value)) {
                    projectile.hitDef.hitSoundGroup = values->group;
                    projectile.hitDef.hitSoundIndex = values->index;
                    projectile.hitDef.hitSoundForceCommon = values->forceCommon;
                    projectile.hitDef.hitSoundGroupExpression = values->groupExpression;
                    projectile.hitDef.hitSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* guardSound = findProperty(section, "guardsound")) {
                if (const auto values = parseSoundValue(guardSound->value)) {
                    projectile.hitDef.guardSoundGroup = values->group;
                    projectile.hitDef.guardSoundIndex = values->index;
                    projectile.hitDef.guardSoundForceCommon = values->forceCommon;
                    projectile.hitDef.guardSoundGroupExpression = values->groupExpression;
                    projectile.hitDef.guardSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* groundType = findProperty(section, "ground.type")) {
                projectile.hitDef.groundType = trim(groundType->value);
            }
            if (const auto* groundSlideTime = findProperty(section, "ground.slidetime")) {
                projectile.hitDef.groundSlideTime = parseIntValue(groundSlideTime->value, projectile.hitDef.groundSlideTime);
                projectile.hitDef.groundSlideTimeExpression = trim(groundSlideTime->value);
            }
            if (const auto* groundHitTime = findProperty(section, "ground.hittime")) {
                projectile.hitDef.groundHitTime = parseIntValue(groundHitTime->value, projectile.hitDef.groundHitTime);
                projectile.hitDef.groundHitTimeExpression = trim(groundHitTime->value);
            }
            if (const auto* groundVelocity = findProperty(section, "ground.velocity")) {
                const auto values = parseFloatPairValue(groundVelocity->value);
                projectile.hitDef.groundVelocityX = values.first;
                projectile.hitDef.groundVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(groundVelocity->value);
                projectile.hitDef.groundVelocityXExpression = expressions.first;
                projectile.hitDef.groundVelocityYExpression = expressions.second;
            }
            if (const auto* airVelocity = findProperty(section, "air.velocity")) {
                const auto values = parseFloatPairValue(airVelocity->value);
                projectile.hitDef.hasAirVelocity = true;
                projectile.hitDef.airVelocityX = values.first;
                projectile.hitDef.airVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(airVelocity->value);
                projectile.hitDef.airVelocityXExpression = expressions.first;
                projectile.hitDef.airVelocityYExpression = expressions.second;
            }
            if (const auto* airHitTime = findProperty(section, "air.hittime")) {
                projectile.hitDef.airHitTime = parseIntValue(airHitTime->value, projectile.hitDef.airHitTime);
                projectile.hitDef.airHitTimeExpression = trim(airHitTime->value);
            }
            if (const auto* fall = findProperty(section, "fall")) {
                projectile.hitDef.fall = parseIntValue(fall->value, 0) != 0;
                projectile.hitDef.fallExpression = trim(fall->value);
            }
            if (const auto* airFall = findProperty(section, "air.fall")) {
                projectile.hitDef.airFall = parseIntValue(airFall->value, 0) != 0;
                projectile.hitDef.airFallExpression = trim(airFall->value);
            }
            if (const auto* fallRecover = findProperty(section, "fall.recover")) {
                projectile.hitDef.fallRecover = parseIntValue(fallRecover->value, 1) != 0;
                projectile.hitDef.fallRecoverExpression = trim(fallRecover->value);
            }
            if (const auto* fallRecoverTime = findProperty(section, "fall.recovertime")) {
                projectile.hitDef.fallRecoverTime = parseIntValue(fallRecoverTime->value, projectile.hitDef.fallRecoverTime);
                projectile.hitDef.fallRecoverTimeExpression = trim(fallRecoverTime->value);
            }
            if (const auto* fallDamage = findProperty(section, "fall.damage")) {
                projectile.hitDef.fallDamage = parseIntValue(fallDamage->value, 0);
                projectile.hitDef.fallDamageExpression = trim(fallDamage->value);
            }
            if (const auto* fallXVelocity = findProperty(section, "fall.xvelocity")) {
                projectile.hitDef.hasFallXVelocity = true;
                projectile.hitDef.fallXVelocity = parseFloatValue(fallXVelocity->value, projectile.hitDef.fallXVelocity);
                projectile.hitDef.fallXVelocityExpression = trim(fallXVelocity->value);
            }
            if (const auto* fallYVelocity = findProperty(section, "fall.yvelocity")) {
                projectile.hitDef.hasFallYVelocity = true;
                projectile.hitDef.fallYVelocity = parseFloatValue(fallYVelocity->value, projectile.hitDef.fallYVelocity);
                projectile.hitDef.fallYVelocityExpression = trim(fallYVelocity->value);
            }
            if (const auto* yAccel = findProperty(section, "yaccel")) {
                projectile.hitDef.hasYAccel = true;
                projectile.hitDef.yAccel = parseFloatValue(yAccel->value, 0.0f);
                projectile.hitDef.yAccelExpression = trim(yAccel->value);
            }
            if (const auto* guardVelocity = findProperty(section, "guard.velocity")) {
                const auto values = parseFloatPairValue(guardVelocity->value);
                projectile.hitDef.hasGuardVelocity = true;
                projectile.hitDef.guardVelocityX = values.first;
                projectile.hitDef.guardVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(guardVelocity->value);
                projectile.hitDef.guardVelocityXExpression = expressions.first;
                projectile.hitDef.guardVelocityYExpression = expressions.second;
            } else {
                projectile.hitDef.guardVelocityX = projectile.hitDef.groundVelocityX;
                projectile.hitDef.guardVelocityY = projectile.hitDef.groundVelocityY;
            }
            if (const auto* envShakeTime = findProperty(section, "envshake.time")) {
                projectile.hitDef.envShake.time = std::max(0, parseIntValue(envShakeTime->value, projectile.hitDef.envShake.time));
                projectile.hitDef.envShake.timeExpression = trim(envShakeTime->value);
                projectile.hitDef.envShake.enabled = projectile.hitDef.envShake.time > 0;
            }
            if (const auto* envShakeFrequency = findProperty(section, "envshake.freq")) {
                projectile.hitDef.envShake.frequency = std::max(1, parseIntValue(envShakeFrequency->value, projectile.hitDef.envShake.frequency));
                projectile.hitDef.envShake.frequencyExpression = trim(envShakeFrequency->value);
            }
            if (const auto* envShakeAmplitude = findProperty(section, "envshake.ampl")) {
                projectile.hitDef.envShake.amplitude = parseFloatValue(envShakeAmplitude->value, projectile.hitDef.envShake.amplitude);
                projectile.hitDef.envShake.amplitudeExpression = trim(envShakeAmplitude->value);
                projectile.hitDef.envShake.enabled = projectile.hitDef.envShake.time > 0 && std::abs(projectile.hitDef.envShake.amplitude) > 0.001f;
            }
            if (const auto* envShakePhase = findProperty(section, "envshake.phase")) {
                projectile.hitDef.envShake.phase = parseIntValue(envShakePhase->value, projectile.hitDef.envShake.phase);
                projectile.hitDef.envShake.phaseExpression = trim(envShakePhase->value);
            }
            if (const auto* fallEnvShakeTime = findProperty(section, "fall.envshake.time")) {
                projectile.hitDef.fallEnvShake.time = std::max(0, parseIntValue(fallEnvShakeTime->value, projectile.hitDef.fallEnvShake.time));
                projectile.hitDef.fallEnvShake.timeExpression = trim(fallEnvShakeTime->value);
                projectile.hitDef.fallEnvShake.enabled = projectile.hitDef.fallEnvShake.time > 0;
            }
            if (const auto* fallEnvShakeFrequency = findProperty(section, "fall.envshake.freq")) {
                projectile.hitDef.fallEnvShake.frequency = std::max(1, parseIntValue(fallEnvShakeFrequency->value, projectile.hitDef.fallEnvShake.frequency));
                projectile.hitDef.fallEnvShake.frequencyExpression = trim(fallEnvShakeFrequency->value);
            }
            if (const auto* fallEnvShakeAmplitude = findProperty(section, "fall.envshake.ampl")) {
                projectile.hitDef.fallEnvShake.amplitude = parseFloatValue(fallEnvShakeAmplitude->value, projectile.hitDef.fallEnvShake.amplitude);
                projectile.hitDef.fallEnvShake.amplitudeExpression = trim(fallEnvShakeAmplitude->value);
                projectile.hitDef.fallEnvShake.enabled = projectile.hitDef.fallEnvShake.time > 0 && std::abs(projectile.hitDef.fallEnvShake.amplitude) > 0.001f;
            }
            if (const auto* fallEnvShakePhase = findProperty(section, "fall.envshake.phase")) {
                projectile.hitDef.fallEnvShake.phase = parseIntValue(fallEnvShakePhase->value, projectile.hitDef.fallEnvShake.phase);
                projectile.hitDef.fallEnvShake.phaseExpression = trim(fallEnvShakePhase->value);
            }
            projectile.hitDef.palFx = parsePaletteEffectSpec(section);
            if (projectile.anim >= 0) {
                state.projectiles.push_back(std::move(projectile));
            }
            continue;
        }

        if (startsWithNoCase(controllerType, "MakeDust")) {
            if (!controllerTrigger) {
                continue;
            }
            auto& state = states[static_cast<size_t>(currentStateIndex)];
            StateMakeDustController makeDust;
            makeDust.id = nextRuntimeControllerId++;
            makeDust.trigger = *controllerTrigger;
            if (const auto* pos = findProperty(section, "pos")) {
                const auto values = parseExplodPosition(pos->value, constants);
                makeDust.x = values.first;
                makeDust.y = values.second;
            }
            if (const auto* pos2 = findProperty(section, "pos2")) {
                const auto values = parseExplodPosition(pos2->value, constants);
                makeDust.hasPos2 = true;
                makeDust.x2 = values.first;
                makeDust.y2 = values.second;
            }
            if (const auto* spacing = findProperty(section, "spacing")) {
                makeDust.spacing = std::max(1, parseIntValue(spacing->value, makeDust.spacing));
            }
            state.makeDusts.push_back(std::move(makeDust));
            continue;
        }
