#pragma once

#include <SDL3/SDL_render.h>

#include <string>
#include <string_view>
#include <vector>

namespace dragon {

enum class CommandInputTokenKind {
    Chip,
    Operator,
    Space,
};

enum class CommandInputChipTone {
    Normal,
    Selected,
    Pending,
    Current,
    Matched,
};

enum class CommandInputDirectionPresentation {
    Relative,
    Physical,
};

struct CommandInputToken {
    CommandInputTokenKind kind = CommandInputTokenKind::Chip;
    std::string text;
};

struct CommandInputIconAtlasView {
    SDL_Texture* texture = nullptr;
    int textureWidth = 0;
    int textureHeight = 0;
    int cellWidth = 24;
    int cellHeight = 16;
    int columns = 8;
};

struct CommandInputIconAtlas {
    SDL_Texture* texture = nullptr;
    int textureWidth = 0;
    int textureHeight = 0;
    int cellWidth = 24;
    int cellHeight = 16;
    int columns = 8;

    CommandInputIconAtlasView view() const {
        return CommandInputIconAtlasView{
            texture,
            textureWidth,
            textureHeight,
            cellWidth,
            cellHeight,
            columns,
        };
    }
};

struct CommandInputRenderOptions {
    float scale = 1.0f;
    CommandInputChipTone tone = CommandInputChipTone::Normal;
    CommandInputIconAtlasView iconAtlas;
    bool preferBitmapIcons = true;
    CommandInputDirectionPresentation directionPresentation = CommandInputDirectionPresentation::Relative;
    int facing = 1;
    float visualScale = 1.0f;
};

std::vector<CommandInputToken> commandInputTokens(const std::string& input);
std::string commandInputIconId(std::string_view text);
std::string commandInputPresentedIconId(std::string_view text, const CommandInputRenderOptions& options);
std::string commandInputPresentedText(std::string_view text, const CommandInputRenderOptions& options);
std::string commandInputPresentedInput(const std::string& input, const CommandInputRenderOptions& options);
bool commandInputIconAtlasReady(const CommandInputIconAtlasView& atlas);
float commandInputChipWidth(std::string_view text);
float commandInputTokenWidth(const CommandInputToken& token);
float commandInputTokenWidth(const CommandInputToken& token, const CommandInputRenderOptions& options);
float commandInputWidth(const std::string& input);
float commandInputWidth(const std::string& input, const CommandInputRenderOptions& options);

float drawCommandInputChip(
    SDL_Renderer* renderer,
    float x,
    float y,
    const std::string& text,
    const CommandInputRenderOptions& options = {});

bool drawCommandInputIconGlyph(
    SDL_Renderer* renderer,
    float x,
    float y,
    float w,
    float h,
    const std::string& text,
    const CommandInputRenderOptions& options = {});

float drawCommandInputChips(
    SDL_Renderer* renderer,
    float x,
    float y,
    float maxWidth,
    const std::string& input,
    const CommandInputRenderOptions& options = {});

} // namespace dragon
