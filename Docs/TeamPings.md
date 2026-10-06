# Team pings

Middle-click a hovered active enemy puck. The sender and their teammates receive
a left-side message: `Sender: Target Owner - Puck type`, in Blue/cyan or Orange
team colour. The action can be rebound in Controls. It works outside your own
turn too, without changing aiming or puck physics.

Middle-click a hovered Switchyard switch to send `Sender: Switch 03 / NORTH`
(using that switch's actual number/direction) through the same team-only feed.
The complete visible switch disc is selectable, not just its activation dot.
Pucks take hover priority so switches cannot be pinged through them. Switch
selection is read-only: a ping never toggles its divider.

Default cooldown is 2 seconds per sender, enforced locally and on the server.
Messages remain for 7 seconds, fading over the final 2 seconds; at most 4 are
retained. These values are tuneable on `UFlickTeamPingComponent`.

The server resolves the puck ID or validates the switch index against the active
arena and builds the message itself. Pucks and switches share one cooldown.
Only active enemy pucks and existing switches during live aiming/kickoff/physics
phases are accepted. Spectators, menus,
replays and completed matches cannot initiate pings. Delivery uses individual
teammate-controller Client RPCs: there is no globally replicated chat/history
available to opponents or spectators. Receivers also verify their team and match.
Old messages are hidden on team changes, rematches, and leaving gameplay.

Automation: `FLICK.Match.TeamPings` checks actual server routing, recipient
privacy, enemy/switch validity, shared local/server cooldown, fading, match reset,
non-activation and switch picking across 1v1/2v2/3v3 transformed layouts.
