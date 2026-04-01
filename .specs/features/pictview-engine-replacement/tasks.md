# PictView Engine Replacement Tasks

**Design**: `.specs/features/pictview-engine-replacement/design.md`
**Status**: Draft

---

## Execution Plan

### Phase 1: Foundation (Sequential)

Tasks that establish the seam and the backend skeleton.

```text
T1 -> T2 -> T3
```

### Phase 2: Core Implementation (Parallel OK)

After the seam exists, decode, render, transform, and save work can be split.

```text
          /-> T4 [P] -> T8 [P] --\
T3 -> T5 -|-> T6 [P] -> T9 [P] ---|-> T12
          \-> T7 [P] -> T10 [P] -/

T3 -> T11 [P] --------------------/
```

### Phase 3: Integration and Validation (Sequential)

Bring the slices together and validate the supported workflow end to end.

```text
T12 -> T13 -> T14
```

---

## Task Breakdown

### T1: Create the open image engine contract

**What**: Introduce a local engine interface and shared handle/state types that mirror the current PV-style operations without depending on `PVW32Cnv`.
**Where**: `src/plugins/pictview/engine/`
**Depends on**: None
**Reuses**: `src/plugins/pictview/pictview.h`, `src/plugins/pictview/lib/PVW32DLL.h`

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Backend-facing interface types exist for open, decode, draw, transform, save, and metadata operations
- [ ] The new types isolate WIC/COM details from the rest of the plugin
- [ ] The contract is narrow enough to replace current `PVW32DLL` calls incrementally

**Verify**:

- Inspect headers for a single backend contract usable by viewer, thumbnail, and save flows

---

### T2: Add backend bootstrap for open builds

**What**: Replace the current proprietary load path with a local bootstrap that initializes the open backend for repository builds.
**Where**: `src/plugins/pictview/pictview.cpp`
**Depends on**: T1
**Reuses**: Existing `LoadPictViewDll()` initialization and error reporting patterns

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Open builds no longer require `LoadLibrary("PVW32Cnv.dll")` for the main path
- [ ] Existing startup error handling remains coherent
- [ ] The plugin initializes the new backend through the current seam

**Verify**:

- Build `src/plugins/pictview/vcxproj/pictview.vcxproj`
- Confirm no startup path still hard-fails due to missing `PVW32Cnv.dll`

---

### T3: Remove mandatory proprietary build dependency

**What**: Eliminate or guard the remaining hard build dependency on `PVW32Cnv.lib` and the envelope-only path for open builds.
**Where**: `src/plugins/pictview/vcxproj/salpvenv.vcxproj`, related pictview project files
**Depends on**: T2
**Reuses**: Existing project configuration layout in `src/plugins/pictview/vcxproj/`

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] The repository can build the PictView project without proprietary `.lib` artifacts
- [ ] x64 no longer depends on the 32-bit envelope for the open backend path
- [ ] Legacy envelope code is either bypassed or clearly isolated from the MVP build

**Verify**:

- Build `Debug|Win32`
- Build `Debug|x64`
- Confirm the linker does not require `PVW32Cnv.lib`

---

### T4: Implement WIC image open and metadata read [P]

**What**: Add WIC-backed file open, decoder creation, frame access, and metadata extraction for the MVP formats.
**Where**: `src/plugins/pictview/engine/wic/`
**Depends on**: T3
**Reuses**: Existing image-info expectations from `src/plugins/pictview/render1.cpp` and `src/plugins/pictview/thumbs.cpp`

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] The backend opens BMP, GIF, ICO, JPEG, PNG, TIFF, and DDS through WIC
- [ ] Width, height, frame count, and key flags are exposed through the new handle
- [ ] Metadata needed for autorotate and basic EXIF-aware behavior is accessible

**Verify**:

- Open sample files in each supported format
- Confirm metadata-dependent orientation can be read for JPEG or TIFF samples

---

### T5: Implement decoded image surface and DIB bridge

**What**: Introduce the decoded in-memory image surface that bridges WIC output to the plugin's GDI-oriented renderer.
**Where**: `src/plugins/pictview/engine/`
**Depends on**: T3
**Reuses**: GDI/DIB usage patterns from `src/common/dib.cpp`, `src/plugins/pictview/render1.cpp`

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] The backend can materialize decoded pixels in a renderer-compatible DIB-backed surface
- [ ] Surface state supports at least one decoded frame at a time
- [ ] Pixel ownership and cleanup are explicit and leak-safe within current project style

**Verify**:

- Decode a supported image and inspect that a DIB-backed buffer is produced
- Smoke-check repeated open/close on the same image without crashes

---

### T6: Implement draw and stretch path [P]

**What**: Replace `PVReadImage2`, `PVDrawImage`, and stretch behavior with backend-backed rendering that preserves current zoom and fit workflows.
**Where**: `src/plugins/pictview/render1.cpp`, `src/plugins/pictview/engine/`
**Depends on**: T3
**Reuses**: Existing viewer zoom, fit, and paint flow

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] The viewer can paint supported images through the new backend
- [ ] Stretch, fit, and 100% view modes still work
- [ ] Full-screen and normal viewer paths both render correctly

**Verify**:

- Manual smoke in normal window and full-screen
- Test zoom in, zoom out, fit to window, and fit width on JPEG and PNG

---

### T7: Implement frame sequence support [P]

**What**: Support multi-frame image access needed by GIF and TIFF viewing and thumbnail flows.
**Where**: `src/plugins/pictview/engine/wic/`, `src/plugins/pictview/render1.cpp`
**Depends on**: T4
**Reuses**: Existing sequence assumptions around `PVReadImageSequence`

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] The backend exposes frame count and current frame selection
- [ ] Animated GIF and multi-frame TIFF access works at least for viewer consumption
- [ ] Single-frame formats keep the simple fast path

**Verify**:

- Open an animated GIF
- Open a multi-page TIFF
- Confirm frame navigation or sequence consumption does not regress the viewer

---

### T8: Implement pixel access and histogram compatibility [P]

**What**: Restore the pixel-read path used by pipette and histogram logic on top of the new image surface.
**Where**: `src/plugins/pictview/PixelAccess.cpp`, `src/plugins/pictview/histwnd.cpp`, `src/plugins/pictview/engine/`
**Depends on**: T5
**Reuses**: Existing histogram and cursor-read behavior

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Per-pixel RGB lookup works on decoded surfaces
- [ ] Histogram generation works for supported RGB/grayscale images
- [ ] Unsupported colorspace cases fail predictably

**Verify**:

- Open an image and sample pixels with the pipette path
- Open histogram on JPEG or PNG and confirm non-empty output

---

### T9: Implement transform operations for viewer edits [P]

**What**: Restore rotate, flip, and crop operations using WIC transform pipeline or backend surface operations.
**Where**: `src/plugins/pictview/render1.cpp`, `src/plugins/pictview/engine/wic/`
**Depends on**: T6
**Reuses**: Existing command handlers and viewer state transitions

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Rotate left, rotate right, rotate 180, and flip operations update the displayed image
- [ ] Crop updates the active image state correctly
- [ ] Thumbnail and save paths can consume transformed state

**Verify**:

- Manual smoke for rotate and crop on JPEG and PNG
- Reopen or refresh after transform and confirm consistent display

---

### T10: Implement clipboard image loading [P]

**What**: Replace `PVLoadFromClipboard` behavior with a WIC/backend path for `CF_DIB` or `CF_BITMAP`.
**Where**: `src/plugins/pictview/render1.cpp`, `src/plugins/pictview/engine/`
**Depends on**: T5
**Reuses**: Existing clipboard command flow and save-path handling

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] A bitmap copied to the clipboard can be opened in PictView
- [ ] Clipboard-loaded images flow through the same viewer logic as file-backed images
- [ ] Saving a clipboard-loaded image still works through supported save formats

**Verify**:

- Copy an image to the clipboard
- Use the existing "View bitmap from clipboard" flow
- Save the result as PNG or BMP

---

### T11: Implement thumbnail generation [P]

**What**: Rework thumbnail decode and generation on top of the new backend for panel thumbnails and viewer-related thumbnail updates.
**Where**: `src/plugins/pictview/thumbs.cpp`, `src/plugins/pictview/Thumbnailer.cpp`, `src/plugins/pictview/engine/`
**Depends on**: T4
**Reuses**: Existing thumbnail size rules, megapixel limit behavior, and panel integration

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Supported image formats generate thumbnails through the open backend
- [ ] Existing "ignore embedded thumbnails" behavior remains implementable
- [ ] JPEG autorotate and orientation-sensitive thumbnails behave correctly

**Verify**:

- Generate panel thumbnails for JPEG, PNG, and GIF files
- Compare rotated-photo thumbnail orientation with viewer orientation

---

### T12: Implement save-as and capability mapping

**What**: Replace `PVIsOutCombSupported` and `PVSaveImage` usage with backend capability mapping and WIC encoders for the MVP formats.
**Where**: `src/plugins/pictview/saveas.cpp`, `src/plugins/pictview/engine/wic/`
**Depends on**: T4, T5, T7, T9, T11
**Reuses**: Existing save dialog, progress UI, and cancellation path

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Save-as exposes only formats the new backend can actually encode
- [ ] BMP, GIF, JPEG, PNG, and TIFF save paths work
- [ ] Unsupported historical targets are hidden or disabled rather than failing late

**Verify**:

- Save one opened image as BMP, PNG, JPEG, and TIFF
- Reopen each saved file successfully

---

### T13: Integrate EXIF-aware viewer behavior

**What**: Preserve autorotate and metadata-dependent viewer behavior while keeping the existing EXIF dialog flow stable for the MVP.
**Where**: `src/plugins/pictview/render1.cpp`, `src/plugins/pictview/dialogs.cpp`, backend metadata bridge
**Depends on**: T4, T6, T12
**Reuses**: Existing EXIF initialization and dialog behavior

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Autorotate still works for JPEG or TIFF files with orientation metadata
- [ ] The existing EXIF dialog keeps working for supported metadata cases
- [ ] Lack of metadata does not block image display

**Verify**:

- Open an EXIF-rotated sample image
- Confirm displayed orientation is corrected
- Open the EXIF dialog successfully

---

### T14: Validate open-build PictView MVP end to end

**What**: Run the practical build and smoke validation for the new backend and document residual gaps against historical PictView coverage.
**Where**: `src/plugins/pictview/`, `.specs/features/pictview-engine-replacement/`
**Depends on**: T13
**Reuses**: Current build scripts and manual plugin smoke workflow

**Tools**:

- MCP: NONE
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] `Debug|Win32` build passes for the affected solution or project
- [ ] `Debug|x64` build passes for the affected solution or project
- [ ] Manual smoke confirms open, zoom, rotate, crop, thumbnail, clipboard, and save-as on MVP formats
- [ ] Residual unsupported formats are documented explicitly

**Verify**:

- Run the relevant MSBuild or repo build script for `Debug|Win32`
- Run the relevant MSBuild or repo build script for `Debug|x64`
- Execute manual smoke checklist for BMP, GIF, ICO, JPEG, PNG, TIFF, and DDS

---

## Parallel Execution Map

```text
Phase 1 (Sequential):
  T1 -> T2 -> T3

Phase 2 (Parallel after T3):
  T4 [P]
  T5 [P]
  T6 [P]
  T11 [P]

Phase 2b:
  T5 -> T8 [P]
  T5 -> T10 [P]
  T4 -> T7 [P]
  T6 -> T9 [P]

Phase 3 (Sequential merge):
  T4, T5, T7, T9, T11 -> T12 -> T13 -> T14
```

---

## Task Granularity Check

| Task | Scope | Status |
| --- | --- | --- |
| T1: Create backend contract | 1 cohesive interface slice | ✅ Granular |
| T2: Add bootstrap path | 1 startup slice | ✅ Granular |
| T3: Remove proprietary build dependency | 1 build/config slice | ✅ Granular |
| T4: Implement WIC open and metadata | 1 backend decode slice | ✅ Granular |
| T12: Implement save-as and capability mapping | 1 save/conversion slice | ✅ Granular |

---

## Validation Baseline

This repository currently relies primarily on build validation and manual smoke testing rather than a unified automated test suite. For this feature, the default proof points should be:

- targeted build validation for `Debug|Win32`
- targeted build validation for `Debug|x64`
- manual plugin smoke tests for supported formats
- explicit documentation of unsupported historical formats
