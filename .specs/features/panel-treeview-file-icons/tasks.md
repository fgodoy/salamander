# Panel TreeView File Icons Tasks

**Design**: `.specs/features/panel-treeview-file-icons/design.md`
**Spec**: `.specs/features/panel-treeview-file-icons/spec.md`
**Status**: Implemented, build validated, manual smoke pending

---

## Execution Plan

### Phase 1: Tree Model Foundation (Sequential)

These tasks establish the typed node model and icon source for the tree.

```text
T1 -> T2
```

### Phase 2: Mixed Population (Sequential)

These tasks change the tree from directory-only to mixed directory/file content.

```text
T2 -> T3 -> T4
```

### Phase 3: Selection Integration (Sequential)

These tasks wire file-node behavior back into the active panel model.

```text
T4 -> T5 -> T6
```

### Phase 4: Verification and Polish (Sequential)

These tasks confirm buildability and lock down the intended UX.

```text
T6 -> T7
```

---

## Task Breakdown

### T1: Introduce Typed Tree Node Payload and Cleanup Helpers

**What**: Replace the current raw path-only `lParam` usage with a typed heap payload that can represent either a directory or a file node safely.
**Where**: `src/fileswnd.h`, `src/fileswn1.cpp`
**Depends on**: None
**Reuses**: current `InsertTreeViewItem()` / `GetTreeViewItemPath()` payload pattern

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Tree nodes can distinguish directory vs file type.
- [x] Payload stores enough data for icon and selection behavior.
- [x] Tree cleanup releases payload memory without leaks or stale pointers.

**Verify**:

- `rg -n "TreeViewNode|NodeType|lParam" src/fileswnd.h src/fileswn1.cpp`

---

### T2: Bind the Tree to a Small Shell Image List

**What**: Add TreeView image-list lifecycle and shell icon index resolution so nodes can display recognizable folder and file-type icons.
**Where**: `src/fileswn2.cpp`, `src/fileswn1.cpp`
**Depends on**: T1
**Reuses**: `TreeView_SetImageList(...)` patterns in `src/dialogs5.cpp`, shell icon helpers in `src/geticon.cpp`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] The tree owns or attaches to a small-icon image list correctly.
- [x] Directory nodes can resolve closed/open folder icon indexes.
- [x] File nodes can resolve type-appropriate icon indexes.

**Verify**:

- `rg -n "TreeView_SetImageList|SHGetFileInfo|GetFileIcon|iImage|iSelectedImage" src/fileswn1.cpp src/fileswn2.cpp`

---

### T3: Extend Tree Population from Directories Only to Mixed Dir/File Nodes

**What**: Update child enumeration so expanded directory nodes can insert both subdirectories and files, with predictable grouping and sorting.
**Where**: `src/fileswn1.cpp`
**Depends on**: T2
**Reuses**: current `PopulateTreeViewItem()` disk enumeration and sorting flow

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Expanded nodes can contain both directories and files.
- [x] `.` and `..` remain excluded.
- [x] Directories appear before files in stable alphabetical order.
- [x] File nodes are inserted as leaves.

**Verify**:

- `rg -n "PopulateTreeViewItem|FindFirstFile|TreeView_InsertItem|SortChildren" src/fileswn1.cpp`

---

### T4: Honor Baseline Visibility Rules for Mixed Tree Content

**What**: Keep mixed-node population aligned with the active panel's baseline visibility expectations, especially hidden/system handling and disk-only support.
**Where**: `src/fileswn1.cpp`, `src/fileswn0.cpp`
**Depends on**: T3
**Reuses**: current disk-only gating and panel configuration flags

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Non-disk modes remain disabled or empty.
- [x] Hidden/system baseline is handled consistently with the active panel configuration.
- [x] The tree does not expose misleading nodes that the panel would clearly suppress under baseline rules.

**Verify**:

- `rg -n "NotHiddenSystemFiles|ptDisk|RefreshTreeView|PopulateTreeViewItem" src/fileswn0.cpp src/fileswn1.cpp`

---

### T5: Branch Tree Selection Behavior for Directory vs File Nodes

**What**: Keep directory-node navigation as-is, but route file-node selection into the existing file-focus flow of the active panel.
**Where**: `src/fileswnb.cpp`, `src/fileswn1.cpp`
**Depends on**: T4
**Reuses**: current `TVN_SELCHANGED` handling and `WM_USER_FOCUSFILE`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Selecting a directory node still calls `ChangePathToDisk(...)`.
- [x] Selecting a file node sends the active panel to the correct parent path and focuses the file.
- [x] File selection does not accidentally trigger file open/execute behavior.

**Verify**:

- `rg -n "TVN_SELCHANGED|WM_USER_FOCUSFILE|ChangePathToDisk" src/fileswnb.cpp src/fileswn1.cpp`

---

### T6: Keep Refresh, Expansion, and Node-State Semantics Honest

**What**: Ensure directory expandability, selected-image behavior, and refresh flows remain correct once file nodes are added.
**Where**: `src/fileswn1.cpp`, `src/fileswnb.cpp`
**Depends on**: T5
**Reuses**: current tree refresh and expansion model

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Only directory-like nodes appear expandable.
- [x] Folder open/closed icon state behaves correctly when available.
- [x] Tree refresh does not leave stale file nodes or stale payload state behind.

**Verify**:

- `rg -n "TVIF_CHILDREN|TVE_EXPAND|RefreshTreeView|DeleteAllItems|TreeView_DeleteItem" src/fileswn1.cpp src/fileswnb.cpp`

---

### T7: Validate Build and Smoke the Mixed Tree UX

**What**: Validate the feature through supported build flow and a targeted manual smoke checklist for icons, file selection, and non-disk behavior.
**Where**: `src/vcxproj/salamand.sln`, treeview runtime behavior
**Depends on**: T6
**Reuses**: existing `build.cmd` / `rebuild.cmd` workflow

**Tools**:

- MCP: NONE
- Local tools: `shell_command`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] The main solution builds in at least one supported local configuration.
- [ ] Manual smoke verifies dir/file icons, file selection, and disk-only gating.
- [ ] No obvious regressions appear in tree toggle, panel switch, or tree refresh flows.

**Verify**:

- `src\\vcxproj\\build.cmd Debug Win32`
- Smoke checklist:
  - expand a folder and confirm directories and files appear
  - confirm folders and files use different recognizable icons
  - select a folder node and confirm panel navigation still works
  - select a file node and confirm the active panel focuses the file
  - switch active panel and confirm the left tree rebinds cleanly
  - switch to archive/plugin FS and confirm mixed file nodes do not appear

---

## Parallel Execution Map

```text
Phase 1:
  T1 -> T2

Phase 2:
  T2 -> T3 -> T4

Phase 3:
  T4 -> T5 -> T6

Phase 4:
  T6 -> T7
```

## Recommended First Implementation Task

Start with `T1`.

Reason:

- the current `lParam` payload is too weak for mixed dir/file behavior
- typed payloads unlock both icon binding and selection branching cleanly
- it reduces risk before touching population and notification logic

## Assumed Tooling For Execution

Unless you direct otherwise, I will execute these tasks with:

- skill: `tlc-spec-driven`
- local repo editing via `apply_patch`
- repo inspection and build via `shell_command`

No external MCP or web dependency is required for this feature as currently scoped.
