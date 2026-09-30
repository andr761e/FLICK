# High-detail pucks in the divider test arena

Play > Test > Switchyard uses the nine high-detail presentation meshes. Other
playlists retain their existing presentation. Each imported mesh is attached to
the original simulated cylinder with collision disabled, so selection, mass,
friction, restitution, impulses and ring-out detection are unchanged.

Geometry is shared by all players. Classic Blue and Classic Orange are freely
equippable starter skins, not team assignments. Only the emissive rim/type-symbol
materials change; authored graphite, silver and recessed materials are retained.
Team ownership is shown by a base ring and a quick, team-coloured
`Name - Puck type` hover label, independent of cosmetic colours. There are no
baked player-number variants. Missing art logs a warning and falls back to the
procedural visual.

The local viewer's own pucks have no ownership ring. Teammates and opponents
retain blue/orange rings. This cue is local-only, independent of turn order,
cosmetic skin and selection; private controlled seats are respected. The
Interface colour-blind assistance option makes opponent rings wider and adds
YOU / TEAMMATE / OPPONENT text to puck hover labels.

## Rebuild and import

1. Replace the desired base FBXs under `AssetDevelopment/Pucks/ClassicBlue/exports`.
2. Close Unreal Editor and run `.\update-flick-pucks.cmd` to reimport all nine
   pucks and refresh the orange material swap. Blender is not needed.
3. Build `FLICKEditor` and run `FLICK.Visuals.WorkshopPucks`,
   `FLICK.Visuals.PuckSkins`, and `FLICK.Cosmetics.InspectionAndOwnership`.

The older source-model exporters documented in `AssetDevelopment/Pucks/ClassicBlue/README.md`
overwrite base FBXs; do not run them to import a newly supplied FBX replacement.

The importers read `AssetDevelopment/Pucks/ClassicBlue/exports`, its manifests
and `AssetDevelopment/Pucks/ClassicOrange/set.json`. The archived procedural
workshop is not part of this workflow. The Pucks root contains set folders only;
Orange shares geometry rather than duplicating Blue FBXs.

## Production note

These are detailed prototype meshes without shipping LODs or baked surface
maps. Optimization and packaged-build profiling remain future art-production
work; they are deliberately separate from the gameplay physics bodies.
