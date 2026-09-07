# Blender pucks in the divider test arena

Play > Test > Switchyard uses the nine imported workshop meshes. Other playlists
continue using the original procedural puck visuals. The imported component has
no collision and follows the original simulated cylinder. That cylinder still
owns selection traces, mass, friction, restitution, impulses and elimination.
The existing selection halo remains visible; team light emission responds to
selection and impacts. A missing mesh falls back to the procedural presentation
and logs a warning.

Only nine static meshes are needed. Each class shares the same asset across both
teams, with a dynamic material instance supplying blue or orange emission.
Imported art preserves the current tuned diameter while using authored, art-only
height proportions after cancelling the cylinder's inherited scale. Collision,
mass and gameplay thickness remain unchanged. The test arena enables stronger specular lighting
and a reflection cubemap derived from Unreal's bundled daylight environment, so
the metallic shoulders remain readable even on an empty map. The cubemap is
copied into the test asset folder for cooking.

Assets live in `/Game/TestArena/Pucks`, explicitly included for cooking. This is
an opt-in exception to avoiding binary art dependencies: the requested Blender
meshes are now the test arena's artwork. The Python import tools are editor-only;
the packaged game does not require Blender, Python or editor scripting plugins.

## Rebuild the art

1. Run `AssetDevelopment/Pucks/scripts/export_unreal.py` in background Blender
   with `PuckWorkshop.blend` loaded. It writes origin-centered FBX files to
   `AssetDevelopment/Pucks/exports/Unreal`, with per-face planar UVs for tangents.
2. Run `Tools/Import-FlickPucks.py` using Unreal's PythonScript commandlet with
   `PythonScriptPlugin,EditorScriptingUtilities` enabled. It imports meshes,
   rebuilds tangents, assigns six native materials, checks units/pivots and saves
   `Saved/PuckImportReport.json`.
3. Build FLICKEditor, run `FLICK.Visuals.WorkshopPucks`, then capture TestArena
   using `Tools/Capture-FlickUI.ps1`.

The FBX source paths are recorded by Unreal. These are high-detail prototype meshes;
the procedural Blender surface grain is not baked into textures. Native Unreal
materials reproduce base color, roughness, metal and emission. LOD/texture baking
and packaged performance review remain separate production work.

## Validation (2026-09-07)

- FLICKEditor Win64 Development compiled successfully.
- All 26 FLICK automation tests passed, with zero test warnings or failures.
- WorkshopPucks checks all nine classes for both teams: shared meshes, team
  accents, tuned bounds, selection visibility, unchanged collision mesh/mass
  and active physics simulation.
- Final import completed without the earlier tangent/binormal diagnostics.
- TestArena and TestArenaStates were captured at 1600x900 and visually reviewed.
- A packaged build and production performance profiling have not been run.
