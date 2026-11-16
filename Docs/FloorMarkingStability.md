# Knockout floor markings — 2026-11-16

The remaining straight-line artifacts came from a different presentation path
than the paused replay lighting. The white guides used extremely flattened cube
instances, and the concourse joints/ticks were separate shallow meshes above the
floor. Their subpixel silhouettes, material normals and depth intersections could
change coverage independently of the floor, especially while seeking or moving.

`M_ArchitecturalMetal` now draws those markings on the existing host surfaces.
`DeckGuides` enables the cross and five circular guides on the playing deck;
`ConcourseSeams` enables the authored radial/concentric joints and timing ticks on
the surrounding floor. Both default to zero, so other uses of the material do
not opt in. The original radius-dependent layout and quadrant breaks remain.

Stroke coverage integrates the line interval over the pixel footprint. It
derives that footprint from the signed coordinate before taking an absolute
value; differentiating the absolute distance can collapse at a narrow line's
centre and reintroduce shimmer. Marking coverage blends colour, roughness,
metallic, specular, anisotropy and normal together. The deck and concourse retain
their existing reflective finish outside the markings.

The replacement guides do not build any additional mesh instances. The former
guide meshes remain available as a missing-material fallback. On the stadium's
graphite material only, `ConcourseSeamGeometry` moves the shallow floor joint/tick
vertices under the visual floor. Its height/radius selection excludes the
benches, rails, walls and soffit. Stadium ray tracing evaluates those same visual
offsets, keeping hidden detail out of the traced floor surface.

No collider, physics material, puck motion, replay recording or lighting
preference changes are involved. The previous stable-light/paused-sampling
implementation is retained. No resolution or reflection-quality increase is
required.

Rebuild the existing material with `Tools/Build-FlickArchitecturalMaterial.py`.
Its exposed parameters include `GuideWidth`, `GuideColor`, `SeamColor` and separate
guide/seam roughness/specular/metallic/anisotropy values. A new cook is
required after changing the material; copying the executable alone is insufficient.

The exact previous source, material, packaged executable and cooked containers
are in `Saved/ReplayImplementation/FloorLineRollback`. `Restore.ps1` restores
this pass while preserving the earlier replay and clip work. Rebuild the loose
editor/game binaries after a source rollback.

## Validation

Both Editor and standalone Development targets compiled, and the material was
regenerated and cooked/staged successfully. All 11 focused arena, lighting,
camera, private-match and replay automation tests passed with no test warnings
or failures. The concept test exercises 1v1, 2v2 and 3v3 radius changes, menu
round-trips, retained collision and disabled duplicate guide instances.

A controlled DX12 1920x1080 replay comparison used source time 24 seconds, 12
paused captures and 10 repeated same-time seeks. Settled paused sampling was
disabled in both runs to expose the material's actual temporal behaviour.
In narrow bands around the straight guides, mean temporal luminance standard
deviation fell from 1.4677 to 0.5516 (62%); immediately after seeks it fell from
1.6454 to 0.6849 (58%). Whole-deck mean luminance was 118.51 before and 118.87
after on a 0–255 scale. The sampled concourse bands changed much less:
0.2216 to 0.2147 paused, and 0.2641 to 0.2561 after seeks. These measurements do
not establish that all temporal lighting noise is eliminated.

The two six-second playback windows measured 9.106 ms before and 9.100 ms after
on average; this is a short scene check, not a general performance benchmark.
Light identities survived the repeated seeks, free-camera movement woke temporal
rendering, and free/opposite-player views settled correctly.

Captures and measurements are under
`Saved/ReplayImplementation/LightingStability/FloorBefore` and `FloorFinalLive`;
the analysis script is `Saved/ReplayImplementation/AnalyzeFloorLines.py`.
An initial seam finish appeared too bright and was adjusted back to a dark
joint treatment before the final packaged comparison.

The final production paused-sampling audit also passed; settled temporal
standard deviation was below 0.0004 in both sampled line regions. The rendered
1280x720 clip test passed live START/END dragging, limits, release, review,
explicit save confirmation, looping, all camera/player targets and persistence.
A fresh packaged BOB replay passed record/save/load, forward/backward seeking,
stable paused poses, free-camera movement and profile/library checks. Original
player replay payload hashes remained unchanged, and the staged executable's
SHA-256 matches the compiled executable.
