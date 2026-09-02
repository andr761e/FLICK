# FLICK Single-Host Online Deployment

This deployment runs one coordinator and its allocated Windows dedicated-server processes on the same public host. It is suitable for private internet playtests and the next round of online validation. It is not the final multi-region Steam release architecture.

## Build

1. Build Unreal Engine 5.6 from Epic's source distribution with Win64 server support.
2. Set `FLICK_UNREAL_ENGINE_ROOT` to that engine root.
3. Run `package-flick-online-stack.cmd`.
4. Transfer the complete `Builds\OnlineStack` directory to a Windows host with the .NET 8 runtime.

## Configure

Set the variables listed in `production.env.example` through the host or service manager. Use the actual `FLICKServer.exe` path from the transferred archive. `FLICK_STEAM_WEB_API_KEY` must be a publisher Web API key kept in the host's secret store; it must never enter Unreal config, a client package, logs, or source control. `FLICK_STEAM_TICKET_IDENTITY` must match Unreal's `SteamTicketAudience` setting (`FLICK` by default).

The coordinator's internal listener can remain HTTP (`ASPNETCORE_URLS=http://0.0.0.0:8090`) behind a reverse proxy, but `FLICK_COORDINATOR_PUBLIC_URL` must be its externally reachable HTTPS URL. Forward the configured UDP game-port range to this machine and set `FLICK_SERVER_PUBLIC_HOST` to the hostname clients can reach.

## Run And Observe

Start `run-flick-coordinator-production.cmd`. Monitor:

- `/health/live` for process liveness.
- `/health/ready` for configuration and allocator readiness.
- structured process logs for allocation, first heartbeat, timeout, crash, and completion events.

The coordinator does not return a server address to clients until that server sends its first heartbeat. It kills and fails allocations after startup timeout, heartbeat loss, process exit, or credential expiry. Every allocated process receives its own random workload credential.

## Release Boundary

Before a public Steam launch, replace the single-host JSON rating store and in-memory allocation state with a transactional database, add multi-node allocation locking, regional capacity providers, metrics/alerts, log aggregation, rate limiting, backup/restore, DDoS-aware ingress, and rolling deployment. Those require a chosen hosting provider and operational credentials and are intentionally outside this repository-only implementation.
