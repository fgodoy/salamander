# Open Salamander

Open Salamander is a fast and reliable two-panel file manager for Windows.

## Origin

The original version of Servant Salamander was developed by Petr Šolín during his studies at the Czech Technical University. He released it as freeware in 1997. After graduation, Petr Šolín founded the company [Altap](https://www.altap.cz/) in cooperation with Jan Ryšavý. In 2001 they released the first shareware version of the program. In 2007 a new version was renamed to Altap Salamander 2.5. Many other programmers and translators [contributed](AUTHORS) to the project. In 2019, Altap was acquired by [Fine](https://www.finesoftware.eu/). After this acquisition, Altap Salamander 4.0 was released as freeware. In 2023, the project was open sourced under the GPLv2 license as Open Salamander 5.0.

The name Servant Salamander came about when Petr Šolín and his friend Pavel Schreib were brainstorming name for this project. At that time, the well-known file managers were the aging Norton Commander and the rising Windows Commander. They questioned why a file manager should be named Commander, which implied that it commanded instead of served. This thought led to the birth of the name Servant Salamander.

Please bear with us as Salamander was our first major project where we learned to program in C++. From a technology standpoint, it does not use [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), smart pointers, [RAII](https://en.cppreference.com/w/cpp/language/raii), [STL](https://github.com/microsoft/STL), or [WIL](https://github.com/microsoft/wil), all of which were just beginning to evolve during the time Salamander was created. Many of the comments are written in Czech, but this is manageable due to recent progress in AI-powered translation. Salamander is a pure WinAPI application and does not use any frameworks, such as MFC.

We would like to thank [Fine company](https://www.finesoftware.eu/) for making the open sourced Salamander release possible.

## Development

### Prerequisites
- Windows 11 or newer
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)
- [Desktop development with C++](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170) workload installed in VS2022
- [Windows 11 (10.0.26100.4654) SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/) optional component installed in VS2022

### Optional requirements
- [Git](https://git-scm.com/downloads)
- [PowerShell 7.4](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-windows) or newer
- [HTMLHelp Workshop 1.3](https://learn.microsoft.com/en-us/answers/questions/265752/htmlhelp-workshop-download-for-chm-compiler-instal)
- Set the ```OPENSAL_BUILD_DIR``` environment variable to specify the build directory. Make sure the path has a trailing backslah, e.q. ```D:\Build\OpenSal\```

### Building

Solution ```\src\vcxproj\salamand.sln``` may be built from within Visual Studio or from the command-line using ```\src\vcxproj\rebuild.cmd``` or ```\src\vcxproj\build.cmd```.

Use ```\src\vcxproj\!populate_build_dir.cmd``` to populate build directory with files required to run Open Salamander.

### Project Scripts

- ```\src\vcxproj\build.cmd``` builds a selected solution configuration from the command line and resolves modern Visual Studio / MSBuild locations automatically
- ```\src\vcxproj\rebuild.cmd``` rebuilds the selected solution configuration from the command line
- ```\src\vcxproj\!populate_build_dir.cmd``` copies runtime files required to launch Open Salamander from the build output
- ```\normalize.ps1``` documents the repository normalization and formatting workflow

Keep this list updated whenever project-facing scripts are added or existing script behavior changes.

### Project Timeline

- 2026-03-27: Panel TreeView feature completed with menu/config toggle, fixed left-side host, active-panel synchronization, persisted width, and successful ```Debug|Win32``` solution build validation
- 2026-03-27: Double click on the central splitter was adjusted so, with the tree open, the visible file panes rebalance using the tree-reserved width and end up with matching list area widths
- 2026-03-27: The centered splitter mode was made sticky across resize, maximize, and treeview show/hide transitions so balanced visible file-pane widths are preserved after reflow
- 2026-03-27: The treeview toggle command path was finalized so `Ctrl+Shift+T` reapplies the centered split calculation at the end of the command, preventing residual misalignment after hide/show
- 2026-03-27: UI modernization study started to evaluate incremental refresh, shell refactor, hybrid surfaces, and full rewrite paths
- 2026-03-27: TreeView rich file-icon feature specified to support recognizable folder/file icons and file-location behavior from the tree
- 2026-03-27: TreeView rich file-icon design completed with shell-image-list reuse, mixed dir/file node model, and file-focus integration via ```WM_USER_FOCUSFILE```
- 2026-03-27: TreeView rich file-icon tasks prepared with an implementation order centered on typed node payloads, shell icons, mixed population, and file-focus behavior
- 2026-03-27: TreeView rich file-icon implementation completed with typed node payloads, shell small icons, mixed folder/file population, file-focus routing, and successful ```Debug|Win32``` build validation
- 2026-03-27: TreeView color sync implemented so background, text, and selection colors follow the user-defined panel color scheme, with successful ```Debug|Win32``` build validation
- 2026-03-27: TreeView selection color mapping was corrected so unfocused selection uses the panel `SELECTED` colors and focused selection uses `FOCSEL`, with successful ```Debug|Win32``` build validation
- 2026-03-27: TreeView selection rendering was corrected again so it follows the main-window active state rather than the tree HWND focus, matching the active panel semantics, with successful ```Debug|Win32``` build validation
- 2026-03-27: TreeView visual styles were disabled for the panel-hosted tree so Windows theming stops overriding the custom panel color mapping, with successful ```Debug|Win32``` build validation
- 2026-03-27: TreeView selection background was aligned to the same effective panel brushes used in the file panels, preserving the configured highlight color such as ```8080ff```, with successful ```Debug|Win32``` build validation
- 2026-03-27: TreeView selected-item default painting was neutralized so the control stops repainting the classic blue selection over the custom panel highlight, with successful ```Debug|Win32``` build validation
- 2026-03-28: Portuguese localization feature specified to add distinct `pt-BR` and `pt-PT` support on top of the existing `.slt` / `.slg` translation workflow and current `LANGID`-based locale preference behavior
- 2026-03-28: Portuguese localization design completed with a reuse-first approach centered on variant-specific `LANGID` metadata, locale-coded `pt-br` / `pt-pt` asset lines, main-app-first rollout, and minimal expected runtime code changes
- 2026-03-28: Portuguese localization tasks prepared with a phased rollout centered on main-app Portuguese assets first, runtime locale-selection validation, optional selector hardening only if needed, and follow-up plugin and packaging extension slices
- 2026-04-01: PictView engine replacement feature was specified and designed around a WIC-first backend that preserves the current engine seam, restores an open build path, and leaves room for optional long-tail format fallback
- 2026-04-01: PictView engine replacement tasks were prepared with phased work for backend seam creation, open-build bootstrap, WIC decode/render/save support, EXIF-aware behavior, thumbnails, clipboard flow, and Win32/x64 validation
- 2026-04-01: PictView open-build foundation was implemented so the plugin now boots through a local open backend, no longer requires `PVW32Cnv.dll`/`PVW32Cnv.lib` on the main path, and builds successfully in `Debug|Win32` and `Debug|x64`
- 2026-04-01: The first WIC-backed PictView slice now opens supported static image files through WIC, bridges decoded pixels into a `24bpp BGR` surface compatible with `PVImageInfo`/`PVImageHandles`, supports attached `HBITMAP` and clipboard bitmap sources, and renders through the existing viewer seam
- 2026-04-01: The open PictView backend gained an initial transform/save pipeline with in-memory crop support, WIC-backed file save for BMP/JPEG/PNG/TIFF, and `RAW` callback output for preview and thumbnail generation paths, with successful `Debug|Win32` and `Debug|x64` plugin builds
- 2026-04-01: The open PictView backend gained initial animated GIF support through `PVImageSequence`, composing frames from WIC decoder metadata into a viewer-consumable sequence while keeping single-frame formats on the simpler path
- 2026-04-01: The WIC save pipeline now negotiates encoder-supported pixel formats instead of assuming fixed `24bpp BGR`, which enables internal conversion for `GIF` save in addition to BMP/JPEG/PNG/TIFF, with successful `Debug|Win32` and `Debug|x64` plugin builds
- 2026-04-01: JPEG/TIFF EXIF orientation is now bridged from WIC metadata into the historical `PVFF_*` rotation/mirror flags, and the viewer can fall back to those flags for autorotate when `EXIF.DLL` does not provide orientation details, with successful `Debug|Win32` and `Debug|x64` plugin builds
- 2026-04-01: A local `Debug_x86` runtime tree was prepared under `.localbuild\` with `salamand.exe`, `english.slg`, `pictview.spl`, and `exif.dll`, and a controlled boot smoke confirmed that the app stays alive long enough for manual testing
- 2026-04-01: Local `Release_x86` and `Release_x64` runtime trees were prepared under `.localbuild\` with `salamand.exe`, `salmon.exe`, `pictview.spl`, `checkver.spl`, `demoplug.spl`, and the required language/resources for practical manual smoke on both architectures
- 2026-04-01: The open PictView backend decode path now reuses already-loaded current-frame metadata instead of reloading it immediately before the first decode, trimming redundant work on the common open/view flow

### Contributing

This project welcomes contributions to build and enhance Open Salamander!

## Repository Content

```
\convert         Conversion tables for the Convert command
\doc             Documentation
\help            User manual source files
\src             Open Salamander core source code
\src\common      Shared libraries
\src\common\dep  Shared third-party libraries
\src\lang        English resources
\src\plugins     Plugins source code
\src\reglib      Access to Windows Registry files
\src\res         Image resources
\src\salmon      Crash detecting and reporting
\src\salopen     Open files helper
\src\salspawn    Process spawning helper
\src\setup       Installer and uinstaller
\src\sfx7zip     Self-extractor based on 7-Zip
\src\shellext    Shell extension DLL
\src\translator  Translate Salamander UI to other languages
\src\tserver     Trace Server to display info and error messages
\src\vcxproj     Visual Studio project files
\tools           Minor utilities
\translations    Translations into other languages
```

A few Altap Salamander 4.0 plugins are either not included or cannot be compiled. For instance, the PictView engine ```pvw32cnv.dll``` is not open-sourced, so we should consider switching to [WIC](https://learn.microsoft.com/en-us/windows/win32/wic/-wic-about-windows-imaging-codec) or another library. The Encrypt plugin is incompatible with modern SSD disks and has been deprecated. The UnRAR plugin lacks [unrar.dll](https://www.rarlab.com/rar_add.htm), and the FTP plugin is missing [OpenSSL](https://www.openssl.org/) libraries. Both issues are solvable as both projects are open source. To build WinSCP plugin you need Embarcadero C++ Builder.

All the source code uses UTF-8-BOM encoding and is formatted with ```clang-format```. Refer to the ```\normalize.ps1``` script for more information.

## Resources

- [Altap Salamander Website](https://www.altap.cz/)
- Altap Salamander 4.0 [features](https://www.altap.cz/salamander/features/)
- Altap Salamander 4.0 [documentation](https://www.altap.cz/salamander/help/)
- Servant Salamander and Altap Salamander [changelogs](https://www.altap.cz/salamander/changelogs/)
- [User Community Forum](https://forum.altap.cz/)
- Altap Salamander on [Wikipedia](https://en.wikipedia.org/wiki/Altap_Salamander)

## License

Open Salamander is open source software licensed [GPLv2](doc/license_gpl.txt) and later.
Individual [files and libraries](doc/third_party.txt) have a different, but compatible license.
