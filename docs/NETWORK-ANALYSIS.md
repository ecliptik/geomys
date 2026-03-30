# Network Traffic Analysis

Date: 2026-03-28
Dataset: 91 requests captured via TCP proxy (Geomys in Snow → local Gopher test server)

## Methodology

A transparent TCP proxy sat between Geomys (running in Snow on emulated Mac Plus) and the local Gopher test server. The proxy logged: selector, request bytes, response bytes, response chunk count, total connection lifetime, and time-to-first-byte (TTFB) for every request.

Coverage: root menus, text files, directories (small/medium/huge), downloads (1KB-200KB), images (GIF/PNG), Gopher+ attributes/bulk/ASK forms, search, CSO phonebook, stress tests (large text, deep nesting, long lines, mixed line endings).

## Raw Data Summary

| Metric | Value |
|--------|-------|
| Total requests | 91 |
| Unique selectors | ~35 |
| Median TTFB | 399ms |
| TTFB range | 326ms – 1013ms |
| Median connection lifetime | 46,305ms |
| Connection lifetime range | 940ms – 60,002ms |
| Total data received | ~1.1MB |
| Requests hitting 60s timeout | 8 (all >50KB responses) |

## Key Findings

### 1. CRITICAL: Directory connections leak for ~46 seconds

**The single biggest issue.** Every directory, search, and CSO response holds its TCP connection open for ~46 seconds after the response is fully received.

**Root cause** (`gopher.c:732`): When the `.\r\n` terminator is detected, `gs->receiving` is set to `false`. This causes `gopher_idle()` to bail at line 314 on every subsequent call, so `conn_idle()` is never called again. The connection sits in RECEIVING state — nobody polls it, nobody closes it. It only gets cleaned up when the user navigates to a new page (`gopher_navigate` → `conn_close` at line 144).

**Text/download/image responses are NOT affected** — they rely on the server closing the connection (FIN), which `conn_idle()` detects via TIME_WAIT and cleans up properly.

**Impact:**
- Each idle connection holds a MacTCP stream (max 10). Browsing 10+ directories without the old connections closing causes stream exhaustion and "could not connect" errors.
- The connection's 8KB receive buffer and 4KB read buffer stay allocated (~12KB wasted per leaked connection).
- On a 1MB Mac Plus, 5 leaked connections waste 60KB — 6% of total RAM.

**Evidence from traffic data:**
```
Small directory (320B response):  total=45,911ms  ttfb=375ms
Root menu (1,774B response):      total=46,638ms  ttfb=397ms
Gopher+ dir (966B response):      total=46,142ms  ttfb=393ms
```
All ~46 seconds despite data arriving in <400ms. The 46 seconds corresponds to the time until the user happened to navigate next, triggering cleanup.

**Fix:** Close the connection immediately when `.\r\n` is detected:
```c
/* gopher.c, after line 732 */
if (gs->line_len == 1 && gs->line_buf[0] == '.') {
    gs->receiving = false;
    conn_close(&gs->conn);          /* <-- add this */
    gs->conn.state = CONN_STATE_DONE;
    return;
}
```
Same pattern needed for CSO end-of-response detection.

### 2. HIGH: TTFB of 326-1013ms for localhost connections

Every request takes 326-1013ms before the first response byte, even though the server and client are on the same machine. Breakdown:

| Phase | Estimated time |
|-------|---------------|
| DNS lookup (cache hit after first) | 0-5ms |
| MacTCP → Snow DaynaPORT → host TCP | ~50-100ms |
| Server processing | <1ms |
| Response → DaynaPORT → MacTCP | ~250-350ms |

The ~350ms baseline is Snow's emulated network stack overhead. This is not fixable in Geomys — it's inherent to the emulation. But DNS prefetching (see #5) can hide the DNS component for subsequent requests.

**Notable outliers:**
- CSO phonebook query: 1,013ms TTFB (the highest) — CSO uses a multi-line request/response protocol, adding an extra round-trip.
- Root menu after heavy load: 820-932ms — likely MacTCP processing backlog from leaked connections.

### 3. HIGH: Large transfers (>50KB) never complete cleanly

All responses over 50KB hit the proxy's 60-second timeout:

```
/stress/text/huge     (102KB, 13 chunks):  60,002ms
/stress/dir/huge      (91KB, 12 chunks):   60,000ms
/stress/download/large.bin (205KB, 26 chunks): 60,000ms
/stress/text/large    (51KB, 7 chunks):    49,196ms (borderline)
```

This is the same connection leak as #1 — the connection never closes, so the proxy relay thread times out. The data itself arrives quickly (TTFB 350-600ms, all chunks within a few seconds). For the 51KB text, the 49-second lifetime is connection idle time, not transfer time.

For downloads/images (which rely on server FIN), the issue is different: Snow's emulated MacTCP may be slow to propagate the FIN through the DaynaPORT stack.

### 4. MEDIUM: Binary downloads have higher TTFB than text

| Type | Median TTFB |
|------|------------|
| Text files | 370ms |
| Directories | 390ms |
| Downloads | 560ms |
| Images | 520ms |
| Gopher+ | 370ms |

Downloads and images consistently show 150-200ms higher TTFB than text. Possible causes:
- Download path allocates file on disk (FSWrite setup) before starting to read
- Image path waits to fill the 26-byte sniff buffer before processing

### 5. MEDIUM: No DNS prefetching

Every navigation requires a fresh DNS lookup (or cache hit). The DNS cache holds 8 entries with LRU eviction. During this capture, all requests went to the same server so the cache was always warm. In real-world browsing across multiple servers, the 5-second DNS timeout per UDP attempt would be noticeable.

**Opportunity:** When a directory response is fully parsed, all item hostnames are known. Resolving unique hostnames in the background while the user reads the menu would eliminate DNS latency for the next click.

### 6. LOW: Responses arrive in few large chunks

The server sends responses in ≤32KB segments. Geomys reads 4KB per `conn_idle()` call. For a 102KB response:
- Server sends: 3-4 segments (32KB each)
- Geomys reads: 26 iterations (4KB each)
- At 60Hz event loop: ~433ms minimum to drain the buffer

Doubling `TCP_READ_BUFSIZ` to 8KB would halve the iteration count and reduce the drain time to ~217ms. For small responses (<4KB), this makes no difference.

### 7. LOW: Some requests sent 0-byte selectors

8 requests logged `req=0B` — the proxy captured no request data:
```
[19] /image/small.gif    req=0B
[23] /stress/text/huge   req=0B
[25] /stress/dir/huge    req=0B
...
```

These all still received valid responses, so the selector WAS sent — the proxy's relay thread just didn't capture the first chunk before the response arrived. This is a proxy artifact, not a Geomys bug.

## Improvement Plan (prioritized)

### P0 — Fix immediately

| # | Issue | File | Change |
|---|-------|------|--------|
| 1 | Close connection after `.\r\n` terminator | gopher.c:732 | Add `conn_close()` + set CONN_STATE_DONE after `receiving = false` |
| 2 | Same for CSO end-of-response | gopher.c (CSO path) | Same pattern |

### P1 — Fix before next release

| # | Issue | File | Change |
|---|-------|------|--------|
| 3 | Directory parsing byte-at-a-time | gopher.c:718-745 | Use `memchr()` like text mode |
| 4 | Gopher+ sync blocking | gopherplus.c:585-649 | Convert to async polling |
| 5 | Read buffer too small | connection.h:25 | `TCP_READ_BUFSIZ` 4096 → 8192 |

### P2 — Improve later

| # | Issue | File | Change |
|---|-------|------|--------|
| 6 | DNS prefetch on directory load | connection.c | Background resolve after directory parse |
| 7 | DNS cache too small | connection.c:37 | `DNS_CACHE_SIZE` 8 → 16 |
| 8 | Text buffer 32KB cap | gopher.h:16 | Stream to disk past threshold |
| 9 | Initial items array conservative | gopher.h:11 | `GOPHER_INIT_ITEMS` 128 → 256 |
| 10 | Line buffer truncation | gopher.c:89 | 512 → 1024 bytes |
