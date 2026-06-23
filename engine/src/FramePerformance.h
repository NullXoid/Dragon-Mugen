#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace dragon {

struct UiRenderContext;

enum class PerformanceHudMode {
    Off,
    Fps,
    Perf,
};

enum class FramePerfSection {
    EventPump = 0,
    FixedUpdate,
    StoryArenaRuntime,
    FighterUpdate,
    HelperUpdate,
    ProjectileUpdate,
    CollisionHitRouting,
    AudioQueue,
    StageDraw,
    ActorDraw,
    HudUiDraw,
    Present,
    Count,
};

struct FramePerfCounters {
    int fighters = 0;
    int activeStoryEnemies = 0;
    int helpers = 0;
    int projectiles = 0;
    int effects = 0;
    int activeSounds = 0;
    int stageBgElements = 0;
    int drawCalls = 0;
    int skippedDraws = 0;
    int globalPauseTicks = 0;
    int hitpauseActors = 0;
    int superpauseTicks = 0;
    int fixedSteps = 0;
    int droppedAccumulatorFrames = 0;
};

struct FramePerfFrame {
    double totalMs = 0.0;
    double externalElapsedMs = 0.0;
    std::array<double, static_cast<size_t>(FramePerfSection::Count)> sectionMs{};
    FramePerfCounters counters;
    bool paused = false;
    bool hitpause = false;
    bool superpause = false;
};

struct FramePerfSummary {
    int frameCount = 0;
    int gameplayFrameCount = 0;
    double fps = 0.0;
    double avgFrameMs = 0.0;
    double p95FrameMs = 0.0;
    double worstFrameMs = 0.0;
    double fpsEquivalent = 0.0;
    std::array<double, static_cast<size_t>(FramePerfSection::Count)> avgSectionMs{};
    FramePerfCounters latestCounters;
    FramePerfSection dominantSection = FramePerfSection::FixedUpdate;
};

class FramePerfState {
public:
    void beginFrame(double externalElapsedSeconds);
    void endFrame(bool paused, bool hitpause, bool superpause);
    void addSectionTime(FramePerfSection section, double milliseconds);
    void setCounters(const FramePerfCounters& counters);
    void addDrawCall(int count = 1);
    void addSkippedDraw(int count = 1);
    void addFixedStep();
    void addDroppedAccumulatorFrame();
    void updateFps(double elapsedSeconds);
    void resetHistory();

    double currentFps() const { return fps_; }
    const FramePerfCounters& currentCounters() const { return current_.counters; }
    const FramePerfFrame& latestFrame() const { return latest_; }
    FramePerfSummary summary(bool excludePauseFrames = true) const;

private:
    using Clock = std::chrono::steady_clock;

    std::array<FramePerfFrame, 240> history_{};
    size_t historyWrite_ = 0;
    size_t historyCount_ = 0;
    FramePerfFrame current_{};
    FramePerfFrame latest_{};
    Clock::time_point frameStart_{};
    double fpsWindowSeconds_ = 0.0;
    int fpsWindowFrames_ = 0;
    double fps_ = 0.0;
    bool frameOpen_ = false;
};

class FramePerfScope {
public:
    FramePerfScope(FramePerfState& state, FramePerfSection section);
    FramePerfScope(const FramePerfScope&) = delete;
    FramePerfScope& operator=(const FramePerfScope&) = delete;
    ~FramePerfScope();

private:
    FramePerfState* state_ = nullptr;
    FramePerfSection section_ = FramePerfSection::FixedUpdate;
    std::chrono::steady_clock::time_point start_{};
};

std::string_view framePerfSectionLabel(FramePerfSection section);
PerformanceHudMode cyclePerformanceHudMode(PerformanceHudMode mode, int direction);
std::string performanceHudModeText(PerformanceHudMode mode);
bool performanceOverlayEnvEnabled();
bool performanceLogEnvEnabled();
void drawFramePerformanceHud(const UiRenderContext& ui, const FramePerfState& perf, PerformanceHudMode mode, bool suppress);
void appendFramePerformanceLog(const std::filesystem::path& gameRoot, const FramePerfState& perf, int frameNumber);

} // namespace dragon
