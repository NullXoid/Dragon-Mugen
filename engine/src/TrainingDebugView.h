#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace dragon {

struct TrainingDebugColorView {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct TrainingDebugLineView {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    TrainingDebugColorView color;
};

struct TrainingDebugRectView {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    TrainingDebugColorView color;
    bool doubleOutline = false;
};

struct TrainingDebugReadoutView {
    bool visible = false;
    std::span<const std::string> lines;
};

struct TrainingDebugView {
    std::span<const TrainingDebugLineView> lines;
    std::span<const TrainingDebugRectView> boxes;
    TrainingDebugReadoutView readout;
};

} // namespace dragon
