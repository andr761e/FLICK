# Team quick chats

Settings → Quick Chat has sixteen immediately saved slots, arranged into four groups. Any slot can use any of the 24 approved phrases. Restore Quick Chat Defaults resets messages only; Controls rebinds the four keys independently.

By default, press F1–F4 to open Tactics, Reactions, Responses or Sportsmanship, then F1–F4 to choose one of its four messages. The chooser appears above the left-side team feed, closes after three seconds, and Escape cancels it without opening pause. This does not interrupt a prepared shot.

Quick chats reuse `UFlickTeamPingComponent`: server-generated sender names and whitelisted phrase IDs, teammate-controller-only RPC delivery, team-coloured text, five feed rows, seven-second lifetime and two-second fade. The latest five messages remain in local history after fading; each new received message brings those five back and restarts their fade timer. History is scoped to the current match and team. Pings and quick chats share a two-second cooldown (tuneable on the component). Attempting to send during cooldown shows a local-only countdown beneath the feed, without consuming a history row or extending the cooldown. Spectators cannot send or receive team messages. Messages and the chooser are hidden during menus and replays; communication is available during aiming, kickoff planning and shot resolution, as with pings.

`FlickQuickChats` owns the stable phrase IDs and local `FLICK.QuickChats` GameUserSettings configuration. It changes no simulation or gameplay parameters. Automated coverage is in `FLICK.Match.TeamPings`, including privacy, configured two-key selection, expiration, invalid requests, menu gating and shared rate limits.
