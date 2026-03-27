# Panel TreeView

## Summary

Add an optional directory tree pane to the main Salamander window. The tree must be toggleable in the same way as other visibility-oriented UI elements, remember its visibility across sessions, and always reflect the currently active panel.

## Problem

The current two-panel workflow is efficient for file operations, but weak for deep hierarchical navigation. Users can change path through the directory line, drive bar, history, and commands, but they cannot see or traverse the parent/child directory structure directly inside the main window.

## How should this work?

1. The user enables or disables the tree from `Options > Visible`, from a checkbox in the configuration dialog, or from a fixed built-in hotkey.
2. The tree visibility is persisted in configuration and restored on the next start.
3. Only one logical tree is active at a time: it always follows the active panel.
4. When the active panel changes, the tree immediately rebinds to that panel's current path.
5. When the active panel path changes, the tree updates to the new location.
6. Selecting a directory in the tree navigates the active panel to that directory.
7. Showing the tree does not automatically steal keyboard focus from the file list. Focus moves to the tree only if the user clicks it or explicitly tabs into it.

## Goals

- Add a persistent TreeView-based navigation pane to the main window.
- Reuse the existing Salamander pattern for visible UI toggles.
- Keep the feature aligned with the active panel instead of introducing a third independent navigation model.
- Minimize architectural risk by reusing existing Win32 TreeView patterns already present in the codebase.

## Non-goals

- A generic user-configurable shortcut editor for the main application.
- Separate independently persistent trees for left and right panels.
- Full archive and plugin-FS hierarchy browsing in the first iteration.
- Reworking the global main-window rebar or toolbar architecture.

## Proposed MVP

- Add a new `Tree View` item under `Options > Visible`.
- Add a checkbox on the Panels-related configuration page bound to the same setting.
- Add a persisted boolean setting such as `Configuration.TreeViewVisible`.
- Add a fixed built-in accelerator for the command.
- Show the tree only for disk paths.
- If the active panel is in archive or plugin FS mode, keep the pane visible but show a disabled or empty informational state instead of pretending there is a directory hierarchy.
- Navigation from tree selection changes the active panel path using the same path-change flow used elsewhere in the panel.

## UX Decisions

- The feature should behave like an optional visibility-oriented pane, not like a modal tool window.
- The tree should be visually attached to the active panel area, because the requirement is panel-relative, not application-global.
- The tree should not silently represent the non-active panel. The user must always be able to infer: "this tree belongs to the panel that is active now."

## Configuration And Shortcut Scope

The repository already has a strong pattern for persisted visibility toggles, but it does not have a general-purpose, user-configurable shortcut system for core commands. For that reason:

- Configuration parity with other visible elements should be implemented now.
- A fixed built-in hotkey should be implemented in the first iteration.
- A user-editable hotkey should be treated as a separate follow-up feature, not part of this MVP.

## Edge Cases

- Active panel on a drive root.
- Active panel on a UNC path.
- Active panel on an inaccessible or disappearing directory.
- Active panel on archive or plugin FS content.
- Refresh after path changes triggered outside the tree itself.
- Panel activation changes while the tree is visible.

## Acceptance Criteria

- The user can toggle the tree from menu and shortcut.
- Tree visibility is saved and restored across sessions.
- The tree tracks the active panel after `ChangePanel` and `FocusPanel`.
- The tree tracks active-panel path changes triggered by normal panel navigation.
- Selecting a node in the tree changes the active panel directory.
- Non-disk paths do not crash or show misleading fake nodes.

## Open Questions

- Which fixed accelerator should be used without colliding with existing `IDA_MAINACCELS1` and `IDA_MAINACCELS2` mappings?
- Should the tree reserve a fixed width globally, or should width be stored and restored independently from the visible flag?
