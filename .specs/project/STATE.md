# State

**Last Updated:** 2026-04-01T18:16:00-03:00
**Current Work:** PictView WIC backend implementation after animated GIF sequence support and GIF-capable WIC save-path negotiation, with manual smoke and EXIF-aware follow-up still pending

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

---

## Preferences

**Model Guidance Shown:** never
**Commit Message Preference:** Always suggest commit text at the end of a task.
