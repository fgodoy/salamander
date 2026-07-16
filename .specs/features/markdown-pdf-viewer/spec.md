# Markdown and Mermaid PDF Viewer Specification

## Problem Statement

Open Salamander currently uses an Internet Explorer-based viewer (`ieviewer`) which relies on the obsolete MSHTML engine (emulating IE11). While it has basic Markdown rendering capabilities using `cmark-gfm`, it cannot render modern web components, including Mermaid.js diagrams. Many developers use Markdown with embedded Mermaid diagrams for design documents, roadmaps, and architectures (e.g., in `.specs/`). 

Furthermore, there is currently no way to export the rendered Markdown (with its styling and diagrams) to a portable PDF format directly from the F3 viewer, forcing users to use external browsers or tools to read and share these documents.

## Goals

- [ ] Render Markdown (`.md`, `.markdown`, `.mdown`) files in the F3 viewer with full support for modern HTML/CSS and Mermaid.js diagrams.
- [ ] Provide a native and user-friendly way to export the rendered Markdown document as a PDF.
- [ ] Ensure the exported PDF preserves the exact layout, custom styles (e.g., GitHub Markdown styling), and rendered SVG diagrams (Mermaid).
- [ ] Keep the viewer fully offline-capable so that rendering works without an active internet connection.

## Out of Scope

- [ ] Replacing `ieviewer` entirely for all general HTML/XML files in this phase, unless a general WebView2 upgrade is decided. This spec focuses on Markdown and Mermaid rendering.
- [ ] Live-editing of Markdown files inside the F3 viewer (the viewer remains read-only).
- [ ] Native rendering of Mermaid diagrams to image formats other than PDF (e.g., PNG/JPEG export), unless provided by the browser control's context menu.

---

## User Stories

### P1: Render Markdown with Mermaid Diagrams ⭐ MVP

**User Story**: As a software engineer, I want the Salamander F3 viewer to render Markdown files with embedded Mermaid.js blocks so that I can read formatted documentation and view charts, graphs, and sequence diagrams directly inside the file manager.

**Why P1**: This is the core viewing capability. Without rendering the diagrams, the document visual parity is lost.

**Acceptance Criteria**:

1. WHEN the user presses F3 on a Markdown file (`.md`, `.markdown`, `.mdown`) THEN the viewer SHALL render the markdown content into formatted HTML.
2. WHEN the Markdown content contains code blocks tagged as `mermaid` THEN the viewer SHALL execute Mermaid.js to render them as SVG diagrams in place of the raw text.
3. WHEN rendering the document THEN the system SHALL apply a clean, readable CSS theme (e.g., GitHub Markdown style) suitable for both light and dark backgrounds.
4. WHEN the document is rendered THEN the rendering engine SHALL support modern ES6+ Javascript and SVG rendering capabilities (requiring a modern engine like WebView2).

**Independent Test**: Open a Markdown file containing a Mermaid flowchart/sequence diagram using F3 in Salamander and verify that the layout is rendered as HTML and the diagram is displayed as an SVG instead of raw code.

---

### P1: Export Rendered Document to PDF ⭐ MVP

**User Story**: As a user viewing a Markdown document, I want to export the rendered view as a PDF file so that I can easily print or share the styled documentation with external users.

**Why P1**: Portable sharing is a primary requirement. PDF is the standard for document sharing and printing.

**Acceptance Criteria**:

1. WHEN the viewer is active THEN the user SHALL be able to trigger a "Save/Export as PDF" action (via a menu option, toolbar button, or hotkey like Ctrl+Shift+P).
2. WHEN the export is triggered THEN the system SHALL present a standard Save File dialog prompting for the target PDF path.
3. WHEN the user confirms the path THEN the system SHALL write a PDF file containing the rendered markdown and rendered Mermaid diagrams.
4. WHEN the PDF is generated THEN the formatting, fonts, and inline SVG elements (Mermaid charts) SHALL be preserved with high fidelity.

**Independent Test**: Open a Markdown file with Mermaid diagrams, press the PDF export button, choose a destination file, and verify that the output PDF contains the rendered HTML text and SVG diagrams exactly as seen in the viewer.

---

### P2: Offline-Capable Rendering

**User Story**: As an offline developer, I want all markdown and diagram rendering to work without an internet connection so that I can read project specifications and docs in isolated environments.

**Why P2**: Open Salamander is a local desktop application and must not depend on cloud APIs or CDNs for viewing local files.

**Acceptance Criteria**:

1. WHEN the viewer compiles the final HTML stream THEN it SHALL inline or reference local JavaScript assets (like `mermaid.min.js`) and local CSS styling.
2. WHEN there is no network connection THEN the Markdown parser, CSS styles, and Mermaid diagrams SHALL render with zero degradation.

**Independent Test**: Disconnect the network, open a Markdown file containing a Mermaid diagram, and confirm the document and diagrams load and display correctly.

---

### P3: Interactive PDF Print Configuration

**User Story**: As a user exporting to PDF, I want to configure the output page settings (margins, orientation, print background) so that the document is optimized for my specific printing or viewing needs.

**Why P3**: Standard printing defaults might cut off wider diagrams or text; customization allows adapting to different page formats.

**Acceptance Criteria**:

1. WHEN exporting to PDF THEN the system SHALL allow configuration of page orientation (Portrait/Landscape), margin sizes, and whether background graphics/colors are printed.
2. WHEN background colors are enabled THEN the exported PDF SHALL preserve the document theme's background styling (e.g., dark mode if selected).

**Independent Test**: Export a wide sequence diagram to Landscape PDF, verify it fits the horizontal space, and check that background styles are rendered correctly.

---

## Edge Cases

- **Malformed Mermaid Syntax**: WHEN a Mermaid block contains syntax errors THEN the renderer SHALL display a readable error message inline instead of crashing the viewer.
- **Large Markdown Files / Huge Diagrams**: WHEN loading very large files THEN the rendering engine SHALL load asynchronously without freezing the Salamander UI.
- **Missing WebView2 Runtime**: WHEN the system does not have Microsoft Edge WebView2 runtime installed THEN the plugin SHALL degrade gracefully (e.g., show a message prompt directing the user to download the runtime, or fall back to the basic MSHTML text viewer).
- **Extremely Wide Diagrams**: WHEN a diagram exceeds the screen width THEN the viewer/PDF SHALL support horizontal scrolling or fit-to-width scaling.

---

## Success Criteria

- [ ] Markdown files are rendered with visual parity to GitHub/modern Markdown rendering.
- [ ] Mermaid diagrams (flowcharts, sequence diagrams, class diagrams, etc.) are rendered dynamically in the viewer.
- [ ] A PDF export action is available and successfully writes a PDF file containing the styled markdown and SVG diagrams.
- [ ] The entire flow runs offline without requesting external network resources.
- [ ] The implementation works on standard Windows 10/11 environments with pre-installed Edge/WebView2.
