# Project Structure

**Root:** `C:\Projetos\salamander`

## Directory Tree

```text
.
|-- .github/
|   `-- workflows/
|-- convert/
|   |-- centeuro/
|   |-- cyrillic/
|   `-- westeuro/
|-- doc/
|   `-- license/
|-- help/
|   `-- src/
|-- src/
|   |-- common/
|   |-- lang/
|   |-- plugins/
|   |-- reglib/
|   |-- res/
|   |-- salmon/
|   |-- salopen/
|   |-- salspawn/
|   |-- setup/
|   |-- shellext/
|   |-- sfx7zip/
|   |-- translator/
|   |-- tserver/
|   `-- vcxproj/
|-- tools/
|   |-- comments/
|   |-- natvis/
|   |-- salbreak/
|   `-- utfnames/
`-- translations/
    |-- czech/
    |-- german/
    |-- spanish/
    `-- ...
```

## Module Organization

### Core Application

**Purpose:** executável principal do file manager, UI, painéis, operações, cache, menu, viewer e integração shell.

**Location:** `src/`

**Key files:** `src/salamdr1.cpp`, `src/mainwnd.h`, `src/fileswnd.h`, `src/worker.h`, `src/plugins.h`

### Shared Runtime / Internal Libraries

**Purpose:** abstrações internas sobre WinAPI, strings, tracing, regex, heap, arrays e helpers comuns.

**Location:** `src/common/`

**Key files:** `src/common/winlib.h`, `src/common/handles.h`, `src/common/trace.h`, `src/common/str.h`

### Plugin SDK and Shared Plugin Assets

**Purpose:** contrato binário e headers compartilhados entre core e plugins.

**Location:** `src/plugins/shared/`

**Key files:** `spl_base.h`, `spl_com.h`, `spl_arc.h`, `spl_fs.h`, `spl_view.h`, `vcxproj/plugin_base.props`

### Product Plugins

**Purpose:** arquivadores, viewers, integrações e ferramentas opcionais.

**Location:** `src/plugins/`

**Key files:** `7zip/`, `zip/`, `tar/`, `ftp/`, `pictview/`, `diskmap/`, `checkver/`, `demoplug/`

### Auxiliary Executables / DLLs

**Purpose:** crash reporting, shell helpers, process helpers, installer, translator e trace server.

**Location:** `src/salmon/`, `src/salopen/`, `src/salspawn/`, `src/setup/`, `src/translator/`, `src/tserver/`, `src/shellext/`

**Key files:** `src/salmon/salmon.cpp`, `src/salopen/salopen.cpp`, `src/translator/translator.cpp`

### Content / Data

**Purpose:** ajuda CHM, traduções e tabelas de conversão.

**Location:** `help/`, `translations/`, `convert/`

**Key files:** `help/src/`, `translations/*/*.slt`, `convert/*/*.tab`

### Tooling

**Purpose:** scripts de build, normalização e análise de comentários/traduções.

**Location:** `tools/`, `src/vcxproj/`, raiz

**Key files:** `src/vcxproj/rebuild.cmd`, `normalize.ps1`, `tools/pyproject.toml`, `tools/README.md`

## Where Things Live

**Main app startup:**

- `src/salamdr1.cpp`

**Main window and panels:**

- `src/mainwnd*.cpp`, `src/mainwnd.h`
- `src/fileswn*.cpp`, `src/fileswnd.h`

**Long-running file operations:**

- `src/worker.cpp`, `src/worker.h`

**Plugin loading and dispatch:**

- `src/plugins.h`
- `src/plugins/shared/`

**Crash reporting:**

- `src/salmon/`

**Visual Studio build definitions:**

- `src/vcxproj/`
- `src/plugins/*/vcxproj/`

**Language resources:**

- `src/lang/`
- `translations/`

**Third-party embedded code:**

- `src/common/dep/`
- várias subárvores em `src/plugins/*`

## Structural Observations

- O repositório é grande e fortemente orientado a build de solução Visual Studio.
- A estrutura separa bem conteúdo, plugins e auxiliares, mas o core ainda é concentrado em arquivos extensos.
- Há muitos subprojetos compiláveis de forma independente dentro da mesma solution.
- O acoplamento principal acontece por headers compartilhados e ABI do SDK de plugin.
