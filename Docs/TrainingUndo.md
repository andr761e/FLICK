# Free Play shot undo

Press **Backspace** or use **Undo Last Shot** in Training Tools to restore the board immediately before the last accepted shot. The binding is configurable in Settings → Controls. Undo works while pucks are moving or after the shot settles; it consumes the one-step snapshot. Another shot captures a new snapshot.

Puck IDs, teams, types, ownership, transforms, velocities, sleep state, skins and effects are captured. Removed or knocked-out pucks are recreated using the normal gameplay physics configuration. BOB striker references, raised Switchyard dividers, player statistics, turn number and shot counts are restored; pending switch deployments and resolution effects are cleared.

This is offline Free Play only—not tutorial, bot matches, private matches or online games. Entering the board editor or resetting/rebuilding the board invalidates the snapshot. **Reset Setup / R** still restores the separately saved practice setup.

Regression coverage: `FLICK.Training.ShotUndo` and `FLICK.Settings.ControlBindings`.
