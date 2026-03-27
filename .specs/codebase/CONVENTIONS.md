# Code Conventions

## Naming Conventions

**Files:**

Arquivos são em geral minúsculos, curtos e históricos. Implementações grandes são frequentemente quebradas em séries numeradas.

Examples: `salamdr1.cpp`, `mainwnd4.cpp`, `fileswn9.cpp`, `worker.h`, `spl_base.h`

**Functions/Methods:**

Mistura de estilo PascalCase para métodos/classes e nomes WinAPI-like para callbacks e entrypoints.

Examples: `InitWorker`, `ReleaseListingBody`, `WinMainBody`, `SalamanderPluginEntry`, `GetUnassignedHotPathIndex`

**Variables:**

Uso forte de tipos e convenções herdadas de Win32/C++ antigo: `BOOL`, `DWORD`, `HINSTANCE`, ponteiros crus e muitos globais.

Examples: `SalamanderBusy`, `MainThreadID`, `Windows10AndLater`, `DLLInstance`, `HLanguage`

**Constants/Macros:**

Constantes e macros ficam em `UPPER_CASE`, flags usam prefixos e enums usam prefixos curtos.

Examples: `HOT_PATHS_COUNT`, `PLUGIN_REQVER`, `OPFL_COPY_ADS`, `MENU_EVENT_TRUE`, `ocCopyFile`

**Types/Classes:**

Classes normalmente usam prefixo `C`. Structs também costumam usar `C`, seguindo a tradição do projeto.

Examples: `CMainWindow`, `CFilesWindow`, `COperations`, `CSalamanderRegistryAbstract`

## Code Organization

**Include order:**

Arquivos `.cpp` normalmente começam com `precomp.h`, depois headers do sistema/CRT e depois headers do projeto. A ordenação não é automatizada.

Example from `src/salamdr1.cpp`:

- `#include "precomp.h"`
- headers CRT/Windows como `<time.h>`, `<rtcapi.h>`, `<uxtheme.h>`
- headers internos como `"menu.h"`, `"plugins.h"`, `"fileswnd.h"`

**File structure:**

Há um padrão frequente:

1. cabeçalho SPDX
2. `#pragma once` ou includes
3. macros/defines
4. forward declarations
5. enums/structs/classes
6. funções e implementação

Headers extensos concentram contrato e comentários; `.cpp` grandes concentram implementação procedural.

## Formatting

**Observed style:**

- indentação de 4 espaços
- CRLF
- braces em Allman
- ponteiros com alinhamento à esquerda (`char* Name`)
- `ColumnLimit: 0`
- includes não são reordenados automaticamente

**Source of truth:**

- `.editorconfig`
- `.clang-format`

## Type Safety / Modern C++

**Approach:**

O projeto compila com padrão moderno (`stdcpplatest`), mas o estilo de código é explicitamente legado.

**Observed patterns:**

- uso dominante de `BOOL`, `DWORD`, `LPSTR`, `HANDLE`
- pouca ou nenhuma presença de STL moderna no core
- sem namespaces como convenção dominante
- sem RAII generalizado
- gerenciamento manual de memória e lifecycle

**Example:**

`CHotPathItem` em `src/mainwnd.h` aloca e libera `Name`/`Path` com `DupStr` e `free`.

## Error Handling

**Pattern:**

Erros são tratados por combinação de:

- retorno `BOOL`/códigos de erro
- `MessageBox`/dialogs para falhas operacionais
- tracing via `TRACE_*`
- wrappers instrumentados de handles/registry/process

**Examples:**

- `MyEntryPoint()` mostra `MessageBox` se `SalmonInit()` falha
- wrappers em `src/common/handles.cpp` verificam `CreateProcess`, `LoadLibrary`, `RegOpenKeyEx`
- plugins tendem a usar mensagens e returns simples em vez de exceções

## Comments / Documentation

**Style:**

- muitos comentários históricos em tcheco
- parte do código já tem comentários traduzidos para inglês
- arquivos traduzidos marcam `CommentsTranslationProject: TRANSLATED`
- comentários são densos e frequentemente explicam motivos históricos ou limitações do Windows

**Examples:**

- `src/salamdr1.cpp` mistura comentários traduzidos e históricos
- `src/common/winlib.h` e `src/plugins/shared/spl_base.h` documentam o contrato com bastante detalhe

## Plugin Conventions

Plugins seguem um padrão bastante consistente:

- globais para nome, idioma, ponteiros de interface e configuração
- `DllMain` opcional para bootstrap mínimo
- entrypoints padronizados exportados
- especializações em interfaces como archiver/viewer/menu/FS/thumb loader

**Example:** `src/plugins/demoplug/demoplug.cpp`

## Exceptions / Variations

- dependências terceiras em `src/common/dep/` e alguns plugins não seguem plenamente o estilo local
- há coexistência de C e C++
- plugins específicos têm subárvores próprias com convenções externas, como `7zip`, `winscp`, `cmark-gfm`, `libexif`

## Workflow Directives

Estas diretivas passam a valer para o fluxo de trabalho e manutenção dos artefatos em `.specs/` e `README.md`.

- sempre enviar uma sugestão de commit ao concluir uma tarefa
- sempre incluir nos `design.md` de features um diagrama de comunicação entre componentes e um diagrama de sequência do fluxo principal
- os diagramas em `design.md` devem usar cores com contraste suficiente para leitura em temas escuros
- sempre atualizar `.specs/project/ROADMAP.md` com o que foi finalizado
- sempre atualizar `README.md` com scripts relevantes criados ou mantidos no fluxo do projeto
- sempre atualizar `README.md` com o progresso registrado na timeline do projeto
