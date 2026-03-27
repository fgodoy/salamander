# Panel TreeView Tasks

**Design**: `.specs/features/panel-treeview/design.md`
**Spec**: `.specs/features/panel-treeview/spec.md`
**Status**: Approved

---

## Execution Plan

### Phase 1: Foundation (Sequential)

These tasks establish the command, texts, persisted settings, and configuration UI contract.

```text
T1 -> T2 -> T3
```

### Phase 2: Panel Infrastructure (Sequential with narrow scope)

These tasks add the panel-owned tree host and internal layout support.

```text
T3 -> T4 -> T5
```

### Phase 3: Behavior Integration (Mostly Sequential)

These tasks wire the tree to panel activation, path changes, and navigation.

```text
T5 -> T6 -> T7 -> T8
```

### Phase 4: Verification and Polish (Sequential)

These tasks validate the feature and lock in the edge-case behavior.

```text
T8 -> T9
```

---

## Task Breakdown

### T1: Define TreeView Command, Menu Text, and Accelerator

**What**: Add the new toggle command and expose it in the same command/text/resource layers used by existing `Options > Visible` items.
**Where**: `src/resource.rh2`, `src/menu4.cpp`, `src/texts.rh2`, `src/lang/texts.rc2`, `src/salamand.rc`
**Depends on**: None
**Reuses**: `CM_TOGGLETOPTOOLBAR`, `CM_TOGGLEBOTTOMTOOLBAR`, `IDS_MENU_OPT_VSB_*`, `IDA_MAINACCELS1`, `IDA_MAINACCELS2`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] `CM_TOGGLETREEVIEW` exists in the command-id layer.
- [x] A visible-menu text resource exists for the new item.
- [x] `menu4.cpp` includes the new item under `CML_OPTIONS_VISIBLE`.
- [x] A fixed built-in accelerator is assigned without conflicting with current bindings.

**Verify**:

- `rg -n "CM_TOGGLETREEVIEW|IDS_MENU_OPT_VSB_TREE" src`
- `rg -n "IDA_MAINACCELS1|IDA_MAINACCELS2" src/salamand.rc`

---

### T2: Persist TreeView Visibility and Width in Configuration

**What**: Add persistent config fields for tree visibility and width, with defaults plus load/save support.
**Where**: `src/cfgdlg.h`, `src/dialogs4.cpp`, `src/mainwnd2.cpp`
**Depends on**: T1
**Reuses**: `TopToolBarVisible`, `BottomToolBarVisible`, `HotPathsBarVisible`, `DriveBarVisible`, `DriveBarWidth`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] `Configuration.TreeViewVisible` exists and has a startup default.
- [x] `Configuration.TreeViewWidth` exists and has a startup default.
- [x] Both values are written to persistent storage.
- [x] Both values are read back during startup and used to reconstruct state.

**Verify**:

- `rg -n "TreeViewVisible|TreeViewWidth" src/cfgdlg.h src/dialogs4.cpp src/mainwnd2.cpp`

---

### T3: Add TreeView Checkbox to Configuration UI

**What**: Add a configuration checkbox bound to the same persisted visibility flag used by the menu command.
**Where**: `src/lang/lang.rh`, `src/lang/lang.rc`, `src/dialogs5.cpp`
**Depends on**: T2
**Reuses**: `CCfgPagePanels::Transfer`, existing checkbox controls in `IDD_CFGPAGE_PANELS`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] The Panels configuration page contains a checkbox for the tree.
- [x] The checkbox maps to `Configuration.TreeViewVisible`.
- [x] The checkbox does not introduce layout overlap in `IDD_CFGPAGE_PANELS`.

**Verify**:

- `rg -n "TreeViewVisible|IDC_.*TREE" src/dialogs5.cpp src/lang/lang.rh src/lang/lang.rc`

---

### T4: Add Panel-Owned TreeView State and Layout Hooks

**What**: Extend `CFilesWindow` with the state needed to host a tree pane and reserve width inside the panel layout.
**Where**: `src/fileswnd.h`, `src/fileswn1.cpp`, `src/fileswnb.cpp`
**Depends on**: T3
**Reuses**: `DirectoryLineVisible`, `StatusLineVisible`, `HeaderLineVisible`, `LayoutListBoxChilds()`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] `CFilesWindow` contains explicit tree-related members.
- [x] Panel layout can reserve horizontal space for the tree pane.
- [x] The tree pane does not require changes to the global split-bar calculation.

**Verify**:

- `rg -n "HTreeView|TreeViewActive|TreeViewWidth" src/fileswnd.h src/fileswn1.cpp src/fileswnb.cpp`

---

### T5: Implement Tree Host Creation, Destruction, and Visibility

**What**: Create the actual Win32 TreeView host inside the panel and manage its lifecycle from panel visibility/state.
**Where**: `src/fileswnd.h`, `src/fileswn0.cpp`, `src/fileswn1.cpp`
**Depends on**: T4
**Reuses**: TreeView patterns from `src/dialogs5.cpp`, `src/common/sheets.cpp`, `src/translator/wndtree.cpp`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] The active panel can create and destroy the TreeView control safely.
- [x] Showing or hiding the pane updates panel layout correctly.
- [x] No tree control is leaked when the panel or main window is torn down.

**Verify**:

- `rg -n "WC_TREEVIEW|CreateWindowEx|DestroyWindow" src/fileswn0.cpp src/fileswn1.cpp`

---

### T6: Wire Main-Window Toggle Command to TreeView State

**What**: Handle the new command from menu and accelerator, and reflect checked state in the `Options > Visible` popup.
**Where**: `src/mainwnd.h`, `src/mainwnd1.cpp`, `src/mainwnd3.cpp`
**Depends on**: T5
**Reuses**: `ToggleTopToolBar()`, `ToggleDriveBar()`, `CML_OPTIONS_VISIBLE` checked-state logic

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] `WM_COMMAND` handles `CM_TOGGLETREEVIEW`.
- [x] The visible-menu checkmark reflects the real tree visibility state.
- [x] Toggle requests relayout instead of relying on stale window state.

**Verify**:

- `rg -n "CM_TOGGLETREEVIEW" src/mainwnd1.cpp src/mainwnd3.cpp`

---

### T7: Sync TreeView with Active Panel Changes

**What**: Make tree ownership and selection follow `ChangePanel()` and `FocusPanel()`.
**Where**: `src/mainwnd4.cpp`, `src/fileswnd.h`, `src/fileswn1.cpp`
**Depends on**: T6
**Reuses**: `ChangePanel()`, `FocusPanel()`, `UpdateDriveBars()`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Activating the other panel moves logical tree ownership to it.
- [x] The inactive panel no longer presents itself as tree-backed.
- [x] Tree selection expands to the newly active panel path.

**Verify**:

- `rg -n "ChangePanel|FocusPanel|Tree" src/mainwnd4.cpp src/fileswnd.h src/fileswn1.cpp`

---

### T8: Sync TreeView with Path Changes and Tree Navigation

**What**: Keep the tree synchronized when the active panel path changes, and let tree selection navigate the active panel back to disk paths.
**Where**: `src/fileswnd.h`, `src/fileswn0.cpp`, `src/fileswn1.cpp`, `src/fileswn2.cpp`, `src/fileswnb.cpp`
**Depends on**: T7
**Reuses**: `GetPath()`, `ChangePathToDisk()`, directory-line/path update flows

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Normal panel navigation updates tree selection.
- [x] Tree selection changes the active panel path.
- [x] Recursive re-entry is prevented when tree sync and path changes trigger each other.

**Verify**:

- `rg -n "ChangePathToDisk|GetPath|Tree" src/fileswnd.h src/fileswn0.cpp src/fileswn1.cpp src/fileswn2.cpp src/fileswnb.cpp`

---

### T9: Handle Non-Disk States and Run Build Verification

**What**: Finalize disabled/placeholder behavior for archive and plugin FS states, then validate the feature through build plus targeted smoke checklist.
**Where**: `src/fileswn0.cpp`, `src/fileswn1.cpp`, `src/fileswnb.cpp`, `src/vcxproj/salamand.sln`
**Depends on**: T8
**Reuses**: Existing panel type checks such as `ptDisk`, `ptZIPArchive`, `ptPluginFS`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [x] Non-disk modes keep the pane honest: disabled, empty, or placeholder only.
- [x] The main solution builds for at least one supported CI configuration.
- [ ] Manual smoke checklist is documented and executed for toggle, panel switch, and disk-path navigation.

**Verify**:

- `msbuild src/vcxproj/salamand.sln /m /p:Configuration=Debug /p:Platform=Win32`
- Smoke checklist:
  - show/hide tree from menu
  - show/hide tree from hotkey
  - flip active panel and confirm tree follows
  - navigate in tree and confirm active panel path changes
  - switch active panel to archive/plugin FS and confirm disabled/placeholder behavior

**Current validation note**:

- `src/vcxproj/build.cmd Debug Win32` resolves MSBuild, selects `MSVC 14.41.x` (`v143`), and the full `src/vcxproj/salamand.sln` build succeeds in the current Visual Studio instance.
- Manual smoke validation is still pending for menu toggle, accelerator toggle, panel handoff, tree navigation, and non-disk placeholder behavior.

---

## Parallel Execution Map

```text
Phase 1:
  T1 -> T2 -> T3

Phase 2:
  T3 -> T4 -> T5

Phase 3:
  T5 -> T6 -> T7 -> T8

Phase 4:
  T8 -> T9
```

## Recommended First Implementation Task

Start with `T1`.

Reason:

- it fixes the public command contract first
- it narrows the hotkey decision early
- it unlocks both config persistence and command handling without touching panel internals yet

## Assumed Tooling For Execution

Unless you direct otherwise, I will execute these tasks with:

- skill: `tlc-spec-driven`
- local repo editing via `apply_patch`
- repo inspection/build via `shell_command`

No external MCP or web dependency is required for this feature as currently scoped.
