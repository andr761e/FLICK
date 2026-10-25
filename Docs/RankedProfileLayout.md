# Competitive ranks: single-screen layout

The Profile competitive-ranks page no longer uses a scroll container. Tighter
badge/section spacing fits all four playlist ranks and all eight rank tiers
(22 division badges) inside the existing panel. A bounded-width, down-only
scale box keeps wrapped text predictable and prevents clipping in smaller views.

All MMR ranges, placement progress, explanatory copy and Profile Menu navigation
remain. Ranking rules, ratings and saved data are unchanged.

Verified in actual viewport captures at 1280x720 and 1920x1080:
`Deliverables/Ranked-Profile-Fit-2026-10-24`.
The real UI smoke test checks the 26 total badges, retained footer, lack of a
rank-content scroll container and navigation back to the overview. It passed.
Editor compilation passed; this is a layout-only change.
