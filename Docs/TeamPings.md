# Team pings

Middle-click a hovered active enemy puck. The sender and their teammates receive
a left-side message: `Sender: Target Owner - Puck type`, in Blue/cyan or Orange
team colour. The action can be rebound in Controls. It works outside your own
turn too, without changing aiming or puck physics.

Default cooldown is 2 seconds per sender, enforced locally and on the server.
Messages remain for 7 seconds, fading over the final 2 seconds; at most 4 are
retained. These values are tuneable on `UFlickTeamPingComponent`.

The server resolves the puck ID and builds names/types itself. Only active enemy
pucks during live aiming/kickoff/physics phases are accepted. Spectators, menus,
replays and completed matches cannot initiate pings. Delivery uses individual
teammate-controller Client RPCs: there is no globally replicated chat/history
available to opponents or spectators. Receivers also verify their team and match.
Old messages are hidden on team changes, rematches, and leaving gameplay.

Automation: `FLICK.Match.TeamPings` checks actual server routing, recipient
privacy, enemy validity, authoritative cooldown, fading, and match reset.
