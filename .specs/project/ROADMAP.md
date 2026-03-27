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
- Panel TreeView Color Sync
  - fundo do tree alinhado com `ITEM_BK_NORMAL`
  - texto do tree alinhado com `ITEM_FG_NORMAL`
  - seleção do tree alinhada com as cores focadas do painel
  - seleção sem foco corrigida para usar as cores `SELECTED` do painel
  - seleção do tree corrigida para seguir o estado ativo da janela principal (`FOCSEL` quando a app está ativa)
  - visual style `explorer` removido do tree para não sobrescrever as cores customizadas
  - fundo de seleção do tree alinhado aos mesmos brushes efetivos usados pelo painel para preservar o highlight configurado, incluindo `8080ff`
  - desenho default de item selecionado do `TreeView` neutralizado para não reaplicar o azul clássico do controle sobre o highlight do painel
  - validação por build `Debug|Win32`

## In Progress

- Manual smoke and follow-up polish for recent UI changes
- UI modernization discovery and path selection
- Manual smoke for TreeView rich file icons

## Backlog

- Reduce runtime issues caused by optional or non-open-source plugin dependencies in development builds
- UI modernization implementation waves after path decision
