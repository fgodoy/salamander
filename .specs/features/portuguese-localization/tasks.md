# Portuguese Localization Tasks

**Design**: `.specs/features/portuguese-localization/design.md`
**Spec**: `.specs/features/portuguese-localization/spec.md`
**Status**: Draft

---

## Execution Plan

### Phase 1: Main-App Asset Foundation (Sequential)

These tasks establish the Portuguese variant asset structure and main application metadata.

```text
T1 -> T2 -> T3
```

### Phase 2: Runtime Validation and Hardening (Mostly Sequential)

These tasks prove that the current runtime selection behavior already serves the Portuguese pair and only add code if the selector display is unclear.

```text
T3 -> T4 -> T5
```

### Phase 3: Incremental Ecosystem Extension (Can branch after validation)

After the main app path is working, plugin packs and hardcoded packaging lists can be extended in slices.

```text
       -> T6 -> T8
T5 -> |
       -> T7
```

### Phase 4: Verification and Project Hygiene (Sequential)

These tasks validate the shipping behavior and keep project documentation aligned.

```text
T5 -> T9 -> T10
```

---

## Task Breakdown

### T1: Create Portuguese Variant Directories and Main-App Archive Baseline

**What**: Create the repository structure for `pt-br` and `pt-pt` and add the main `salamand.slt` baseline in the standard per-language location.
**Where**: `translations/pt-br/`, `translations/pt-pt/`
**Depends on**: None
**Reuses**: `translations/czech/salamand.slt`, `translations/german/salamand.slt`, `translations/spanish/salamand.slt`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] `translations/pt-br/` exists.
- [ ] `translations/pt-pt/` exists.
- [ ] Each directory contains a main-application `salamand.slt` archive seed.
- [ ] The archive headers still follow the standard Translator export shape.

**Verify**:

- `Get-ChildItem translations\\pt-br`
- `Get-ChildItem translations\\pt-pt`
- `rg -n "^LANGID,|^AUTHOR,|^WEB,|^COMMENT,|^HELPDIR,|^SLGINCOMPLETE," translations/pt-br/salamand.slt translations/pt-pt/salamand.slt`

---

### T2: Set Variant-Specific Metadata for pt-BR and pt-PT

**What**: Give the two main-app archives distinct locale identity and metadata so the runtime can differentiate them through the existing pipeline.
**Where**: `translations/pt-br/salamand.slt`, `translations/pt-pt/salamand.slt`
**Depends on**: T1
**Reuses**: `LANGID`, `HELPDIR`, and `SLGINCOMPLETE` patterns already used in `translations/*/salamand.slt`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] `pt-BR` and `pt-PT` have distinct `LANGID` values.
- [ ] Author, web, and comment metadata are present in both files.
- [ ] `HELPDIR` is set consistently with the current main-app pattern when localized help is absent.
- [ ] Incomplete-state metadata uses the existing `SLGINCOMPLETE` contract instead of a new field.

**Verify**:

- `rg -n "^LANGID,|^HELPDIR,|^SLGINCOMPLETE," translations/pt-br/salamand.slt translations/pt-pt/salamand.slt`
- manual diff of the two headers to confirm they are not variant-collapsed

---

### T3: Build or Import Main Portuguese SLG Modules Through the Existing Translator Flow

**What**: Produce the main Portuguese `.slg` modules from the `.slt` sources without introducing any Portuguese-specific build path.
**Where**: Translator workflow output, then normal `lang/` deployment location
**Depends on**: T2
**Reuses**: `src/translator/`, current `.slt` import/export model, existing `.slg` deployment flow

**Tools**:

- MCP: NONE
- Local tools: `shell_command`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] A `pt-BR` main `.slg` can be generated from the existing Translator process.
- [ ] A `pt-PT` main `.slg` can be generated from the existing Translator process.
- [ ] The generated modules load as valid `.slg` files alongside the existing language packs.
- [ ] No special-case code path is required just to build Portuguese packs.

**Verify**:

- confirm Translator import/export succeeds for both variants
- confirm the generated `.slg` files appear in the normal `lang/` location
- `rg -n "SLGAuthor|SLGComment|SLGHelpDir|SLGIncomplete" src/salamdr2.cpp src/translator/trldata.cpp`

---

### T4: Validate Automatic Locale Selection for pt-BR and pt-PT

**What**: Verify that the current runtime selector already chooses the correct Portuguese pack by exact locale and then by primary-language fallback.
**Where**: runtime behavior driven by `src/dialogs2.cpp`, `src/salamdr2.cpp`
**Depends on**: T3
**Reuses**: `CLanguageSelectorDialog::GetPreferredLanguageIndex(...)`, `CLanguage::Init(...)`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] A `pt-BR` environment prefers the `pt-BR` pack when both variants exist.
- [ ] A `pt-PT` environment prefers the `pt-PT` pack when both variants exist.
- [ ] If one variant is absent, the other can still be reached via primary-language fallback.
- [ ] English fallback behavior remains unchanged when no Portuguese pack is available.

**Verify**:

- targeted runtime smoke under both locale settings
- `rg -n "GetPreferredLanguageIndex|GetUserDefaultUILanguage|PRIMARYLANGID" src/dialogs2.cpp`

---

### T5: Harden Manual Selector Display Only If Portuguese Variants Are Ambiguous

**What**: Add the smallest possible selector/UI hardening only if Windows locale naming does not make `pt-BR` and `pt-PT` sufficiently distinguishable in practice.
**Where**: `src/salamdr2.cpp`, `src/dialogs2.cpp`
**Depends on**: T4
**Reuses**: `CLanguage::GetLanguageName(...)`, existing language-list rendering in `CLanguageSelectorDialog::LoadListView()`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] No code change is made if the current selector already distinguishes both variants clearly enough.
- [ ] If a code change is needed, it is limited to display-layer naming or list rendering.
- [ ] Automatic selection logic remains unchanged.
- [ ] Manual selection clearly differentiates the two Portuguese variants.

**Verify**:

- manual language-selector smoke with both packs present
- `rg -n "GetLanguageName|LoadListView" src/salamdr2.cpp src/dialogs2.cpp`

---

### T6: Add Portuguese Plugin Translation Assets Incrementally

**What**: Extend Portuguese coverage from the main app to selected plugins using the same per-component archive pattern.
**Where**: `translations/pt-br/*.slt`, `translations/pt-pt/*.slt`
**Depends on**: T5
**Reuses**: current per-plugin archive naming seen across `translations/*/*.slt`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] At least one plugin archive is added for `pt-BR` through the normal translation structure.
- [ ] The same plugin archive pattern is usable for `pt-PT`.
- [ ] Plugin absence still degrades gracefully when a Portuguese plugin pack is not yet present.

**Verify**:

- `Get-ChildItem translations\\pt-br`
- `Get-ChildItem translations\\pt-pt`
- compare plugin filenames with another mature language directory

---

### T7: Update Hardcoded Packaging Lists That Need Portuguese Awareness

**What**: Patch only the explicit language-list scripts or auxiliary packaging assets that would otherwise omit Portuguese from a shipped surface.
**Where**: candidate files include `src/plugins/zip/vcxproj/selfextr/sfxmake.bat`, `src/plugins/zip/vcxproj/selfextr/makeall.bat`, and related self-extractor language assets
**Depends on**: T5
**Reuses**: existing per-language script entries and self-extractor language layout

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Every packaging flow touched in the current rollout includes Portuguese intentionally rather than accidentally omitting it.
- [ ] No unrelated language list is broadened without corresponding assets.
- [ ] The scope stays limited to the surfaces being shipped in the current wave.

**Verify**:

- `rg -n "czech|slovak|german|spanish|romanian|hungarian|russian|chinesesimplified|dutch|french" src/plugins/zip/vcxproj/selfextr src/plugins/zip/selfextr`
- review of changed language-list entries

---

### T8: Update Credits and Translation Inventory for Portuguese Variants

**What**: Align repo-facing attribution and translation inventory with the new Portuguese variant structure.
**Where**: `AUTHORS`, optionally `README.md` if the translation inventory section needs explicit mention
**Depends on**: T6
**Reuses**: existing translator credit format in `AUTHORS`

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Contributor attribution remains accurate after splitting Portuguese into explicit variants.
- [ ] The repository no longer implies a single undifferentiated Portuguese line if that is no longer true.

**Verify**:

- `Get-Content AUTHORS`
- `rg -n "Portuguese|Brazilian Portuguese|pt-br|pt-pt" AUTHORS README.md`

---

### T9: Validate End-to-End Main-App Behavior

**What**: Run the targeted smoke checklist that proves Portuguese behaves correctly in the real product, including fallback cases.
**Where**: runtime behavior
**Depends on**: T5
**Reuses**: existing language-selection dialog, startup language loading, help fallback, incomplete-translation signaling

**Tools**:

- MCP: NONE
- Local tools: `shell_command`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] Startup with `pt-BR` and `pt-PT` each resolves to the expected pack when both exist.
- [ ] Manual language selection is clear and stable.
- [ ] English help fallback works if localized help is absent.
- [ ] Incomplete-translation metadata does not break loading.
- [ ] Missing plugin packs do not crash or break main-app language loading.

**Verify**:

- smoke checklist:
  - start with both variants available and confirm locale-specific preference
  - remove one variant and confirm primary-language fallback to the remaining Portuguese pack
  - remove both and confirm non-Portuguese fallback remains unchanged
  - open manual language selector and confirm both variants are distinguishable
  - invoke help and confirm expected fallback behavior

---

### T10: Record Project Progress and Keep Planning Artifacts in Sync

**What**: Update roadmap and timeline entries to reflect whichever Portuguese localization slices were actually completed in the implementation wave.
**Where**: `.specs/project/ROADMAP.md`, `README.md`
**Depends on**: T9
**Reuses**: existing project timeline and roadmap format

**Tools**:

- MCP: NONE
- Local tools: `shell_command`, `apply_patch`
- Skill: `tlc-spec-driven`

**Done when**:

- [ ] `ROADMAP.md` reflects the completed Portuguese-localization work accurately.
- [ ] `README.md` timeline records the completed Portuguese-localization milestone(s).
- [ ] The entries describe what actually shipped, not what was only planned.

**Verify**:

- `Get-Content .specs\\project\\ROADMAP.md`
- `Get-Content README.md`

---

## Parallel Execution Map

```text
Phase 1:
  T1 -> T2 -> T3

Phase 2:
  T3 -> T4 -> T5

Phase 3:
  T5 complete, then:
    ├── T6
    └── T7
  T6 -> T8

Phase 4:
  T5 -> T9 -> T10
```

## Recommended First Implementation Task

Start with `T1`.

Reason:

- it creates the repository contract for the two Portuguese lines
- it lets every later task reuse the same directory and asset assumptions
- it keeps the first code-free slice small and easy to verify

## Assumed Tooling For Execution

Unless you direct otherwise, I will execute these tasks with:

- skill: `tlc-spec-driven`
- local repo editing via `apply_patch`
- repo inspection and validation via `shell_command`

No external MCP or web dependency is required for the current feature scope.
