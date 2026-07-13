#pragma once

// Internal App.cpp implementation shard.
// Actor runtime helpers, clip lookup, action switching, and controller tracking.

bool runtimeHasLoadedData(const ArenaCharacterRuntime& runtime) {
    return !runtime.stateDefs.empty() || !runtime.clips.empty();
}

const ArenaCharacterRuntime* characterRuntimeForFighterIndex(const AppState& state, size_t fighterIndex) {
    if (const auto* runtime = arenaRuntimeForFighterIndex(state, fighterIndex)) {
        return runtime;
    }
    if (state.frontend.pendingMode == PendingMode::SingleFight
        && fighterIndex == 1
        && runtimeHasLoadedData(state.opponentRuntime)) {
        return &state.opponentRuntime;
    }
    return nullptr;
}

const std::vector<DecodedSoundSample>* arenaCharacterSamplesForOwner(const AppState& state, int ownerIndex) {
    if (ownerIndex < 0) {
        return nullptr;
    }
    if (const auto* runtime = characterRuntimeForFighterIndex(state, static_cast<size_t>(ownerIndex))) {
        return &runtime->samples;
    }
    return nullptr;
}

const AnimationClip* findClipForFighter(const AppState& state, size_t fighterIndex, int action) {
    if (const auto* runtime = characterRuntimeForFighterIndex(state, fighterIndex)) {
        return findClipInSet(runtime->clips, action);
    }
    if ((state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story)
        && fighterIndex < state.arenaFighterClips.size()
        && !state.arenaFighterClips[fighterIndex].empty()) {
        return findClipInSet(state.arenaFighterClips[fighterIndex], action);
    }
    if (fighterIndex == 1 && !state.opponentCharacterClips.empty()) {
        return findClipInSet(state.opponentCharacterClips, action);
    }
    return findClip(state, action);
}

const AnimationClip* findExactClipInSet(const std::vector<AnimationClip>& clips, int action) {
    for (const auto& clip : clips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    return nullptr;
}

const AnimationClip* findExactClipForFighter(const AppState& state, size_t fighterIndex, int action) {
    if (const auto* runtime = characterRuntimeForFighterIndex(state, fighterIndex)) {
        return findExactClipInSet(runtime->clips, action);
    }
    if ((state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story)
        && fighterIndex < state.arenaFighterClips.size()
        && !state.arenaFighterClips[fighterIndex].empty()) {
        return findExactClipInSet(state.arenaFighterClips[fighterIndex], action);
    }
    if (fighterIndex == 1 && !state.opponentCharacterClips.empty()) {
        return findExactClipInSet(state.opponentCharacterClips, action);
    }
    return findExactClipInSet(state.characterClips, action);
}

const AnimationClip* findExactClip(const AppState& state, int action) {
    for (const auto& clip : state.characterClips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    return nullptr;
}

int ownedHelperCount(const AppState& state, const FighterState& fighter, std::optional<int> helperId = std::nullopt) {
    int ownerIndex = fighter.helper ? fighter.ownerIndex : -1;
    if (!fighter.helper) {
        const auto* first = state.fighters.data();
        const auto* current = &fighter;
        if (current >= first && current < first + state.fighters.size()) {
            ownerIndex = static_cast<int>(current - first);
        }
    }
    if (ownerIndex < 0) {
        return 0;
    }

    int count = 0;
    for (const auto& helper : state.helpers) {
        if (helper.destroyRequested || helper.ownerIndex != ownerIndex) {
            continue;
        }
        if (helperId && helper.helperId != *helperId) {
            continue;
        }
        ++count;
    }
    return count;
}

int ownedProjectileCount(const AppState& state, const FighterState& fighter, std::optional<int> projectileId = std::nullopt) {
    int ownerIndex = fighter.helper ? fighter.ownerIndex : -1;
    if (!fighter.helper) {
        const auto* first = state.fighters.data();
        const auto* current = &fighter;
        if (current >= first && current < first + state.fighters.size()) {
            ownerIndex = static_cast<int>(current - first);
        }
    }
    if (ownerIndex < 0) {
        return 0;
    }

    int count = 0;
    for (const auto& projectile : state.projectiles) {
        if (projectile.ownerIndex != ownerIndex || projectile.removing) {
            continue;
        }
        if (projectileId && projectile.id != *projectileId) {
            continue;
        }
        ++count;
    }
    return count;
}

const FighterState* ownedHelperById(const AppState& state, const FighterState& fighter, int helperId) {
    int ownerIndex = fighter.helper ? fighter.ownerIndex : -1;
    if (!fighter.helper) {
        const auto* first = state.fighters.data();
        const auto* current = &fighter;
        if (current >= first && current < first + state.fighters.size()) {
            ownerIndex = static_cast<int>(current - first);
        }
    }
    if (ownerIndex < 0) {
        return nullptr;
    }
    for (const auto& helper : state.helpers) {
        if (!helper.destroyRequested && helper.ownerIndex == ownerIndex && helper.helperId == helperId) {
            return &helper;
        }
    }
    return nullptr;
}

const FighterState* storedTargetFighter(const AppState& state, const FighterState& fighter) {
    if (fighter.targetTicks <= 0 || fighter.targetIndex < 0 || fighter.targetIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(fighter.targetIndex)];
}

FighterState* fighterOwner(AppState& state, const FighterState& fighter) {
    if (!fighter.helper || fighter.ownerIndex < 0 || fighter.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(fighter.ownerIndex)];
}

const FighterState* fighterOwner(const AppState& state, const FighterState& fighter) {
    if (!fighter.helper || fighter.ownerIndex < 0 || fighter.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(fighter.ownerIndex)];
}

int actorClipOwnerIndex(const AppState& state, const FighterState& fighter) {
    if (fighter.helper) {
        return fighter.ownerIndex >= 0 && fighter.ownerIndex < static_cast<int>(state.fighters.size())
            ? fighter.ownerIndex
            : -1;
    }
    if (fighter.customStateOwnerIndex >= 0
        && fighter.customStateOwnerIndex < static_cast<int>(state.fighters.size())) {
        return fighter.customStateOwnerIndex;
    }
    if (state.fighters.empty()) {
        return -1;
    }
    const auto* first = state.fighters.data();
    const auto* current = &fighter;
    if (current >= first && current < first + state.fighters.size()) {
        return static_cast<int>(current - first);
    }
    return 0;
}

int actorAnimationClipOwnerIndex(const AppState& state, const FighterState& fighter) {
    if (fighter.actionClipOwnerIndex >= 0
        && fighter.actionClipOwnerIndex < static_cast<int>(state.fighters.size())) {
        return fighter.actionClipOwnerIndex;
    }
    return actorClipOwnerIndex(state, fighter);
}

const AnimationClip* findClipForActor(const AppState& state, const FighterState& fighter, int action) {
    const int ownerIndex = actorAnimationClipOwnerIndex(state, fighter);
    if (ownerIndex >= 0) {
        return findClipForFighter(state, static_cast<size_t>(ownerIndex), action);
    }
    return findClip(state, action);
}

const AnimationClip* findExactClipForActor(const AppState& state, const FighterState& fighter, int action) {
    const int ownerIndex = actorAnimationClipOwnerIndex(state, fighter);
    if (ownerIndex >= 0) {
        return findExactClipForFighter(state, static_cast<size_t>(ownerIndex), action);
    }
    return findExactClip(state, action);
}

int firstExistingActionForActor(const AppState& state, const FighterState& fighter, std::initializer_list<int> actions) {
    for (const int action : actions) {
        if (findExactClipForActor(state, fighter, action)) {
            return action;
        }
    }
    return 0;
}

int firstExistingAction(const AppState& state, std::initializer_list<int> actions) {
    for (const int action : actions) {
        if (findExactClip(state, action)) {
            return action;
        }
    }
    return 0;
}

const CompatibilityContext& compatibilityContextForActor(const AppState& state, const FighterState& fighter) {
    const int ownerIndex = actorClipOwnerIndex(state, fighter);
    if ((state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story)
        && ownerIndex >= 0) {
        if (const auto* runtime = arenaRuntimeForFighterIndex(state, static_cast<size_t>(ownerIndex))) {
            return runtime->compatibility;
        }
    }
    if (ownerIndex == 0) {
        return state.characterCompatibility;
    }
    if (ownerIndex == 1) {
        return state.opponentRuntime.compatibility;
    }
    return state.characterCompatibility;
}

const AnimationClip* findFightFxClip(const AppState& state, int action) {
    for (const auto& clip : state.fightFxClips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    return nullptr;
}

const AnimationClip* findExactClipForRuntimeEffect(const AppState& state, const RuntimeEffect& effect) {
    if (effect.fromFightFx) {
        return findFightFxClip(state, effect.action);
    }
    if (effect.clipOwnerIndex >= 0 && effect.clipOwnerIndex < static_cast<int>(state.fighters.size())) {
        return findExactClipForFighter(state, static_cast<size_t>(effect.clipOwnerIndex), effect.action);
    }
    return findExactClip(state, effect.action);
}

const StateDefinition* findStateDefinition(const AppState& state, int stateNo) {
    for (const auto& stateDef : state.stateDefs) {
        if (stateDef.stateNo == stateNo) {
            return &stateDef;
        }
    }
    return nullptr;
}

const StateDefinition* findStateDefinitionInSet(const std::vector<StateDefinition>& stateDefs, int stateNo) {
    for (const auto& stateDef : stateDefs) {
        if (stateDef.stateNo == stateNo) {
            return &stateDef;
        }
    }
    return nullptr;
}

const ArenaCharacterRuntime* characterRuntimeForActor(const AppState& state, const FighterState& fighter) {
    const int ownerIndex = actorClipOwnerIndex(state, fighter);
    return ownerIndex >= 0 ? characterRuntimeForFighterIndex(state, static_cast<size_t>(ownerIndex)) : nullptr;
}

const CharacterConstants& characterConstantsForActor(const AppState& state, const FighterState& fighter) {
    if (const auto* runtime = characterRuntimeForActor(state, fighter)) {
        return runtime->constants;
    }
    return state.characterConstants;
}

const std::vector<StateDefinition>& stateDefinitionsForActor(const AppState& state, const FighterState& fighter) {
    if (const auto* runtime = characterRuntimeForActor(state, fighter)) {
        return runtime->stateDefs;
    }
    return state.stateDefs;
}

const std::vector<HitDefinition>& hitDefinitionsForActor(const AppState& state, const FighterState& fighter) {
    if (const auto* runtime = characterRuntimeForActor(state, fighter)) {
        return runtime->hitDefs;
    }
    return state.hitDefs;
}

const std::vector<CommandDefinition>& commandDefinitionsForActor(const AppState& state, const FighterState& fighter) {
    if (const auto* runtime = characterRuntimeForActor(state, fighter)) {
        return runtime->commandDefinitions;
    }
    return state.commandDefinitions;
}

const std::vector<CommandStateEntry>& commandEntriesForActor(const AppState& state, const FighterState& fighter) {
    if (const auto* runtime = characterRuntimeForActor(state, fighter)) {
        return runtime->commandEntries;
    }
    return state.commandEntries;
}

const std::vector<std::string>& victoryQuotesForActor(const AppState& state, const FighterState& fighter) {
    if (const auto* runtime = characterRuntimeForActor(state, fighter)) {
        return runtime->victoryQuotes;
    }
    return state.victoryQuotes;
}

const StateDefinition* findStateDefinitionForActor(const AppState& state, const FighterState& fighter, int stateNo) {
    return findStateDefinitionInSet(stateDefinitionsForActor(state, fighter), stateNo);
}

#include "StateDefinitionCompatibility.h"

void applyStateDefinitionPowerAdd(const AppState& state, FighterState& fighter, const StateDefinition& stateDef) {
    if (stateDef.powerAddExpression.empty()) {
        return;
    }
    const auto value = evalMugenExpression(state, fighter, stateDef.powerAddExpression, nullptr, nullptr);
    if (!value) {
        return;
    }
    fighter.power = std::clamp(
        fighter.power + static_cast<int>(std::lround(*value)),
        0,
        std::max(0, characterConstantsForActor(state, fighter).maxPower));
}

std::array<const StateDefinition*, 3> runtimeControllerStateDefinitions(const AppState& state, const FighterState& fighter) {
    return {
        findStateDefinitionForActor(state, fighter, -3),
        findStateDefinitionForActor(state, fighter, -2),
        findStateDefinitionForActor(state, fighter, fighter.stateNo),
    };
}

template <typename Fn>
void forEachRuntimeControllerStateDefinition(const AppState& state, const FighterState& fighter, Fn&& fn) {
    const auto stateDefs = runtimeControllerStateDefinitions(state, fighter);
    for (auto stateDefIt = stateDefs.begin(); stateDefIt != stateDefs.end(); ++stateDefIt) {
        const StateDefinition* stateDef = *stateDefIt;
        if (!stateDef || std::find(stateDefs.begin(), stateDefIt, stateDef) != stateDefIt) {
            continue;
        }
        if (!fn(*stateDef)) {
            break;
        }
    }
}

int effectiveClipTick(const AnimationClip& clip, int animTick) {
    const int totalTicks = std::max(1, clip.loopTicks);
    const int loopStart = std::clamp(clip.loopStartTick, 0, totalTicks - 1);
    int tick = std::max(0, animTick);
    if (clip.hasInfiniteDuration && tick >= clip.infiniteStartTick) {
        return std::clamp(clip.infiniteStartTick, 0, totalTicks - 1);
    }
    if (tick >= totalTicks) {
        const int loopLength = std::max(1, totalTicks - loopStart);
        tick = loopStart + ((tick - loopStart) % loopLength);
    }
    return tick;
}

int animElemStartTickForClip(const AnimationClip& clip, int elem) {
    if (clip.frames.empty()) {
        return 0;
    }

    const int targetElem = std::clamp(elem, 1, static_cast<int>(clip.frames.size()));
    int tick = 0;
    for (int i = 0; i < targetElem - 1; ++i) {
        tick += clip.frames[static_cast<size_t>(i)].duration;
    }
    return tick;
}

int animElemTimeForClip(const AnimationClip& clip, int animTick, int elem) {
    if (clip.frames.empty()) {
        return 0;
    }
    if (elem <= 0 || elem > static_cast<int>(clip.frames.size())) {
        return -1000000000;
    }

    const int elemStartTick = animElemStartTickForClip(clip, elem);
    return animTick - elemStartTick;
}

const AnimationFrame* frameForClip(const AnimationClip& clip, int animTick) {
    if (clip.frames.empty()) {
        return nullptr;
    }

    int tick = effectiveClipTick(clip, animTick);

    for (const auto& frame : clip.frames) {
        if (tick < frame.duration) {
            return &frame;
        }
        tick -= frame.duration;
    }
    return &clip.frames.back();
}

int frameIndexForClip(const AnimationClip& clip, int animTick) {
    if (clip.frames.empty()) {
        return -1;
    }

    int tick = effectiveClipTick(clip, animTick);

    for (size_t i = 0; i < clip.frames.size(); ++i) {
        if (tick < clip.frames[i].duration) {
            return static_cast<int>(i);
        }
        tick -= clip.frames[i].duration;
    }
    return static_cast<int>(clip.frames.size() - 1);
}

std::string fixed1(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

void setFighterAction(FighterState& fighter, int action) {
    if (fighter.action != action) {
        fighter.action = action;
        fighter.animTick = 0;
        fighter.appliedHitDefIds.clear();
    }
    fighter.actionClipOwnerIndex = -1;
}

const AnimationClip* findExactClipForActorWithOwner(const AppState& state, const FighterState& fighter, int action, int ownerIndex) {
    if (ownerIndex >= 0 && ownerIndex < static_cast<int>(state.fighters.size())) {
        return findExactClipForFighter(state, static_cast<size_t>(ownerIndex), action);
    }
    return findExactClipForActor(state, fighter, action);
}

bool setFighterActionElementWithOwner(const AppState& state, FighterState& fighter, int action, int elem, int ownerIndex) {
    const AnimationClip* clip = findExactClipForActorWithOwner(state, fighter, action, ownerIndex);
    if (!clip) {
        return false;
    }
    fighter.action = action;
    fighter.actionClipOwnerIndex = ownerIndex >= 0 ? ownerIndex : -1;
    fighter.animTick = animElemStartTickForClip(*clip, elem);
    return true;
}

void clearStateRuntimeControllerTracking(FighterState& fighter) {
    fighter.firedStateSoundControllerIds.clear();
    fighter.firedStateRuntimeControllerIds.clear();
    fighter.firedStateRuntimeControllerFrameKeys.clear();
    fighter.stateRuntimeControllerCooldowns.clear();
    fighter.hitPauseChangeStateControllerId = 0;
}

void tickStateRuntimeControllerTracking(FighterState& fighter) {
    for (auto& cooldown : fighter.stateRuntimeControllerCooldowns) {
        if (cooldown.ticks > 0) {
            --cooldown.ticks;
        }
    }
    fighter.stateRuntimeControllerCooldowns.erase(
        std::remove_if(
            fighter.stateRuntimeControllerCooldowns.begin(),
            fighter.stateRuntimeControllerCooldowns.end(),
            [](const StateControllerCooldown& cooldown) {
                return cooldown.ticks <= 0;
            }),
        fighter.stateRuntimeControllerCooldowns.end());
    if (fighter.hitPauseTicks <= 0) {
        fighter.hitPauseChangeStateControllerId = 0;
    }
}

int chooseMovementAction(const AppState& state, const FighterState& fighter);
const AnimationFrame* currentFrameForFighter(const AppState& state, const FighterState& fighter);
int currentAnimElemForFighter(const AppState& state, const FighterState& fighter);
int animElemTimeForFighter(const AppState& state, const FighterState& fighter, int elem);
bool shouldPlayFightSounds(const AppState& state);
bool fighterAnimationEnded(const AppState& state, const FighterState& fighter);
void finishStateIfAnimationEnded(const AppState& state, FighterState& fighter);
void enterCommonLandingState(const AppState& state, FighterState& fighter);
