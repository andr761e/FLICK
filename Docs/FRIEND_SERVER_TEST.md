# PC-hosted Casual or Competitive 1v1 test

This is a Development-build test of the existing matchmaking coordinator and
headless Unreal server. It uses direct IP networking with Steam disabled. Use a
private LAN or VPN; the coordinator uses HTTP and development credentials.

1. Install Unreal Engine 5.8 and .NET on the host PC. Connect both PCs to the
   same LAN or private VPN. Find the host PC's IPv4 address on that network.
2. From the project root, run `package-flick-development.cmd` once to create
   the Development client.
3. On the host PC, run `start-flick-friend-server.cmd HOST_IP` and leave its
   console open. For example: `start-flick-friend-server.cmd 100.101.102.103`.
   The command builds and starts the coordinator, which launches one headless
   Unreal server per allocated match. It also writes `friend-server.txt` into
   `Builds\Development\Windows` so `FLICK.exe` knows where to search.
   Zip the **entire Windows folder after starting the server** and send it to
   your friend. A new VPN address requires a new `friend-server.txt`.
4. Allow inbound TCP 8090 and UDP 7780 on the host PC's firewall for the VPN
   or LAN network. Each additional simultaneous match uses the next UDP port.
   The friend must be able to reach `http://HOST_IP:8090/health/ready`.
5. Both players double-click `FLICK.exe` inside their Windows
   package. In the game, choose Casual or Competitive, select 1v1 and the same
   arena/mode variant, then press Search. The game reads `friend-server.txt`
   at startup, supplies a stable ID for each PC/user, and selects direct-IP
   networking for this Development test. The
   coordinator allocates a server after both players queue.

Keep the host console open until the test ends. Close it with Ctrl+C to stop
the coordinator and its child match servers. For a second test, launch the
clients again. Ratings and match history are stored under `Saved\Coordinator`.
`join-flick-friend-test.cmd HOST_IP PLAYER_ID` remains available to join the
Casual 1v1 queue automatically for a smoke test.

If a client cannot reach the coordinator, check the VPN address, TCP 8090, and
the Windows firewall. If both queue but cannot enter the allocated match,
check UDP 7780 and the host's `Saved\Logs` Unreal logs. Do not use a Shipping
client: this Development test deliberately uses HTTP and no Steam tickets.

After the initial package has been shared, the routine is simply: start the
server command, open FLICK.exe on both PCs, and search in the same playlist.
The server command remembers the last address: `start-flick-friend-server.cmd`
with no arguments starts it again using that address.
There is no client command to run. Keep friend-server.txt beside FLICK.exe.
If the host address changes, send your friend the updated text file before
they open the game; no rebuild is required. Removing that file restores the
package's usual network configuration on the next launch.
