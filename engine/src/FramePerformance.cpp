#include "FramePerformance.h"

#include "UiRenderContext.h"
#include "UiRenderPrimitives.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace dragon {
namespace {

constexpr size_t kSectionCount = static_cast<size_t>(FramePerfSection::Count);

bool envFlagEnabled(const char* name) {
    const char* value = SDL_getenv(name);
    if (!value || value[0] == '\0') {
        return false;
    }
    std::string text(value);
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text != "0" && text != "false" && text != "off" && text != "no";
}

std::string fixed1(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

double percentile(std::vector<double> values, double fraction) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const auto rawIndex = static_cast<long long>(std::lround((static_cast<double>(values.size()) - 1.0) * fraction));
    const size_t index = static_cast<size_t>(std::clamp<long long>(rawIndex, 0, static_cast<long long>(values.size() - 1)));
    return values[index];
}

} // namespace

void FramePerfState::beginFrame(double externalElapsedSeconds) {
    current_ = FramePerfFrame{};
    current_.externalElapsedMs = std::max(0.0, externalElapsedSeconds) * 1000.0;
    frameStart_ = Clock::now();
    frameOpen_ = true;
    updateFps(externalElapsedSeconds);
}

void FramePerfState::endFrame(bool paused, bool hitpause, bool superpause) {
    if (!frameOpen_) {
        return;
    }
    const auto end = Clock::now();
    current_.totalMs = std::chrono::duration<double, std::milli>(end - frameStart_).count();
    current_.paused = paused;
    current_.hitpause = hitpause;
    current_.superpause = superpause;
    latest_ = current_;
    history_[historyWrite_] = current_;
    historyWrite_ = (historyWrite_ + 1) % history_.size();
    historyCount_ = std::min(historyCount_ + 1, history_.size());
    frameOpen_ = false;
}

void FramePerfState::addSectionTime(FramePerfSection section, double milliseconds) {
    const size_t index = static_cast<size_t>(section);
    if (index >= kSectionCount) {
        return;
    }
    current_.sectionMs[index] += std::max(0.0, milliseconds);
}

void FramePerfState::setCounters(const FramePerfCounters& counters) {
    FramePerfCounters merged = counters;
    merged.drawCalls = current_.counters.drawCalls;
    merged.skippedDraws = current_.counters.skippedDraws;
    merged.fixedSteps = current_.counters.fixedSteps;
    merged.droppedAccumulatorFrames = current_.counters.droppedAccumulatorFrames;
    current_.counters = merged;
}

void FramePerfState::addDrawCall(int count) {
    current_.counters.drawCalls += std::max(0, count);
}

void FramePerfState::addSkippedDraw(int count) {
    current_.counters.skippedDraws += std::max(0, count);
}

void FramePerfState::addFixedStep() {
    ++current_.counters.fixedSteps;
}

void FramePerfState::addDroppedAccumulatorFrame() {
    ++current_.counters.droppedAccumulatorFrames;
}

void FramePerfState::updateFps(double elapsedSeconds) {
    fpsWindowSeconds_ += std::max(0.0, elapsedSeconds);
    ++fpsWindowFrames_;
    if (fpsWindowSeconds_ < 0.25) {
        return;
    }
    fps_ = fpsWindowSeconds_ > 0.0 ? static_cast<double>(fpsWindowFrames_) / fpsWindowSeconds_ : 0.0;
    fpsWindowSeconds_ = 0.0;
    fpsWindowFrames_ = 0;
}

void FramePerfState::resetHistory() {
    history_ = {};
    historyWrite_ = 0;
    historyCount_ = 0;
    current_ = {};
    latest_ = {};
    fpsWindowSeconds_ = 0.0;
    fpsWindowFrames_ = 0;
    fps_ = 0.0;
    frameOpen_ = false;
}

FramePerfSummary FramePerfState::summary(bool excludePauseFrames) const {
    FramePerfSummary out;
    out.fps = fps_;
    out.latestCounters = latest_.counters;
    if (historyCount_ == 0) {
        return out;
    }

    std::vector<double> frameTimes;
    frameTimes.reserve(historyCount_);
    std::array<double, kSectionCount> sectionTotals{};
    for (size_t i = 0; i < historyCount_; ++i) {
        const size_t index = (historyWrite_ + history_.size() - historyCount_ + i) % history_.size();
        const FramePerfFrame& frame = history_[index];
        ++out.frameCount;
        const bool excluded = excludePauseFrames && (frame.paused || frame.hitpause || frame.superpause);
        if (excluded) {
            continue;
        }
        ++out.gameplayFrameCount;
        frameTimes.push_back(frame.totalMs);
        out.avgFrameMs += frame.totalMs;
        out.worstFrameMs = std::max(out.worstFrameMs, frame.totalMs);
        for (size_t section = 0; section < kSectionCount; ++section) {
            sectionTotals[section] += frame.sectionMs[section];
        }
    }

    if (out.gameplayFrameCount <= 0) {
        return out;
    }
    out.avgFrameMs /= static_cast<double>(out.gameplayFrameCount);
    out.p95FrameMs = percentile(frameTimes, 0.95);
    out.fpsEquivalent = out.avgFrameMs > 0.0 ? 1000.0 / out.avgFrameMs : 0.0;

    double dominant = -1.0;
    for (size_t section = 0; section < kSectionCount; ++section) {
        out.avgSectionMs[section] = sectionTotals[section] / static_cast<double>(out.gameplayFrameCount);
        if (out.avgSectionMs[section] > dominant) {
            dominant = out.avgSectionMs[section];
            out.dominantSection = static_cast<FramePerfSection>(section);
        }
    }
    return out;
}

FramePerfScope::FramePerfScope(FramePerfState& state, FramePerfSection section)
    : state_(&state), section_(section), start_(std::chrono::steady_clock::now()) {}

FramePerfScope::~FramePerfScope() {
    if (!state_) {
        return;
    }
    const auto end = std::chrono::steady_clock::now();
    state_->addSectionTime(section_, std::chrono::duration<double, std::milli>(end - start_).count());
}

std::string_view framePerfSectionLabel(FramePerfSection section) {
    switch (section) {
    case FramePerfSection::EventPump: return "EVT";
    case FramePerfSection::FixedUpdate: return "UPD";
    case FramePerfSection::StoryArenaRuntime: return "MODE";
    case FramePerfSection::FighterUpdate: return "FTR";
    case FramePerfSection::HelperUpdate: return "HELP";
    case FramePerfSection::ProjectileUpdate: return "PROJ";
    case FramePerfSection::CollisionHitRouting: return "HIT";
    case FramePerfSection::AudioQueue: return "AUD";
    case FramePerfSection::StageDraw: return "STG";
    case FramePerfSection::ActorDraw: return "ACT";
    case FramePerfSection::HudUiDraw: return "HUD";
    case FramePerfSection::Present: return "PRES";
    case FramePerfSection::Count: break;
    }
    return "UNK";
}

PerformanceHudMode cyclePerformanceHudMode(PerformanceHudMode mode, int direction) {
    constexpr int count = 3;
    int value = 0;
    switch (mode) {
    case PerformanceHudMode::Off: value = 0; break;
    case PerformanceHudMode::Fps: value = 1; break;
    case PerformanceHudMode::Perf: value = 2; break;
    }
    value = (value + (direction > 0 ? 1 : -1) + count) % count;
    switch (value) {
    case 0: return PerformanceHudMode::Off;
    case 2: return PerformanceHudMode::Perf;
    case 1:
    default: return PerformanceHudMode::Fps;
    }
}

std::string performanceHudModeText(PerformanceHudMode mode) {
    switch (mode) {
    case PerformanceHudMode::Off: return "OFF";
    case PerformanceHudMode::Perf: return "PERF";
    case PerformanceHudMode::Fps:
    default: return "FPS";
    }
}

bool performanceOverlayEnvEnabled() {
    return envFlagEnabled("DRAGON_PERF_OVERLAY");
}

bool performanceLogEnvEnabled() {
    return envFlagEnabled("DRAGON_PERF_LOG");
}

void drawFramePerformanceHud(const UiRenderContext& ui, const FramePerfState& perf, PerformanceHudMode mode, bool suppress) {
    if (suppress || mode == PerformanceHudMode::Off || !ui.renderer) {
        return;
    }

    const int fps = static_cast<int>(std::lround(std::max(0.0, perf.currentFps())));
    if (mode == PerformanceHudMode::Fps) {
        const std::string text = "FPS " + std::to_string(fps);
        const float w = static_cast<float>(text.size()) * 8.0f + 8.0f;
        const float x = static_cast<float>(ui.logicalWidth) - w - 4.0f;
        constexpr float y = 4.0f;
        const Uint8 red = fps < 50 ? 222 : 118;
        const Uint8 green = fps < 50 ? 80 : 226;
        setColor(ui.renderer, 4, 7, 12, 182);
        fillRect(ui.renderer, x, y, w, 12.0f);
        setColor(ui.renderer, red, green, 160, 220);
        drawRect(ui.renderer, x, y, w, 12.0f);
        setColor(ui.renderer, 224, 238, 242, 240);
        debugText(ui.renderer, x + 4.0f, y + 3.0f, text);
        return;
    }

    const FramePerfSummary summary = perf.summary(true);
    const FramePerfCounters& counters = summary.latestCounters;
    constexpr float w = 154.0f;
    constexpr float h = 50.0f;
    const float x = std::max(4.0f, static_cast<float>(ui.logicalWidth) - w - 4.0f);
    constexpr float y = 4.0f;
    const Uint8 alert = summary.fpsEquivalent > 0.0 && summary.fpsEquivalent < 55.0 ? 222 : 118;
    setColor(ui.renderer, 4, 7, 12, 196);
    fillRect(ui.renderer, x, y, w, h);
    setColor(ui.renderer, alert, summary.fpsEquivalent < 55.0 ? 96 : 226, 180, 226);
    drawRect(ui.renderer, x, y, w, h);
    setColor(ui.renderer, 224, 238, 242, 240);
    debugText(ui.renderer, x + 4.0f, y + 4.0f, "FPS " + std::to_string(fps)
        + " AVG " + fixed1(summary.avgFrameMs) + "ms");
    setColor(ui.renderer, 170, 210, 245, 238);
    debugText(ui.renderer, x + 4.0f, y + 14.0f, "P95 " + fixed1(summary.p95FrameMs)
        + " W " + fixed1(summary.worstFrameMs));
    setColor(ui.renderer, 245, 220, 124, 238);
    const size_t dominantIndex = static_cast<size_t>(summary.dominantSection);
    const double dominantMs = dominantIndex < summary.avgSectionMs.size() ? summary.avgSectionMs[dominantIndex] : 0.0;
    debugText(ui.renderer, x + 4.0f, y + 24.0f, "DOM "
        + std::string(framePerfSectionLabel(summary.dominantSection)) + " " + fixed1(dominantMs));
    setColor(ui.renderer, 190, 224, 210, 238);
    debugText(ui.renderer, x + 4.0f, y + 34.0f, "F" + std::to_string(counters.fighters)
        + " H" + std::to_string(counters.helpers)
        + " P" + std::to_string(counters.projectiles)
        + " FX" + std::to_string(counters.effects)
        + " D" + std::to_string(counters.drawCalls)
        + "/" + std::to_string(counters.skippedDraws));
}

void appendFramePerformanceLog(const std::filesystem::path& gameRoot, const FramePerfState& perf, int frameNumber) {
    if (!performanceLogEnvEnabled() || frameNumber % 60 != 0) {
        return;
    }
    const std::filesystem::path dir = gameRoot / "artifacts" / "perf";
    std::error_code error;
    std::filesystem::create_directories(dir, error);
    if (error) {
        return;
    }
    std::ofstream out(dir / "latest.tsv", std::ios::app);
    if (!out) {
        return;
    }
    const FramePerfSummary summary = perf.summary(true);
    const FramePerfCounters counters = summary.latestCounters;
    out << frameNumber << '\t'
        << fixed1(summary.avgFrameMs) << '\t'
        << fixed1(summary.p95FrameMs) << '\t'
        << fixed1(summary.worstFrameMs) << '\t'
        << fixed1(summary.fpsEquivalent) << '\t'
        << framePerfSectionLabel(summary.dominantSection) << '\t'
        << counters.fighters << '\t'
        << counters.activeStoryEnemies << '\t'
        << counters.helpers << '\t'
        << counters.projectiles << '\t'
        << counters.effects << '\t'
        << counters.activeSounds << '\t'
        << counters.drawCalls << '\t'
        << counters.skippedDraws << '\n';
}

} // namespace dragon
