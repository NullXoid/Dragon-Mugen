#include "VerificationScenario.h"

#include "AppTypes.h"
#include "dragon/MugenData.h"
#include "dragon/Sff.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace dragon::verification {
namespace {

enum class Status { Pass, Fail, Blocked };

struct Counts {
    int pass = 0;
    int fail = 0;
    int blocked = 0;
};

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass:
        return "PASS";
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
    } else if (status == Status::Fail) {
        ++counts.fail;
    } else {
        ++counts.blocked;
    }
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass << " partial=0 fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

int exitCode(const Counts& counts) {
    if (counts.fail > 0) return 1;
    if (counts.blocked > 0) return 2;
    return 0;
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool containsNoCase(const std::string& value, std::string_view needle) {
    return lowercase(value).find(lowercase(std::string(needle))) != std::string::npos;
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

void put16(std::vector<std::uint8_t>& bytes, size_t offset, std::uint16_t value) {
    bytes[offset + 0] = static_cast<std::uint8_t>(value & 0xff);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void put32(std::vector<std::uint8_t>& bytes, size_t offset, std::uint32_t value) {
    bytes[offset + 0] = static_cast<std::uint8_t>(value & 0xff);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
    bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xff);
    bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xff);
}

bool writeText(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output) {
        return false;
    }
    output << text;
    return true;
}

bool writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return true;
}

std::filesystem::path tempScenarioRoot(std::string_view name) {
    return std::filesystem::temp_directory_path() / ("dragon_mugen_" + std::string(name));
}

bool waitForActiveFight(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight)) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight);
}

bool decodedSpriteHasVisibleColor(const DecodedSprite& decoded) {
    for (size_t i = 0; i + 3 < decoded.rgba.size(); i += 4) {
        const bool visible = decoded.rgba[i + 3] > 0;
        const bool colored = decoded.rgba[i + 0] != 0 || decoded.rgba[i + 1] != 0 || decoded.rgba[i + 2] != 0;
        if (visible && colored) {
            return true;
        }
    }
    return false;
}

const StageSlot* findStage(const std::vector<StageSlot>& stages, std::string_view name) {
    for (const auto& stage : stages) {
        if (containsNoCase(stage.displayName, name) || containsNoCase(stage.id, name)) {
            return &stage;
        }
    }
    return nullptr;
}

} // namespace

int runSffV2PngDecode(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY sff-v2-png-decode\n";

    const auto gameRoot = std::filesystem::path(runtime.rootText());
    const auto pngPath = gameRoot / "data" / "ui" / "command_complete_check.png";
    const auto png = readBytes(pngPath);
    if (png.empty()) {
        record(out, counts, Status::Blocked, "png_fixture", "missing " + pngPath.string());
        summary(out, counts);
        return exitCode(counts);
    }

    constexpr size_t kSpriteTableOffset = 0x200;
    constexpr size_t kDataOffset = kSpriteTableOffset + 28;
    std::vector<std::uint8_t> sff(kDataOffset + 4 + png.size(), 0);
    const char signature[] = "ElecbyteSpr";
    std::copy(signature, signature + 11, sff.begin());
    sff[12] = 0;
    sff[13] = 1;
    sff[14] = 0;
    sff[15] = 2;
    put32(sff, 0x24, static_cast<std::uint32_t>(kSpriteTableOffset));
    put32(sff, 0x28, 1);
    put32(sff, 0x34, static_cast<std::uint32_t>(kDataOffset));
    put32(sff, 0x38, static_cast<std::uint32_t>(4 + png.size()));

    put16(sff, kSpriteTableOffset + 0, 7);
    put16(sff, kSpriteTableOffset + 2, 9);
    put16(sff, kSpriteTableOffset + 4, 16);
    put16(sff, kSpriteTableOffset + 6, 16);
    put16(sff, kSpriteTableOffset + 8, 3);
    put16(sff, kSpriteTableOffset + 10, 4);
    put16(sff, kSpriteTableOffset + 12, 0);
    sff[kSpriteTableOffset + 14] = 10;
    sff[kSpriteTableOffset + 15] = 32;
    put32(sff, kSpriteTableOffset + 16, 0);
    put32(sff, kSpriteTableOffset + 20, static_cast<std::uint32_t>(4 + png.size()));
    put16(sff, kSpriteTableOffset + 24, 0);
    put16(sff, kSpriteTableOffset + 26, 0);
    std::copy(png.begin(), png.end(), sff.begin() + static_cast<std::ptrdiff_t>(kDataOffset + 4));

    const auto fixture = tempScenarioRoot("sff_v2_png_decode") / "fixture.sff";
    if (!writeBytes(fixture, sff)) {
        record(out, counts, Status::Blocked, "sff_fixture_write", fixture.string());
        summary(out, counts);
        return exitCode(counts);
    }

    const auto archive = loadSffArchive(fixture);
    const auto* sprite = findSprite(archive, 7, 9);
    const auto decoded = sprite ? decodeSffSprite(archive, *sprite) : std::optional<DecodedSprite>{};
    record(out, counts, archive.version == SffArchiveVersion::V2 ? Status::Pass : Status::Fail,
        "archive_reports_v2",
        "sprites=" + std::to_string(archive.sprites.size()));
    record(out, counts, sprite && sprite->encoding == SffSpriteEncoding::Png ? Status::Pass : Status::Fail,
        "sprite_reports_png",
        sprite ? "group=7 image=9" : "missing");
    record(out, counts, decoded && decoded->width > 0 && decoded->height > 0 ? Status::Pass : Status::Fail,
        "png_sprite_decodes",
        decoded ? "size=" + std::to_string(decoded->width) + "x" + std::to_string(decoded->height) : "decode failed");

    summary(out, counts);
    return exitCode(counts);
}

int runIkemenSelectSlotParsing(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY ikemen-select-slot-parsing\n";

    const auto root = tempScenarioRoot("ikemen_select_slot_parsing");
    std::filesystem::remove_all(root);
    writeText(root / "data" / "system.def", "[Files]\nselect = select.def\n");
    writeText(root / "chars" / "Hero" / "Hero.def", "[Info]\nname = \"Hero\"\ndisplayname = \"Hero\"\n");
    writeText(root / "stages" / "Keep.def", "[Info]\nname = \"Keep\"\ndisplayname = \"Keep\"\n[Camera]\nboundleft = -100\nboundright = 100\n[StageInfo]\nzoffset = 200\n");
    writeText(root / "data" / "select.def",
        "[Characters]\n"
        "Hero, stages/Keep.def\n"
        "slot = {\n"
        "FakeSlotCharacter, stages/Fake.def\n"
        "}\n"
        "randomselect\n"
        "[ExtraStages]\n"
        "stages/stages/Keep.def\n");

    const auto characters = loadCharacters(root);
    const auto stages = loadStages(root);
    record(out, counts, characters.size() == 1 && characters.front().displayName == "Hero" ? Status::Pass : Status::Fail,
        "slot_block_skips_fake_character",
        "characters=" + std::to_string(characters.size()));
    record(out, counts, stages.size() == 1 && containsNoCase(stages.front().displayName, "Keep") ? Status::Pass : Status::Fail,
        "duplicate_stage_prefix_resolves_once",
        "stages=" + std::to_string(stages.size()));

    summary(out, counts);
    return exitCode(counts);
}

int runExternalStageMount(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY external-stage-mount\n";
    const auto stages = loadStages(std::filesystem::path(runtime.rootText()));
    const auto* tram = findStage(stages, "Tram_Rooftop");
    if (!tram) {
        record(out, counts, Status::Blocked, "external_tram_stage", "Tram_Rooftop not mounted");
        summary(out, counts);
        return exitCode(counts);
    }

    record(out, counts, tram->externalContent ? Status::Pass : Status::Fail,
        "stage_marked_external",
        "package=\"" + tram->externalPackageName + "\" root=\"" + tram->externalRoot.string() + "\"");
    record(out, counts, containsNoCase(tram->bgMusicPath.string(), ".mp3") ? Status::Pass : Status::Fail,
        "external_stage_music_path",
        tram->bgMusicPath.string());
    record(out, counts, std::filesystem::exists(tram->defPath) && std::filesystem::exists(tram->bgMusicPath) ? Status::Pass : Status::Fail,
        "external_stage_files_exist",
        "def=\"" + tram->defPath.string() + "\" music=\"" + tram->bgMusicPath.string() + "\"");
    try {
        auto sffPath = tram->defPath;
        sffPath.replace_extension(".sff");
        const auto archive = loadSffArchive(sffPath);
        const auto* floor = findSprite(archive, 0, 0);
        const auto decoded = floor ? decodeSffSprite(archive, *floor) : std::optional<DecodedSprite>{};
        record(out, counts, decoded && decodedSpriteHasVisibleColor(*decoded) ? Status::Pass : Status::Fail,
            "external_stage_sff_v2_palette_decode",
            decoded ? "size=" + std::to_string(decoded->width) + "x" + std::to_string(decoded->height) : "decode failed");
    } catch (const std::exception& ex) {
        record(out, counts, Status::Fail, "external_stage_sff_v2_palette_decode", ex.what());
    }

    summary(out, counts);
    return exitCode(counts);
}

int runStageMusicCodecDecode(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY stage-music-codec-decode\n";
    if (!runtime.setup("Dcat_Leo", "Tram_Rooftop", ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Story setup failed for Tram_Rooftop");
        summary(out, counts);
        return exitCode(counts);
    }

    const bool active = waitForActiveFight(runtime, 420);
    const auto snap = runtime.snapshot();
    record(out, counts, active ? Status::Pass : Status::Fail,
        "story_fight_ready",
        "phase=" + std::to_string(snap.matchPhase));
    record(out, counts, containsNoCase(snap.selectedStageMusicPath, ".mp3") ? Status::Pass : Status::Fail,
        "mp3_stage_music_selected",
        snap.selectedStageMusicPath);
    record(out, counts, snap.activeSounds > 0 ? Status::Pass : Status::Fail,
        "mp3_stage_music_active",
        "active_sounds=" + std::to_string(snap.activeSounds));

    summary(out, counts);
    return exitCode(counts);
}

int runStoryScottTramRooftop(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-scott-tram-rooftop\n";
    if (!runtime.setup("Dcat_Leo", "Tram_Rooftop", ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Story setup failed for Tram_Rooftop");
        summary(out, counts);
        return exitCode(counts);
    }

    const bool active = waitForActiveFight(runtime, 420);
    const auto snap = runtime.snapshot();
    record(out, counts, active && !snap.loadingProgressFailed ? Status::Pass : Status::Fail,
        "story_scott_stage_enters_fight",
        "stage=\"" + runtime.stageName() + "\" phase=" + std::to_string(snap.matchPhase));
    record(out, counts, containsNoCase(runtime.stageName(), "Tram_Rooftop") ? Status::Pass : Status::Fail,
        "story_scott_stage_selected",
        runtime.stageName());
    record(out, counts, snap.selectedStageHasMusic && containsNoCase(snap.selectedStageMusicPath, "Run Scott Run.mp3") ? Status::Pass : Status::Fail,
        "story_scott_stage_music_metadata",
        snap.selectedStageMusicPath);
    record(out, counts, snap.stageBackgroundCount > 0 ? Status::Pass : Status::Fail,
        "story_scott_stage_background_loaded",
        "backgrounds=" + std::to_string(snap.stageBackgroundCount));
    record(out, counts, snap.fighterCount == 4 && snap.storyActiveEnemies == 1 ? Status::Pass : Status::Fail,
        "story_runtime_remains_wave_mode",
        "fighters=" + std::to_string(snap.fighterCount)
            + " active=" + std::to_string(snap.storyActiveEnemies));
    if (const char* screenshotPath = std::getenv("DRAGON_SCREENSHOT_PATH"); screenshotPath && *screenshotPath) {
        const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
        record(out, counts, captured ? Status::Pass : Status::Fail, "screenshot_captured", screenshotPath);
    }

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
