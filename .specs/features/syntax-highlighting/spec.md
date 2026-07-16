# Syntax Highlighting Specification

## Problem Statement

Source code and configuration files displayed via the F3 viewer currently render as unstyled plain text, making it difficult to read, inspect, and analyze code structures. We need a fast, modern, and offline-capable syntax highlighting engine integrated into the Edge WebView2 viewer to display the 20 most popular development and markup languages with high readability.

---

## Goals

- [ ] Support syntax highlighting for the 20 most common programming, scripting, and markup languages.
- [ ] Auto-detect the language dynamically based on the file extension.
- [ ] Render all code formatting offline without requiring external network connections.
- [ ] Integrate with the existing "Export to PDF" button to print styled source code.
- [ ] Fall back gracefully to clean, unstyled plain text if the file extension is not supported.

---

## Out of Scope

- Real-time editing of the files (F3 is strictly a viewer).
- Code auto-completion, linting, syntax error reporting, or compiler diagnostics.
- Advanced IDE features like code folding or "Go to definition".

---

## User Stories

### P1: Offline Syntax Highlighting ⭐ MVP

**User Story**: As a developer, I want to open any supported source file (e.g., `.cpp`, `.py`, `.json`) in the F3 viewer so that I can inspect the code with clear visual formatting.

**Why P1**: This is the core capability that enables developers to read code comfortably within Salamander.

**Acceptance Criteria**:
1. WHEN a file is opened in the F3 viewer THEN the system SHALL infer its language using a predefined list of extension mappings.
2. WHEN a supported code file is loaded THEN the system SHALL format the source text and render it with syntax highlighting using a bundled, offline copy of **Prism.js** (or similar library).
3. WHEN a file extension is not recognized THEN the system SHALL render the file as plain text (monospaced font, no colors) rather than displaying a blank page or an error.
4. WHEN loading a file THEN the system SHALL execute the rendering entirely offline.

**Independent Test**:
- Open a `.py` file, a `.cpp` file, and an unmapped `.txt` file, then verify that the first two are highlighted with correct language tokens (keywords, comments, strings) and the third falls back to standard text.

---

### P2: PDF Export of Styled Code

**User Story**: As a developer, I want to click "Export to PDF" or use `Ctrl+P` on a code file so that I can save a PDF with syntax highlighting.

**Why P2**: Reuses the PDF printing infrastructure we just created, providing value for documentation and printing code directly.

**Acceptance Criteria**:
1. WHEN the "Export to PDF" button is clicked (or `Ctrl+P`/`Ctrl+Shift+P` is pressed) THEN the system SHALL export the rendered code to a PDF.
2. WHEN the PDF is generated THEN the syntax highlighting styles and color themes SHALL be preserved in the output PDF document.

**Independent Test**:
- Open a `.js` file, click "Export to PDF", and verify that the output PDF preserves the exact highlighting colors seen in the viewer.

---

### P3: Line Numbers

**User Story**: As a developer, I want to see line numbers next to each line of code so that I can easily navigate and reference sections of the file.

**Why P3**: Essential quality-of-life feature for code inspection, but not strictly required to read code.

**Acceptance Criteria**:
1. WHEN a supported code file is loaded THEN the system SHALL render sequential line numbers to the left of the source lines.
2. WHEN exporting to PDF THEN the line numbers SHALL also be visible and aligned in the printed PDF page.

**Independent Test**:
- Open any source code file and verify that line numbers (1, 2, 3, etc.) are shown on the left panel, updating correctly when scrolling.

---

## Language Support (Top 20 Target)

The following 20 programming/configuration languages must be mapped by extension and styled:

| Language | Associated Extensions |
| :--- | :--- |
| **C / C++** | `.c`, `.cpp`, `.h`, `.hpp`, `.cc` |
| **C#** | `.cs` |
| **Java** | `.java` |
| **Python** | `.py` |
| **Go** | `.go` |
| **Rust** | `.rs` |
| **Ruby** | `.rb` |
| **PHP** | `.php` |
| **Swift** | `.swift` |
| **Kotlin** | `.kt`, `.kts` |
| **JavaScript** | `.js`, `.mjs`, `.cjs` |
| **TypeScript** | `.ts`, `.tsx` |
| **HTML** | `.html`, `.htm` |
| **CSS** | `.css` |
| **SQL** | `.sql` |
| **JSON** | `.json` |
| **XML** | `.xml`, `.xsd` |
| **YAML** | `.yml`, `.yaml` |
| **Markdown** | `.md` *(Supported via Markdown parser)* |
| **Shell/Batch** | `.sh`, `.bat`, `.cmd`, `.ps1` |

---

## Edge Cases

- **Large Code Files (10MB+)**: WHEN loading very large files THEN the rendering engine SHALL load smoothly without freezing the Salamander UI.
- **Lines with Extremely Long Content**: WHEN a line exceeds the screen width THEN it SHALL wrap or support horizontal scroll.
- **Embedded Code Characters**: WHEN the source file contains HTML/XML special characters (e.g. `<` or `&`) THEN the parser SHALL escape them correctly so they don't break HTML parsing.

---

## Success Criteria

- [ ] Syntax highlighting works on F3 for all 20 target languages.
- [ ] Mapped extensions dynamically load the correct highlighting rules.
- [ ] Exporting to PDF prints colored code correctly.
- [ ] No external CDN calls are made during load.
- [ ] Unknown code files degrade gracefully to monospaced text preview.
