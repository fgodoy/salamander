# Testing

**Analyzed:** 2026-03-26

## Current Strategy

O repositório não mostra uma estratégia moderna de testes automatizados por unidade, integração e E2E. A validação principal é baseada em build, smoke/manual testing e utilitários específicos.

## What Exists

### CI Build Verification

**Location:** `.github/workflows/pr-msbuild.yml`

**Behavior:**

- compila a solution principal `src/vcxproj/salamand.sln`
- executa matriz `Debug|Win32` e `Debug|x64`
- usa `/warnaserror`
- roda em `windows-2022`

Isso é hoje a verificação automatizada mais importante encontrada.

### PR Guardrails for Translation/Comment Work

**Location:** `.github/workflows/pr-comments-guard.yml`

**Behavior:**

- valida PRs rotulados como `comments translation`
- pré-processa fontes C/C++ com `clang`
- tenta impedir mudanças funcionais onde a intenção é alterar apenas comentários

É uma verificação especializada de fluxo, não uma suite de testes de produto.

### Local Build Scripts

**Location:** `src/vcxproj/rebuild.cmd`, `src/vcxproj/build.cmd`

**Behavior:**

- recompilação de perfis `Debug`, `Release` e `Utils (Release)`
- logs separados para erros e warnings
- dependência de `OPENSAL_BUILD_DIR`

### Ad-hoc Testers / Samples

**Examples:**

- `src/reglib/src/tester.cpp`
- scripts de exemplo em `src/plugins/automation/scripts/`
- plugins demo (`demoplug`, `demomenu`, `demoview`)

Esses artefatos ajudam exploração/manual testing, mas não formam uma suite automatizada unificada.

## What Was Not Found

- framework como GoogleTest, Catch2, doctest ou Boost.Test no código do produto
- diretório dedicado `tests/` com cobertura do core
- suíte E2E da UI
- harness de regressão para plugins
- métricas de cobertura automatizadas

## Risk Assessment

- Regressões de comportamento tendem a ser detectadas tarde, especialmente em fluxos de UI e shell integration.
- O acoplamento do core com WinAPI e estado global dificulta introdução retroativa de testes unitários.
- Plugins e compatibilidade binária ampliam a matriz de risco sem proteção automatizada correspondente.

## Practical Testing Entry Points

Para alterações futuras, os pontos mais viáveis de validação parecem ser:

- build da solution principal em `Debug|Win32` e `Debug|x64`
- smoke test manual do executável principal
- smoke tests manuais dos plugins afetados
- validação de crash/report apenas quando tocar `salmon`
- execução dos utilitários Python de `tools/` quando a mudança envolver comentários/traduções

## Recommendation Baseline

Antes de mudanças maiores, faz sentido adotar ao menos:

- build CI expandido para `Release`
- smoke checklist por área alterada
- harness mínimo para módulos mais isoláveis (`common`, parsers, helpers, tooling)
