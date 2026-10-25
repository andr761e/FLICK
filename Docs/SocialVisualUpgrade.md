# Social drawer visual update (2026-10-24)

The Social drawer follows the session/menu palette: refined near-black panels,
full cyan angular framing, paper headings and lime selected/action accents.

- Floating inset panel with a clearer FLICK NETWORK / SOCIAL heading.
- All / Online / Recent counts, filters and collapsible groups are retained.
- Selected tabs get a lime outline and restrained tinted fill.
- Friend, recent-player and party rows use the same dark material treatment.
- Existing avatars, presence pips, names, statuses and leader controls remain.
- Invite is a lime action; Refresh and destructive party actions share the new
  native-vector arrow treatment with keyboard/hover feedback.
- Empty lists have a framed, illustrated text state rather than a thin grey box.
- Service text no longer incorrectly appends ONLINE to local/offline mode.

This is native Slate styling only. No generated images, new binary assets,
networking, matchmaking or gameplay changes are required. Real invite, refresh,
promote, remove, leave and disband callbacks retain their authority/visibility
checks. Public queue controls remain hidden as before.

## Verification

- FLICKEditor Win64 Development compiled successfully.
- FLICK Win64 Development compiled successfully. No packaged archive was rebuilt.
- Actual game-viewport startup smoke passed, including Social open/close,
  All / Online / Recent selection and offline Refresh callbacks; Quit passed.
- Before/after captures at 1280x720 are saved in
  `Deliverables/Social-Visual-Upgrade-2026-10-24`.
- Visual/input checks use `-nosteam` and isolated user directories. They do not
  verify live Steam invitations or real online friend data.
