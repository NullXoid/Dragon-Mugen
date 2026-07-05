#include "VerificationScenario.h"

#include "dragon/Compatibility.h"
#include "dragon/MugenData.h"

#include "AppTypes.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>

namespace dragon::verification {
namespace {

enum class Status { Pass, Partial, Fail, Blocked };

struct Counts {
    int pass = 0;
    int partial = 0;
    int fail = 0;
    int blocked = 0;
};

struct AttackProbe { std::string_view name; SymbolicInput input; };

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass:
        return "PASS";
    case Status::Partial:
        return "PARTIAL";
    case Status::Fail:
        return "FAIL";
    case Status::Blocked:
    default:
        return "BLOCKED";
    }
}

void record(std::ostream& out, Counts& counts, Status status, std::string_view name, std::string_view detail) {
    out << statusText(status) << ' ' << name << "\n";
    if (!detail.empty()) {
        out << "  " << detail << "\n";
    }
    if (status == Status::Pass) {
        ++counts.pass;
    } else if (status == Status::Partial) {
        ++counts.partial;
    } else if (status == Status::Fail) {
        ++counts.fail;
    } else {
        ++counts.blocked;
    }
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass
        << " partial=" << counts.partial
        << " fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

void captureRosterProofFrame(
    RuntimeProbe& runtime,
    std::ostream& out,
    Counts& counts,
    std::string_view characterPrefix,
    std::string_view poseName) {
    const char* directory = std::getenv("DRAGON_ROSTER_SCREENSHOT_DIR");
    if (!directory || !*directory) {
        return;
    }

    const std::filesystem::path path =
        std::filesystem::path(directory) / (std::string(characterPrefix) + "_" + std::string(poseName) + ".bmp");
    const bool captured = runtime.captureScreenshot(path);
    record(out, counts, captured ? Status::Pass : Status::Fail,
        std::string(characterPrefix) + "_" + std::string(poseName) + "_screenshot",
        path.string());
}

int exitCode(const Counts& counts) {
    if (counts.fail > 0) return 1;
    if (counts.blocked > 0) return 2;
    return 0;
}

std::string sanitizeName(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const unsigned char ch : text) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == '_' || ch == '-') {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        return "character";
    }
    return out;
}

std::string fighterDetail(const FighterSnapshot& fighter) {
    return "state=" + std::to_string(fighter.stateNo)
        + " action=" + std::to_string(fighter.action)
        + " time=" + std::to_string(fighter.stateTime)
        + " life=" + std::to_string(fighter.life)
        + "/" + std::to_string(fighter.maxLife)
        + " power=" + std::to_string(fighter.power)
        + " scale=" + std::to_string(fighter.scaleX)
        + "x" + std::to_string(fighter.scaleY)
        + " move=" + std::string(1, fighter.moveType)
        + " type=" + std::string(1, fighter.stateType)
        + " phys=" + std::string(1, fighter.physics)
        + " ctrl=" + std::to_string(fighter.ctrl ? 1 : 0)
        + " ground=" + std::to_string(fighter.onGround ? 1 : 0);
}

std::string runtimeDetail(const RuntimeSnapshot& snap) {
    return "states=" + std::to_string(snap.p1RuntimeStates)
        + " commands=" + std::to_string(snap.p1RuntimeCommandEntries)
        + " hitdefs=" + std::to_string(snap.p1RuntimeHitDefs)
        + " profile=" + snap.p1CompatibilityProfile
        + " localcoord=" + std::to_string(snap.p1LocalCoordWidth)
        + "x" + std::to_string(snap.p1LocalCoordHeight)
        + " mugen=" + std::to_string(snap.p1UsesMugenSemantics ? 1 : 0);
}

bool waitForFight(RuntimeProbe& runtime, int maxFrames) {
    const int fightPhase = static_cast<int>(MatchPhase::Fight);
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == fightPhase) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == fightPhase;
}

bool waitForP1Idle(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        const auto snap = runtime.snapshot();
        if (snap.p1.stateNo == 0 && snap.p1.ctrl && snap.p1.onGround && snap.p1.moveType == 'I') {
            return true;
        }
        runtime.step({}, 1);
    }
    const auto snap = runtime.snapshot();
    return snap.p1.stateNo == 0 && snap.p1.ctrl && snap.p1.onGround && snap.p1.moveType == 'I';
}

bool p1LooksPlayable(const RuntimeSnapshot& snap) {
    return snap.fighterCount >= 2
        && snap.p1.life > 0
        && snap.p1.maxLife > 0
        && snap.p1.scaleX > 0.0f
        && snap.p1.scaleY > 0.0f;
}

const std::array<AttackProbe, 6>& attackProbes() {
    static const std::array<AttackProbe, 6> probes{ {
        { "x", SymbolicInput{ .x = true } },
        { "y", SymbolicInput{ .y = true } },
        { "z", SymbolicInput{ .z = true } },
        { "a", SymbolicInput{ .a = true } },
        { "b", SymbolicInput{ .b = true } },
        { "c", SymbolicInput{ .c = true } },
    } };
    return probes;
}

bool detectJump(RuntimeProbe& runtime, RuntimeSnapshot& observed) {
    runtime.positionFighters(-100.0f, 170.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterLife(0, 1000);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.step({}, 3);
    if (!waitForP1Idle(runtime, 120)) {
        observed = runtime.snapshot();
        return false;
    }

    for (int i = 0; i < 90; ++i) {
        runtime.step(i < 3 ? SymbolicInput{ .up = true } : SymbolicInput{}, 1);
        observed = runtime.snapshot();
        if (observed.p1.life > 0
            && !observed.p1.onGround
            && observed.p1.y < -45.0f
            && (observed.p1.action == 41 || observed.p1.action == 44)) {
            return true;
        }
    }
    return false;
}

bool detectDirectionalJump(RuntimeProbe& runtime, const SymbolicInput& jumpInput, int expectedAction, RuntimeSnapshot& observed) {
    runtime.positionFighters(-100.0f, 170.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterLife(0, 1000);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.step({}, 3);
    if (!waitForP1Idle(runtime, 120)) {
        observed = runtime.snapshot();
        return false;
    }

    for (int i = 0; i < 90; ++i) {
        runtime.step(i < 3 ? jumpInput : SymbolicInput{}, 1);
        observed = runtime.snapshot();
        if (observed.p1.life > 0 && observed.p1.action == expectedAction) {
            return observed.p1.action == expectedAction;
        }
    }
    return false;
}

bool detectMovement(RuntimeProbe& runtime, RuntimeSnapshot& before, RuntimeSnapshot& after) {
    runtime.positionFighters(-100.0f, 170.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterLife(0, 1000);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.step({}, 3);
    if (!waitForP1Idle(runtime, 120)) {
        before = runtime.snapshot();
        after = before;
        return false;
    }

    before = runtime.snapshot();
    runtime.step(SymbolicInput{ .right = true }, 45);
    after = runtime.snapshot();
    return std::abs(after.p1.x - before.p1.x) > 0.5f;
}

bool detectSimpleAttack(RuntimeProbe& runtime, AttackProbe& matched, RuntimeSnapshot& observed) {
    for (const auto& probe : attackProbes()) {
        runtime.positionFighters(-55.0f, 55.0f);
        runtime.forceFighterState(0, 0);
        runtime.forceFighterState(1, 0);
        runtime.setFighterLife(0, 1000);
        runtime.setFighterLife(1, 1000);
        runtime.setFighterControl(0, true);
        runtime.setFighterControl(1, false);
        runtime.step({}, 3);
        if (!waitForP1Idle(runtime, 120)) {
            observed = runtime.snapshot();
            continue;
        }

        const auto before = runtime.snapshot().p1;
        for (int i = 0; i < 60; ++i) {
            runtime.step(i < 3 ? probe.input : SymbolicInput{}, 1);
            observed = runtime.snapshot();
            if (observed.p1.moveType == 'A'
                || observed.p1.stateNo != before.stateNo
                || observed.p1.action != before.action) {
                matched = probe;
                return true;
            }
        }
    }
    return false;
}

bool detectSpecificAttack(RuntimeProbe& runtime, const AttackProbe& attack, int expectedState, RuntimeSnapshot& observed) {
    runtime.positionFighters(-55.0f, 55.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterLife(0, 1000);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.step({}, 3);
    if (!waitForP1Idle(runtime, 120)) {
        observed = runtime.snapshot();
        return false;
    }

    for (int i = 0; i < 60; ++i) {
        runtime.step(i < 3 ? attack.input : SymbolicInput{}, 1);
        observed = runtime.snapshot();
        if (observed.p1.stateNo == expectedState || observed.p1.action == expectedState) {
            runtime.step({}, 4);
            observed = runtime.snapshot();
            return true;
        }
    }
    return false;
}

bool detectSimpleContact(RuntimeProbe& runtime, const AttackProbe& attack, RuntimeSnapshot& observed) {
    runtime.setTrainingDummyGuardMode("off");
    runtime.positionFighters(-12.0f, 14.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterLife(0, 1000);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.step({}, 3);
    if (!waitForP1Idle(runtime, 120)) {
        observed = runtime.snapshot();
        return false;
    }

    for (int i = 0; i < 120; ++i) {
        runtime.step(i < 3 ? attack.input : SymbolicInput{}, 1);
        observed = runtime.snapshot();
        if (observed.p1.moveContact
            || observed.p1.moveHit
            || observed.p1.moveGuarded
            || observed.lastHitText.find("P1 hit ") != std::string::npos
            || observed.lastHitText.find("P1 guard ") != std::string::npos) {
            return observed.p1.life > 0
                && (observed.p1.moveContact
                    || observed.p1.moveHit
                    || observed.p1.moveGuarded
                    || observed.lastHitText.find("P1 hit ") != std::string::npos
                    || observed.lastHitText.find("P1 guard ") != std::string::npos);
        }
    }
    return false;
}

std::string readTextFile(const std::filesystem::path& path) {
    if (std::ifstream in(path); in) {
        return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
    }
    return {};
}

int pngCount(const std::filesystem::path& directory) {
    if (!std::filesystem::is_directory(directory)) {
        return 0;
    }

    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (ext == ".png") {
            ++count;
        }
    }
    return count;
}

void verifyCharacter(RuntimeProbe& runtime, std::ostream& out, Counts& counts, const RosterCharacterInfo& character) {
    const std::string prefix = sanitizeName(character.id);
    out << "CHARACTER " << character.id
        << " display=\"" << character.displayName << "\""
        << " profile=" << character.compatibilityProfile
        << " def=\"" << character.defPath << "\"\n";

    if (!runtime.setup(character.id, "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, prefix + "_setup", "Training setup failed");
        return;
    }

    const bool fightReady = waitForFight(runtime, 480);
    const bool idleReady = fightReady && waitForP1Idle(runtime, 480);
    const auto readySnap = runtime.snapshot();
    record(out, counts, fightReady && idleReady ? Status::Pass : Status::Fail,
        prefix + "_fight_idle_ready",
        runtimeDetail(readySnap) + " p1{" + fighterDetail(readySnap.p1) + "}");
    if (!fightReady || !idleReady) {
        return;
    }
    captureRosterProofFrame(runtime, out, counts, prefix, "standing");

    record(out, counts, p1LooksPlayable(readySnap) ? Status::Pass : Status::Fail,
        prefix + "_playable_basics",
        "fighters=" + std::to_string(readySnap.fighterCount) + " p1{" + fighterDetail(readySnap.p1) + "}");
    record(out, counts, readySnap.p1RuntimeStates > 0 && readySnap.p1RuntimeCommandEntries > 0 ? Status::Pass : Status::Fail,
        prefix + "_runtime_bundle",
        runtimeDetail(readySnap));
    record(out, counts, readySnap.p1RuntimeHitDefs > 0 ? Status::Pass : Status::Partial,
        prefix + "_hitdef_data",
        runtimeDetail(readySnap));

    runtime.step({}, 60);
    const auto stableSnap = runtime.snapshot();
    const bool stable = stableSnap.matchPhase == static_cast<int>(MatchPhase::Fight)
        && stableSnap.p1.life > 0
        && stableSnap.p1.stateNo == 0
        && stableSnap.p1.ctrl
        && stableSnap.p1.onGround;
    record(out, counts, stable ? Status::Pass : Status::Fail,
        prefix + "_idle_stability",
        "p1{" + fighterDetail(stableSnap.p1) + "} text=\"" + stableSnap.lastHitText + "\"");

    RuntimeSnapshot beforeMove;
    RuntimeSnapshot afterMove;
    const bool moved = detectMovement(runtime, beforeMove, afterMove);
    record(out, counts, moved ? Status::Pass : Status::Fail,
        prefix + "_walks_right",
        "x_before=" + std::to_string(beforeMove.p1.x)
        + " x_after=" + std::to_string(afterMove.p1.x)
        + " p1{" + fighterDetail(afterMove.p1) + "}");
    if (moved) {
        captureRosterProofFrame(runtime, out, counts, prefix, "walking");
    }

    RuntimeSnapshot jumpSnap;
    const bool jumped = detectJump(runtime, jumpSnap);
    record(out, counts, jumped ? Status::Pass : Status::Partial,
        prefix + "_jump_response",
        "p1{" + fighterDetail(jumpSnap.p1) + "}");
    if (jumped) {
        captureRosterProofFrame(runtime, out, counts, prefix, "jump");
    }

    RuntimeSnapshot forwardJumpSnap;
    const bool forwardJumped = detectDirectionalJump(
        runtime,
        SymbolicInput{ .right = true, .up = true },
        42,
        forwardJumpSnap);
    record(out, counts, forwardJumped ? Status::Pass : Status::Partial,
        prefix + "_jump_forward_response",
        "p1{" + fighterDetail(forwardJumpSnap.p1) + "}");
    if (forwardJumped) {
        captureRosterProofFrame(runtime, out, counts, prefix, "jump_forward");
    }

    RuntimeSnapshot backJumpSnap;
    const bool backJumped = detectDirectionalJump(
        runtime,
        SymbolicInput{ .left = true, .up = true },
        43,
        backJumpSnap);
    record(out, counts, backJumped ? Status::Pass : Status::Partial,
        prefix + "_jump_back_response",
        "p1{" + fighterDetail(backJumpSnap.p1) + "}");
    if (backJumped) {
        captureRosterProofFrame(runtime, out, counts, prefix, "jump_back");
    }

    AttackProbe matchedAttack{};
    RuntimeSnapshot attackSnap;
    const bool attacked = detectSimpleAttack(runtime, matchedAttack, attackSnap);
    record(out, counts, attacked ? Status::Pass : Status::Partial,
        prefix + "_button_attack_response",
        attacked
            ? "button=" + std::string(matchedAttack.name) + " p1{" + fighterDetail(attackSnap.p1) + "}"
            : "no x/y/z/a/b/c attack transition detected p1{" + fighterDetail(attackSnap.p1) + "}");

    RuntimeSnapshot punchSnap;
    const AttackProbe punchProbe{ "x", SymbolicInput{ .x = true } };
    const bool punched = detectSpecificAttack(runtime, punchProbe, 200, punchSnap);
    record(out, counts, punched ? Status::Pass : Status::Partial,
        prefix + "_punch_pose_response",
        "button=x p1{" + fighterDetail(punchSnap.p1) + "}");
    if (punched) {
        captureRosterProofFrame(runtime, out, counts, prefix, "punch");
    }

    RuntimeSnapshot kickSnap;
    const AttackProbe kickProbe{ "a", SymbolicInput{ .a = true } };
    const bool kicked = detectSpecificAttack(runtime, kickProbe, 230, kickSnap);
    record(out, counts, kicked ? Status::Pass : Status::Partial,
        prefix + "_kick_pose_response",
        "button=a p1{" + fighterDetail(kickSnap.p1) + "}");
    if (kicked) {
        captureRosterProofFrame(runtime, out, counts, prefix, "kick");
    }

    if (!attacked) {
        return;
    }

    RuntimeSnapshot contactSnap;
    const bool contacted = detectSimpleContact(runtime, matchedAttack, contactSnap);
    record(out, counts, contacted ? Status::Pass : Status::Partial,
        prefix + "_simple_contact",
        "button=" + std::string(matchedAttack.name) + " p1{" + fighterDetail(contactSnap.p1)
            + "} p2{" + fighterDetail(contactSnap.p2) + "} text=\"" + contactSnap.lastHitText + "\"");
}

std::vector<RosterCharacterInfo> compatibilitySmokeRoster(RuntimeProbe& runtime) {
    std::vector<RosterCharacterInfo> out = runtime.selectableCharacters();
    const std::filesystem::path gameRoot(runtime.rootText());
    const auto fixtures = loadCharactersFromSelectFile(gameRoot, gameRoot / "data" / "compatibility_select.def");
    for (const auto& character : fixtures) {
        const auto duplicate = std::any_of(out.begin(), out.end(), [&](const RosterCharacterInfo& existing) {
            return existing.id == character.id || existing.defPath == character.defPath.string();
        });
        if (!duplicate) {
            out.push_back(RosterCharacterInfo{
                character.id,
                character.displayName,
                character.defPath.string(),
                compatibilityProfileName(character.compatibilityProfile),
            });
        }
    }
    return out;
}

void verifyOwnedABen(RuntimeProbe& runtime, std::ostream& out, Counts& counts, const RosterCharacterInfo& character) {
    const std::filesystem::path root(runtime.rootText());
    const auto frameRoot = root / "chars" / "A.Ben" / "source_art" / "curated_game_sprites" / "frames";
    const std::array<std::pair<std::string_view, int>, 9> requiredActions{ {
        { "idle", 4 },
        { "crouch", 3 },
        { "walk", 6 },
        { "dash", 4 },
        { "jump", 4 },
        { "jump_forward", 4 },
        { "jump_back", 4 },
        { "punch", 4 },
        { "kick", 4 },
    } };

    for (const auto& [action, minimum] : requiredActions) {
        const int count = pngCount(frameRoot / std::string(action));
        record(out, counts, count >= minimum ? Status::Pass : Status::Fail,
            "aben_curated_" + std::string(action) + "_frames",
            "count=" + std::to_string(count) + " minimum=" + std::to_string(minimum));
    }

    const auto setupABenProbe = [&]() {
        if (!runtime.setup(character.id, "Mountainside", ScenarioMode::Training, out)) {
            return false;
        }
        if (!waitForFight(runtime, 480)) {
            return false;
        }
        runtime.setFighterLife(0, 1000);
        runtime.setFighterLife(1, 1000);
        return waitForP1Idle(runtime, 480);
    };

    if (!runtime.setup(character.id, "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "aben_setup", "Training setup failed");
        return;
    }

    const bool ready = waitForFight(runtime, 480) && waitForP1Idle(runtime, 480);
    record(out, counts, ready ? Status::Pass : Status::Fail,
        "aben_fight_idle_ready",
        "p1{" + fighterDetail(runtime.snapshot().p1) + "}");
    if (!ready) {
        return;
    }
    captureRosterProofFrame(runtime, out, counts, "aben", "standing");

    RuntimeSnapshot beforeMove;
    RuntimeSnapshot afterMove;
    setupABenProbe();
    const bool moved = detectMovement(runtime, beforeMove, afterMove);
    record(out, counts, moved ? Status::Pass : Status::Fail,
        "aben_walks_right",
        "x_before=" + std::to_string(beforeMove.p1.x) + " x_after=" + std::to_string(afterMove.p1.x));
    if (moved) {
        captureRosterProofFrame(runtime, out, counts, "aben", "walking");
    }

    RuntimeSnapshot jumpSnap;
    setupABenProbe();
    const bool jumped = detectJump(runtime, jumpSnap);
    record(out, counts, jumped ? Status::Pass : Status::Fail,
        "aben_neutral_jump",
        "p1{" + fighterDetail(jumpSnap.p1) + "}");
    if (jumped) {
        captureRosterProofFrame(runtime, out, counts, "aben", "jump");
    }

    RuntimeSnapshot crouchSnap;
    setupABenProbe();
    runtime.step(SymbolicInput{ .down = true }, 12);
    crouchSnap = runtime.snapshot();
    const bool crouched = crouchSnap.p1.onGround
        && (crouchSnap.p1.stateNo == 10 || crouchSnap.p1.stateNo == 11 || crouchSnap.p1.stateNo == 12)
        && (crouchSnap.p1.action == 10 || crouchSnap.p1.action == 11 || crouchSnap.p1.action == 12);
    record(out, counts, crouched ? Status::Pass : Status::Fail,
        "aben_crouch",
        "p1{" + fighterDetail(crouchSnap.p1) + "}");
    if (crouched) {
        captureRosterProofFrame(runtime, out, counts, "aben", "crouch");
    }

    RuntimeSnapshot forwardJumpSnap;
    setupABenProbe();
    const bool forwardJumped = detectDirectionalJump(runtime, SymbolicInput{ .right = true, .up = true }, 42, forwardJumpSnap);
    record(out, counts, forwardJumped ? Status::Pass : Status::Fail,
        "aben_forward_diagonal_jump",
        "p1{" + fighterDetail(forwardJumpSnap.p1) + "}");
    if (forwardJumped) {
        captureRosterProofFrame(runtime, out, counts, "aben", "jump_forward");
    }

    RuntimeSnapshot backJumpSnap;
    setupABenProbe();
    const bool backJumped = detectDirectionalJump(runtime, SymbolicInput{ .left = true, .up = true }, 43, backJumpSnap);
    record(out, counts, backJumped ? Status::Pass : Status::Fail,
        "aben_back_diagonal_jump",
        "p1{" + fighterDetail(backJumpSnap.p1) + "}");
    if (backJumped) {
        captureRosterProofFrame(runtime, out, counts, "aben", "jump_back");
    }

    RuntimeSnapshot punchSnap;
    const AttackProbe punchProbe{ "x", SymbolicInput{ .x = true } };
    setupABenProbe();
    const bool punched = detectSpecificAttack(runtime, punchProbe, 200, punchSnap);
    record(out, counts, punched ? Status::Pass : Status::Fail,
        "aben_punch",
        "button=x p1{" + fighterDetail(punchSnap.p1) + "}");
    if (punched) {
        captureRosterProofFrame(runtime, out, counts, "aben", "punch");
    }

    RuntimeSnapshot kickSnap;
    const AttackProbe kickProbe{ "a", SymbolicInput{ .a = true } };
    setupABenProbe();
    const bool kicked = detectSpecificAttack(runtime, kickProbe, 230, kickSnap);
    record(out, counts, kicked ? Status::Pass : Status::Fail,
        "aben_kick",
        "button=a p1{" + fighterDetail(kickSnap.p1) + "}");
    if (kicked) {
        captureRosterProofFrame(runtime, out, counts, "aben", "kick");
    }

    RuntimeSnapshot contactSnap;
    setupABenProbe();
    const bool contacted = punched && detectSimpleContact(runtime, punchProbe, contactSnap);
    record(out, counts, contacted ? Status::Pass : Status::Fail,
        "aben_punch_contact",
        "p1{" + fighterDetail(contactSnap.p1) + "} p2{" + fighterDetail(contactSnap.p2) + "} text=\"" + contactSnap.lastHitText + "\"");
}

void verifyOwnedIChie(RuntimeProbe& runtime, std::ostream& out, Counts& counts, const RosterCharacterInfo& character) {
    const std::filesystem::path root(runtime.rootText());
    const auto charRoot = root / "chars" / "I.Chie";
    const auto dragonDefText = readTextFile(charRoot / "I.Chie.dragon.def");
    const auto sourceNotes = readTextFile(charRoot / "source_art" / "README.md");
    const bool shopkeeperTagged = dragonDefText.find("role = \"shop-saleswoman\"") != std::string::npos
        && dragonDefText.find("shopkeeper = 1") != std::string::npos
        && dragonDefText.find("shop.action = 9100") != std::string::npos
        && dragonDefText.find("shop.state = 9100") != std::string::npos;
    record(out, counts, shopkeeperTagged ? Status::Pass : Status::Fail,
        "ichie_shopkeeper_metadata",
        shopkeeperTagged ? "shop action/state 9100" : "missing Dragon shopkeeper metadata");

    const bool shopAssets = std::filesystem::exists(charRoot / "I.Chie_shopkeeper_pose.png")
        && std::filesystem::exists(charRoot / "shop" / "shopkeeper_pose.png")
        && std::filesystem::exists(charRoot / "I.Chie.sff");
    record(out, counts, shopAssets ? Status::Pass : Status::Fail,
        "ichie_shop_assets",
        "shopkeeper pose and SFF");

    if (!runtime.setup(character.id, "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "ichie_setup", "Training setup failed");
    } else {
        const bool ready = waitForFight(runtime, 480) && waitForP1Idle(runtime, 480);
        record(out, counts, ready ? Status::Pass : Status::Fail,
            "ichie_runtime_loads",
            "p1{" + fighterDetail(runtime.snapshot().p1) + "}");
        if (ready) {
            captureRosterProofFrame(runtime, out, counts, "ichie", "standing_placeholder");
        }
    }

    const bool fighterWorkDeferred = sourceNotes.find("Future fighter pass") != std::string::npos
        && sourceNotes.find("standing, walk, jump, punch, kick") != std::string::npos;
    record(out, counts, fighterWorkDeferred ? Status::Pass : Status::Fail,
        "ichie_fighter_art_deferred_explicitly",
        fighterWorkDeferred ? "source_art notes require future fighter action folders" : "I.Chie fighter-art status is ambiguous");
}

} // namespace

int runRosterCompatibilitySmoke(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY roster-compatibility-smoke\n"
        << "root: " << runtime.rootText() << "\n";

    const auto characters = compatibilitySmokeRoster(runtime);
    record(out, counts, characters.empty() ? Status::Blocked : Status::Pass,
        "selectable_roster_loaded",
        "count=" + std::to_string(characters.size()));
    if (characters.empty()) {
        summary(out, counts);
        return exitCode(counts);
    }

    for (const auto& character : characters) {
        verifyCharacter(runtime, out, counts, character);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runOwnedCharacterReadiness(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY owned-character-readiness\n"
        << "root: " << runtime.rootText() << "\n";

    const auto characters = runtime.selectableCharacters();
    record(out, counts, !characters.empty() ? Status::Pass : Status::Blocked,
        "selectable_roster_loaded",
        "count=" + std::to_string(characters.size()));
    if (characters.empty()) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto isOwned = [](const RosterCharacterInfo& character) {
        return character.id == "A.Ben" || character.id == "I.Chie";
    };
    const bool onlyOwned = std::all_of(characters.begin(), characters.end(), isOwned);
    record(out, counts, onlyOwned ? Status::Pass : Status::Fail,
        "player_roster_only_owned_characters",
        "count=" + std::to_string(characters.size()));

    const auto aBen = std::find_if(characters.begin(), characters.end(), [](const RosterCharacterInfo& character) {
        return character.id == "A.Ben";
    });
    const auto iChie = std::find_if(characters.begin(), characters.end(), [](const RosterCharacterInfo& character) {
        return character.id == "I.Chie";
    });
    record(out, counts, aBen != characters.end() ? Status::Pass : Status::Fail,
        "aben_selectable",
        aBen != characters.end() ? aBen->defPath : "missing");
    record(out, counts, iChie != characters.end() ? Status::Pass : Status::Fail,
        "ichie_selectable",
        iChie != characters.end() ? iChie->defPath : "missing");

    if (aBen != characters.end()) {
        verifyOwnedABen(runtime, out, counts, *aBen);
    }
    if (iChie != characters.end()) {
        verifyOwnedIChie(runtime, out, counts, *iChie);
    }

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
