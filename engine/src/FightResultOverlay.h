#pragma once

#include "FightPresentationView.h"
#include "UiRenderContext.h"

namespace dragon {

void drawRoundStartOverlay(const UiRenderContext& ui, const FightRoundCalloutView& view);
void drawRoundFinishOverlay(const UiRenderContext& ui, const FightRoundCalloutView& view);
void drawRoundResultOverlay(const UiRenderContext& ui, const FightRoundResultView& view);
void drawMatchResultScreen(const UiRenderContext& ui, const FightMatchResultView& view);

} // namespace dragon
