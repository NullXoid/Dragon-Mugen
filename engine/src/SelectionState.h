#pragma once

#include "AppTypes.h"
#include "dragon/MugenData.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dragon {

struct SelectionState {
    std::vector<CharacterSlot> characters;
    std::vector<StageSlot> stages;

    int selectedCharacter = 0;
    int selectedP2Character = 0;
    bool p1CharacterConfirmed = false;
    bool p2CharacterConfirmed = false;
    int loadedP1Character = -1;
    int selectedStage = 0;

    FightSessionSlots sessionSlots;
};

const CharacterSlot* characterSlotAt(const SelectionState& selection, int index);
const CharacterSlot* selectedCharacterSlot(const SelectionState& selection);
const CharacterSlot* sessionP1CharacterSlot(const SelectionState& selection);
const CharacterSlot* loadedP1CharacterSlot(const SelectionState& selection);
int sessionP1CharacterIndex(const SelectionState& selection);
int safeCharacterIndex(const SelectionState& selection, int index);
int defaultP2CharacterIndex(const SelectionState& selection, int p1Index);

const StageSlot* stageSlotAt(const SelectionState& selection, int index);
const StageSlot* selectedStageSlot(const SelectionState& selection);
int findStageIndexByDefPath(const SelectionState& selection, const std::filesystem::path& def);

std::string selectedCharacterName(const SelectionState& selection);
std::string selectedStageName(const SelectionState& selection);
std::string characterPreferredStageName(const SelectionState& selection, int characterIndex);

} // namespace dragon
