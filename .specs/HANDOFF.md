# Handoff

**Date:** 2026-03-27T19:16:00-03:00
**Feature:** panel-treeview
**Task:** quick follow-ups on treeview layout and panel split behavior - paused after validation

## Completed ✓

- Kept the tree host fixed on the left panel and aligned the central splitter double-click so visible list areas rebalance when the tree is open.
- Added sticky centered-split behavior so the balanced layout is preserved across resize and maximize.
- Finalized the `Ctrl+Shift+T` command path so tree hide/show reapplies the centered split calculation at the end of the toggle command.
- Updated project tracking docs in `.specs/project/ROADMAP.md` and `README.md` after each completed quick task.
- Validated the latest code with the official build flow: `src\\vcxproj\\build.cmd Debug Win32` -> `Build succeeded. 0 Warning(s), 0 Error(s)`.

## In Progress

- No code is actively in progress.
- Remaining validation is manual UX smoke in the built app.

## Pending

- Resume with the next feature or quick task requested by the user.
- If treeview color parity becomes important again, the next real step is owner-draw for the selected row because the stock `WC_TREEVIEW` still limits highlight fidelity.

## Blockers

- No current build blockers.
- Known non-blocker runtime issue remains: optional plugin dependencies such as `PVW32Cnv.dll` are still absent in dev builds.

## Context

- Branch: `main`
- Uncommitted: latest session touched `.specs/project/ROADMAP.md`, `README.md`, `src/fileswnd.h`, `src/fileswn2.cpp`, `src/mainwnd.h`, `src/mainwnd1.cpp`, `src/mainwnd3.cpp`, and `src/mainwnd4.cpp`.
- Related decisions:
  - Tree host stays on the left panel only.
  - Tree content mirrors the active panel.
  - Splitter double-click should balance visible file-pane widths, not raw panel widths, when the tree is visible.
  - Native `WC_TREEVIEW` highlight rendering was accepted as “good enough for now”; no full owner-draw implemented.
