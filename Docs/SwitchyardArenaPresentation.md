# Switchyard arena presentation

The new design is permanent for 1v1, 2v2 and 3v3. Guides and rim accents scale
with arena radius; socket frames follow the authoritative layout and actual
divider lengths. BOB is unchanged.

The old-design toggle, material rollback cache and original-design capture flag
have been removed. Existing imported assets remain necessary source geometry
and material parents, not a separate backup map.

All added dressing is non-colliding. Gameplay dimensions, layouts, friction,
restitution, switch timing and puck physics are unchanged.

Menu and gameplay now share the 250 softbox multiplier, light shapes/colours,
camera-relative reflections, deck finish and Switchyard colour grade. Gameplay
does not enable menu depth of field. Menu and Gameplay use one shared preference
preset. Existing menu values are retained as the shared settings; legacy gameplay
values are ignored. Changes from either screen apply to both.

The key and warm rectangular sources are broad panels (1100x1200 cm and
1000x900 cm at the 1v1 radius), scaled with each arena. Their source areas,
not their 250 power multiplier, soften the overlapping left-side mirror patches
visible in full-board gameplay views. Dimensions are tuneable in the lighting
rig's `FParameters`; no extra light actors are added.

Tests: `FLICK.Arena.ConceptPresentation`, `FLICK.Arena.DividerLift`,
`FLICK.Arena.DividerRimClearance`.

Render checks: `Tools/Capture-FlickUI.ps1 -Screen TestArena,TestArenaStates
-PlayersPerTeam 2` (or 1/3).
