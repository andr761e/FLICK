# High-detail pucks in the divider test arena

Play > Test > Switchyard uses the nine high-detail presentation meshes. Other
playlists retain their existing presentation. Each imported mesh is attached to
the original simulated cylinder with collision disabled, so selection, mass,
friction, restitution, impulses and ring-out detection are unchanged.

Geometry is shared between teams. Dynamic material instances replace cyan with
orange for Player 2 while retaining the authored graphite, silver and recessed
materials. Missing art logs a warning and falls back to the procedural visual.

## Rebuild and import

1. Rebuild FBX files with the two exporters documented in
   `AssetDevelopment/Pucks/HighDetail/README.md`.
2. Run `Tools/Import-FlickBlueStandardPrototype.py` for Standard.
3. Run `Tools/Import-FlickHighDetailPucks.py` for the other eight archetypes.
4. Build `FLICKEditor` and run `FLICK.Visuals.WorkshopPucks`.

The importers read only `AssetDevelopment/Pucks/HighDetail/exports` and its
manifests. The archived procedural workshop is not part of this workflow.

## Production note

These are detailed prototype meshes without shipping LODs or baked surface
maps. Optimization and packaged-build profiling remain future art-production
work; they are deliberately separate from the gameplay physics bodies.
