# Nearby transmog sharing

`Sharing.lua` adds independent, opt-in broadcast and receive controls to Internet
Settings and an address/access-key setup dialog. The relay is a separate private
repository (`../sharing-relay` in this development workspace). Its OPERATIONS.md
specifies deployment, authentication, analytics and the little-endian protocol.
Do not include that server repository or its credentials/database in addon ZIPs.

The DLL exchanges 409-byte complete snapshots (417-byte publication / 429-byte
delivery). This includes body details, hide/inherit/replace equipment state,
independent carried weapons, stowed visibility, precision fits and five bags. Protocol version 6 also carries each bag's physics
toggle/amplitude and the weapon physics toggle; it requires the matching relay.
The game thread copies bounded data into a mailbox. Windows HTTP/WebSocket calls
run on dedicated threads, with certificate verification, no redirects or cookie
sharing, connection deadlines, capped input and reconnect backoff. Optional
WebSocket entry points are resolved dynamically, so unsupported Windows/Wine
implementations disable sharing without disabling the addon. WinHTTP WebSocket
support is documented at
https://learn.microsoft.com/windows/win32/api/winhttp/nf-winhttp-winhttpwebsocketcompleteupgrade.

Discovery uses build-5875's visible-object enumeration (0x468380), verified
against the actual executable and public VanillaHelpers declarations. No game
chat/addon channels, coordinates or global user list are transmitted. Server,
realm, character and game version are sent only in the authenticated handshake.
The server records connection-time analytics, disclosed before either checkbox
is enabled. Access keys belong to characters and are not bundled in a release.

Remote appearances are keyed by GUID and revalidated against the current game
object. Body changes reuse the model-name/compositor-copy hooks, without writing
unit identity fields. Armor substitutes private visible-item records only for
audited render callers (5FB551, 5ED8B8, 5EE673, 5EE802); inventory and tooltip
reads are untouched. Remote weapon/bag contexts are separate from local preview
contexts. Received fits never replace the user's global precision profiles.
Malformed body/item/model/fit data fails closed. Disabled receiving, departures,
logout and lost relay state restore native appearance. Appearance rebuilds are
limited to four players per update to avoid one crowded-area rebuild burst.

Tests: `sharing.lua`, `sharing_ui.lua`, `sharing_protocol.cpp`,
`sharing_appearance.cpp`, `sharing_weapon_renderer.cpp`, existing
`weapon_renderer.cpp`, and the relay's unit
and live loopback integration tests. Game ABI compatibility uses pinned client
signatures. These checks do not replace a two-client, in-game visual test of
race changes, loading equipment, combat, mounts, zoning and disconnect cleanup.

Readme and screenshot editorial content remains user-maintained.

For actual Windows transport coverage on Linux, build the sibling relay and run
`CLOSET_TOOLCHAIN=/path/to/llvm-mingw python3 tests/sharing_wss_wine.py`.
This tests authenticated WSS exchange, edit debounce and cancellation with two
WinHTTP clients under Wine. Certificates, credentials and Wine state are confined
to a temporary fixture directory. It requires free loopback ports 8787, 8788 and
19443, Wine and OpenSSL. No live game credentials are used.

Wire version 6 uses flag 4 for weapon physics. Bytes 85–87 carry shared Bounce, Rocking and Jump lift percentages (0–200, default 100). Bytes 364–403 carry ten slot overrides (inherit/off/on and three percentages); the quiver remains inherited. Bytes 404–408 carry cape customization enabled and walk/run/idle/airborne percentages. Race animation replacement remains removed.
