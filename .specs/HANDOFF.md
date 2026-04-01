# Handoff

**Date:** 2026-04-01T19:18:00-03:00
**Feature:** `pictview-engine-replacement`
**Task:** `T14` - implementation complete in repo, paused awaiting manual smoke and feature closeout

## Completed ✓

- Replaced the main open-build PictView dependency on `PVW32Cnv.dll` / `PVW32Cnv.lib` with a local WIC-backed backend seam
- Implemented open/decode/render/save support for MVP formats in `src/plugins/pictview/openimagebackend.cpp`
- Added GIF sequence support, EXIF orientation bridge, clipboard open, crop, save-as, and pixel-access compatibility
- Built and boot-smoked local `Release_x86` and `Release_x64` runtime trees in `.localbuild`
- Expanded `.specs/features/pictview-engine-replacement/validation.md` into a step-by-step operational smoke script
- Updated repository conventions so manual validation docs must be executable scripts instead of ambiguous checklists

## In Progress

- Manual feature smoke for PictView on `Release_x64`
- Specific location: `.specs/features/pictview-engine-replacement/validation.md`

## Pending

- Run the `validation.md` smoke script on `C:\Projetos\salamander\.localbuild\salamander\Release_x64\salamand.exe`
- If all checks pass, mark the feature complete in `.specs/features/pictview-engine-replacement/tasks.md`, `.specs/project/ROADMAP.md`, and `.specs/project/STATE.md`
- If smoke finds issues, fix the concrete regressions and re-run targeted validation

## Blockers

- Manual smoke has not yet been executed end-to-end by the user, so the feature cannot be declared complete - impacts final closeout only

## Context

- Branch: current branch not explicitly checked this session
- Uncommitted:
  - `.specs/codebase/CONVENTIONS.md`
  - `.specs/features/pictview-engine-replacement/validation.md`
  - `.specs/project/ROADMAP.md`
  - `.specs/project/STATE.md`
  - `README.md`
- Related decisions:
  - `AD-001` in `.specs/project/STATE.md`: always suggest commit text at task close
  - `AD-002` in `.specs/project/STATE.md`: validation docs must be operational scripts
