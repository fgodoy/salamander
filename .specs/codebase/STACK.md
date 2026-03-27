# Tech Stack

**Analyzed:** 2026-03-26

## Core

- Framework: aplicação desktop Windows nativa, sem framework UI externo; base própria sobre WinAPI (`src/precomp.h`, `src/common/winlib.h`)
- Language: C++ com partes em C; projeto principal compila com `stdcpplatest`, mas o estilo do código é legado/pre-STL (`src/vcxproj/salamand.vcxproj`)
- Runtime: Win32/Win64, `Subsystem=Windows`, alvo Windows SDK 10.0 e toolset MSVC `v143`
- Package manager: não há package manager para o core C++; utilitários Python usam `uv` em `tools/`

## Frontend

- UI Framework: WinAPI puro com wrappers internos como `CWindow`, `CMainWindow`, `CFilesWindow`
- Styling: recursos Win32 (`.rc`, `.rh`, `.ico`, `.bmp`, `.svg`)
- State Management: estado global e objetos singleton/globais no processo
- Form Handling: diálogos Win32 e classes próprias de janela/dialog

## Backend

- API Style: não aplicável; trata-se de aplicação desktop monolítica
- Database: SQLite embarcado no repositório (`src/vcxproj/sqlite/sqlite.vcxproj`, `src/common/dep/sqlite`)
- Authentication: não há camada central de auth; plugins específicos usam APIs/sistemas externos quando necessário

## Plugin/Extension Model

- Plugin SDK: headers `spl_*` em `src/plugins/shared/`
- Plugin binary format: `.spl` (`src/plugins/shared/vcxproj/plugin_base.props`)
- Localization modules: projetos `lang_*` por plugin e base `lang.vcxproj`
- Shell integration: shell extension x86/x64 em `src/vcxproj/shellext/`

## Testing

- Unit: não há framework de testes unitários dedicado no repositório
- Integration: validação principal via build MSBuild e alguns utilitários/testadores isolados
- E2E: não há suite E2E automatizada
- CI: GitHub Actions compila `Debug|Win32` e `Debug|x64` com warnings como erro (`.github/workflows/pr-msbuild.yml`)

## External Services

- OS APIs: WinAPI, COM/OLE, Shell, Registry, HTML Help, Winsock, WinINet
- Crash reporting: `DbgHelp` + minidumps no módulo `salmon`
- Networking: WinINet no plugin `checkver`, Winsock no `salmon`, FTP/OpenSSL em plugin dedicado
- Localization: recursos `.slt`, `.slg` e ferramenta `translator`

## Third-Party Libraries

- 7-Zip
- zlib
- bzip2
- libexif
- CHMLIB
- SQLite
- OpenSSL
- LibTomCrypt
- Nano SVG
- cmark-gfm
- WIL

## Development Tools

- Build system: Visual Studio 2022 + MSBuild
- Formatting: `.clang-format` + `.editorconfig`
- Scripting: batch/cmd, PowerShell, Python 3.13+ com `uv`
- CI: GitHub Actions
