#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local runtime expression/CNS types and helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

bool compareIntValue(int lhs, CompareOp op, int rhs) {
    switch (op) {
    case CompareOp::Equal:
        return lhs == rhs;
    case CompareOp::NotEqual:
        return lhs != rhs;
    case CompareOp::Greater:
        return lhs > rhs;
    case CompareOp::GreaterEqual:
        return lhs >= rhs;
    case CompareOp::Less:
        return lhs < rhs;
    case CompareOp::LessEqual:
        return lhs <= rhs;
    default:
        return false;
    }
}

bool compareFloatValue(float lhs, CompareOp op, float rhs) {
    switch (op) {
    case CompareOp::Equal:
        return std::fabs(lhs - rhs) < 0.0001f;
    case CompareOp::NotEqual:
        return std::fabs(lhs - rhs) >= 0.0001f;
    case CompareOp::Greater:
        return lhs > rhs;
    case CompareOp::GreaterEqual:
        return lhs >= rhs;
    case CompareOp::Less:
        return lhs < rhs;
    case CompareOp::LessEqual:
        return lhs <= rhs;
    default:
        return false;
    }
}

float animationTimeValue(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findExactClipForActor(state, fighter, fighter.action);
    if (!clip) {
        return 0.0f;
    }
    if (clip->hasInfiniteDuration && fighter.animTick >= clip->infiniteStartTick) {
        return -1.0f;
    }
    return static_cast<float>(std::min(0, fighter.animTick + 1 - clip->loopTicks));
}

std::vector<std::string> collectFighterCommands(
    const FighterInputState& input,
    const FighterState& fighter,
    const std::vector<CommandDefinition>& definitions);
bool commandActive(const std::vector<std::string>& commands, std::string_view command);

std::vector<std::string> collectCurrentFighterCommands(const AppState& state, const FighterState& fighter) {
    if (fighter.inputHistory.empty()) {
        return {};
    }
    return collectFighterCommands(fighter.inputHistory.back().input, fighter, commandDefinitionsForActor(state, fighter));
}

float p2AxisDistXValue(const FighterState& fighter, const FighterState* opponent);
float p2AxisDistYValue(const FighterState& fighter, const FighterState* opponent);
float p2BodyDistXExpressionValue(const FighterState& fighter, const FighterState* opponent);
float p2BodyDistYExpressionValue(const FighterState& fighter, const FighterState* opponent);
float edgeDistExpressionValue(const AppState& state, const FighterState& fighter, const StageSlot* stage, bool front);

float stateTriggerSubjectValue(
    const AppState& state,
    const FighterState& fighter,
    StateTriggerSubject subject,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    switch (subject) {
    case StateTriggerSubject::Time:
        return static_cast<float>(fighter.stateTime);
    case StateTriggerSubject::Anim:
        return static_cast<float>(fighter.action);
    case StateTriggerSubject::AnimTime:
        return animationTimeValue(state, fighter);
    case StateTriggerSubject::VelX:
        return fighter.vx;
    case StateTriggerSubject::VelY:
        return fighter.vy;
    case StateTriggerSubject::PosX:
        return fighter.x;
    case StateTriggerSubject::PosY:
        return fighter.triggerY;
    case StateTriggerSubject::P2BodyDistX:
        return p2BodyDistXExpressionValue(fighter, opponent);
    case StateTriggerSubject::P2BodyDistY:
        return p2BodyDistYExpressionValue(fighter, opponent);
    case StateTriggerSubject::P2DistX:
        return p2AxisDistXValue(fighter, opponent);
    case StateTriggerSubject::P2DistY:
        return p2AxisDistYValue(fighter, opponent);
    case StateTriggerSubject::FrontEdgeBodyDist:
        return edgeDistExpressionValue(state, fighter, stage, true);
    case StateTriggerSubject::BackEdgeBodyDist:
        return edgeDistExpressionValue(state, fighter, stage, false);
    case StateTriggerSubject::HitShakeOver:
        return fighter.hitPauseTicks <= 0 ? 1.0f : 0.0f;
    default:
        return 0.0f;
    }
}

bool variableRefInRange(const MugenVariableRef& ref) {
    switch (ref.bank) {
    case MugenVariableBank::Var:
    case MugenVariableBank::SysVar:
        return ref.index >= 0 && ref.index < 60;
    case MugenVariableBank::FVar:
    case MugenVariableBank::SysFVar:
        return ref.index >= 0 && ref.index < 40;
    default:
        return false;
    }
}

float fighterVariableValue(const FighterState& fighter, const MugenVariableRef& ref) {
    if (!variableRefInRange(ref)) {
        return 0.0f;
    }
    switch (ref.bank) {
    case MugenVariableBank::Var:
        return static_cast<float>(fighter.vars[static_cast<size_t>(ref.index)]);
    case MugenVariableBank::SysVar:
        return static_cast<float>(fighter.sysVars[static_cast<size_t>(ref.index)]);
    case MugenVariableBank::FVar:
        return fighter.fvars[static_cast<size_t>(ref.index)];
    case MugenVariableBank::SysFVar:
        return fighter.sysFvars[static_cast<size_t>(ref.index)];
    default:
        return 0.0f;
    }
}

int pseudoMugenRandom(const AppState& state, const FighterState& fighter, int salt = 0) {
    uint32_t value = static_cast<uint32_t>(state.frame * 1103515245u)
        ^ static_cast<uint32_t>((fighter.stateNo + 37) * 2654435761u)
        ^ static_cast<uint32_t>((fighter.stateTime + 101) * 2246822519u)
        ^ static_cast<uint32_t>((fighter.action + 17) * 3266489917u)
        ^ static_cast<uint32_t>(salt * 374761393u);
    value ^= value >> 16;
    value *= 2246822519u;
    value ^= value >> 13;
    return static_cast<int>(value % 1000u);
}

float mugenStateTypeValue(char stateType) {
    switch (static_cast<char>(SDL_toupper(static_cast<unsigned char>(stateType)))) {
    case 'C':
        return 1.0f;
    case 'A':
        return 2.0f;
    case 'L':
        return 3.0f;
    case 'S':
    default:
        return 0.0f;
    }
}

float mugenMoveTypeValue(char moveType) {
    switch (static_cast<char>(SDL_toupper(static_cast<unsigned char>(moveType)))) {
    case 'A':
        return 1.0f;
    case 'H':
        return 2.0f;
    case 'I':
    default:
        return 0.0f;
    }
}

float p2AxisDistXValue(const FighterState& fighter, const FighterState* opponent) {
    return opponent ? (opponent->x - fighter.x) * static_cast<float>(fighter.facing) : 0.0f;
}

float p2AxisDistYValue(const FighterState& fighter, const FighterState* opponent) {
    return opponent ? opponent->y - fighter.y : 0.0f;
}

float p2BodyDistXExpressionValue(const FighterState& fighter, const FighterState* opponent) {
    if (!opponent) {
        return 0.0f;
    }
    const float axisDistance = p2AxisDistXValue(fighter, opponent);
    const float fighterFront = fighter.playerWidthFront >= 0.0f ? fighter.playerWidthFront : 0.0f;
    const float opponentBack = opponent->playerWidthBack >= 0.0f ? opponent->playerWidthBack : 0.0f;
    return axisDistance >= 0.0f ? axisDistance - fighterFront - opponentBack : axisDistance;
}

float p2BodyDistYExpressionValue(const FighterState& fighter, const FighterState* opponent) {
    return p2AxisDistYValue(fighter, opponent);
}

float edgeDistExpressionValue(const AppState& state, const FighterState& fighter, const StageSlot* stage, bool front) {
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float visibleLeft = state.cameraX - halfWidth;
    const float visibleRight = state.cameraX + halfWidth;
    const float leftEdge = stage ? std::max(stage->leftbound, visibleLeft) : visibleLeft;
    const float rightEdge = stage ? std::min(stage->rightbound, visibleRight) : visibleRight;
    if (front) {
        return fighter.facing >= 0 ? rightEdge - fighter.x : fighter.x - leftEdge;
    }
    return fighter.facing >= 0 ? fighter.x - leftEdge : rightEdge - fighter.x;
}

float mugenRoundStateValue(const AppState& state) {
    if (!isMatchMode(state)) {
        return 2.0f;
    }
    switch (state.matchPhase) {
    case MatchPhase::RoundStart:
        return 1.0f;
    case MatchPhase::Fight:
        return 2.0f;
    case MatchPhase::RoundFinish:
        return 3.0f;
    case MatchPhase::RoundResult:
    case MatchPhase::MatchResult:
        return 4.0f;
    default:
        return 2.0f;
    }
}

std::optional<float> evalMugenExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr);

std::optional<std::pair<float, float>> parseMugenFloatRangeExpression(const std::string& expression) {
    const std::string value = trim(expression);
    if (value.size() < 5 || value.front() != '[' || value.back() != ']') {
        return std::nullopt;
    }
    const auto parts = splitCsv(std::string(std::string_view(value).substr(1, value.size() - 2)));
    if (parts.empty()) {
        return std::nullopt;
    }
    const auto minValue = parsePlainFloatValue(parts[0]);
    const auto maxValue = parts.size() >= 2 ? parsePlainFloatValue(parts[1]) : minValue;
    if (!minValue || !maxValue) {
        return std::nullopt;
    }
    return std::pair<float, float>{ std::min(*minValue, *maxValue), std::max(*minValue, *maxValue) };
}

std::optional<float> evalMugenFunctionExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent,
    const StageSlot* stage) {
    const auto open = expression.find('(');
    if (open == std::string::npos || expression.back() != ')') {
        return std::nullopt;
    }

    const std::string name = lowercaseCopy(trim(std::string_view(expression).substr(0, open)));
    const std::string body = trim(std::string_view(expression).substr(open + 1, expression.size() - open - 2));
    if (name == "floor") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::floor(*value) } : std::nullopt;
    }
    if (name == "ceil") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::ceil(*value) } : std::nullopt;
    }
    if (name == "sin") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::sin(*value) } : std::nullopt;
    }
    if (name == "cos") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::cos(*value) } : std::nullopt;
    }
    if (name == "abs") {
        const auto value = evalMugenExpression(state, fighter, body, opponent, stage);
        return value ? std::optional<float>{ std::fabs(*value) } : std::nullopt;
    }
    if (name == "ifelse") {
        const auto parts = splitCsv(body);
        if (parts.size() < 3) {
            return std::nullopt;
        }
        const auto condition = evalMugenExpression(state, fighter, parts[0], opponent, stage);
        if (!condition) {
            return std::nullopt;
        }
        return evalMugenExpression(state, fighter, *condition != 0.0f ? parts[1] : parts[2], opponent, stage);
    }
    if (name == "const") {
        return lookupCharacterConstant(characterConstantsForActor(state, fighter), body);
    }
    if (name == "gethitvar") {
        const std::string key = lowercaseCopy(trim(body));
        if (key == "animtype") {
            return static_cast<float>(fighter.getHitAnimType);
        }
        if (key == "groundtype") {
            return static_cast<float>(fighter.getHitGroundType);
        }
        if (key == "airtype") {
            return static_cast<float>(fighter.getHitAirType);
        }
        if (key == "slidetime") {
            return static_cast<float>(fighter.getHitSlideTime);
        }
        if (key == "hittime") {
            return static_cast<float>(fighter.getHitHitTime);
        }
        if (key == "ctrltime") {
            return static_cast<float>(fighter.getHitCtrlTime);
        }
        if (key == "xvel") {
            return fighter.hitVelocityX;
        }
        if (key == "yvel") {
            return fighter.hitVelocityY;
        }
        if (key == "yaccel") {
            return fighter.hitFallYAccel;
        }
        if (key == "fall") {
            return fighter.hitFall ? 1.0f : 0.0f;
        }
        if (key == "fall.recover") {
            return fighter.hitFallRecover ? 1.0f : 0.0f;
        }
        if (key == "fall.recovertime") {
            return static_cast<float>(fighter.hitFallRecoverTime);
        }
        if (key == "fall.damage") {
            return static_cast<float>(fighter.hitFallDamage);
        }
        if (key == "down.recover") {
            return fighter.hitDownRecover ? 1.0f : 0.0f;
        }
        if (key == "down.recovertime") {
            return static_cast<float>(fighter.hitDownRecoverTime);
        }
        if (key == "down.velocity.x") {
            return fighter.hitDownVelocityX;
        }
        if (key == "down.velocity.y") {
            return fighter.hitDownVelocityY;
        }
        if (key == "fall.xvel") {
            return fighter.hitFallBounceXVelocity;
        }
        if (key == "fall.yvel") {
            return fighter.hitFallBounceYVelocity;
        }
        if (key == "hitcount") {
            return static_cast<float>(fighter.getHitHitCount);
        }
        return std::nullopt;
    }
    if (name == "selfanimexist") {
        const auto action = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!action) {
            return std::nullopt;
        }
        return findExactClipForActor(state, fighter, static_cast<int>(std::lround(*action))) ? 1.0f : 0.0f;
    }
    if (name == "numhelper") {
        if (body.empty()) {
            return static_cast<float>(ownedHelperCount(state, fighter));
        }
        const auto helperId = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!helperId) {
            return std::nullopt;
        }
        return static_cast<float>(ownedHelperCount(state, fighter, static_cast<int>(std::lround(*helperId))));
    }
    if (name == "numprojid") {
        const auto projectileId = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!projectileId) {
            return std::nullopt;
        }
        return static_cast<float>(ownedProjectileCount(state, fighter, static_cast<int>(std::lround(*projectileId))));
    }
    if (name == "projcontacttime" || name == "projguardedtime") {
        const auto projectileId = evalMugenExpression(state, fighter, body, opponent, stage);
        if (!projectileId) {
            return std::nullopt;
        }
        const int id = static_cast<int>(std::lround(*projectileId));
        if (name == "projcontacttime") {
            return fighter.projectileContactId == id && fighter.projectileContactTicks > 0 ? 1.0f : -1.0f;
        }
        return fighter.projectileGuardedId == id && fighter.projectileGuardedTicks > 0 ? 1.0f : -1.0f;
    }
    return std::nullopt;
}

std::optional<float> evalMugenExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent,
    const StageSlot* stage) {
    std::string value = stripOuterParens(expression);
    if (value.empty()) {
        return std::nullopt;
    }

    const size_t comma = value.find(',');
    if (comma != std::string::npos) {
        const std::string prefix = lowercaseCopy(trim(std::string_view(value).substr(0, comma)));
        if (prefix == "parent" || prefix == "root") {
            if (const FighterState* owner = fighterOwner(state, fighter)) {
                return evalMugenExpression(state, *owner, trim(std::string_view(value).substr(comma + 1)), opponent, stage);
            }
            return std::nullopt;
        }
        if (prefix == "enemy" || prefix == "enemynear") {
            if (!opponent) {
                return std::nullopt;
            }
            return evalMugenExpression(state, *opponent, trim(std::string_view(value).substr(comma + 1)), &fighter, stage);
        }
        if (prefix == "target") {
            const FighterState* target = storedTargetFighter(state, fighter);
            if (!target) {
                return std::nullopt;
            }
            return evalMugenExpression(state, *target, trim(std::string_view(value).substr(comma + 1)), &fighter, stage);
        }
        if (startsWithNoCase(prefix, "helper(") && prefix.back() == ')') {
            const std::string helperBody = trim(std::string_view(prefix).substr(std::string_view("helper(").size(), prefix.size() - std::string_view("helper(").size() - 1));
            const auto helperId = evalMugenExpression(state, fighter, helperBody, opponent, stage);
            if (!helperId) {
                return std::nullopt;
            }
            const FighterState* helper = ownedHelperById(state, fighter, static_cast<int>(std::lround(*helperId)));
            if (!helper) {
                return std::nullopt;
            }
            return evalMugenExpression(state, *helper, trim(std::string_view(value).substr(comma + 1)), opponent, stage);
        }
    }

    const auto orClauses = splitTopLevelClauses(value, "||", true);
    if (orClauses.size() > 1) {
        for (const auto& clause : orClauses) {
            const auto evaluated = evalMugenExpression(state, fighter, clause, opponent, stage);
            if (evaluated && *evaluated != 0.0f) {
                return 1.0f;
            }
        }
        return 0.0f;
    }

    const auto andClauses = splitTopLevelClauses(value, "&&");
    if (andClauses.size() > 1) {
        for (const auto& clause : andClauses) {
            const auto evaluated = evalMugenExpression(state, fighter, clause, opponent, stage);
            if (!evaluated || *evaluated == 0.0f) {
                return 0.0f;
            }
        }
        return 1.0f;
    }

    if (value.front() == '!') {
        const auto evaluated = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(1)), opponent, stage);
        if (!evaluated) {
            return std::nullopt;
        }
        return *evaluated == 0.0f ? 1.0f : 0.0f;
    }

    if (const auto compare = findTopLevelCompareOp(value)) {
        const auto [op, pos] = *compare;
        const size_t opLength = op == CompareOp::GreaterEqual || op == CompareOp::LessEqual || op == CompareOp::NotEqual ? 2 : 1;
        const auto lhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(0, pos)), opponent, stage);
        const std::string rhsExpression = trim(std::string_view(value).substr(pos + opLength));
        if (const auto range = parseMugenFloatRangeExpression(rhsExpression)) {
            if (!lhs || (op != CompareOp::Equal && op != CompareOp::NotEqual)) {
                return std::nullopt;
            }
            const bool inRange = *lhs >= range->first && *lhs <= range->second;
            return (op == CompareOp::Equal ? inRange : !inRange) ? 1.0f : 0.0f;
        }
        const auto rhs = evalMugenExpression(state, fighter, rhsExpression, opponent, stage);
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        return compareFloatValue(*lhs, op, *rhs) ? 1.0f : 0.0f;
    }

    if (const auto op = findTopLevelBinaryOperator(value, { "+", "-" })) {
        const char token = value[*op];
        const auto lhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(0, *op)), opponent, stage);
        const auto rhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(*op + 1)), opponent, stage);
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        return token == '+' ? *lhs + *rhs : *lhs - *rhs;
    }
    if (const auto op = findTopLevelBinaryOperator(value, { "*", "/", "%" })) {
        const char token = value[*op];
        const auto lhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(0, *op)), opponent, stage);
        const auto rhs = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(*op + 1)), opponent, stage);
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        if ((token == '/' || token == '%') && *rhs == 0.0f) {
            return 0.0f;
        }
        if (token == '*') {
            return *lhs * *rhs;
        }
        if (token == '%') {
            return std::fmod(*lhs, *rhs);
        }
        return *lhs / *rhs;
    }

    if (value.front() == '-') {
        const auto evaluated = evalMugenExpression(state, fighter, trim(std::string_view(value).substr(1)), opponent, stage);
        return evaluated ? std::optional<float>{ -*evaluated } : std::nullopt;
    }

    if (const auto plain = parsePlainFloatValue(value)) {
        return plain;
    }
    if (const auto ref = parseMugenVariableRef(value)) {
        return fighterVariableValue(fighter, *ref);
    }
    if (const auto functionValue = evalMugenFunctionExpression(state, fighter, value, opponent, stage)) {
        return functionValue;
    }

    const std::string lowered = lowercaseCopy(value);
    if (lowered == "random") {
        return static_cast<float>(pseudoMugenRandom(state, fighter));
    }
    if (lowered == "pi") {
        return static_cast<float>(std::numbers::pi);
    }
    if (lowered == "numhelper") {
        return static_cast<float>(ownedHelperCount(state, fighter));
    }
    if (lowered == "numproj") {
        return static_cast<float>(ownedProjectileCount(state, fighter));
    }
    if (startsWithNoCase(lowered, "projhit")) {
        const std::string suffix = trim(std::string_view(lowered).substr(std::string_view("projhit").size()));
        const int id = suffix.empty() ? 0 : parseIntValue(suffix, 0);
        return (fighter.projectileHitTicks > 0 && (id == 0 || fighter.projectileHitId == id)) ? 1.0f : 0.0f;
    }
    if (startsWithNoCase(lowered, "projcontact")) {
        const std::string suffix = trim(std::string_view(lowered).substr(std::string_view("projcontact").size()));
        const int id = suffix.empty() ? 0 : parseIntValue(suffix, 0);
        return (fighter.projectileContactTicks > 0 && (id == 0 || fighter.projectileContactId == id)) ? 1.0f : 0.0f;
    }
    if (lowered == "ailevel") {
        return 0.0f;
    }
    if (lowered == "time") {
        return static_cast<float>(fighter.stateTime);
    }
    if (lowered == "animtime") {
        return animationTimeValue(state, fighter);
    }
    if (lowered == "anim") {
        return static_cast<float>(fighter.action);
    }
    if (lowered == "stateno") {
        return static_cast<float>(fighter.stateNo);
    }
    if (lowered == "power") {
        return static_cast<float>(fighter.power);
    }
    if (lowered == "powermax") {
        return static_cast<float>(characterConstantsForActor(state, fighter).maxPower);
    }
    if (lowered == "palno") {
        return static_cast<float>(fighter.paletteNo);
    }
    if (lowered == "hitcount") {
        return static_cast<float>(fighter.hitCount);
    }
    if (lowered == "numtarget") {
        return fighter.targetTicks > 0 && fighter.targetIndex >= 0 ? 1.0f : 0.0f;
    }
    if (lowered == "movecontact") {
        return fighter.moveContact ? 1.0f : 0.0f;
    }
    if (lowered == "movehit") {
        return fighter.moveHit ? 1.0f : 0.0f;
    }
    if (lowered == "moveguarded") {
        return fighter.moveGuarded ? 1.0f : 0.0f;
    }
    if (lowered == "ctrl") {
        return fighter.ctrl ? 1.0f : 0.0f;
    }
    if (lowered == "roundstate") {
        return mugenRoundStateValue(state);
    }
    if (lowered == "roundno") {
        return static_cast<float>(std::max(1, state.currentRound));
    }
    if (lowered == "roundsexisted") {
        return static_cast<float>(std::max(0, state.currentRound - 1));
    }
    if (lowered == "teammode") {
        return 0.0f;
    }
    if (lowered == "single") {
        return 0.0f;
    }
    if (lowered == "simul") {
        return 1.0f;
    }
    if (lowered == "turns") {
        return 2.0f;
    }
    if (lowered == "life") {
        return static_cast<float>(fighter.life);
    }
    if (lowered == "alive") {
        return fighter.life > 0 ? 1.0f : 0.0f;
    }
    if (lowered == "p2life") {
        return opponent ? static_cast<float>(opponent->life) : 0.0f;
    }
    if (lowered == "p2stateno") {
        return opponent ? static_cast<float>(opponent->stateNo) : 0.0f;
    }
    if (lowered == "p2statetype") {
        return opponent ? mugenStateTypeValue(opponent->stateType) : 0.0f;
    }
    if (lowered == "p2movetype") {
        return opponent ? mugenMoveTypeValue(opponent->moveType) : 0.0f;
    }
    if (lowered == "statetype") {
        return mugenStateTypeValue(fighter.stateType);
    }
    if (lowered == "movetype") {
        return mugenMoveTypeValue(fighter.moveType);
    }
    if (lowered == "i") {
        return mugenMoveTypeValue('I');
    }
    if (lowered == "h") {
        return mugenMoveTypeValue('H');
    }
    if (lowered == "s") {
        return mugenStateTypeValue('S');
    }
    if (lowered == "c") {
        return mugenStateTypeValue('C');
    }
    if (lowered == "a") {
        return mugenStateTypeValue('A');
    }
    if (lowered == "l") {
        return mugenStateTypeValue('L');
    }
    if (lowered == "facing") {
        return static_cast<float>(fighter.facing);
    }
    if (lowered == "pos x") {
        return fighter.x;
    }
    if (lowered == "pos y") {
        return fighter.triggerY;
    }
    if (lowered == "vel x") {
        return fighter.vx;
    }
    if (lowered == "vel y") {
        return fighter.vy;
    }
    if (lowered == "p2bodydist x") {
        return p2BodyDistXExpressionValue(fighter, opponent);
    }
    if (lowered == "p2bodydist y") {
        return p2BodyDistYExpressionValue(fighter, opponent);
    }
    if (lowered == "p2dist x") {
        return p2AxisDistXValue(fighter, opponent);
    }
    if (lowered == "p2dist y") {
        return p2AxisDistYValue(fighter, opponent);
    }
    if (lowered == "frontedgedist" || lowered == "frontedgebodydist") {
        return edgeDistExpressionValue(state, fighter, stage, true);
    }
    if (lowered == "backedgedist" || lowered == "backedgebodydist") {
        return edgeDistExpressionValue(state, fighter, stage, false);
    }
    return std::nullopt;
}

bool evalMugenExpressionCondition(
    const AppState& state,
    const FighterState& fighter,
    const MugenExpressionCondition& condition,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    const auto lhs = evalMugenExpression(state, fighter, condition.lhs, opponent, stage);
    const auto rhs = evalMugenExpression(state, fighter, condition.rhs, opponent, stage);
    return lhs && rhs && compareFloatValue(*lhs, condition.op, *rhs);
}

bool stateTriggerGroupActive(
    const AppState& state,
    const FighterState& fighter,
    const StateTriggerGroup& group,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (group.requiresMoveContact && !fighter.moveContact) {
        return false;
    }
    if (!group.requiredCommands.empty() || !group.forbiddenCommands.empty()) {
        const auto commands = collectCurrentFighterCommands(state, fighter);
        for (const auto& required : group.requiredCommands) {
            if (!commandActive(commands, required)) {
                return false;
            }
        }
        for (const auto& forbidden : group.forbiddenCommands) {
            if (commandActive(commands, forbidden)) {
                return false;
            }
        }
    }
    for (const auto& condition : group.floatConditions) {
        if (!compareFloatValue(stateTriggerSubjectValue(state, fighter, condition.subject, opponent, stage), condition.op, condition.value)) {
            return false;
        }
    }
    for (const auto& condition : group.rangeConditions) {
        const float value = stateTriggerSubjectValue(state, fighter, condition.subject, opponent, stage);
        if (value < condition.minValue || value > condition.maxValue) {
            return false;
        }
    }
    for (const auto& condition : group.expressionConditions) {
        if (!evalMugenExpressionCondition(state, fighter, condition, opponent, stage)) {
            return false;
        }
    }
    for (const auto& expression : group.booleanExpressions) {
        const auto value = evalMugenExpression(state, fighter, expression, opponent, stage);
        if (!value || *value == 0.0f) {
            return false;
        }
    }
    for (const auto& condition : group.stateTypeConditions) {
        const bool matches = fighter.stateType == condition.stateType;
        if (condition.negated ? matches : !matches) {
            return false;
        }
    }
    for (const auto& condition : group.animElemTimeConditions) {
        if (!compareIntValue(animElemTimeForFighter(state, fighter, condition.elem), condition.op, condition.value)) {
            return false;
        }
    }
    return true;
}

bool anyStateTriggerGroupActive(
    const AppState& state,
    const FighterState& fighter,
    const std::vector<StateTriggerGroup>& groups,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    return std::any_of(groups.begin(), groups.end(), [&state, &fighter, opponent, stage](const StateTriggerGroup& group) {
        return stateTriggerGroupActive(state, fighter, group, opponent, stage);
    });
}

bool stateControllerTriggerActive(
    const AppState& state,
    const FighterState& fighter,
    const StateControllerTrigger& trigger,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (!trigger.hasTrigger) {
        return false;
    }
    for (const auto& expressionGroups : trigger.triggerAllExpressions) {
        if (!anyStateTriggerGroupActive(state, fighter, expressionGroups, opponent, stage)) {
            return false;
        }
    }
    if (trigger.triggerGroups.empty()) {
        return true;
    }
    return anyStateTriggerGroupActive(state, fighter, trigger.triggerGroups, opponent, stage);
}

bool simpleControllerTriggerActive(const AppState& state, const FighterState& fighter, int triggerTime, int triggerAnimElem) {
    bool hasTrigger = false;
    if (triggerTime >= 0) {
        hasTrigger = true;
        if (fighter.stateTime != triggerTime) {
            return false;
        }
    }
    if (triggerAnimElem > 0) {
        hasTrigger = true;
        if (animElemTimeForFighter(state, fighter, triggerAnimElem) != 0) {
            return false;
        }
    }
    return hasTrigger;
}
