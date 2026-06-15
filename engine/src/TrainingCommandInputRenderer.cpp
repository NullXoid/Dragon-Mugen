#include "TrainingCommandInputRenderer.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <optional>
#include <string_view>

namespace dragon {

namespace {

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size()) * 8.0f;
}

bool explicitButtonToken(std::string_view text);

void fillChipPill(SDL_Renderer* renderer, float scale, float x, float y, float w, float h) {
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }
    const float radius = h * 0.5f;
    fillScaledRect(renderer, scale, x + radius, y, std::max(0.0f, w - radius * 2.0f), h);

    const int rowCount = std::max(1, static_cast<int>(std::ceil(h)));
    const float centerY = y + radius;
    for (int row = 0; row < rowCount; ++row) {
        const float sampleY = y + static_cast<float>(row) + 0.5f;
        const float dy = sampleY - centerY;
        const float span = std::sqrt(std::max(0.0f, radius * radius - dy * dy));
        fillScaledRect(renderer, scale, x + radius - span, y + static_cast<float>(row), span, 1.0f);
        fillScaledRect(renderer, scale, x + w - radius, y + static_cast<float>(row), span, 1.0f);
    }
}

bool isDirectionIconId(std::string_view iconId) {
    return iconId == "U"
        || iconId == "D"
        || iconId == "F"
        || iconId == "B"
        || iconId == "UF"
        || iconId == "UB"
        || iconId == "DF"
        || iconId == "DB";
}

bool isDirectionToken(std::string_view text, const CommandInputRenderOptions& options = {}) {
    if (explicitButtonToken(text)) {
        return false;
    }
    return isDirectionIconId(commandInputPresentedIconId(text, options));
}

void setOuterChipColor(SDL_Renderer* renderer, CommandInputChipTone tone, bool direction) {
    switch (tone) {
    case CommandInputChipTone::Selected:
        setColor(renderer, 248, 220, 166, 224);
        return;
    case CommandInputChipTone::Matched:
        setColor(renderer, 72, 160, 124, 224);
        return;
    case CommandInputChipTone::Current:
        setColor(renderer, 238, 202, 118, 230);
        return;
    case CommandInputChipTone::Pending:
        setColor(renderer, 82, 92, 108, 206);
        return;
    case CommandInputChipTone::Normal:
    default:
        setColor(renderer, direction ? 82 : 98, direction ? 112 : 104, direction ? 150 : 124, 208);
        return;
    }
}

void setInnerChipColor(SDL_Renderer* renderer, CommandInputChipTone tone, bool direction) {
    switch (tone) {
    case CommandInputChipTone::Selected:
        setColor(renderer, direction ? 22 : 28, direction ? 34 : 28, direction ? 48 : 38, 238);
        return;
    case CommandInputChipTone::Matched:
        setColor(renderer, 20, 72, 56, 238);
        return;
    case CommandInputChipTone::Current:
        setColor(renderer, 76, 54, 22, 238);
        return;
    case CommandInputChipTone::Pending:
        setColor(renderer, 18, 24, 34, 232);
        return;
    case CommandInputChipTone::Normal:
    default:
        setColor(renderer, direction ? 14 : 18, direction ? 22 : 24, direction ? 34 : 34, 232);
        return;
    }
}

void setChipTextColor(SDL_Renderer* renderer, CommandInputChipTone tone, bool direction) {
    switch (tone) {
    case CommandInputChipTone::Selected:
        setColor(renderer, 250, 242, 212);
        return;
    case CommandInputChipTone::Matched:
        setColor(renderer, 172, 236, 198);
        return;
    case CommandInputChipTone::Current:
        setColor(renderer, 255, 236, 176);
        return;
    case CommandInputChipTone::Pending:
        setColor(renderer, 174, 184, 196);
        return;
    case CommandInputChipTone::Normal:
    default:
        setColor(renderer, direction ? 186 : 220, direction ? 210 : 222, direction ? 236 : 228);
        return;
    }
}

void setOperatorColor(SDL_Renderer* renderer, CommandInputChipTone tone) {
    switch (tone) {
    case CommandInputChipTone::Selected:
        setColor(renderer, 255, 228, 196);
        return;
    case CommandInputChipTone::Matched:
        setColor(renderer, 142, 218, 178);
        return;
    case CommandInputChipTone::Current:
        setColor(renderer, 246, 218, 132);
        return;
    case CommandInputChipTone::Pending:
        setColor(renderer, 152, 164, 180);
        return;
    case CommandInputChipTone::Normal:
    default:
        setColor(renderer, 152, 164, 180);
        return;
    }
}

std::string upperTrimmed(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }

    std::string out;
    out.reserve(text.size());
    for (const char ch : text) {
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool explicitButtonToken(std::string_view text) {
    const std::string token = upperTrimmed(text);
    return token.rfind("BTN_", 0) == 0;
}

std::optional<int> commandInputIconIndexForId(std::string_view id) {
    static constexpr std::array<std::string_view, 40> kIconIds{
        "U", "D", "F", "B", "UF", "UB", "DF", "DB",
        "LP", "MP", "SP", "LK", "MK", "SK", "P", "K",
        "SQ", "TRI", "L1", "X", "O", "R1", "A", "B",
        "Y", "LB", "RB", "START", "MENU", "OPT", "HOLD", "MASH",
        "+", "/", "..", "-", "L3", "R3", "TP", "SEL",
    };

    const auto found = std::find(kIconIds.begin(), kIconIds.end(), id);
    if (found == kIconIds.end()) {
        return std::nullopt;
    }
    return static_cast<int>(std::distance(kIconIds.begin(), found));
}

std::optional<int> commandInputIconIndex(std::string_view text, const CommandInputRenderOptions& options = {}) {
    return commandInputIconIndexForId(commandInputPresentedIconId(text, options));
}

float commandInputIconWidthForId(std::string_view id) {
    if (id == "+" || id == "/" || id == "-" || id == "..") {
        return id == ".." ? 12.0f : 8.0f;
    }
    if (id == "U" || id == "D" || id == "F" || id == "B"
        || id == "UF" || id == "UB" || id == "DF" || id == "DB") {
        return 14.0f;
    }
    if (id == "HOLD" || id == "MASH" || id == "START" || id == "MENU" || id == "OPT") {
        return 25.0f;
    }
    if (id == "SQ" || id == "TRI" || id == "SEL") {
        return 21.0f;
    }
    return 17.0f;
}

float commandInputIconWidth(std::string_view text, const CommandInputRenderOptions& options = {}) {
    return commandInputIconWidthForId(commandInputPresentedIconId(text, options));
}

void drawScaledTexture(
    SDL_Renderer* renderer,
    float scale,
    SDL_Texture* texture,
    const SDL_FRect& src,
    float x,
    float y,
    float w,
    float h) {
    float oldX = 1.0f;
    float oldY = 1.0f;
    SDL_GetRenderScale(renderer, &oldX, &oldY);
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_FRect dst{ x / scale, y / scale, w / scale, h / scale };
    SDL_RenderTexture(renderer, texture, &src, &dst);
    SDL_SetRenderScale(renderer, oldX, oldY);
}

bool drawCommandInputIcon(
    SDL_Renderer* renderer,
    float x,
    float y,
    std::string_view text,
    const CommandInputRenderOptions& options,
    bool drawBacking) {
    if (!commandInputIconAtlasReady(options.iconAtlas) || !options.preferBitmapIcons) {
        return false;
    }
    const std::string iconId = commandInputPresentedIconId(text, options);
    const auto iconIndex = commandInputIconIndexForId(iconId);
    if (!iconIndex) {
        return false;
    }

    const auto& atlas = options.iconAtlas;
    const int columns = std::max(1, atlas.columns);
    const int col = *iconIndex % columns;
    const int row = *iconIndex / columns;
    const int srcX = col * atlas.cellWidth;
    const int srcY = row * atlas.cellHeight;
    if (srcX + atlas.cellWidth > atlas.textureWidth || srcY + atlas.cellHeight > atlas.textureHeight) {
        return false;
    }

    const float iconW = commandInputIconWidthForId(iconId);
    const float iconH = 10.0f;
    const bool direction = isDirectionIconId(iconId);
    if (drawBacking) {
        setOuterChipColor(renderer, options.tone, direction);
        fillChipPill(renderer, options.scale, x, y, iconW, iconH);
        setInnerChipColor(renderer, options.tone, direction);
        fillChipPill(renderer, options.scale, x + 1.0f, y + 1.0f, iconW - 2.0f, iconH - 2.0f);
        setChipTextColor(renderer, options.tone, direction);
    } else {
        setOperatorColor(renderer, options.tone);
    }

    Uint8 r = 255;
    Uint8 g = 255;
    Uint8 b = 255;
    Uint8 a = 255;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    Uint8 oldR = 255;
    Uint8 oldG = 255;
    Uint8 oldB = 255;
    Uint8 oldA = 255;
    SDL_BlendMode oldBlend = SDL_BLENDMODE_BLEND;
    SDL_GetTextureColorMod(atlas.texture, &oldR, &oldG, &oldB);
    SDL_GetTextureAlphaMod(atlas.texture, &oldA);
    SDL_GetTextureBlendMode(atlas.texture, &oldBlend);
    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(atlas.texture, r, g, b);
    SDL_SetTextureAlphaMod(atlas.texture, a);

    const SDL_FRect src{
        static_cast<float>(srcX),
        static_cast<float>(srcY),
        static_cast<float>(atlas.cellWidth),
        static_cast<float>(atlas.cellHeight),
    };
    drawScaledTexture(renderer, options.scale, atlas.texture, src, x, y, iconW, iconH);

    SDL_SetTextureColorMod(atlas.texture, oldR, oldG, oldB);
    SDL_SetTextureAlphaMod(atlas.texture, oldA);
    SDL_SetTextureBlendMode(atlas.texture, oldBlend);
    return true;
}

} // namespace

std::vector<CommandInputToken> commandInputTokens(const std::string& input) {
    const std::string source = input.empty() ? "-" : input;
    std::vector<CommandInputToken> tokens;
    std::string token;
    const auto flushToken = [&]() {
        if (!token.empty()) {
            tokens.push_back({ CommandInputTokenKind::Chip, token });
            token.clear();
        }
    };

    for (const char ch : source) {
        if (ch == '+' || ch == '/') {
            flushToken();
            tokens.push_back({ CommandInputTokenKind::Operator, std::string(1, ch) });
            continue;
        }
        if (ch == ' ') {
            flushToken();
            if (tokens.empty() || tokens.back().kind != CommandInputTokenKind::Space) {
                tokens.push_back({ CommandInputTokenKind::Space, " " });
            }
            continue;
        }
        token.push_back(ch);
    }
    flushToken();

    if (tokens.empty()) {
        tokens.push_back({ CommandInputTokenKind::Chip, "-" });
    }
    return tokens;
}

std::string commandInputIconId(std::string_view text) {
    std::string token = upperTrimmed(text);
    if (token.size() > 2 && token.front() == '(' && token.back() == ')') {
        token = token.substr(1, token.size() - 2);
    }
    if (token == "DOWN") return "D";
    if (token == "UP") return "U";
    if (token == "FWD" || token == "FORWARD") return "F";
    if (token == "BACK") return "B";
    if (token == "SQUARE") return "SQ";
    if (token == "TRIANGLE") return "TRI";
    if (token == "CIRCLE") return "O";
    if (token == "CROSS") return "X";
    if (token == "BTN_X") return "X";
    if (token == "BTN_Y") return "Y";
    if (token == "BTN_A") return "A";
    if (token == "BTN_B") return "B";
    if (token == "SELECT") return "SEL";
    if (token == "...") return "..";
    return token;
}

std::string commandInputPresentedIconId(std::string_view text, const CommandInputRenderOptions& options) {
    std::string id = commandInputIconId(text);
    if (options.directionPresentation != CommandInputDirectionPresentation::Physical
        || explicitButtonToken(text)
        || options.facing >= 0
        || !isDirectionIconId(id)) {
        return id;
    }

    if (id == "F") return "B";
    if (id == "B") return "F";
    if (id == "UF") return "UB";
    if (id == "UB") return "UF";
    if (id == "DF") return "DB";
    if (id == "DB") return "DF";
    return id;
}

std::string commandInputPresentedText(std::string_view text, const CommandInputRenderOptions& options) {
    const std::string id = commandInputPresentedIconId(text, options);
    if (isDirectionIconId(id) || explicitButtonToken(text)) {
        return id;
    }
    return std::string(text);
}

std::string commandInputPresentedInput(const std::string& input, const CommandInputRenderOptions& options) {
    std::string output;
    for (const auto& token : commandInputTokens(input)) {
        if (token.kind == CommandInputTokenKind::Chip) {
            output += commandInputPresentedText(token.text, options);
        } else {
            output += token.text;
        }
    }
    return output;
}

bool commandInputIconAtlasReady(const CommandInputIconAtlasView& atlas) {
    return atlas.texture
        && atlas.textureWidth > 0
        && atlas.textureHeight > 0
        && atlas.cellWidth > 0
        && atlas.cellHeight > 0
        && atlas.columns > 0;
}

bool drawCommandInputIconGlyph(
    SDL_Renderer* renderer,
    float x,
    float y,
    float w,
    float h,
    const std::string& text,
    const CommandInputRenderOptions& options) {
    if (!commandInputIconAtlasReady(options.iconAtlas) || !options.preferBitmapIcons || w <= 0.0f || h <= 0.0f) {
        return false;
    }

    const std::string iconId = commandInputPresentedIconId(text, options);
    const auto iconIndex = commandInputIconIndexForId(iconId);
    if (!iconIndex) {
        return false;
    }

    const auto& atlas = options.iconAtlas;
    const int columns = std::max(1, atlas.columns);
    const int col = *iconIndex % columns;
    const int row = *iconIndex / columns;
    const int srcX = col * atlas.cellWidth;
    const int srcY = row * atlas.cellHeight;
    if (srcX + atlas.cellWidth > atlas.textureWidth || srcY + atlas.cellHeight > atlas.textureHeight) {
        return false;
    }

    Uint8 r = 255;
    Uint8 g = 255;
    Uint8 b = 255;
    Uint8 a = 255;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    Uint8 oldR = 255;
    Uint8 oldG = 255;
    Uint8 oldB = 255;
    Uint8 oldA = 255;
    SDL_BlendMode oldBlend = SDL_BLENDMODE_BLEND;
    SDL_GetTextureColorMod(atlas.texture, &oldR, &oldG, &oldB);
    SDL_GetTextureAlphaMod(atlas.texture, &oldA);
    SDL_GetTextureBlendMode(atlas.texture, &oldBlend);
    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(atlas.texture, r, g, b);
    SDL_SetTextureAlphaMod(atlas.texture, a);

    const SDL_FRect src{
        static_cast<float>(srcX),
        static_cast<float>(srcY),
        static_cast<float>(atlas.cellWidth),
        static_cast<float>(atlas.cellHeight),
    };
    drawScaledTexture(renderer, options.scale, atlas.texture, src, x, y, w, h);

    SDL_SetTextureColorMod(atlas.texture, oldR, oldG, oldB);
    SDL_SetTextureAlphaMod(atlas.texture, oldA);
    SDL_SetTextureBlendMode(atlas.texture, oldBlend);
    return true;
}

float commandInputChipWidth(std::string_view text) {
    return std::clamp(debugTextWidth(std::string(text)) + 6.0f, 12.0f, 42.0f);
}

float commandInputTokenWidth(const CommandInputToken& token) {
    return commandInputTokenWidth(token, {});
}

float commandInputTokenWidth(const CommandInputToken& token, const CommandInputRenderOptions& options) {
    if (commandInputIconAtlasReady(options.iconAtlas)
        && options.preferBitmapIcons
        && commandInputIconIndex(token.text, options)) {
        if (token.kind == CommandInputTokenKind::Space) {
            return 4.0f;
        }
        return commandInputIconWidth(token.text, options);
    }

    switch (token.kind) {
    case CommandInputTokenKind::Chip:
        return commandInputChipWidth(token.text);
    case CommandInputTokenKind::Operator:
        return 7.0f;
    case CommandInputTokenKind::Space:
        return 4.0f;
    }
    return 0.0f;
}

float commandInputWidth(const std::string& input) {
    return commandInputWidth(input, {});
}

float commandInputWidth(const std::string& input, const CommandInputRenderOptions& options) {
    float width = 0.0f;
    for (const auto& token : commandInputTokens(input)) {
        width += commandInputTokenWidth(token, options);
    }
    return width;
}

float drawCommandInputChip(
    SDL_Renderer* renderer,
    float x,
    float y,
    const std::string& text,
    const CommandInputRenderOptions& options) {
    if (drawCommandInputIcon(renderer, x, y, text, options, true)) {
        return commandInputIconWidth(text, options);
    }

    const std::string displayText = commandInputPresentedText(text, options);
    const float chipW = commandInputChipWidth(displayText);
    const float chipH = 9.0f;
    const bool direction = isDirectionToken(text, options);

    setOuterChipColor(renderer, options.tone, direction);
    fillChipPill(renderer, options.scale, x, y, chipW, chipH);
    setInnerChipColor(renderer, options.tone, direction);
    fillChipPill(renderer, options.scale, x + 1.0f, y + 1.0f, chipW - 2.0f, chipH - 2.0f);
    setChipTextColor(renderer, options.tone, direction);

    const float textX = x + std::max(2.0f, (chipW - debugTextWidth(displayText)) * 0.5f);
    scaledDebugText(renderer, options.scale, textX, y + 2.0f, displayText);
    return chipW;
}

float drawCommandInputChips(
    SDL_Renderer* renderer,
    float x,
    float y,
    float maxWidth,
    const std::string& input,
    const CommandInputRenderOptions& options) {
    const float maxX = x + std::max(0.0f, maxWidth);
    float penX = x;

    for (const auto& token : commandInputTokens(input)) {
        const float tokenW = commandInputTokenWidth(token, options);
        if (penX + tokenW > maxX) {
            const CommandInputToken ellipsis{ CommandInputTokenKind::Chip, ".." };
            const float ellipsisW = commandInputTokenWidth(ellipsis, options);
            if (penX + ellipsisW <= maxX) {
                drawCommandInputChip(renderer, penX, y, "..", options);
                penX += ellipsisW;
            }
            return penX - x;
        }

        if (token.kind == CommandInputTokenKind::Chip) {
            penX += drawCommandInputChip(renderer, penX, y, token.text, options);
        } else if (token.kind == CommandInputTokenKind::Operator) {
            if (!drawCommandInputIcon(renderer, penX, y, token.text, options, false)) {
                setOperatorColor(renderer, options.tone);
                scaledDebugText(renderer, options.scale, penX, y + 2.0f, token.text);
            }
            penX += tokenW;
        } else {
            penX += tokenW;
        }
    }

    return penX - x;
}

} // namespace dragon
