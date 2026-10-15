# FLICK Ranked Backend Contract

This contract is consumed only by an authoritative FLICK game server. The backend owns Steam identity validation, seasons, MMR, match history, abandonment policy, and idempotency. Request ratings are diagnostic snapshots; the backend must load canonical ratings from its own database.

## Authentication

`POST /v1/auth/steam`

```json
{
  "claimed_account_id": "76561198000000000",
  "steam_ticket": "hex-encoded-web-api-ticket",
  "steam_ticket_audience": "FLICK",
  "season_id": "SEASON_1",
  "playlist": "CLASSIC_2V2"
}
```

```json
{
  "authenticated": true,
  "account_id": "76561198000000000",
  "progress": {
    "rating": 1124,
    "matches_played": 19,
    "wins": 11,
    "losses": 7,
    "draws": 1
  }
}
```

The service must validate the ticket with Steam, verify its App ID and audience, and require its Steam ID to equal `claimed_account_id`. A ticket is never accepted solely because its account ID matches the network connection.

## Match Registration

`POST /v1/ranked/matches`

```json
{
  "match_id": "a-server-generated-guid",
  "server_id": "eu-north-01-instance-42",
  "season_id": "SEASON_1",
  "playlist": "CLASSIC_2V2",
  "variant": 0,
  "players_per_team": 2,
  "started_unix_time": 1787677200,
  "participants": [
    {
      "account_id": "76561198000000001",
      "team": 1,
      "player_slot": 0,
      "authenticated_rating": 1100
    }
  ]
}
```

```json
{
  "accepted": true,
  "match_id": "a-server-generated-guid"
}
```

Registration must reject duplicate accounts, incomplete teams, duplicate team slots, inactive seasons, invalid playlists, accounts not authenticated for this server, and parties outside the configured MMR policy. Repeating an identical registration with the same idempotency key may return the original successful response.

## Match Settlement

`POST /v1/ranked/matches/{match_id}/result`

The request repeats the immutable registration fields and adds:

```json
{
  "outcome": 1,
  "forfeit": false,
  "completed_unix_time": 1787677620
}
```

```json
{
  "accepted": true,
  "duplicate": false,
  "match_id": "a-server-generated-guid",
  "updates": [
    {
      "account_id": "76561198000000001",
      "old_rating": 1100,
      "new_rating": 1116,
      "matches_played": 20,
      "old_tier": 3,
      "new_tier": 3,
      "old_division": 3,
      "new_division": 3
    }
  ]
}
```

Settlement must be one database transaction: lock the registered match, return the stored response when it is already settled, update every participant once, record history, and commit the final response. A server retry uses `X-Idempotency-Key: {match_id}:result`.

## Rank ladder and rating policy

Canonical implementation: `Source/FLICK/Core/FlickRankRules.cpp`. See
`Docs/CompetitiveRanks.md` for all 22 badges and thresholds. Deploy the same policy
in the hosted service before enabling competitive matchmaking; this repository
contains the Unreal authority adapter and local-development service, not a hosted
HTTP service implementation.

- New playlist rating: 1000. Complete five matches per playlist to receive a rank.
- Each season/variant/team size has an independent rating. Preserve existing MMR
  and placement counts when updating the ladder; do not reset accounts.
- Expected score: `1 / (1 + 10^((opponentTeamMean - playerRating) / 400))`.
- Actual score: win 1, draw 0.5, loss 0. K = 56 before the fifth completed match,
  otherwise 32. Round `K * (actual - expected)` with Unreal's floor(x + 0.5)
  convention. Equal established opponents give +16/-16; placements give +28/-28.
- Clamp final rating to 0..1,000,000 (transport/storage safety bound, not a ladder
  tier). The former 3000 ceiling is removed. Stats never influence MMR. A forfeit
  is settled as a loss, not an additional fabricated rank penalty.
- Tier wire values: Unranked=0, Carbon=1, Alloy=2, Tempered=3, Vector=4, Prism=5,
  Overdrive=6, Singularity=7, Zenith=8. Division is 1..3, or 0 for Unranked/Zenith.
- Tier and division are derived metadata, never authority over rating. Unreal
  derives these from the trusted old/new ratings and completed-match count so an
  obsolete service display label cannot award a wrong badge. The hosted service
  still owns rating calculation, identity and transactional settlement.

## Server Authentication

Every request includes:

```text
Authorization: Bearer <FLICK_BACKEND_SERVER_KEY>
X-Flick-Server-Id: <FLICK_SERVER_ID>
X-Idempotency-Key: <operation-key>
```

Use short-lived workload credentials or a managed secret in production. The environment-variable bearer token is the first deployment integration and must be rotated without rebuilding the game server. Never distribute it with a client build.

## Failure Policy

- Return `401` or `403` for invalid server credentials or Steam identity.
- Return `409` for a conflicting registration or an illegal lifecycle transition.
- Return `422` for an invalid roster, season, playlist, or outcome.
- Return `429` for throttling and `5xx` only for retryable service failures.
- Return a JSON `error` string for operator logs; do not expose secrets or Steam tickets.

The game server retries timeouts, `408`, `429`, and `5xx` responses with the same idempotency key. Durable reconciliation after the game-server process is lost belongs in the hosted service and match allocator.

