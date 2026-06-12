# Arena Mode Test Plan

- Arena menu entry appears.
- Arena opens character select and setup.
- 1P vs 1 CPU starts.
- 1P vs 2 CPUs starts.
- 1P vs 3 CPUs starts.
- Defeated fighters are ignored by the win condition.
- Match ends when one fighter remains.
- Winner is recorded.
- Winner screen appears.
- End screen appears.
- Return to menu works.
- Z Axis can be toggled on/off from Arena Setup.
- Holding Shift or left trigger moves along depth only while Z Axis is on and uses the fighter's walk animation.
- Double-tapping Shift or left trigger performs a short sidestep using the fighter's walk animation during movement.
- `OpenBOR Scroll Test` starts in Arena and scrolls forward only.
- `OpenBOR Scroll Test` camera clamps at the configured end and does not scroll backward.
- Classic mode still works.

## Manual Smoke Script

1. Launch game.
2. Select Arena Mode.
3. Pick player character.
4. Set CPU count to 3.
5. Toggle Z Axis on, select `OpenBOR Scroll Test`, and start match.
6. Confirm four fighters load.
7. Hold Shift or left trigger with Up/Down and confirm depth movement.
8. Move right and confirm the stage scrolls forward and eventually clamps.
9. Move left and confirm the camera does not scroll backward.
10. Defeat or simulate defeated CPU fighters.
11. Confirm fight ends when one fighter remains.
12. Confirm winner screen appears.
13. Confirm end screen appears.
14. Return to main menu.
15. Launch classic mode and confirm it still works.
