#include "VerificationScenarioCommon.h"

#include <array>
#include <optional>

namespace dragon::verification {

int runVideoResolutionPresentationE2e(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY video-resolution-presentation-e2e\n";

    struct Case {
        int profileIndex;
        int outputWidth;
        int outputHeight;
        const char* name;
        const char* resolutionValue;
    };
    constexpr std::array<Case, 5> cases{ {
        { 0, 320, 240, "classic_320x240", "320x240 CLASSIC" },
        { 1, 426, 240, "wide_426x240", "426x240 WIDE" },
        { 2, 480, 240, "extra_480x240", "480x240 EXTRA" },
        { 3, 854, 480, "sd_854x480", "854x480 SD 480P" },
        { 4, 1280, 720, "hd_1280x720", "1280x720 HD 720P" },
    } };
    constexpr std::array<const char*, 5> expectedLabels{
        "RESOLUTION",
        "UI SCALE",
        "FPS CAP",
        "PERFORMANCE HUD",
        "BACK",
    };

    std::optional<std::uint64_t> referenceStaticUiHash;
    std::optional<std::uint64_t> referenceMenuBodyHash;
    std::optional<std::vector<std::string>> referenceStableValues;
    std::optional<std::vector<ResolutionScreenProbe>> referenceScreens;
    for (const Case& item : cases) {
        if (!runtime.setupVideoOptions(out)) {
            record(out, counts, Status::Blocked,
                std::string(item.name) + "_setup_video_options",
                "SDL Video Options setup failed");
            continue;
        }
        out << "E2E SDL Video Options ready for " << item.name << "\n" << std::flush;
        out << "E2E render " << item.name << "\n" << std::flush;
        const PresentationFrameProbe probe = runtime.videoOptionsPresentation(item.profileIndex);
        out << "E2E readback " << item.name << " complete\n" << std::flush;
        const std::string dimensions =
            "profile=" + std::to_string(probe.selectedOutputWidth) + "x" + std::to_string(probe.selectedOutputHeight)
            + " target=" + std::to_string(probe.renderTargetWidth) + "x" + std::to_string(probe.renderTargetHeight)
            + " physical=" + std::to_string(probe.physicalWidth) + "x" + std::to_string(probe.physicalHeight)
            + " readback=" + std::to_string(probe.readbackWidth) + "x" + std::to_string(probe.readbackHeight);

        record(out, counts,
            probe.selectedOutputWidth == item.outputWidth && probe.selectedOutputHeight == item.outputHeight
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_profile_selected",
            dimensions);
        record(out, counts,
            probe.renderTargetWidth == 1280 && probe.renderTargetHeight == 720
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_production_target_is_native",
            dimensions);
        record(out, counts,
            probe.readbackOk
                    && probe.physicalWidth == 1280 && probe.physicalHeight == 720
                    && probe.readbackWidth == 1280 && probe.readbackHeight == 720
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_physical_frame_readback",
            dimensions);
        record(out, counts,
            probe.sampledDistinctByteValues >= 16 ? Status::Pass : Status::Fail,
            std::string(item.name) + "_static_ui_region_is_not_blank",
            "distinct_byte_values=" + std::to_string(probe.sampledDistinctByteValues));

        bool labelsMatch = probe.menuRows.size() == expectedLabels.size();
        for (size_t i = 0; labelsMatch && i < expectedLabels.size(); ++i) {
            labelsMatch = probe.menuRows[i].label == expectedLabels[i];
        }
        record(out, counts,
            labelsMatch ? Status::Pass : Status::Fail,
            std::string(item.name) + "_video_menu_row_order_consistent",
            "rows=" + std::to_string(probe.menuRows.size()));

        const bool resolutionValueMatches = !probe.menuRows.empty()
            && probe.menuRows.front().value == item.resolutionValue;
        record(out, counts,
            resolutionValueMatches ? Status::Pass : Status::Fail,
            std::string(item.name) + "_resolution_value_matches_profile",
            probe.menuRows.empty() ? "missing resolution row" : probe.menuRows.front().value);

        std::vector<std::string> stableValues;
        for (size_t i = 1; i < probe.menuRows.size(); ++i) {
            stableValues.push_back(probe.menuRows[i].value);
        }
        if (!referenceStableValues) {
            referenceStableValues = stableValues;
        }
        record(out, counts,
            labelsMatch && stableValues == *referenceStableValues ? Status::Pass : Status::Fail,
            std::string(item.name) + "_non_resolution_values_match_reference",
            "stable_values=" + std::to_string(stableValues.size()));

        bool rowStatesMatch = probe.menuRows.size() == expectedLabels.size();
        for (size_t i = 0; rowStatesMatch && i < probe.menuRows.size(); ++i) {
            rowStatesMatch = probe.menuRows[i].selected == (i == 0)
                && probe.menuRows[i].adjustable == (i < expectedLabels.size() - 1)
                && !probe.menuRows[i].disabled;
        }
        record(out, counts,
            rowStatesMatch ? Status::Pass : Status::Fail,
            std::string(item.name) + "_selection_and_adjustability_consistent",
            "resolution selected; adjustable rows 0-3; BACK passive");

        record(out, counts,
            probe.menuTitle == "VIDEO OPTIONS"
                    && probe.menuFooter == "UP/DN MOVE  L/R CHANGE  ENT  ESC"
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_title_and_footer_consistent",
            "title=" + probe.menuTitle + " footer=" + probe.menuFooter);

        record(out, counts,
            probe.menuBodyDistinctByteValues >= 16 ? Status::Pass : Status::Fail,
            std::string(item.name) + "_menu_body_is_not_blank",
            "distinct_byte_values=" + std::to_string(probe.menuBodyDistinctByteValues));

        if (!referenceStaticUiHash) {
            referenceStaticUiHash = probe.staticUiHash;
        }
        record(out, counts,
            probe.readbackOk && probe.staticUiHash == *referenceStaticUiHash
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_static_ui_matches_reference_composition",
            "hash=" + std::to_string(probe.staticUiHash)
                + " reference=" + std::to_string(*referenceStaticUiHash));

        if (!referenceMenuBodyHash) {
            referenceMenuBodyHash = probe.menuBodyHash;
        }
        record(out, counts,
            probe.readbackOk && probe.menuBodyHash == *referenceMenuBodyHash
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_rendered_menu_body_matches_reference",
            "hash=" + std::to_string(probe.menuBodyHash)
                + " reference=" + std::to_string(*referenceMenuBodyHash)
                + " resolution value cell masked");

        const std::vector<ResolutionScreenProbe> screens = runtime.resolutionScreenPresentation(item.profileIndex);
        record(out, counts,
            screens.size() == 27 ? Status::Pass : Status::Fail,
            std::string(item.name) + "_all_engine_screens_enumerated",
            "screens=" + std::to_string(screens.size()) + " expected=27");
        if (!referenceScreens) {
            referenceScreens = screens;
        }
        for (const ResolutionScreenProbe& screen : screens) {
            const std::string prefix = std::string(item.name) + "_" + screen.name;
            record(out, counts,
                screen.readbackOk
                        && screen.readbackWidth == 1280
                        && screen.readbackHeight == 720
                    ? Status::Pass : Status::Fail,
                prefix + "_physical_frame_readback",
                std::to_string(screen.readbackWidth) + "x" + std::to_string(screen.readbackHeight));
            record(out, counts,
                screen.distinctByteValues >= 16 ? Status::Pass : Status::Fail,
                prefix + "_frame_is_not_blank",
                "distinct_byte_values=" + std::to_string(screen.distinctByteValues));
            record(out, counts,
                screen.proofSaved ? Status::Pass : Status::Fail,
                prefix + "_proof_saved",
                screen.proofPath.string());

            const auto reference = std::find_if(
                referenceScreens->begin(),
                referenceScreens->end(),
                [&](const ResolutionScreenProbe& candidate) { return candidate.name == screen.name; });
            const bool profileSpecificValue = screen.name == "options_video";
            record(out, counts,
                reference != referenceScreens->end()
                        && (profileSpecificValue || screen.frameHash == reference->frameHash)
                    ? Status::Pass : Status::Fail,
                prefix + "_composition_matches_classic",
                profileSpecificValue
                    ? "resolution value is profile-specific; masked menu assertion used"
                    : "hash=" + std::to_string(screen.frameHash)
                        + " classic=" + (reference == referenceScreens->end()
                            ? std::string("missing")
                            : std::to_string(reference->frameHash)));
        }
    }

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
