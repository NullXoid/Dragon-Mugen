#pragma once

// Internal App.cpp implementation shard for Story Mode reward feedback.
// Combat/progression ownership stays in StoryModeRuntime and DragonProgression;
// this file only owns transient visual feedback for rewards already awarded.

void spawnStoryRewardFeedback(
    AppState& state,
    const FighterState& defeatedEnemy,
    const DragonProgressionAwardResult& award) {
    if (!award.applied || (award.xpGained <= 0 && award.goldGained <= 0)) {
        return;
    }

    StoryRewardPopup popup;
    popup.x = defeatedEnemy.x;
    popup.y = defeatedEnemy.y - 42.0f;
    popup.depthZ = defeatedEnemy.depthZ;
    popup.xp = award.xpGained;
    popup.gold = award.goldGained;
    popup.oldLevel = award.oldLevel;
    popup.newLevel = award.newLevel;
    state.story.rewardPopups.push_back(popup);

    if (award.goldGained <= 0) {
        return;
    }

    const int coinCount = std::clamp((award.goldGained + 9) / 10, 1, 8);
    for (int i = 0; i < coinCount; ++i) {
        const float side = (i % 2 == 0) ? -1.0f : 1.0f;
        const float spread = static_cast<float>((i / 2) + 1);
        StoryRewardCoin coin;
        coin.x = defeatedEnemy.x + side * spread * 4.0f;
        coin.y = defeatedEnemy.y - 14.0f;
        coin.depthZ = defeatedEnemy.depthZ;
        coin.vx = side * (0.55f + spread * 0.12f);
        coin.vy = -1.6f - static_cast<float>(i % 3) * 0.28f;
        coin.ageTicks = i * -3;
        state.story.rewardCoins.push_back(coin);
    }
}

void updateStoryRewardFeedback(AppState& state) {
    for (auto& popup : state.story.rewardPopups) {
        ++popup.ageTicks;
        popup.y -= 0.18f;
    }
    state.story.rewardPopups.erase(
        std::remove_if(
            state.story.rewardPopups.begin(),
            state.story.rewardPopups.end(),
            [](const StoryRewardPopup& popup) {
                return popup.ageTicks >= popup.durationTicks;
            }),
        state.story.rewardPopups.end());

    for (auto& coin : state.story.rewardCoins) {
        ++coin.ageTicks;
        if (coin.ageTicks < 0) {
            continue;
        }
        coin.x += coin.vx;
        coin.y += coin.vy;
        coin.vy += 0.085f;
    }
    state.story.rewardCoins.erase(
        std::remove_if(
            state.story.rewardCoins.begin(),
            state.story.rewardCoins.end(),
            [](const StoryRewardCoin& coin) {
                return coin.ageTicks >= coin.durationTicks;
            }),
        state.story.rewardCoins.end());
}

void drawStoryRewardCoin(SDL_Renderer* renderer, const ArenaProjectedPoint& projected, int alpha) {
    setColor(renderer, 0, 0, 0, static_cast<Uint8>(std::clamp(alpha / 2, 0, 255)));
    fillRect(renderer, projected.screenX - 3.0f, projected.screenY - 2.0f, 7.0f, 7.0f);
    setColor(renderer, 250, 210, 72, static_cast<Uint8>(std::clamp(alpha, 0, 255)));
    fillRect(renderer, projected.screenX - 2.0f, projected.screenY - 4.0f, 6.0f, 6.0f);
    setColor(renderer, 255, 246, 160, static_cast<Uint8>(std::clamp(alpha, 0, 255)));
    fillRect(renderer, projected.screenX - 1.0f, projected.screenY - 3.0f, 2.0f, 2.0f);
}

void drawStoryRewardFeedback(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    if (!isStoryMode(state)) {
        return;
    }

    for (const auto& coin : state.story.rewardCoins) {
        if (coin.ageTicks < 0) {
            continue;
        }
        const int alpha = std::clamp(
            255 - (coin.ageTicks * 255 / std::max(1, coin.durationTicks)),
            0,
            255);
        const ArenaProjectedPoint projected = projectArenaWorldPoint(state, stage, coin.x, coin.y, coin.depthZ);
        drawStoryRewardCoin(renderer, projected, alpha);
    }

    for (const auto& popup : state.story.rewardPopups) {
        const int alpha = std::clamp(
            255 - (popup.ageTicks * 210 / std::max(1, popup.durationTicks)),
            0,
            255);
        const ArenaProjectedPoint projected = projectArenaWorldPoint(state, stage, popup.x, popup.y, popup.depthZ);
        std::string text;
        if (popup.xp > 0) {
            text += "+";
            text += std::to_string(popup.xp);
            text += "XP";
        }
        if (popup.gold > 0) {
            if (!text.empty()) {
                text += " ";
            }
            text += "+";
            text += std::to_string(popup.gold);
            text += "G";
        }
        if (popup.newLevel > popup.oldLevel) {
            text += " LV ";
            text += std::to_string(popup.oldLevel);
            text += ">";
            text += std::to_string(popup.newLevel);
        }

        setColor(renderer, 0, 0, 0, static_cast<Uint8>(std::clamp(alpha / 2, 0, 255)));
        debugTextCentered(renderer, projected.screenX + 1.0f, projected.screenY + 1.0f, text);
        setColor(renderer, 120, 255, 188, static_cast<Uint8>(alpha));
        debugTextCentered(renderer, projected.screenX, projected.screenY, text);
    }
}
