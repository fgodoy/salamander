# Project Roadmap

## Completed

### 2026-03-27

- Panel TreeView
  - toggle global por menu, configuração e atalho `Ctrl+Shift+T`
  - host fixo no painel esquerdo
  - conteúdo sincronizado com o painel ativo
  - redimensionamento lateral com persistência de largura
  - double click no splitter central passa a centralizar as áreas úteis dos listboxes quando o tree está aberto
  - modo centralizado do splitter passa a persistir em resize, maximize e toggle de visibilidade do tree
  - atalho `Ctrl+Shift+T` reaplica o recálculo centralizado no passo final do comando para evitar desalinhamento residual
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

### 2026-04-01

- PictView Open-Backend Foundation
  - bootstrap do plugin movido para um backend local open-source em vez de `PVW32Cnv.dll`
  - dependência obrigatória de `PVW32Cnv.lib` e do envelope x64 removida do build principal do plugin
  - backend WIC inicial implementado para abrir e renderizar imagens estáticas em `24bpp BGR` com `PVImageInfo`/`PVImageHandles` compatíveis
  - suporte inicial para abrir por arquivo, `HBITMAP` anexado, clipboard bitmap e rotação de 90 graus em memória
  - suporte inicial de transform/salvamento adicionado para crop em memória, `Save As` WIC para BMP/JPEG/PNG/TIFF e saída `RAW` por callback para preview/thumbnail flow
  - suporte inicial para GIF animado adicionado via `PVImageSequence` com composição de frames e leitura de metadata nativa do WIC
  - pipeline de `Save As` do backend open ajustado para negociar o pixel format real do encoder WIC, destravando conversão interna para `GIF` além de BMP/JPEG/PNG/TIFF
  - metadata de orientação EXIF agora é lida via WIC para JPEG/TIFF e projetada em `PVFF_ROTATE90`/`PVFF_FLIP_HOR`/`PVFF_BOTTOMTOTOP`, com fallback local de autorotate no viewer quando a `EXIF.DLL` não resolve a orientação
  - árvore local de runtime `Debug_x86` preparada em `.localbuild\` com `salamand.exe`, `english.slg`, `pictview.spl` e `exif.dll`, e smoke de boot confirmou que a app permanece aberta pelo menos 5 segundos
  - árvores locais de runtime `Release_x86` e `Release_x64` preparadas em `.localbuild\` com `salamand.exe`, `salmon.exe`, `pictview.spl`, `checkver.spl`, `demoplug.spl` e resources mínimos, e smoke de boot confirmou processo principal e bug reporter ativos nas duas arquiteturas
  - caminho de decode do backend open ajustado para reaproveitar metadados já carregados do frame atual, reduzindo um reload redundante no primeiro decode após o open/getinfo
  - validação por build `Debug|Win32` e `Debug|x64`

## In Progress

- Manual smoke and follow-up polish for recent UI changes
- UI modernization discovery and path selection
- Manual smoke for TreeView rich file icons
- Portuguese localization planning for distinct `pt-BR` and `pt-PT` support using the existing translation workflow
- PictView WIC backend completion for manual smoke, residual thumbnail/performance review, and TIFF/page-flow polish

## Backlog

- Reduce runtime issues caused by optional or non-open-source plugin dependencies in development builds
- UI modernization implementation waves after path decision
