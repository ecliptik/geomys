# TODO

## Bugs

- [x] External URL dialog dismissal leaves unredrawn content gap.
      Root-caused during the review-cleanup-v3 sweep: not reproducible on
      current `main`. `dismiss_modal` already queues a full-window
      `InvalRect` and `handle_update` does a full repaint; the original
      repro was against the pre-fix `feature/internet-config` branch
      (fixed by 7e5c165 `browser_note_alert`). The same investigation did
      find and fix a real latent bug in the same path — `std_dlg_filter`
      returned a dialog result through an uninitialized `item` — now
      resolved on `fix/review-cleanup-v3`. Retest in Snow to confirm and
      close.

## Enhancements

- [ ] Helper-app lookup by creator code (desktop database), between
      the IC-first path and the current filename hunt.

      **Why.** The current fallback in `do_html_url_dialog` and
      `do_telnet_dialog` calls `FSMakeFSSpec(0, 0, "\pNetscape", ...)`
      etc., which only matches an exact file name in the boot volume's
      current directory. An app installed as `Netscape Navigator™
      2.02` in `Applications:Netscape 2.0.2:` is invisible even though
      Netscape is obviously present. Creator-code lookup via the
      File Manager's desktop database finds the app wherever it lives
      on any mounted volume.

      **Scope.** A separate branch off `main` (not off
      `feature/internet-config`). Keep the IC-first path and the
      NoteAlert untouched — this slots in between them.

      **Approach.**

      1. New module `src/appfind.c` + `src/appfind.h` with a single
         entry point:
         ```c
         Boolean appfind_by_creator(OSType creator, FSSpec *out);
         ```
         Iterates all mounted volumes, calls `PBDTGetPath` to open
         each desktop database, then `PBDTGetAPPL` with the requested
         creator. Returns the first match.

      2. Use `DTPBRec` (in `Files.h` / multiversal). Sequence per
         volume: `PBHGetVInfoSync` to enumerate vRefNums, then
         `PBDTGetPath` (selector `_GetAPPL`'s parent trap), then
         `PBDTGetAPPL` with `dtPB.ioFileCreator = creator` and
         `dtPB.ioIndex = 0`. On `noErr`, `FSMakeFSSpec(vRefNum,
         ioAPPLParID, ioNamePtr, out)`. Desktop database APIs are
         System 7+ only — gate with the existing Gestalt version
         check pattern.

      3. Patch call sites in `src/main.c`:

         **HTML URL** (`do_html_url_dialog`, currently
         src/main.c:2441-2465) — try a short creator-code list
         before the filename hunt:
         ```
         'MOSS'  Netscape Navigator
         'MWBR'  MacWeb
         'ICAB'  iCab
         'Msft'  Mosaic / early IE (subject to verification)
         ```
         First match wins.

         **Telnet** (`do_telnet_dialog`, currently
         src/main.c:2603-2624) — same pattern with:
         ```
         'FLYN'  Flynn (sibling project)
         'NCSA'  NCSA Telnet
         'brwt'  Better Telnet
         'tn3d'  tn3270
         ```

      4. On no-match, fall through to the existing filename hunt
         (leave as belt-and-braces for old System 6 desktop databases
         that don't support `PBDTGetPath`), then the NoteAlert.

      **Scale.** ~120 lines for appfind.c, ~5 lines per call site,
      no build-flag gating (this is strictly an improvement to the
      existing name-hunt fallback, not a new feature surface). No
      preset changes, no SIZE clamp changes.

      **Verification.**

      - Basilisk II System 7.6.1 with Netscape 2.02 installed in
        `Applications:` (not at boot root). With GEOMYS_IC compiled
        out or IC not installed, clicking a `URL:http://…` item
        should now launch Netscape instead of firing the "no helper"
        alert.
      - Install a second browser (MacWeb or iCab), uninstall
        Netscape, confirm the fallback finds the alternate.
      - Remove all browsers, confirm the alert still fires.
      - System 6 regression: no change (desktop database API is
        System 7+, the call no-ops and falls through to the filename
        hunt).

      **Out of scope.** Doesn't touch IC, doesn't touch
      `ic_launch_url`, doesn't alter the NoteAlert wording.
