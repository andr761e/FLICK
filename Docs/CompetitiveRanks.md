# Competitive ranks

All public competitive formats use this ladder, with separate progress for each
variant and team size. Casual, training and private matches do not award MMR.
Existing accounts keep their MMR and placement progress.

| Tier | I | II | III |
| --- | --- | --- | --- |
| Carbon | 0–199 | 200–399 | 400–599 |
| Alloy | 600–699 | 700–799 | 800–899 |
| Tempered | 900–999 | 1000–1099 | 1100–1199 |
| Vector | 1200–1299 | 1300–1399 | 1400–1499 |
| Prism | 1500–1599 | 1600–1699 | 1700–1799 |
| Overdrive | 1800–1899 | 1900–1999 | 2000–2099 |
| Singularity | 2100–2199 | 2200–2299 | 2300–2399 |

Zenith: **2400+**, one rank without divisions. Unranked: five placements per
playlist. Players start at 1000 MMR; the placement badge is a neutral outlined ring.
Win/loss/draw and opponent strength determine rating. No bonuses for shots,
knockouts, switches or other personal stats. No changes to physics or matchmaking
authority. Promotion/demotion occurs immediately on crossing a boundary.

The existing Elo uses K56 during placements and K32 afterward: equal opponents
give ±28 / ±16 respectively. Ratings are floored at zero. A 1,000,000 sanity bound
protects storage/network input; this is not an attainable designed rank cap.
Remote deployment requirements and rounding are in `RankedBackendApi.md`.

## Artwork and UI

- Approved concept: `AssetDevelopment/UI/Ranks/concepts/rank-symbols-v1.png`.
- Transparent atlas: `AssetDevelopment/UI/Ranks/exports/RankBadges.png` (887×1774).
- Reproducible prompt: `AssetDevelopment/UI/Ranks/exports/RankBadges.prompt.md`.
- Cooked asset: `/Game/UI/Ranks/RankBadges`, imported by
  `Scripts/ImportRankBadges.py` using Unreal Python.
- All 22 symbols share one resident UI texture (~6 MB uncompressed). No per-frame
  image loads or render targets. `SFlickRankBadge` uses measured UV regions and
  aspect-fit rendering. It holds a strong reference to survive garbage collection.
- Main menu, competitive format cards, profile and accepted competitive results
  show rank symbols. Profile → Competitive Ranks shows the ladder and MMR ranges.
- The atlas is included by the existing `/Game/UI` always-cook rule.

Build the editor, run the import script, then run `Automation RunTests FLICK.Ranked`.
Capture `Profile` with `Tools/Capture-FlickUI.ps1 -ProfilePage 2` to inspect the
ladder in the actual game viewport. Live remote-service tests require deployment;
the local-development authority is not a production backend substitute.

`CompetitiveFormat` captures the public competitive cards. `PostMatchRanked`
exercises a Vector III -> Prism I promotion only when launched by the capture tool
with its isolated `Saved/UICaptures` UserDir. Normal previews and shipping builds
never apply this synthetic result.

## Development rank previews

Non-Shipping builds expose **Settings > Dev Ranks**. Enable an override or enter
an exact MMR and press Enter for Knockout 1v1, 2v2, 3v3 or BOB 1v1. Each row shows
its badge and range immediately. Menu rank labels identify overrides with `DEV`.
Use Real Rank disables a single override; Clear All removes all four.

Overrides persist locally in `[FLICK.Development.RankPreview]`, separately from
account/season ratings. Only menu display getters use them. Queue MMR, trusted
rank progress, placement counts and post-match settlement remain authoritative.
Previewing a rank displays completed placements without completing real ones.
Shipping builds neither expose the tab nor read the override configuration.
