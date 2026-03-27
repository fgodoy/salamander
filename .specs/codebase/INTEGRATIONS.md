# Integrations

## Windows Platform APIs

### WinAPI GUI / Shell

**Usage:** base da aplicação e de praticamente toda a UI.

**Evidence:**

- `src/precomp.h` inclui `windows.h`, `shlobj.h`, `commctrl.h`, `exdisp.h`
- `src/common/winlib.h` encapsula janelas/dialogs

### COM / OLE

**Usage:** inicialização do app e alguns plugins/componentes auxiliares.

**Evidence:**

- `src/salamdr1.cpp` faz inicialização COM
- `src/plugins/automation/scriptlist.cpp` usa `CoInitializeEx`
- `src/plugins/diskmap/DiskMapPlugin/DiskMapPlugin.cpp` usa `CoInitialize`

### Windows Registry

**Usage:** configuração da aplicação e de plugins.

**Evidence:**

- `src/common/handles.*` encapsula `RegOpenKeyEx`
- `src/plugins/shared/spl_base.h` expõe `CSalamanderRegistryAbstract`
- `src/salmon/config.cpp`, `src/translator/config.cpp` e outros usam registry diretamente

### Process and Shell Launching

**Usage:** spawn de processos auxiliares, shell open, restart/reexec, helpers.

**Evidence:**

- `src/salamdr1.cpp`, `src/salspawn/salspawn.cpp`, `src/translator/trlipc.cpp`
- `ShellExecute` / `ShellExecuteEx` em `salmon`, `translator`, `checkver`

### Crash Diagnostics / Minidumps

**Usage:** geração de dump e bug reporting.

**Evidence:**

- `src/salmon/minidump.cpp` carrega `DbgHelp` e resolve `MiniDumpWriteDump`

## Networking

### Winsock

**Usage:** upload/report no módulo de crash.

**Evidence:**

- `src/salmon/upload.cpp` usa `WSAStartup`

### WinINet

**Usage:** verificação de atualizações e download/consulta HTTP/FTP no plugin `checkver`.

**Evidence:**

- `src/plugins/checkver/internet.cpp` usa `InternetOpen` e `InternetOpenUrl`

### FTP / OpenSSL

**Usage:** plugin FTP dedicado com dependência opcional/externa.

**Evidence:**

- presença do plugin `src/plugins/ftp/`
- `doc/third_party.txt` registra OpenSSL
- README menciona ausência das libs OpenSSL para build completo do FTP

## Storage / Data

### SQLite

**Usage:** banco embarcado disponível como projeto do ecossistema.

**Evidence:**

- `src/vcxproj/sqlite/sqlite.vcxproj`
- `src/common/dep/sqlite/`

### Conversion Tables

**Usage:** tabelas de conversão para comando Convert.

**Evidence:**

- diretório `convert/`

## Documentation / Help

### HTML Help Workshop / CHM

**Usage:** build do manual e help local.

**Evidence:**

- `help/src/`
- `htmlhelp.lib` em `src/vcxproj/sal_base.props`

### Localization Pipeline

**Usage:** recursos de idioma base e traduções externas.

**Evidence:**

- `src/lang/`
- `translations/*/*.slt`
- `src/translator/`
- projetos `lang_*` na solution

## Plugin SDK / ABI Integration

### Internal SDK

**Usage:** contrato binário entre core e plugins.

**Evidence:**

- `src/plugins/shared/spl_*.h`
- `src/plugins.h`
- plugins exportam `SalamanderPluginEntry`

### Shell Extension

**Usage:** integração com Explorer/shell do Windows.

**Evidence:**

- `src/vcxproj/shellext/salextx86.vcxproj`
- `src/vcxproj/shellext/salextx64.vcxproj`

## Third-Party Embedded Libraries

Dependências identificadas no código e/ou em `doc/third_party.txt`:

- 7-Zip
- zlib
- bzip2
- libexif
- CHMLIB
- LibTomCrypt
- Nano SVG
- cmark-gfm
- OpenSSL
- SQLite
- WIL

## Tooling Integrations

### Python Utilities via `uv`

**Usage:** análise de comentários, status de tradução e guard rails locais.

**Evidence:**

- `tools/pyproject.toml`
- `tools/README.md`

### GitHub Actions

**Usage:** build de PR e políticas de contribuição.

**Evidence:**

- `.github/workflows/pr-msbuild.yml`
- `.github/workflows/pr-comments-guard.yml`
- `.github/workflows/pr-squash-guard.yml`

## Integration Notes

- A maioria das integrações é local/desktop e fortemente dependente do Windows.
- Não há evidência de integrações cloud-first ou serviços remotos centrais no core.
- O sistema de plugins é a principal forma de extensão funcional do produto.
