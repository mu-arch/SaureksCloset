# Update checking (3.4.33)

Settings contains Internet Settings, Links and Version Details navigation. Internet Settings holds the account-wide
`VanityStudioDB.autoCheckUpdates` checkbox. Unset preferences default to true;
false survives reloads/logins. A check is queued three seconds after initialization.
Turning checking off clears the queue, discards the current request generation
and disables the manual check button. Local addon/DLL compatibility checks are
independent of the network preference and always run.

`Updates.lua` declares the exact required renderer integer. The running bridge's
`SaureksClosetRendererVersion()` is compared with it; no version is inferred from
a DLL on disk or cached addon metadata. Missing/different DLLs and newer published
versions generate deduplicated chat alerts and the main title:
`Saurek's Closet (Update Available)`. Installing a DLL requires a full WoW restart.

The bridge fetches this fixed public URL:
`https://raw.githubusercontent.com/mu-arch/SaureksCloset/main/update-version.txt`

The plain-text format is strictly parsed and limited to 1024 bytes:

```
schema=1
addon=3.4.34
dll=30433
```

If and only if the manifest returns HTTP 404, older releases are supported by
reading the exact `## Version:` line in `addon/SaureksCloset/SaureksCloset.toc`
and the literal `version()` return in `native/SaureksCloset.cpp` on the same
branch. Each legacy file is capped at 64 KiB. These are declarations, never code
to execute. A malformed, partial or failed response is reported as an unavailable
check, not an up-to-date result. During development the local version can be
newer than the public repository; that is not an update alert.

`tools/package.py` validates the addon/required-DLL/source-DLL version tuple and
generates `update-version.txt` for both archives. Publish the manifest together
with that release's code, not ahead of the corresponding release. The fallback
allows the checker to operate against the existing repository before the new
manifest is published. The feature does not publish files, download releases,
install updates or execute remote content.

A single worker owns all synchronous WinHTTP handles. Lua API calls set flags,
start a worker or poll numeric results; they never perform HTTP work. Requests
use HTTPS/TLS 1.2 with normal certificate verification, no redirects/cookies/
authentication, finite per-phase timeouts, a bounded read deadline and a one-minute
start cooldown. Cancellation uses a generation number, so no game thread closes
a synchronous HTTP handle while its worker is using it. An already-issued network
operation can finish or time out on the worker after cancellation; its result is
discarded and no further fetch is started for that generation. The UI also has
a 30-second wait limit. The worker never reads or mutates game models or calls Lua.

Website buttons open only three compiled, allowlisted HTTPS URLs: the project,
its releases page, and the existing support Discord invite. Opening a site requires
a button click; it is never triggered by an update response.

Reference: Microsoft WinHTTP concurrency rules:
https://learn.microsoft.com/en-us/windows/win32/winhttp/concurrency-in-winhttp

Validation includes strict parsing/cancelled generation tests, mocked Lua default-on
and persisted-off tests, mismatch/missing DLL alerts, duplicate suppression, errors,
timeout and UI navigation. The Windows bridge is cross-compiled; these tests do
not claim a live WoW or Windows/Wine HTTPS/browser-launch test.
