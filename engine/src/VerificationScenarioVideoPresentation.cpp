#include "VerificationScenarioCommon.h"

#include <array>
#include <optional>

namespace dragon::verification {

int runVideoResolutionPresentationE2e(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY video-resolution-presentation-e2e\n";
    if (!runtime.setupVideoOptions(out)) {
        record(out, counts, Status::Blocked, "setup_video_options", "SDL Video Options setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    out << "E2E SDL Video Options ready\n" << std::flush;

    struct Case {
        int profileIndex;
        int outputWidth;
        int outputHeight;
        const char* name;
    };
    constexpr std::array<Case, 5> cases{ {
        { 0, 320, 240, "classic_320x240" },
        { 1, 426, 240, "wide_426x240" },
        { 2, 480, 240, "extra_480x240" },
        { 3, 854, 480, "sd_854x480" },
        { 4, 1280, 720, "hd_1280x720" },
    } };

    std::optional<std::uint64_t> referenceStaticUiHash;
    for (const Case& item : cases) {
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

        if (!referenceStaticUiHash) {
            referenceStaticUiHash = probe.staticUiHash;
        }
        record(out, counts,
            probe.readbackOk && probe.staticUiHash == *referenceStaticUiHash
                ? Status::Pass : Status::Fail,
            std::string(item.name) + "_static_ui_matches_reference_composition",
            "hash=" + std::to_string(probe.staticUiHash)
                + " reference=" + std::to_string(*referenceStaticUiHash));
    }

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
