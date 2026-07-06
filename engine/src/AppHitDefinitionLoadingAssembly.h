#pragma once

// Internal App.cpp implementation shard.
// HitDef loading and late runtime-loading include wiring.

bool parseP2DistanceXCondition(const std::string& clause, HitDefinition& hitDef) {
    const std::string condition = stripOuterParens(clause);
    size_t subjectEnd = 0;
    const auto subject = parseStateTriggerSubject(condition, subjectEnd);
    if (!subject || (*subject != StateTriggerSubject::P2DistX && *subject != StateTriggerSubject::P2BodyDistX)) {
        return false;
    }

    const std::string tail = trim(std::string_view(condition).substr(subjectEnd));
    const auto compare = findCompareOp(tail);
    if (!compare) {
        return false;
    }

    const auto [op, pos] = *compare;
    const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
    const auto value = parsePlainFloatValue(trim(std::string_view(tail).substr(pos + opLength)));
    if (!value) {
        return false;
    }

    if (*subject == StateTriggerSubject::P2DistX) {
        hitDef.hasP2DistX = true;
        hitDef.p2DistXOp = op;
        hitDef.p2DistX = *value;
    } else {
        hitDef.hasP2BodyDistX = true;
        hitDef.p2BodyDistXOp = op;
        hitDef.p2BodyDistX = *value;
    }
    return true;
}

std::vector<HitDefinition> loadHitDefinitions(const CharacterFiles& files) {
    const auto documents = loadCharacterStateDocuments(files);
    std::vector<HitDefinition> hitDefs;
    int nextHitDefId = 1;
    int defaultSparkNo = 2;
    int defaultGuardSparkNo = 40;
    for (const auto& cns : documents) {
        if (const auto* data = findSection(cns, "Data")) {
            if (const auto* sparkNo = findProperty(*data, "sparkno")) {
                defaultSparkNo = parseIntValue(sparkNo->value, defaultSparkNo);
            }
            if (const auto* guardSparkNo = findProperty(*data, "guard.sparkno")) {
                defaultGuardSparkNo = parseIntValue(guardSparkNo->value, defaultGuardSparkNo);
            }
        }
    }

    for (const auto& cns : documents) {
        int currentState = -1;
        for (const auto& section : cns.sections) {
            if (const auto stateNo = parseStateNumber(section.name, "Statedef ")) {
                currentState = *stateNo;
                continue;
            }
            if (currentState < 0 || !startsWithNoCase(section.name, "State ")) {
                continue;
            }

            const auto* type = findProperty(section, "type");
            if (!type || !startsWithNoCase(trim(type->value), "HitDef")) {
                continue;
            }

            HitDefinition hitDef;
            hitDef.id = nextHitDefId++;
            hitDef.stateNo = currentState;
            hitDef.sparkNo = defaultSparkNo;
            hitDef.guardSparkNo = defaultGuardSparkNo;
            for (const auto& property : section.properties) {
                if (!startsWithNoCase(property.key, "trigger")) {
                    continue;
                }
                const auto trigger = trim(property.value);
                for (const auto& clause : splitAndClauses(trigger)) {
                    if (startsWithNoCase(clause, "Time")) {
                        if (const auto equals = clause.find('='); equals != std::string::npos) {
                            hitDef.triggerTime = parseIntValue(trim(std::string_view(clause).substr(equals + 1)), hitDef.triggerTime);
                        }
                    } else if (startsWithNoCase(clause, "AnimElem") && !startsWithNoCase(clause, "AnimElemTime")) {
                        if (const auto equals = clause.find('='); equals != std::string::npos) {
                            hitDef.triggerAnimElem = parseIntValue(trim(std::string_view(clause).substr(equals + 1)), hitDef.triggerAnimElem);
                        }
                    } else {
                        parseP2DistanceXCondition(clause, hitDef);
                    }
                }
            }
            if (const auto* attr = findProperty(section, "attr")) {
                hitDef.attr = trim(attr->value);
            }
            if (const auto* id = findProperty(section, "id")) {
                hitDef.targetId = std::max(0, parseIntValue(id->value, hitDef.targetId));
            }
            if (const auto* animtype = findProperty(section, "animtype")) {
                hitDef.animtype = trim(animtype->value);
            }
            if (const auto* hitflag = findProperty(section, "hitflag")) {
                hitDef.hitflag = trim(hitflag->value);
            }
            if (const auto* guardflag = findProperty(section, "guardflag")) {
                hitDef.guardflag = trim(guardflag->value);
            }
            if (const auto* damage = findProperty(section, "damage")) {
                const auto values = parseIntPairValue(damage->value);
                hitDef.damage = values.first;
                hitDef.guardDamage = values.second;
                const auto expressions = parseExpressionPairValue(damage->value);
                hitDef.damageExpression = expressions.first;
                hitDef.guardDamageExpression = expressions.second;
            }
            if (const auto* guardDamage = findProperty(section, "guard.damage")) {
                hitDef.guardDamage = parseIntValue(guardDamage->value, hitDef.guardDamage);
                hitDef.guardDamageExpression = trim(guardDamage->value);
            }
            if (const auto* guardDistance = findProperty(section, "guard.dist")) {
                hitDef.guardDistance = parseIntValue(guardDistance->value, hitDef.guardDistance);
            }
            if (const auto* pausetime = findProperty(section, "pausetime")) {
                const auto values = parseIntPairValue(pausetime->value);
                hitDef.pausetimeP1 = values.first;
                hitDef.pausetimeP2 = values.second;
                const auto expressions = parseExpressionPairValue(pausetime->value);
                hitDef.pausetimeP1Expression = expressions.first;
                hitDef.pausetimeP2Expression = expressions.second;
            }
            if (const auto* sparkNo = findProperty(section, "sparkno")) {
                hitDef.sparkNo = parseIntValue(sparkNo->value, hitDef.sparkNo);
                hitDef.sparkNoExpression = trim(sparkNo->value);
            }
            if (const auto* guardSparkNo = findProperty(section, "guard.sparkno")) {
                hitDef.guardSparkNo = parseIntValue(guardSparkNo->value, hitDef.guardSparkNo);
                hitDef.guardSparkNoExpression = trim(guardSparkNo->value);
            }
            if (const auto* sparkxy = findProperty(section, "sparkxy")) {
                const auto values = parseFloatPairValue(sparkxy->value);
                hitDef.sparkX = values.first;
                hitDef.sparkY = values.second;
                const auto expressions = parseExpressionPairValue(sparkxy->value);
                hitDef.sparkXExpression = expressions.first;
                hitDef.sparkYExpression = expressions.second;
            }
            if (const auto* hitSound = findProperty(section, "hitsound")) {
                if (const auto values = parseSoundValue(hitSound->value)) {
                    hitDef.hitSoundGroup = values->group;
                    hitDef.hitSoundIndex = values->index;
                    hitDef.hitSoundForceCommon = values->forceCommon;
                    hitDef.hitSoundGroupExpression = values->groupExpression;
                    hitDef.hitSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* guardSound = findProperty(section, "guardsound")) {
                if (const auto values = parseSoundValue(guardSound->value)) {
                    hitDef.guardSoundGroup = values->group;
                    hitDef.guardSoundIndex = values->index;
                    hitDef.guardSoundForceCommon = values->forceCommon;
                    hitDef.guardSoundGroupExpression = values->groupExpression;
                    hitDef.guardSoundIndexExpression = values->indexExpression;
                }
            }
            if (const auto* groundType = findProperty(section, "ground.type")) {
                hitDef.groundType = trim(groundType->value);
            }
            if (const auto* groundSlideTime = findProperty(section, "ground.slidetime")) {
                hitDef.groundSlideTime = parseIntValue(groundSlideTime->value, hitDef.groundSlideTime);
                hitDef.groundSlideTimeExpression = trim(groundSlideTime->value);
            }
            if (const auto* groundHitTime = findProperty(section, "ground.hittime")) {
                hitDef.groundHitTime = parseIntValue(groundHitTime->value, hitDef.groundHitTime);
                hitDef.groundHitTimeExpression = trim(groundHitTime->value);
            }
            if (const auto* groundVelocity = findProperty(section, "ground.velocity")) {
                const auto values = parseFloatPairValue(groundVelocity->value);
                hitDef.groundVelocityX = values.first;
                hitDef.groundVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(groundVelocity->value);
                hitDef.groundVelocityXExpression = expressions.first;
                hitDef.groundVelocityYExpression = expressions.second;
            }
            if (const auto* airVelocity = findProperty(section, "air.velocity")) {
                const auto values = parseFloatPairValue(airVelocity->value);
                hitDef.hasAirVelocity = true;
                hitDef.airVelocityX = values.first;
                hitDef.airVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(airVelocity->value);
                hitDef.airVelocityXExpression = expressions.first;
                hitDef.airVelocityYExpression = expressions.second;
            }
            if (const auto* airHitTime = findProperty(section, "air.hittime")) {
                hitDef.airHitTime = parseIntValue(airHitTime->value, hitDef.airHitTime);
                hitDef.airHitTimeExpression = trim(airHitTime->value);
            }
            if (const auto* snap = findProperty(section, "snap")) {
                const auto values = parseFloatPairValue(snap->value);
                const auto expressions = parseExpressionPairValue(snap->value);
                hitDef.hasSnap = true;
                hitDef.snapX = values.first;
                hitDef.snapY = values.second;
                hitDef.snapXExpression = expressions.first;
                hitDef.snapYExpression = expressions.second;
            }
            if (const auto* fall = findProperty(section, "fall")) {
                hitDef.fall = parseIntValue(fall->value, 0) != 0;
                hitDef.fallExpression = trim(fall->value);
            }
            if (const auto* airFall = findProperty(section, "air.fall")) {
                hitDef.airFall = parseIntValue(airFall->value, 0) != 0;
                hitDef.airFallExpression = trim(airFall->value);
            }
            if (const auto* fallAnimtype = findProperty(section, "fall.animtype")) {
                hitDef.fallAnimtype = trim(fallAnimtype->value);
            }
            if (const auto* fallRecover = findProperty(section, "fall.recover")) {
                hitDef.fallRecover = parseIntValue(fallRecover->value, 1) != 0;
                hitDef.fallRecoverExpression = trim(fallRecover->value);
            }
            if (const auto* fallRecoverTime = findProperty(section, "fall.recovertime")) {
                hitDef.fallRecoverTime = parseIntValue(fallRecoverTime->value, hitDef.fallRecoverTime);
                hitDef.fallRecoverTimeExpression = trim(fallRecoverTime->value);
            }
            if (const auto* fallDamage = findProperty(section, "fall.damage")) {
                hitDef.fallDamage = parseIntValue(fallDamage->value, 0);
                hitDef.fallDamageExpression = trim(fallDamage->value);
            }
            if (const auto* downRecover = findProperty(section, "down.recover")) {
                hitDef.downRecover = parseIntValue(downRecover->value, 1) != 0;
                hitDef.downRecoverExpression = trim(downRecover->value);
            }
            if (const auto* downRecoverTime = findProperty(section, "down.recovertime")) {
                hitDef.downRecoverTime = parseIntValue(downRecoverTime->value, hitDef.downRecoverTime);
                hitDef.downRecoverTimeExpression = trim(downRecoverTime->value);
            }
            if (const auto* downVelocity = findProperty(section, "down.velocity")) {
                const auto values = parseFloatPairValue(downVelocity->value);
                hitDef.hasDownVelocity = true;
                hitDef.downVelocityX = values.first;
                hitDef.downVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(downVelocity->value);
                hitDef.downVelocityXExpression = expressions.first;
                hitDef.downVelocityYExpression = expressions.second;
            }
            if (const auto* downHitTime = findProperty(section, "down.hittime")) {
                hitDef.downHitTime = parseIntValue(downHitTime->value, hitDef.downHitTime);
                hitDef.downHitTimeExpression = trim(downHitTime->value);
            }
            if (const auto* downBounce = findProperty(section, "down.bounce")) {
                hitDef.downBounce = parseIntValue(downBounce->value, 0) != 0;
                hitDef.downBounceExpression = trim(downBounce->value);
            }
            if (const auto* fallXVelocity = findProperty(section, "fall.xvelocity")) {
                hitDef.hasFallXVelocity = true;
                hitDef.fallXVelocity = parseFloatValue(fallXVelocity->value, hitDef.fallXVelocity);
                hitDef.fallXVelocityExpression = trim(fallXVelocity->value);
            }
            if (const auto* fallYVelocity = findProperty(section, "fall.yvelocity")) {
                hitDef.hasFallYVelocity = true;
                hitDef.fallYVelocity = parseFloatValue(fallYVelocity->value, hitDef.fallYVelocity);
                hitDef.fallYVelocityExpression = trim(fallYVelocity->value);
            }
            if (const auto* yAccel = findProperty(section, "yaccel")) {
                hitDef.hasYAccel = true;
                hitDef.yAccel = parseFloatValue(yAccel->value, 0.0f);
                hitDef.yAccelExpression = trim(yAccel->value);
            }
            if (const auto* guardVelocity = findProperty(section, "guard.velocity")) {
                const auto values = parseFloatPairValue(guardVelocity->value);
                hitDef.hasGuardVelocity = true;
                hitDef.guardVelocityX = values.first;
                hitDef.guardVelocityY = values.second;
                const auto expressions = parseExpressionPairValue(guardVelocity->value);
                hitDef.guardVelocityXExpression = expressions.first;
                hitDef.guardVelocityYExpression = expressions.second;
            } else {
                hitDef.guardVelocityX = hitDef.groundVelocityX;
                hitDef.guardVelocityY = hitDef.groundVelocityY;
            }
            if (const auto* p1StateNo = findProperty(section, "p1stateno")) {
                hitDef.p1StateNo = parseIntValue(p1StateNo->value, -1);
                hitDef.p1StateNoExpression = trim(p1StateNo->value);
            }
            if (const auto* p1Facing = findProperty(section, "p1facing")) {
                hitDef.hasP1Facing = true;
                hitDef.p1Facing = parseIntValue(p1Facing->value, 0);
                hitDef.p1FacingExpression = trim(p1Facing->value);
            }
            if (const auto* p2StateNo = findProperty(section, "p2stateno")) {
                hitDef.p2StateNo = parseIntValue(p2StateNo->value, -1);
                hitDef.p2StateNoExpression = trim(p2StateNo->value);
                hitDef.p2GetP1State = true;
            }
            if (const auto* p2GetP1State = findProperty(section, "p2getp1state")) {
                hitDef.p2GetP1State = parseIntValue(p2GetP1State->value, hitDef.p2GetP1State ? 1 : 0) != 0;
                hitDef.p2GetP1StateExpression = trim(p2GetP1State->value);
            }
            if (const auto* p2Facing = findProperty(section, "p2facing")) {
                hitDef.hasP2Facing = true;
                hitDef.p2Facing = parseIntValue(p2Facing->value, 0);
                hitDef.p2FacingExpression = trim(p2Facing->value);
            }
            if (const auto* envShakeTime = findProperty(section, "envshake.time")) {
                hitDef.envShake.time = std::max(0, parseIntValue(envShakeTime->value, hitDef.envShake.time));
                hitDef.envShake.timeExpression = trim(envShakeTime->value);
                hitDef.envShake.enabled = hitDef.envShake.time > 0;
            }
            if (const auto* envShakeFrequency = findProperty(section, "envshake.freq")) {
                hitDef.envShake.frequency = std::max(1, parseIntValue(envShakeFrequency->value, hitDef.envShake.frequency));
                hitDef.envShake.frequencyExpression = trim(envShakeFrequency->value);
            }
            if (const auto* envShakeAmplitude = findProperty(section, "envshake.ampl")) {
                hitDef.envShake.amplitude = parseFloatValue(envShakeAmplitude->value, hitDef.envShake.amplitude);
                hitDef.envShake.amplitudeExpression = trim(envShakeAmplitude->value);
                hitDef.envShake.enabled = hitDef.envShake.time > 0 && std::abs(hitDef.envShake.amplitude) > 0.001f;
            }
            if (const auto* envShakePhase = findProperty(section, "envshake.phase")) {
                hitDef.envShake.phase = parseIntValue(envShakePhase->value, hitDef.envShake.phase);
                hitDef.envShake.phaseExpression = trim(envShakePhase->value);
            }
            if (const auto* fallEnvShakeTime = findProperty(section, "fall.envshake.time")) {
                hitDef.fallEnvShake.time = std::max(0, parseIntValue(fallEnvShakeTime->value, hitDef.fallEnvShake.time));
                hitDef.fallEnvShake.timeExpression = trim(fallEnvShakeTime->value);
                hitDef.fallEnvShake.enabled = hitDef.fallEnvShake.time > 0;
            }
            if (const auto* fallEnvShakeFrequency = findProperty(section, "fall.envshake.freq")) {
                hitDef.fallEnvShake.frequency = std::max(1, parseIntValue(fallEnvShakeFrequency->value, hitDef.fallEnvShake.frequency));
                hitDef.fallEnvShake.frequencyExpression = trim(fallEnvShakeFrequency->value);
            }
            if (const auto* fallEnvShakeAmplitude = findProperty(section, "fall.envshake.ampl")) {
                hitDef.fallEnvShake.amplitude = parseFloatValue(fallEnvShakeAmplitude->value, hitDef.fallEnvShake.amplitude);
                hitDef.fallEnvShake.amplitudeExpression = trim(fallEnvShakeAmplitude->value);
                hitDef.fallEnvShake.enabled = hitDef.fallEnvShake.time > 0 && std::abs(hitDef.fallEnvShake.amplitude) > 0.001f;
            }
            if (const auto* fallEnvShakePhase = findProperty(section, "fall.envshake.phase")) {
                hitDef.fallEnvShake.phase = parseIntValue(fallEnvShakePhase->value, hitDef.fallEnvShake.phase);
                hitDef.fallEnvShake.phaseExpression = trim(fallEnvShakePhase->value);
            }
            hitDef.palFx = parsePaletteEffectSpec(section, "palfx");
            hitDefs.push_back(std::move(hitDef));
        }
    }

    return hitDefs;
}

bool isSupportedStateType(char value) {
    value = static_cast<char>(SDL_toupper(static_cast<unsigned char>(value)));
    return value == 'S' || value == 'C' || value == 'A' || value == 'L';
}

void addUniqueCommand(std::vector<std::string>& commands, const std::string& command) {
    if (std::find(commands.begin(), commands.end(), command) == commands.end()) {
        commands.push_back(command);
    }
}

size_t findNoCase(std::string_view value, std::string_view needle, size_t start = 0) {
    if (needle.empty() || start >= value.size()) {
        return std::string_view::npos;
    }
    const std::string loweredValue = lowercaseCopy(value);
    const std::string loweredNeedle = lowercaseCopy(needle);
    return loweredValue.find(loweredNeedle, start);
}

const std::vector<DecodedSoundSample>* arenaCharacterSamplesForOwner(const AppState& state, int ownerIndex);

#include "CommandParsing.h"
#include "AudioRuntime.h"
#include "MainMenuPresentationLoading.h"
#include "RuntimeLoading.h"
