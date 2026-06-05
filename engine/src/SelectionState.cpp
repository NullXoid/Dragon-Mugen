#include "SelectionState.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace dragon {
namespace {

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(lhs[i])));
        const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(rhs[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}

} // namespace

const CharacterSlot* characterSlotAt(const SelectionState& selection, int index) {
    if (selection.characters.empty()
        || index < 0
        || index >= static_cast<int>(selection.characters.size())) {
        return nullptr;
    }
    return &selection.characters[static_cast<size_t>(index)];
}

const CharacterSlot* selectedCharacterSlot(const SelectionState& selection) {
    return characterSlotAt(selection, selection.selectedCharacter);
}

const CharacterSlot* sessionP1CharacterSlot(const SelectionState& selection) {
    if (const CharacterSlot* character = characterSlotAt(selection, selection.sessionSlots.p1Character)) {
        return character;
    }
    return selectedCharacterSlot(selection);
}

const CharacterSlot* loadedP1CharacterSlot(const SelectionState& selection) {
    return characterSlotAt(selection, selection.loadedP1Character);
}

int sessionP1CharacterIndex(const SelectionState& selection) {
    return characterSlotAt(selection, selection.sessionSlots.p1Character)
        ? selection.sessionSlots.p1Character
        : selection.selectedCharacter;
}

const StageSlot* stageSlotAt(const SelectionState& selection, int index) {
    if (selection.stages.empty()
        || index < 0
        || index >= static_cast<int>(selection.stages.size())) {
        return nullptr;
    }
    return &selection.stages[static_cast<size_t>(index)];
}

const StageSlot* selectedStageSlot(const SelectionState& selection) {
    return stageSlotAt(selection, selection.selectedStage);
}

int findStageIndexByDefPath(const SelectionState& selection, const std::filesystem::path& def) {
    if (def.empty()) {
        return -1;
    }
    const auto wanted = def.lexically_normal().generic_string();
    for (int i = 0; i < static_cast<int>(selection.stages.size()); ++i) {
        const auto& stage = selection.stages[static_cast<size_t>(i)];
        if (equalsNoCase(stage.defPath.lexically_normal().generic_string(), wanted)) {
            return i;
        }
    }
    return -1;
}

std::string selectedCharacterName(const SelectionState& selection) {
    if (const CharacterSlot* character = sessionP1CharacterSlot(selection)) {
        return character->displayName;
    }
    return "Unknown";
}

std::string selectedStageName(const SelectionState& selection) {
    if (const StageSlot* stage = selectedStageSlot(selection)) {
        return stage->displayName;
    }
    return "Unknown";
}

std::string characterPreferredStageName(const SelectionState& selection, int characterIndex) {
    if (characterIndex < 0 || characterIndex >= static_cast<int>(selection.characters.size())) {
        return "stage select";
    }
    const auto& character = selection.characters[static_cast<size_t>(characterIndex)];
    const int preferredStage = findStageIndexByDefPath(selection, character.preferredStagePath);
    if (preferredStage >= 0 && preferredStage < static_cast<int>(selection.stages.size())) {
        return selection.stages[static_cast<size_t>(preferredStage)].displayName;
    }
    return "stage select";
}

} // namespace dragon
