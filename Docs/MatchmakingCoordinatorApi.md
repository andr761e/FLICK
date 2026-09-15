# FLICK Matchmaking Coordinator Contract

The coordinator sits between a queued Steam party and an authoritative game server. The client never chooses its team, slot, server, match ID, or ranked opponent. Steam lobby matchmaking remains available only as a development fallback.

## Queue A Party

`POST /v1/matchmaking/tickets`

```json
{
  "request_id": "retry-safe-client-generated-id",
  "build_id": "network-compatible-build-id",
  "party_id": "persistent-party-guid",
  "region": "eu-north",
  "variant": 0,
  "players_per_team": 2,
  "ranked": true,
  "members": [
    {
      "account_id": "76561198000000001",
      "display_name": "Player",
      "steam_ticket": "web-api-ticket",
      "party_slot": 0,
      "rating_snapshot": 1120
    }
  ]
}
```

`request_id` makes retries idempotent: resubmitting the same request returns the original ticket, while reusing it for different parameters is rejected. `build_id` prevents network-incompatible clients from sharing a match.

Production services validate every Steam ticket and load canonical ratings. `rating_snapshot` is useful for diagnostics only and is replaced by the coordinator's stored playlist rating before matching. Parties are indivisible and may never be split across teams. Party strength is weighted toward its strongest member, and similarly shaped parties are preferred.

The response contains a `ticket_id`. Poll `GET /v1/matchmaking/tickets/{ticket_id}` until its status is `allocated`; `allocating` means the process exists but has not sent its first heartbeat. Search responses include elapsed seconds and the current rating tolerance. Ranked starts at approximately ±50 MMR and widens every 12 seconds; casual starts wider and expands faster. By default a valid search remains active until cancelled. Cancel with `DELETE` on the same path. Cancelling an allocating party retires that incomplete allocation and returns unaffected parties to search.

An allocation contains a backend-generated match ID, server address, expiry, and one opaque reservation per member of the requesting party. Each reservation fixes that account to one team and player slot. Only deliver a party's own reservations to that party.

## Server Admission

The client connects with its match ID, account ID, and opaque reservation token. The game server calls `POST /v1/matchmaking/reservations/verify` using its server credential. Gameplay and ranked authentication remain disabled until the coordinator returns the canonical account, team, and slot.

Tokens must be random, short-lived, bound to one match and account, stored hashed in production, and safe for repeated verification during the reconnect grace period.

## Server Lifecycle

- Internal allocation state follows `allocated → waiting_for_players → ready_check → playing → completing → result_accepted`.
- `POST /v1/servers/heartbeat` keeps an allocation healthy.
- `POST /v1/servers/matches/{match_id}/started` is accepted only after every reserved account has been verified.
- `POST /v1/servers/matches/{match_id}/complete` marks normal or forfeited completion.
- Missing heartbeats cause the allocator to quarantine or replace the server.
- A healthy server is retired if its complete reserved roster does not arrive before the configured join timeout.
- Completion eventually recycles the process after clients have viewed the post-match screen.

All server endpoints require `Authorization: Bearer <workload credential>` and `X-Flick-Server-Id`. The coordinator creates a random credential for each allocation, stores only its SHA-256 digest, injects the raw value into that server process, and expires it after the configured lifetime. The shared development key is accepted only outside `Production` and can be disabled with `FLICK_ALLOW_DEVELOPMENT_SERVER_KEY=false`.

`GET /health/live` proves the coordinator process is running. `GET /health/ready` verifies that its launch and security configuration is usable. Allocation supervision fails and cleans up a match when its process exits, startup misses its deadline, heartbeats stop, or its workload credential expires.

## Local Emulator

`Tools/FlickCoordinator` implements this contract with .NET 8 standard libraries. It forms exact teams without splitting parties, applies an initial MMR-gap policy, launches either `UnrealEditor-Cmd.exe -server` or a packaged server, validates Steam WebAPI tickets when required, verifies reservations, persists ratings, records ranked history, supervises server health, and retires completed processes.

The included deployment profile is a single Windows game-server host. It does not provide autoscaling, regional routing, DDoS protection, a durable multi-node database, or distributed allocation locking. Those belong in the hosting platform before a public release.
