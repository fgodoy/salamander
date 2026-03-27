# UI Modernization Spec

## Problem Statement

Open Salamander still delivers a strong keyboard-first file manager workflow, but the shell communicates a legacy Win32 product in its spacing, chrome, menu treatment, toolbar density, dialogs, and theme behavior. The modernization effort must improve perceived quality without sacrificing startup speed, panel performance, plugin compatibility, or the established two-panel interaction model.

## Why This Matters

- the current UI limits perceived product quality even when functionality remains strong
- visual inconsistency between shell surfaces makes new features feel bolted on
- the codebase already mixes standard common controls, custom menus, owner-drawn panels, and plugin UIs, which makes ad hoc refreshes risky without a coherent path
- future features such as richer preview, modern settings, command palette, or better accessibility depend on a clearer UI foundation

## Goals

- modernize the visual language of the app while preserving the current information density and keyboard-first workflow
- preserve the two-panel shell, core command model, and plugin ABI for the first modernization waves
- improve theme consistency, spacing, typography, iconography, surface hierarchy, and perceived responsiveness
- enable a phased rollout with low-regret checkpoints and rollback options
- identify a path that can realistically be implemented in this Win32 codebase

## Non-Goals

- rewriting the core file-operation engine
- forcing all plugins to migrate their UI in the first phase
- introducing cross-platform requirements
- committing to a full rewrite before an incremental path has been evaluated against real code constraints

## Constraints

- the main shell is a Win32 desktop monolith centered on `CMainWindow` and `CFilesWindow`
- panel rendering is heavily custom and owner-drawn rather than a thin wrapper around standard list views
- the menu bar and popup menus are custom-painted, not default Windows menus
- a large amount of behavior is tied directly to visual container classes and message handling
- plugin UIs are heterogeneous and cannot all be modernized at once

## Code Reality Anchors

- `src/mainwnd.h` and `src/mainwnd1.cpp` own global shell layout, split management, toolbar toggles, and color refresh
- `src/mainwnd3.cpp` creates the rebar-based top shell and wires command handlers
- `src/fileswnd.h` and `src/fileswn0.cpp` through `src/fileswn9.cpp` own panel composition, layout, and custom drawing
- `src/fileswn4.cpp` and `src/fileswn9.cpp` implement the owner-drawn panel item rendering pipeline
- `src/menu.h` implements a custom menu system with its own painting, state, and tracking behavior
- `src/toolbar*.cpp` implements the toolbar/rebar visual layer

## Decision Criteria

- low risk to shell stability and panel performance
- compatibility with the current plugin model
- ability to ship value incrementally
- ability to support theme improvements including dark-mode-friendly design later
- realistic engineering cost for this codebase size and age
- clear path to modern settings, dialogs, and optional richer surfaces

## Modernization Outcomes We Want

- cleaner shell chrome and clearer hierarchy between menu, toolbar, panels, and status surfaces
- updated spacing and typography without losing density
- more coherent selection, focus, hover, and active/inactive panel states
- modernized dialogs and settings surfaces
- a theme system that can drive future light/dark support rather than one-off color changes
- a modernization plan that explicitly separates quick wins from architectural moves

## Open Product Decisions

- how far should the first wave go: visual refresh only, shell refactor, hybrid surfaces, or full rewrite
- whether plugin UIs should remain visually mixed for a period or get a compatibility theming layer
- whether modern settings should remain Win32-native or move to a hybrid surface later
- whether a richer preview/dashboard experience is part of the modernization program or a later milestone
