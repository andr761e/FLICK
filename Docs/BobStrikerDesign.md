# BOB striker presentation

The striker uses the original Standard mesh, dimensions and physics body.
There is no floating text, cross mesh, decal volume, additional light or texture.
Its centre is a pearl medallion with a dark diamond sight, index marks and a
small skin-coloured dot; the surrounding shell stays graphite and satin metal.
Pocketable pucks retain their existing appearance.

`Tools/Build-FlickBobStrikerMaterial.py` rebuilds the small Unreal material at
`/Game/Cosmetics/Pucks/BOB/M_BobStrikerFace`. Run it with Unreal's
`-run=pythonscript -script=<absolute script path>` commandlet. The existing
`/Game/Cosmetics/Pucks` cook directory already includes it in packaged builds.

The graphic uses mesh rest-space XY coordinates passed through a vertex
interpolator, so it stays attached through rotation, physics, arrival and replay.
It is projected across the source mesh's core, emblem, gasket and housing material
sections; the radial mask keeps the exterior graphite. `FaceRadius`, `BaseColor`,
`HousingColor`, `InkColor`, `AccentColor`, `Metallic` and `Roughness` are material
parameters. Runtime instances retain the chosen skin colour without changing
the shared palettes or mesh assets.

Close-up QA: `Tools/Capture-FlickUI.ps1 -Screen BobStriker -PuckSkin 3 -CaptureDelaySeconds 20`.
The longer delay allows the first uncooked editor shader compile to finish;
packaged builds use cooked shaders.
The close-up mode keeps the real striker stationary; normal BOB preview/matches
are unaffected. `FLICK.Cosmetics.InspectionAndOwnership` checks every skin,
the absence of floating text, and unchanged gameplay dimensions.
