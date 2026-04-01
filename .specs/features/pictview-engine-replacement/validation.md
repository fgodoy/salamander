# PictView Engine Replacement Validation

**Status:** Manual smoke pending
**Primary test runtime:** `C:\Projetos\salamander\.localbuild\salamander\Release_x64\salamand.exe`

---

## Implemented MVP scope

### Open / decode

- Open-backed runtime no longer depends on `PVW32Cnv.dll` or `PVW32Cnv.lib` on the main plugin path
- Supported open formats through WIC: `BMP`, `GIF`, `ICO`, `JPEG`, `PNG`, `TIFF`, `DDS`
- Supported non-file image sources: attached `HBITMAP`, clipboard bitmap
- Decoded surface bridged into the historical `PVImageInfo` / `PVImageHandles` seam with `24bpp BGR` pixel access

### Viewer / transform

- Existing viewer render path works through the open backend seam
- Zoom, fit, stretch, and draw path preserved through `PVReadImage2` / `PVDrawImage`
- `Rotate left` / `Rotate right` supported through backend image mutation
- `Mirror horizontal`, `mirror vertical`, and `rotate 180` remain functional through the viewer's existing mirror-state path
- `Crop` supported through backend surface mutation
- GIF animated playback supported through `PVImageSequence`
- Multi-page image navigation remains available through `CurrentImage` / `NumOfImages`

### Metadata / EXIF

- WIC metadata bridge exposes width, height, frame count, DPI, and basic format flags
- JPEG/TIFF EXIF orientation is mapped into legacy `PVFF_*` flags
- Viewer autorotate can fall back to backend-provided orientation flags when `EXIF.DLL` does not provide orientation details

### Save / thumbnail integration

- `Save As` supported through WIC for `BMP`, `GIF`, `JPEG`, `PNG`, `TIFF`
- `RAW` callback output supported for existing preview / thumbnail flows
- WIC encoder pixel format negotiation implemented instead of assuming fixed `24bpp BGR`
- Thumbnail generation path is wired through the open backend and has already shown working panel thumbnails in manual exploratory testing

---

## Build and runtime validation completed

- `Debug|Win32` build of `src/plugins/pictview/vcxproj/pictview.vcxproj`
- `Debug|x64` build of `src/plugins/pictview/vcxproj/pictview.vcxproj`
- `Release|Win32` build of local runtime tree in `.localbuild`
- `Release|x64` build of local runtime tree in `.localbuild`
- Boot smoke for `Release_x86`: `salamand.exe` remained alive for at least 5 seconds and `salmon.exe` was running
- Boot smoke for `Release_x64`: `salamand.exe` remained alive for at least 5 seconds and `salmon.exe` was running

---

## Residual unsupported or intentionally out-of-scope coverage

- Historical long-tail proprietary PictView open formats outside WIC MVP coverage are still unsupported in the open backend path
- Historical save targets outside `BMP` / `GIF` / `JPEG` / `PNG` / `TIFF` are intentionally not exposed by the open backend
- GIF uses composed `PVImageSequence`; TIFF currently relies on page navigation rather than a TIFF-specific composed sequence object
- Full feature closeout still depends on manual end-to-end smoke in the running app for open, zoom, rotate, crop, thumbnail, clipboard, and save flows on MVP formats

---

## Manual smoke checklist to close the feature

- Open `JPEG`, `PNG`, `GIF`, `TIFF`, and `DDS` in `Release_x64`
- Confirm panel thumbnails appear and orientation-sensitive JPEG thumbnails match the viewer
- Confirm EXIF autorotate on rotated `JPEG` and `TIFF`
- Confirm `Rotate left`, `rotate right`, `mirror`, `rotate 180`, and `crop`
- Confirm `View bitmap from clipboard`
- Confirm `Save As` to `BMP`, `GIF`, `JPEG`, `PNG`, and `TIFF`, then reopen the saved files
