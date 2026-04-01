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

## Test setup

### Runtime

- Launch `C:\Projetos\salamander\.localbuild\salamander\Release_x64\salamand.exe`
- Use this tree for the full smoke pass; it already contains `salmon.exe`, `pictview.spl`, `checkver.spl`, `demoplug.spl`, `exif.dll`, and the required language files

### Suggested sample set

- One normal `JPEG`
- One `JPEG` with EXIF rotation
- One `PNG`
- One animated `GIF`
- One multi-page `TIFF`
- One `DDS`
- One simple bitmap source for clipboard test

If you do not have all of these, start with `JPEG`, `PNG`, `GIF`, and `TIFF`.

### How to open an image in the viewer

Use one of these flows:

1. Navigate to the image in a panel and open it with the usual viewer action.
2. Or use `Ctrl+O` inside the viewer for `File/Open...`.

Expected result:
- the image opens without an error dialog
- the viewer paints the image content correctly
- the app remains responsive

Failure signals:
- blank viewer
- corrupted rendering
- unexpected error dialog
- crash or hang

---

## Quick action map

Use this section when you just need the command without reading the full script.

- `Open image:` open from the file panel normally, or press `Ctrl+O` in the viewer for `File/Open...`
- `Save As:` press `Ctrl+S` or use `File/Save As...`
- `Show EXIF:` press `E` or use `File/Show EXIF...`
- `Rotate Left:` press `L` or use `Edit/Rotate Left`
- `Rotate Right:` press `R` or use `Edit/Rotate Right`
- `Rotate 180:` press `T` or use `Edit/Rotate 180`
- `Flip Horizontal:` press `H` or use `Edit/Flip Horizontal`
- `Flip Vertical:` press `V` or use `Edit/Flip Vertical`
- `Crop:` select a region with the mouse, then press `C` or use `Edit/Crop`
- `Go To Page:` press `G`
- `Previous page:` press `Ctrl+Page Up` or `Ctrl+Left`
- `Next page:` press `Ctrl+Page Down` or `Ctrl+Right`
- `Fit in window:` press `F5`
- `Fit width:` press `F6`
- `Actual size:` press `F7`
- `Full screen:` press `F11`
- `Histogram:` press `Ctrl+H`
- `Pipette:` press `P`
- `View bitmap from clipboard:` use `Plugins/PictView/View Bitmap from Clipboard` or `Ctrl+Shift+B`

---

## Step-by-step smoke script

### 1. Basic open and render

For each of `JPEG`, `PNG`, `GIF`, `TIFF`, and `DDS`:

1. Open the file in the viewer.
2. Confirm the image is drawn.
3. If the file has multiple frames/pages:
   - use `Ctrl+Page Down` or `Ctrl+Right` for next page
   - use `Ctrl+Page Up` or `Ctrl+Left` for previous page
   - use `G` for `Go To Page`
4. Confirm navigation changes the displayed page/frame when applicable.

Expected result:
- no startup dependency error
- no “unsupported page” error for normal test files
- GIF animates if it is animated
- TIFF page navigation works if the file is multipage

### 2. Zoom and fit behavior

With a `JPEG` or `PNG` open:

1. Press `Num +` to zoom in.
2. Press `Num -` to zoom out.
3. Press `F5` for fit in window.
4. Press `F6` for fit width.
5. Press `F7` for actual size.
6. Optionally press `F11` to switch to full screen and repeat one zoom action.

Expected result:
- image repaints correctly after each zoom mode change
- no black or stale areas remain on screen
- full-screen and normal mode both render correctly

### 3. Rotate and mirror

With a non-animated image open:

1. Press `L` for `Edit/Rotate Left`.
2. Verify the image rotates 90 degrees counterclockwise.
3. Press `R` for `Edit/Rotate Right`.
4. Verify the image returns to the previous orientation.
5. Press `T` for `Edit/Rotate 180`.
6. Verify the image flips upside down.
7. Press `H` for `Edit/Flip Horizontal`.
8. Verify left and right are mirrored.
9. Press `V` for `Edit/Flip Vertical`.
10. Verify top and bottom are mirrored.

Expected result:
- each command updates the display immediately
- no crash, redraw corruption, or status desync
- subsequent save still works after transforms

### 4. Crop

With a non-animated image open:

1. Enter selection mode if needed by pressing `S`.
2. Drag with the mouse over a visible rectangular region you want to keep.
3. Press `C` or use `Edit/Crop`.
4. Confirm the viewer now shows only the selected region.

Expected result:
- the image dimensions visibly shrink to the selected area
- no error dialog appears for a valid selection
- after crop, zoom and save still work

Failure signals:
- crop does nothing
- crop uses the wrong rectangle
- image becomes corrupted after crop

### 5. EXIF autorotate and EXIF dialog

With a rotated `JPEG` or `TIFF` that contains orientation metadata:

1. Open the file.
2. Do not rotate it manually.
3. Confirm it displays in the correct human orientation on first load.
3. Press `E` or use `File/Show EXIF...`.
4. Confirm the EXIF dialog opens.

Expected result:
- portrait photos do not open sideways when autorotate is enabled
- EXIF dialog opens without error for supported files

Failure signals:
- image opens sideways even though the source has EXIF orientation
- EXIF dialog crashes or fails to open

### 6. Panel thumbnails

In the file panel, browse to a folder with several images:

1. Switch the panel to a thumbnail-capable view if needed.
2. Wait for thumbs to populate.
3. Compare at least one rotated `JPEG` thumbnail against the viewer result.
4. Open one of the files from the panel.

Expected result:
- thumbnails appear for supported formats
- rotated-photo thumbnails are not obviously inconsistent with the viewer
- opening from the thumbnail view still works

Failure signals:
- thumbnails stay blank
- obviously wrong orientation for rotated JPEGs
- major slowdown beyond initial decode cost

### 7. Clipboard image open

Prepare any bitmap image in the clipboard:

1. Open an image in another app and copy it, or use the viewer's own copy flow on a selected image area with `Ctrl+C`.
2. In Salamander, use `Plugins/PictView/View Bitmap from Clipboard`.
3. Shortcut alternative: `Ctrl+Shift+B`.
4. Confirm the clipboard bitmap opens in the viewer.

Expected result:
- the image from the clipboard opens as a normal viewer image
- the app remains stable

### 8. Save As

With a static image open:

1. Press `Ctrl+S` or use `File/Save As...`.
2. In the dialog, type a new output name so you do not overwrite the source.
3. Choose one target format, starting with `BMP`.
4. Click `Save`.
5. Reopen the saved file and confirm it renders.
6. Repeat for `GIF`, `JPEG`, `PNG`, and `TIFF`.
7. If you previously rotated or cropped the image, repeat at least one save after the transform.

Expected result:
- save completes without an engine error
- each saved output can be reopened
- transformed output preserves the visible transform

Failure signals:
- save dialog offers a format but saving fails
- reopened file is corrupt
- transformed file reopens in the wrong orientation or crop state

### 9. Histogram and pixel access

With a `JPEG` or `PNG` open:

1. Press `Ctrl+H` to open histogram.
2. Confirm the histogram window is populated.
3. Press `P` to activate pipette mode if needed.
4. Move the cursor over several visible pixels.
5. Confirm the status bar updates with pixel information.

Expected result:
- histogram is non-empty
- pipette reads pixel data without error

### 10. Multipage verification

For a multipage `TIFF`:

1. Open the file.
2. Press `Ctrl+Page Down` to advance pages.
3. Press `Ctrl+Page Up` to go back.
4. Press `G` and jump directly to a page.

Expected result:
- page number changes
- the rendered content changes with the page
- no stale frame from the previous page remains

### 11. Animated GIF verification

For an animated `GIF`:

1. Open the file.
2. Watch the image for several seconds.
3. Confirm frames advance automatically.

Expected result:
- animation runs
- no obvious composition artifacts between frames
- no crash when closing or switching away

---

## Result summary template

Use this compact format when reporting results back:

- `Open/render:` pass or fail, with file types tested
- `Zoom/fullscreen:` pass or fail
- `Rotate/mirror/crop:` pass or fail
- `EXIF:` pass or fail
- `Thumbnails:` pass or fail
- `Clipboard:` pass or fail
- `Save As:` pass or fail, with formats tested
- `TIFF pages:` pass or fail
- `GIF animation:` pass or fail
- `Performance notes:` short note if any path still feels slow

---

## Manual smoke checklist to close the feature

Use this as the compact run sheet during testing.

- Open `JPEG`, `PNG`, `GIF`, `TIFF`, and `DDS` in `Release_x64`.
  How: open from the panel or use `Ctrl+O`. For `TIFF`, test `Ctrl+Page Up`, `Ctrl+Page Down`, and `G`.
- Confirm panel thumbnails appear and orientation-sensitive `JPEG` thumbnails match the viewer.
  How: browse a folder in thumbnail view, then compare a rotated-photo thumbnail against the same file opened in the viewer.
- Confirm EXIF autorotate on rotated `JPEG` and `TIFF`.
  How: open a file with orientation metadata and verify it is already upright before any manual rotate; then press `E` for `File/Show EXIF...`.
- Confirm `Rotate left`, `rotate right`, `mirror`, `rotate 180`, and `crop`.
  How: use `L`, `R`, `H`, `V`, `T`, and `C`. For crop, first drag a selection rectangle with the mouse.
- Confirm `View bitmap from clipboard`.
  How: copy a bitmap in any app, then use `Plugins/PictView/View Bitmap from Clipboard` or `Ctrl+Shift+B`.
- Confirm `Save As` to `BMP`, `GIF`, `JPEG`, `PNG`, and `TIFF`, then reopen the saved files.
  How: press `Ctrl+S`, choose a new filename and one of those target formats, save, then reopen the output file.

---

## Feature closeout rule

The feature can be marked complete when:

- the runtime `Release_x64` smoke passes on the main MVP flows above
- any discovered regressions are fixed
- no blocker remains for normal `JPEG` / `PNG` / `GIF` / `TIFF` / `DDS` viewing and save workflows in the open backend path
