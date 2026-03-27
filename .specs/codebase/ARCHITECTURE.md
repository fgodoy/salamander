# Architecture

**Pattern:** desktop monolith with plugin architecture

## High-Level Structure

O repositório é organizado ao redor de um executável principal de file manager (`salamand`) e um conjunto grande de plugins compilados separadamente, mas carregados no mesmo ecossistema. O core expõe um SDK próprio (`spl_*`) e encapsula chamadas de plugin para controlar entrada/saída, threading, tracing e compatibilidade.

Estrutura em alto nível:

- `src/`: núcleo da aplicação, wrappers WinAPI, recursos e utilitários internos
- `src/common/`: bibliotecas compartilhadas do próprio projeto
- `src/plugins/`: plugins, demos, bibliotecas compartilhadas e dependências por plugin
- `src/salmon/`: captura de crash, minidump e upload/report
- `src/salopen/`, `src/salspawn/`, `src/shellext/`, `src/setup/`, `src/translator/`, `src/tserver/`: executáveis e DLLs auxiliares
- `help/`, `translations/`, `convert/`: conteúdo e dados operacionais fora do binário principal

## Identified Patterns

### Monólito GUI orientado a WinAPI

**Location:** `src/salamdr1.cpp`, `src/mainwnd.h`, `src/fileswnd.h`

**Purpose:** centralizar a UI, o loop de mensagens, os painéis de arquivos, comandos e estado global.

**Implementation:** a aplicação entra por `MyEntryPoint`, inicializa o bug reporter (`SalmonInit`) e então transfere para o startup padrão do runtime, que chama `WinMain`. A janela principal e os painéis são classes grandes sobre `CWindow`.

**Example:** `src/salamdr1.cpp` define `MyEntryPoint()` e `WinMain()`; `src/mainwnd.h` e `src/fileswnd.h` concentram a maior parte da UI.

### Mini-framework interna sobre WinAPI

**Location:** `src/common/winlib.h`, `src/common/handles.h`, `src/common/trace.h`

**Purpose:** esconder repetição de WinAPI, padronizar objetos de janela, tracing, alocação e monitoramento de handles.

**Implementation:** classes como `CWindow` e `CWindowsObject` encapsulam criação/destruição de janelas; `handles.*` rastreia operações críticas como `CreateProcess`, `LoadLibrary` e `RegOpenKeyEx`.

**Example:** `src/common/winlib.h` define a hierarquia `CWindowsObject -> CWindow`; `src/common/handles.cpp` implementa wrappers instrumentados.

### SDK de plugins com encapsulamento defensivo

**Location:** `src/plugins.h`, `src/plugins/shared/spl_*.h`, `src/plugins/*`

**Purpose:** permitir extensões para arquivadores, viewers, menu, file systems e thumbnail loaders sem expor diretamente invariantes internos do core.

**Implementation:** o core define interfaces abstratas no SDK (`CPluginInterface*Abstract`) e classes de encapsulamento que sempre chamam `EnterPlugin()`/`LeavePlugin()` antes de delegar ao plugin. Cada plugin exporta entrypoints padronizados.

**Example:** `src/plugins.h` encapsula `ListArchive`, `ViewFile`, `GetMenuItemState`; `src/plugins/demoplug/demoplug.cpp` mostra o contrato completo de plugin.

### Organização híbrida por domínio e por arquivo grande

**Location:** `src/mainwnd1.cpp` ... `src/mainwnd5.cpp`, `src/fileswn0.cpp` ... `src/fileswnb.cpp`

**Purpose:** quebrar unidades muito grandes em múltiplos arquivos sem mudar o modelo mental de “um componente grande por área”.

**Implementation:** vários subsistemas são espalhados em séries numeradas de arquivos que compartilham o mesmo cabeçalho principal.

**Example:** `mainwnd*.cpp` e `fileswn*.cpp` distribuem implementação da janela principal e dos painéis.

### Tooling de manutenção paralela ao produto

**Location:** `tools/`, `.github/workflows/`

**Purpose:** sustentar build, normalização, análise de comentários/traduções e verificações de PR.

**Implementation:** scripts `cmd`/`ps1` para build local; utilitários Python com `uv`; CI compila em matriz Win32/x64 e aplica guardas específicos para PRs de tradução.

**Example:** `src/vcxproj/rebuild.cmd`, `tools/pyproject.toml`, `.github/workflows/pr-msbuild.yml`.

## Data Flow

### Startup da aplicação principal

1. O linker entra em `MyEntryPoint` em `src/salamdr1.cpp`.
2. O processo inicializa `Salmon` para crash reporting.
3. O runtime segue para `WinMainCRTStartup`, que chega ao `WinMain`.
4. `WinMainBody` inicializa COM, bibliotecas Win32, recursos de idioma e configuração.
5. A janela principal e os painéis entram no loop de mensagens.

### Operações de arquivo

1. A UI/painel coleta seleção, critérios e destino.
2. `COperations` em `src/worker.h` representa o script/roteiro da operação.
3. Worker threads executam copy/move/delete/convert com buffers, métricas de throughput e controle de progresso.
4. O resultado volta para UI, status bar, dialogs e refresh de painel.

### Carga de plugins

1. O core localiza e carrega módulos `.spl`.
2. O plugin exporta `SalamanderPluginGetReqVer`, `SalamanderPluginGetSDKVer` e `SalamanderPluginEntry`.
3. O core conecta interfaces especializadas conforme capacidades do plugin.
4. Chamadas ao plugin passam pela camada de encapsulamento para preservar invariantes internos e tracing.

### Crash reporting

1. Falhas do processo são capturadas pelo componente `salmon`.
2. `salmon` gera minidump via `DbgHelp`.
3. O módulo pode compactar e enviar dados usando Winsock/upload próprio.

## Code Organization

**Approach:** monólito por domínio técnico, com submódulos auxiliares e plugins compilados separadamente.

**Structure:**

- Core: UI, comandos, operações, shell, viewers, menus, cache
- Shared libs: strings, tracing, regex, heap, handles, wrappers Win32
- Plugins: arquivadores, viewers, ferramentas, demos, integrações
- Auxiliares: setup, translator, shell extension, crash reporter, trace server

**Module boundaries:**

- `src/` depende fortemente de `src/common/` e do SDK em `src/plugins/shared/`
- `src/plugins/*` dependem do SDK compartilhado e de alguns helpers comuns
- `salmon`, `salopen`, `salspawn`, `translator`, `setup` são binários auxiliares, mas parte do mesmo ecossistema de build
- `help`, `translations`, `convert` ficam fora da lógica do core, porém são insumos do produto final

## Architectural Risks

- Grande volume de estado global no processo, tornando isolamento e teste difíceis
- Forte acoplamento à WinAPI e ao ambiente Windows
- Modelo de memória manual (`malloc/free`, handles, `BOOL`, `char*`) amplia superfície de bugs
- Fronteiras de módulo existem, mas o núcleo continua amplamente monolítico
- Parte da extensibilidade depende de ABI própria e compatibilidade binária histórica do SDK
