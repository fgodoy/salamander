# Project Roadmap

## Completed

### 2026-03-27

- Panel TreeView
  - toggle global por menu, configuração e atalho `Ctrl+Shift+T`
  - host fixo no painel esquerdo
  - conteúdo sincronizado com o painel ativo
  - redimensionamento lateral com persistência de largura
  - validação por build `Debug|Win32`
- Panel TreeView Rich File Icons
  - payload tipado para nós de pasta e arquivo
  - image list small-icon da shell ligado ao tree
  - população mista com diretórios primeiro e arquivos depois
  - seleção de arquivo integrada ao fluxo de foco do painel via `WM_USER_FOCUSFILE`
  - validação por build `Debug|Win32`

## In Progress

- Manual smoke and follow-up polish for recent UI changes
- UI modernization discovery and path selection
- Manual smoke for TreeView rich file icons

## Backlog

- Reduce runtime issues caused by optional or non-open-source plugin dependencies in development builds
- UI modernization implementation waves after path decision
