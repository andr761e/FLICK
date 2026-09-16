# Private match hosting

Private online matches run on the party leader's existing Steam listen server.
Opening or starting a private match does not request a coordinator allocation.
Standalone private play remains available without Steam.

- The local listen-server host must also be party leader to open, configure,
  or launch an online private match. Remote clients cannot launch it via RPC.
- Players choose their own available team slots or spectate. Occupied slots
  cannot be stolen. All playing slots must be filled and ready before launch.
- Changing rules or assignments clears readiness.
- Private matches never enable matchmaking or ranked settlement flags.
- Returning from a match keeps the existing party host and connection.
- Client disconnects use the existing private reconnect grace period.
- Host loss ends the current match: there is no live match host migration.
  Existing party restoration may recreate the party under the next stable slot;
  it does not resume the abandoned match.

Casual and Competitive queues retain the coordinator/dedicated-server path.

## Two-computer verification

1. Launch matching packaged builds under different Steam accounts.
2. Leader invites the friend and opens Private Match.
3. Confirm both see the same settings; only the host can change or launch them.
4. Assign playing slots and ready up. Launch and complete a match.
5. Return together and repeat without creating another party.
6. Disconnect a client and test reconnect within the grace period.
7. Close the host: confirm the match ends and party/menu recovery is offered.
8. Verify private results do not change MMR, and public queueing still searches
   through the coordinator rather than hosting a public listen-server match.
