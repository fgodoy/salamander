# State

**Last Updated:** 2026-04-01T18:58:00-03:00
**Current Work:** PictView WIC backend implementation with bootable local `Release_x86` and `Release_x64` app trees in `.localbuild`; next work is manual feature smoke inside the running app, thumbnail/performance verification, and residual TIFF/page-flow polish

---

## Recent Decisions (Last 60 days)

### AD-001: Always suggest commit text at the end of a task (2026-04-01)

**Decision:** Always include a suggested commit message at the end of a completed task.
**Reason:** The user explicitly asked for this behavior to be persistent across future tasks.
**Trade-off:** Final responses may become slightly longer even for small changes.
**Impact:** Task close-outs should include a concise proposed commit message unless the user asks otherwise.

---

## Active Blockers

None recorded.

---

## Lessons Learned

### LL-001: PictView WIC integration must undefine the historical `INT32` / `UINT32` macros locally before including `wincodec.h`

The plugin precompiled header defines compatibility macros for `INT32` and `UINT32`, which collide with modern Windows SDK declarations pulled by WIC headers.

### LL-002: `PVSaveImage` is the shared seam for Save As, print preview, and thumbnail generation

Restoring `PVSaveImage` even for a limited static-image slice unlocks multiple existing flows at once because the plugin routes file save, raw preview buffers, and thumbnail export through the same contract.

### LL-003: WIC encoder support must be treated as negotiated output, not a fixed `24bpp BGR` contract

`IWICBitmapFrameEncode::SetPixelFormat` may return a different format than requested. The open backend now has to convert the in-memory surface to the encoder-accepted format before writing, which is what makes `GIF` save viable through the same WIC save seam.

### LL-004: Reusing historical `PVFF_*` orientation flags is the lowest-risk bridge into viewer and thumbnail flows

The legacy viewer and thumbnail pipeline already understand `PVFF_ROTATE90`, `PVFF_FLIP_HOR`, and `PVFF_BOTTOMTOTOP`. Projecting WIC EXIF orientation metadata into those existing flags avoids a wider refactor and lets autorotate degrade gracefully even when `EXIF.DLL` cannot provide orientation details.

### LL-005: A testable local app tree can be produced without the interactive populate script by building targeted projects against a workspace-local `OPENSAL_BUILD_DIR`

For quick plugin validation, building `salamand.vcxproj` and the affected plugin project into a local `.localbuild\` tree is enough to get a runnable Win32 app shell for smoke testing, even before a full runtime population flow is automated.

### LL-006: The first decode path should reuse already-loaded frame metadata when the caller has just opened the image or requested current-frame info

The WIC backend originally reloaded the same frame metadata immediately before decode, even when `PVOpenImageEx` or `PVGetImageInfo` had already populated the active frame. Reusing that state avoids redundant decoder setup on the common first-view path and is a safe micro-optimization because frame identity is already tracked in `ImageInfo.CurrentImage`.

---

## Preferences

**Model Guidance Shown:** never
**Commit Message Preference:** Always suggest commit text at the end of a task.
