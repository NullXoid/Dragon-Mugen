#include "TrainingOptionsBehavior.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace dragon {

std::string_view dummyGuardModeStatus(DummyGuardMode mode) {
    switch (mode) {
    case DummyGuardMode::Off:
        return "OFF";
    case DummyGuardMode::Stand:
        return "STAND";
    case DummyGuardMode::Crouch:
        return "CROUCH";
    case DummyGuardMode::Auto:
        return "AUTO";
    default:
        return "OFF";
    }
}

void cycleDummyGuardMode(TrainingOptions& settings) {
    switch (settings.dummyGuardMode) {
    case DummyGuardMode::Off:
        settings.dummyGuardMode = DummyGuardMode::Stand;
        break;
    case DummyGuardMode::Stand:
        settings.dummyGuardMode = DummyGuardMode::Crouch;
        break;
    case DummyGuardMode::Crouch:
        settings.dummyGuardMode = DummyGuardMode::Auto;
        break;
    case DummyGuardMode::Auto:
        settings.dummyGuardMode = DummyGuardMode::Off;
        break;
    }
}

std::string_view trainingPowerModeStatus(TrainingPowerMode mode) {
    switch (mode) {
    case TrainingPowerMode::Normal:
        return "NORMAL";
    case TrainingPowerMode::Max:
        return "MAX";
    default:
        return "NORMAL";
    }
}

void cycleTrainingPowerMode(TrainingOptions& settings) {
    settings.powerMode = settings.powerMode == TrainingPowerMode::Normal
        ? TrainingPowerMode::Max
        : TrainingPowerMode::Normal;
}

std::string_view trainingMoveCategoryStatus(TrainingMoveCategory category) {
    switch (category) {
    case TrainingMoveCategory::All:
        return "ALL";
    case TrainingMoveCategory::Normals:
        return "NORMAL";
    case TrainingMoveCategory::Specials:
        return "SPECIAL";
    case TrainingMoveCategory::Supers:
        return "SUPER";
    default:
        return "ALL";
    }
}

void cycleTrainingMoveCategory(TrainingOptions& settings) {
    switch (settings.moveCategory) {
    case TrainingMoveCategory::All:
        settings.moveCategory = TrainingMoveCategory::Normals;
        break;
    case TrainingMoveCategory::Normals:
        settings.moveCategory = TrainingMoveCategory::Specials;
        break;
    case TrainingMoveCategory::Specials:
        settings.moveCategory = TrainingMoveCategory::Supers;
        break;
    case TrainingMoveCategory::Supers:
        settings.moveCategory = TrainingMoveCategory::All;
        break;
    }
    settings.selectedMoveListEntry = 0;
    settings.moveListScroll = 0;
}

std::string_view trainingOptionLabel(int option) {
    static constexpr std::array<std::string_view, kTrainingOptionCount> labels{
        "HITBOXES",
        "ORIGINS",
        "FLOOR LINE",
        "READOUT",
        "HIT FLASH",
        "HIT SPARKS",
        "HIT LOG",
        "HIT SOUND",
        "DUMMY INV",
        "AUTO LIFE",
        "DUMMY FREEZE",
        "DUMMY GUARD",
        "GUARD DMG",
        "P2 CONTROL",
        "COMMAND HUD",
        "INPUT HUD",
        "POWER",
        "MOVE TYPE",
        "MOVE LIST",
        "RESET POS",
    };
    return labels[static_cast<std::size_t>(std::clamp(option, 0, kTrainingOptionCount - 1))];
}

std::string trainingOptionStatus(const TrainingOptions& settings, int option) {
    switch (option) {
    case 0:
        return settings.showHitboxes ? "ON" : "OFF";
    case 1:
        return settings.showOrigins ? "ON" : "OFF";
    case 2:
        return settings.showFloorLine ? "ON" : "OFF";
    case 3:
        return settings.showDebugReadout ? "ON" : "OFF";
    case 4:
        return settings.showHitFlash ? "ON" : "OFF";
    case 5:
        return settings.showHitSparks ? "ON" : "OFF";
    case 6:
        return settings.showHitLog ? "ON" : "OFF";
    case 7:
        return settings.playHitSounds ? "ON" : "OFF";
    case 8:
        return settings.dummyInvincible ? "ON" : "OFF";
    case 9:
        return settings.dummyAutoLife ? "ON" : "OFF";
    case 10:
        return settings.dummyFrozen ? "ON" : "OFF";
    case 11:
        return std::string(dummyGuardModeStatus(settings.dummyGuardMode));
    case 12:
        return settings.guardDamage ? "ON" : "OFF";
    case kTrainingP2ControlOption:
        return settings.p2Controlled ? "ON" : "OFF";
    case kTrainingCommandHudOption:
        return settings.showCommandHud ? "ON" : "OFF";
    case kTrainingInputHudOption:
        return settings.showInputHud ? "ON" : "OFF";
    case kTrainingPowerOption:
        return std::string(trainingPowerModeStatus(settings.powerMode));
    case kTrainingMoveTypeOption:
        return std::string(trainingMoveCategoryStatus(settings.moveCategory));
    case kTrainingMoveListOption:
        return "OPEN";
    case kTrainingResetOption:
        return "RUN";
    default:
        return "";
    }
}

void toggleTrainingOption(TrainingOptions& settings, int option) {
    switch (option) {
    case 0:
        settings.showHitboxes = !settings.showHitboxes;
        break;
    case 1:
        settings.showOrigins = !settings.showOrigins;
        break;
    case 2:
        settings.showFloorLine = !settings.showFloorLine;
        break;
    case 3:
        settings.showDebugReadout = !settings.showDebugReadout;
        break;
    case 4:
        settings.showHitFlash = !settings.showHitFlash;
        break;
    case 5:
        settings.showHitSparks = !settings.showHitSparks;
        break;
    case 6:
        settings.showHitLog = !settings.showHitLog;
        break;
    case 7:
        settings.playHitSounds = !settings.playHitSounds;
        break;
    case 8:
        settings.dummyInvincible = !settings.dummyInvincible;
        break;
    case 9:
        settings.dummyAutoLife = !settings.dummyAutoLife;
        break;
    case 10:
        settings.dummyFrozen = !settings.dummyFrozen;
        break;
    case 11:
        cycleDummyGuardMode(settings);
        break;
    case 12:
        settings.guardDamage = !settings.guardDamage;
        break;
    case kTrainingP2ControlOption:
        settings.p2Controlled = !settings.p2Controlled;
        break;
    case kTrainingCommandHudOption:
        settings.showCommandHud = !settings.showCommandHud;
        break;
    case kTrainingInputHudOption:
        settings.showInputHud = !settings.showInputHud;
        break;
    case kTrainingPowerOption:
        cycleTrainingPowerMode(settings);
        break;
    case kTrainingMoveTypeOption:
        cycleTrainingMoveCategory(settings);
        break;
    case kTrainingMoveListOption:
        settings.moveListOpen = true;
        settings.selectedMoveListEntry = 0;
        settings.moveListScroll = 0;
        break;
    default:
        break;
    }
}

} // namespace dragon
