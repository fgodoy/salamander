# Portuguese Localization Specification

## Summary

Add Portuguese localization to Open Salamander using the existing translation pipeline, with explicit support for both Brazilian Portuguese (`pt-BR`) and European Portuguese (`pt-PT`).

## Problem

The repository already has a mature translation workflow based on `.slt` source archives, `.slg` language modules, and `LANGID`-driven language selection. It does not currently ship any Portuguese translation artifacts, even though the product already knows how to prefer an exact locale match and fall back to the primary language when needed.

Without first-class Portuguese support, Portuguese-speaking users must stay on English or another language, and maintainers have no defined product contract for how Brazilian and European Portuguese should coexist.

## How should this work?

1. Portuguese localization is added without inventing a new translation format or a new selection mechanism.
2. The product supports two Portuguese variants: Brazilian Portuguese and European Portuguese.
3. Each variant is represented by its own translation metadata and language identity.
4. Automatic language preference continues to use the current behavior:
   - exact locale match first
   - primary-language fallback second
   - English fallback after that
5. Manual language selection must let the user distinguish the two Portuguese variants clearly.
6. The translation assets for the core product and supported plugins continue to live in the standard translation structure already used by other languages.

## Goals

- Add Portuguese translation support without changing the existing translator architecture.
- Support both `pt-BR` and `pt-PT` as distinct language variants.
- Preserve current automatic language selection semantics based on exact `LANGID` match and primary-language fallback.
- Keep the maintenance workflow aligned with the current `.slt` and `.slg` process for the core product and plugins.

## Non-goals

- Replacing the Translator tool or redesigning `.slt`/`.slg`.
- Introducing a generic locale-management subsystem beyond the current `LANGID` behavior.
- Solving every style or wording divergence between Portuguese variants in this phase.
- Translating languages other than Portuguese as part of this feature.
- Changing the existing English fallback behavior for all languages.

## UX Decisions

- Brazilian Portuguese and European Portuguese are distinct user-facing language options, not aliases of one another.
- Automatic selection should feel predictable: exact locale first, broad Portuguese fallback second.
- Manual language selection should make the variant obvious enough that a Portuguese user never has to guess which option is Brazilian and which is European.
- The feature should reuse the current translation packaging model instead of adding a special Portuguese-only workflow.

## Scope Assumptions For MVP

- The MVP covers the main application language pack and the translation asset structure required to extend the same model to plugins.
- The MVP reuses English help content where localized help is unavailable, following the existing `HELPDIR` pattern already used by other translations.
- The MVP allows one Portuguese variant to be more complete than the other, as long as the product behavior stays correct and the incomplete state can still be represented through existing metadata.
- The MVP does not require a new UI for choosing regional variants; the current language selector remains the entry point.

## User Stories

### P1: Locale-aware Portuguese selection

**User Story**: As a Portuguese-speaking Windows user, I want Salamander to pick the Portuguese variant that matches my system locale so that I get the expected wording without manual correction.

**Why P1**: This is the core user-visible value. If the product cannot distinguish `pt-BR` from `pt-PT`, the feature does not solve the actual localization problem.

**Acceptance Criteria**:

1. WHEN the available language packs include `pt-BR` and the user locale is Brazilian Portuguese THEN the system SHALL prefer the Brazilian pack over any other Portuguese pack.
2. WHEN the available language packs include `pt-PT` and the user locale is Portuguese from Portugal THEN the system SHALL prefer the European pack over any other Portuguese pack.
3. WHEN the user's exact Portuguese locale is not available but another Portuguese variant is available THEN the system SHALL fall back through the existing primary-language matching behavior instead of skipping directly to English.
4. WHEN no Portuguese language pack is available THEN the system SHALL keep the current non-Portuguese fallback behavior unchanged.

**Independent Test**: Put both Portuguese language packs in the normal language search location, run the language-selection flow under `pt-BR` and `pt-PT` user locales, and confirm the preferred language changes accordingly.

---

### P1: Standard Portuguese translation artifacts

**User Story**: As a maintainer, I want Portuguese translations to use the same `.slt` and `.slg` workflow as other languages so that the feature can be created, updated, and validated with the existing Translator process.

**Why P1**: A Portuguese feature that depends on ad hoc files or manual patching would break the repository's established translation model.

**Acceptance Criteria**:

1. WHEN Portuguese translation sources are created THEN the system SHALL represent Brazilian and European Portuguese as distinct language identities through the existing translation metadata model.
2. WHEN translation archives are stored in the repository THEN they SHALL follow the same `translations/<language>/...` structure already used by other languages.
3. WHEN a maintainer imports or exports Portuguese translations through the current Translator workflow THEN no Portuguese-specific alternative pipeline SHALL be required.
4. WHEN the main application translation defines help-directory or incomplete-translation metadata THEN it SHALL use the existing `HELPDIR` and `SLGINCOMPLETE` fields instead of introducing new metadata.

**Independent Test**: Create Portuguese translation archives for the main product through the same export/import flow used by another language and verify the result remains loadable by the current Translator tool.

---

### P2: Manual variant selection clarity

**User Story**: As a user who changes language manually, I want both Portuguese variants to be clearly identifiable in the language selector so that I can choose the correct one without trial and error.

**Why P2**: Automatic selection handles the common path, but manual selection must still be unambiguous.

**Acceptance Criteria**:

1. WHEN both Portuguese variants are present in the language selector THEN the system SHALL present them as distinct entries.
2. WHEN a user inspects the language list THEN the system SHALL expose enough language metadata for the user to distinguish Brazilian Portuguese from European Portuguese.
3. WHEN a user manually selects one Portuguese variant THEN the system SHALL persist and reuse that selected language pack according to the current language-selection behavior.

**Independent Test**: Open the language selector with both variants available and confirm the user can identify and select each variant separately.

---

### P2: Incremental completeness across core and plugins

**User Story**: As a maintainer, I want Portuguese translation coverage to grow incrementally across the core product and plugins so that we can ship value without waiting for every optional component to reach full parity on day one.

**Why P2**: The repository stores translation archives per component, and requiring instant parity across every plugin would make the feature harder to land.

**Acceptance Criteria**:

1. WHEN Portuguese is added for the main application before all plugins are translated THEN the system SHALL continue to behave correctly for untranslated components.
2. WHEN a Portuguese translation is incomplete THEN the system SHALL continue to use the existing incomplete-translation signaling model instead of failing the load.
3. WHEN plugin Portuguese translations are added later THEN they SHALL reuse the same per-component archive pattern already used by other languages.

**Independent Test**: Load a build where the main Portuguese pack exists but one or more plugin packs are absent, and confirm the application still loads language data without special-case failures.

## Edge Cases

- WHEN only `pt-BR` is available THEN the system SHALL still allow Portuguese users with another Portuguese locale to reach that variant through primary-language fallback.
- WHEN only `pt-PT` is available THEN the system SHALL still allow Portuguese users with another Portuguese locale to reach that variant through primary-language fallback.
- WHEN both variants are present but one is incomplete THEN the system SHALL still keep exact-locale preference while representing incompleteness through existing metadata.
- WHEN Portuguese help content is not localized THEN the system SHALL be able to reuse English help through the current help-directory mechanism.
- WHEN the user manually chooses one Portuguese variant that differs from the OS locale THEN the system SHALL keep honoring the user's explicit selection according to current language-selection behavior.
- WHEN plugin or optional-component Portuguese archives are missing THEN the system SHALL degrade through the current translation-loading behavior rather than introducing a Portuguese-specific failure mode.

## Acceptance Criteria

- The feature defines Brazilian Portuguese and European Portuguese as separate supported language variants.
- The feature reuses current `LANGID`-based selection behavior rather than introducing a new locale-selection mechanism.
- Exact locale match is preferred over primary-language fallback for Portuguese.
- Primary-language fallback allows one Portuguese variant to serve another Portuguese locale when the exact variant is unavailable.
- Portuguese translation artifacts fit into the existing repository translation structure and Translator workflow.
- Manual language selection can distinguish between the two Portuguese variants.

## Open Questions

- Which directory names should be canonical in `translations/` for the two variants: locale-shaped names such as `portuguese-brazil` / `portuguese-portugal`, or another naming convention that best matches existing repository practice?
- Should the first implementation ship both `pt-BR` and `pt-PT` with equal coverage, or should one variant be treated as the primary initial translation baseline and the other as a follow-up adaptation?
- Do we want Portuguese plugin translations in the first deliverable, or should the MVP land first with the main application pack and let plugin packs follow?

## Success Criteria

- Portuguese users can run Salamander in a Portuguese variant without manual file hacking.
- A `pt-BR` environment and a `pt-PT` environment do not default to the same pack when both variants exist.
- Maintainers can create and evolve Portuguese translation assets through the same workflow already used for other languages.
- The repository gains a clear feature contract for how Portuguese variants are added, selected, and extended over time.
