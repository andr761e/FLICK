# Private-match spectator views

Choose **Spectate** in the private-match role picker. Spectators automatically
follow the first Blue seat. Use the card arrows or the configured Orbit Left /
Orbit Right keys (Q / E by default) to cycle every Blue and Orange seat, including
bots. The card below the arena names the selected player or bot in team colour.
Use X / Free Camera for the existing movable camera; X or Escape returns to the
same followed seat. Joining a team or leaving the room clears follow mode.

Human views mirror their arena camera position, rotation and field of view,
including orbit, height, top-down and aiming camera adjustments. This is a game
camera view, not streaming their desktop, cursor, settings menus or local HUD.
Bots have no client camera, so their view uses the standard camera from their team
side. Empty seats becoming occupied, or players leaving, are resolved live.

`UFlickPrivateSpectatorComponent` publishes human cameras through an unreliable
owner-to-server RPC at 10 Hz, validated/rate-limited on `AFlickPlayerState`, then
replicated to other clients. Camera data never drives gameplay or physics. Public
playlists, spectators and completed matches cannot publish it. The local camera
interpolates the followed view; replays and post-match presentation take priority.
Publish interval and camera blend speed are tuneable C++ component/pawn defaults.

Verification: `FLICK.Match.PrivateSpectatorView` covers target cycling, human
pose/zoom, bot fallback, free-camera return, invalid snapshots and private-only
guards. `Tools/Capture-FlickUI.ps1 -Screen Spectator` captures the actual card/UI
with an isolated profile. A full multi-client playtest is still recommended.
