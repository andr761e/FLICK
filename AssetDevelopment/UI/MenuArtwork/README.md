# Menu illustrations // 22 October 2026

Text-free generated artwork following the approved graphite / cyan / amber UI concepts.
These are static menu illustrations, not gameplay screenshots or authoritative model renders.
Labels, player counts, difficulty, selection, party restrictions, ranks and MMR stay live in Slate.
Home and the locker continue to display the actual arena/equipped puck; this pass does not alter them.

## Playlist refinement // V2

Twelve new masters, generated with the built-in image-generation tool. Full generation and refinement
prompts: [playlist-v2-prompts.json](playlist-v2-prompts.json). Visual reference:
`Deliverables/UI-Visual-Upgrade-2026-10-22/Concepts/02-play-selection-v2.png`.

| Master | Used by |
| --- | --- |
| PlayCasualV2.png | Casual |
| PlayCompetitiveV2.png | Competitive; decorative crest, not the player's actual rank |
| PlayTrainingV2.png | Training |
| PlayPrivateV2.png | Private Match and setup summary |
| TrainingTutorialV2.png | Tutorial / Precision practice |
| TrainingFreePlayV2.png | Free Play |
| TrainingBotV2.png | 1v1 vs Bot / 1v1 format / Knockouts practice |
| TrainingBobV2.png | BOB vs Bot / BOB format / BOB Pockets practice |
| FormatDoublesV2.png | 2v2 format; eight pucks per side |
| FormatTriplesV2.png | 3v3 format; twelve pucks per side |
| PlayBackdropV2.png | Shared static Play-flow stadium backdrop |
| PracticeSwitchV2.png | Switch Control and Practice Packs |

The previous six PNGs are retained. Most V2 masters are 1860x845; BOB and final 3v3 are
1862x845; the backdrop and old practice images are 1672x940-941. Source aspect ratios are
explicit in `FlickMenuArtwork.cpp`, independent of cooked mip dimensions.

`Tools/Import-FlickMenuArtwork.py` imports to `/Game/UI/MenuArtwork` with BC7, sRGB,
generated mips and power-of-two platform data. Cards have a 1024 maximum texture size; only
the full-screen backdrop uses 2048. Existing `/Game/UI` cooking includes the assets.
Masters/prompts live outside ignored Deliverables so another developer can reimport them.

The shared image widget draws on the same eight-sided silhouette as the card frame. Its soft
text gradient follows that silhouette too; rectangular scissor clipping cannot fake cut corners.
Visible pages load textures lazily. No render targets, extra lights or scene captures were added.
Illustrated formations communicate counts, not exact starting coordinates or loadout models.
Actual puck thumbnails elsewhere are rendered from the project's meshes, not generated.
