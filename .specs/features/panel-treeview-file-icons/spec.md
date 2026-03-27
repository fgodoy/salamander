# Panel TreeView File Icons

## Summary

Extend the existing panel tree view so it can display directories and files with recognizable icons, making it easier to visually locate content in the tree. The experience should feel closer in spirit to terminal icon themes such as Terminal-Icons, but adapted to Salamander's native Win32 UI and navigation model.

## Problem

The current tree view is useful for directory navigation, but it is visually sparse and only helps with folders. Users cannot quickly scan for file types, distinguish file nodes at a glance, or use the tree as a pleasant visual locator for both folders and files.

## How should this work?

1. The tree view continues to live in the left-side host and follow the active panel.
2. Directory nodes keep their current navigation role and remain expandable.
3. Expanded directory nodes may contain both child directories and files.
4. Each node shows a small icon that makes the node type easy to recognize.
5. Directories use folder-style icons, with open/closed state when available.
6. Files use a type-appropriate icon based on the file itself or its extension.
7. File nodes are leaves and do not pretend to have child hierarchy.
8. Selecting a directory node keeps the current behavior: the active panel navigates to that directory.
9. Selecting a file node should help the user locate the file in the active panel, not silently do nothing.
10. The feature should stay visually pleasant and readable even in dense trees.

## Goals

- Make the tree visually useful for both folders and files.
- Improve recognition of node types through icons instead of text-only scanning.
- Let users locate files from the tree without abandoning the active-panel model.
- Reuse Salamander's existing icon infrastructure where possible.
- Keep the feature aligned with the current treeview host instead of inventing a separate navigator.

## Non-goals

- Reproducing the exact Terminal-Icons glyph set or style.
- Adding thumbnail previews inside the tree.
- Supporting archive and plugin-FS rich file trees in the first iteration.
- Replacing the main panel file list with the tree.
- Introducing a second independent icon pipeline if the existing shell/icon infrastructure can be reused.

## UX Decisions

- The tree remains a navigation-and-location surface, not a preview surface.
- File icons should be recognizable before they are decorative.
- Directories should appear before files within the same parent node unless a stronger existing panel convention must be preserved.
- File nodes should visually read as terminal leaves.
- Selecting a file node should move the active panel to the file's parent directory if needed and focus or select the file in the panel list.
- Double click or Enter behavior for file nodes should remain a follow-up decision unless the existing panel-default action can be reused cleanly.

## Scope Assumptions For MVP

- Disk paths only.
- Small icons only.
- No thumbnail rendering.
- No custom overlay icons in the first iteration unless they fall out naturally from existing shell icon retrieval.
- Hidden and filtered items should follow the active panel's effective visibility rules as closely as practical.

## Icon Expectations

- folders should show folder icons
- special file types should not all collapse into the same generic document icon when a better type icon is available
- extension-based file types should remain stable and recognizable
- shortcuts, links, and special shell-backed file types should degrade gracefully if exact overlays are not available

## Integration Expectations

- The feature should build on top of the current `panel-treeview` behavior rather than redefining tree ownership or host placement.
- The active panel remains the source of truth for path, visibility, and navigation.
- Existing Salamander icon logic for file panels is a preferred reuse source over ad hoc per-node icon handling.

## Edge Cases

- directories with very large child counts
- trees that contain many mixed file types
- files without extensions
- hidden or system files
- inaccessible directories during expansion
- UNC paths
- reparse points, shortcuts, and linked directories
- tree refresh after file creation, rename, move, or delete
- non-disk states after the tree was already populated

## Acceptance Criteria

- Expanded tree nodes can display both directories and files.
- Directories and files have different, recognizable icons.
- Selecting a directory node still navigates the active panel to that directory.
- Selecting a file node brings the active panel to the correct directory and highlights or focuses the file.
- File nodes do not appear expandable unless they genuinely represent a directory-like object.
- The tree remains readable and responsive on ordinary disk folders.
- Non-disk modes do not crash or expose misleading fake file hierarchies.

## Open Questions

- Should the tree show all files under expanded directories, or should file-node visibility be limited for very large folders?
- How strictly should the tree honor active panel filters, masks, and hidden-file settings?
- Should shell overlay icons be part of the first iteration or deferred?
- Should file activation in the tree stop at focus/selection, or should Enter/double click open the file using the panel-default action?
