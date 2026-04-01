# PictView Engine Replacement Specification

## Problem Statement

The current PictView plugin depends on `PVW32Cnv.dll` / `PVW32Cnv.lib`, which are not open-sourced and therefore block a fully open, reproducible build of the plugin. The goal is to replace that dependency with an open implementation that restores a functional image viewer, thumbnailer, and converter for the formats users actually rely on.

## Goals

- [ ] Ship an open-source image backend that allows `pictview` to build and run without the proprietary `PVW32Cnv` binaries.
- [ ] Restore the core PictView workflow for common formats: open, view, zoom, rotate, crop, thumbnail, clipboard paste/copy, and save-as.
- [ ] Preserve x64 support without requiring the current 32-bit envelope process for the missing proprietary DLL.
- [ ] Keep the design extensible so long-tail format support can be added later without another rewrite.

## Out of Scope

- Full feature parity with every legacy PictView format in the first delivery.
- Rewriting scanner integration, EXIF UI, printing UI, or the viewer shell from scratch.
- Preserving undocumented behavior of the proprietary engine where Salamander does not actually depend on it.
- Adding brand-new image-editing features beyond what the plugin already exposes.

---

## User Stories

### P1: Open and view mainstream images without proprietary binaries ⭐ MVP

**User Story**: As an Open Salamander user, I want PictView to open and display common image files without `PVW32Cnv.dll` so that the plugin is usable in an open build.

**Why P1**: This is the smallest vertical slice that makes the plugin functional again.

**Acceptance Criteria**:

1. WHEN the user opens a BMP, GIF, ICO, JPEG, PNG, TIFF, or DDS image THEN the plugin SHALL decode and display it without loading `PVW32Cnv.dll`.
2. WHEN the image format contains multiple frames supported by the backend THEN the plugin SHALL expose frame access needed by the current viewer and thumbnail flows.
3. WHEN the plugin is built for x64 THEN it SHALL run without relying on the current `salpvenv.exe` bridge to a 32-bit proprietary DLL.

**Independent Test**: Build `pictview`, open sample BMP/JPEG/PNG/TIFF/GIF files, and verify the viewer paints them and can switch frames where applicable.

---

### P1: Restore core viewing and thumbnail workflows

**User Story**: As a user browsing images, I want zoom, rotate, crop, thumbnails, and clipboard image loading to keep working so that PictView remains practical for everyday use.

**Why P1**: The plugin is not meaningfully useful if it only decodes but cannot support its existing core viewer interactions.

**Acceptance Criteria**:

1. WHEN the user zooms or resizes the viewer THEN the backend SHALL provide scaled output suitable for the current rendering pipeline.
2. WHEN the user rotates, flips, or crops an image THEN the backend SHALL update the displayed image and derived thumbnail output accordingly.
3. WHEN the user requests thumbnails or pastes an image from the clipboard THEN the backend SHALL create an image instance from that source without proprietary code.

**Independent Test**: Open an image, zoom, rotate, crop, paste a bitmap from the clipboard, and verify panel thumbnails are generated.

---

### P1: Preserve metadata-driven behaviors where feasible

**User Story**: As a user viewing photos, I want EXIF-aware behavior such as autorotate and metadata access to remain available so that photo viewing does not regress.

**Why P1**: Photo workflows are a primary use case for the plugin and the repo already contains separate EXIF code worth preserving.

**Acceptance Criteria**:

1. WHEN a JPEG or TIFF contains orientation metadata THEN the viewer SHALL preserve the current autorotate behavior.
2. WHEN metadata is available through the new backend or the existing EXIF module THEN the plugin SHALL keep the current EXIF dialog behavior for supported files.
3. WHEN metadata is unavailable for a format THEN the plugin SHALL fail gracefully without breaking image display.

**Independent Test**: Open an EXIF-rotated JPEG and confirm the displayed orientation matches expectations and the EXIF dialog still works.

---

### P2: Save-as for the common open-source backend formats

**User Story**: As a user converting images, I want save-as to continue working for the most common target formats so that basic conversion workflows survive the engine replacement.

**Why P2**: Viewing is the MVP; conversion support matters next but does not need full legacy parity immediately.

**Acceptance Criteria**:

1. WHEN the user invokes save-as on a mainstream image THEN the plugin SHALL save to every format implemented by the new backend and exposed in the UI.
2. WHEN a legacy save target is not supported by the new backend THEN the UI SHALL not advertise it as available.
3. WHEN save-as completes THEN the current progress/cancel flow SHALL continue to work.

**Independent Test**: Save a JPEG as BMP, PNG, and TIFF, then reopen the results in PictView.

---

### P3: Recover long-tail legacy format coverage

**User Story**: As a power user with older or niche image assets, I want additional format support beyond WIC defaults so that PictView approaches its historical breadth over time.

**Why P3**: Valuable, but not required to make the plugin functional again.

**Acceptance Criteria**:

1. WHEN a format is not supported by the primary backend THEN the architecture SHALL allow an additional backend or codec package to be added without redesigning the viewer.
2. WHEN optional fallback support is installed THEN the plugin SHALL route unsupported formats to that fallback backend.

---

## Edge Cases

- WHEN an image uses an unsupported legacy format THEN the plugin SHALL show a clear unsupported-format error instead of a DLL-load failure.
- WHEN a file has multiple frames, an embedded thumbnail, or EXIF orientation THEN the backend SHALL expose that metadata to the existing viewer logic where supported.
- WHEN a file is very large THEN the backend SHALL support cancellation and thumbnail limits consistent with current plugin behavior.
- WHEN the clipboard contains a bitmap THEN the plugin SHALL create an in-memory image object without requiring a temporary proprietary handle format.
- WHEN a file uses CMYK or another non-sRGB colorspace THEN the backend SHALL either convert it predictably for display or reject it explicitly.

---

## Success Criteria

How we know the feature is successful:

- [ ] `pictview` builds in an open workspace without needing proprietary `PVW32Cnv` artifacts.
- [ ] Core viewer and thumbnail workflows work for the mainstream formats covered by the chosen backend.
- [ ] x64 no longer depends on the 32-bit envelope for image decoding.
- [ ] The implementation leaves a clear extension point for long-tail formats beyond the MVP.
