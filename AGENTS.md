# AGENTS.md

FLICK is a small physics-first competitive game.

Permanent engineering principles:

- Core gameplay code is C++ unless there is a strong reason otherwise.
- Avoid essential Level Blueprint logic.
- Avoid unnecessary binary asset dependencies.
- Avoid third-party dependencies unless explicitly approved.
- Keep gameplay parameters tuneable.
- Do not prematurely implement future systems.
- Preserve physics predictability.
- New mechanics should interact with the existing physics rather than bypass it.
- Code must be understandable by one solo developer.
- Inspect current architecture before adding new systems.
- Build/test after meaningful changes whenever environment permits.
- Never claim successful compilation if compilation was not actually run.
- Never hide compiler errors or test failures.
- Prefer small coherent systems over giant manager classes.

Eventually planned features:

- Special pieces
- Piece upgrades
- Drafting
- Multiple arenas
- Local multiplayer
- Online multiplayer
- Ranked multiplayer
- Cosmetics

These are not current scope unless explicitly requested.
