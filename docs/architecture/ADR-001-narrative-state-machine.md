# ADR-001: Narrative State Machine — UGameInstanceSubsystem + GameplayTags

**Status:** Superseded — 引擎迁移至 Godot 4 (2026-04-25)
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Gaia Chronicles requires a single global store for all narrative state: fragment completion,
dialogue progress, puzzle outcomes, ending scores, and any boolean or integer flag that
influences the story. This store must:

- Survive level transitions (fragments load/unload sub-levels)
- Be accessible to every system (interaction, dialogue, puzzle, journal, save/load)
- Support delegate broadcasting so systems can react to state changes without polling
- Serialize cleanly for save/load
- Scale to potentially hundreds of flags without becoming a maintenance burden

The central design question is: **what host class, and what data structures?**

Three candidates were evaluated:

| Option | Host | Data | Pros | Cons |
|--------|------|------|------|------|
| A | `UGameInstanceSubsystem` | `FGameplayTagContainer` + `TMap<FTag,int32>` | Survives level loads; auto-lifecycle; GC-safe; no manual singleton boilerplate | Slightly harder to unit-test than plain C++ class |
| B | Plain singleton (global `GNarrativeState`) | `TSet<FName>` + `TMap<FName,int32>` | Simple access anywhere | Dangerous GC lifetime; manual cleanup; not UObject; harder to UPROPERTY |
| C | `AGameState` actor | Any UObject-compatible structure | Easily replicated (if multiplayer added) | Destroyed on level transition unless persistent level; overkill for single-player |

---

## Decision

**Use Option A: `UNarrativeStateSubsystem : UGameInstanceSubsystem`** with
`FGameplayTagContainer` for the flag store and `TMap<FGameplayTag, int32>` for the
integer store.

**GameplayTags plugin** is required (`Edit > Plugins > GameplayTags`, enabled by default
in UE5 game templates).

Flag naming convention (hierarchical dot-notation):
```
Fragment.<Area>.<Name>.Undiscovered  (implicit — flag absent means undiscovered)
Fragment.<Area>.<Name>.Available
Fragment.<Area>.<Name>.InProgress    (runtime-only; not persisted)
Fragment.<Area>.<Name>.Complete
Dialogue.<NPCTag>.<NodeID>.Done
Dialogue.<NPCTag>.<NodeID>.Partial
Puzzle.<Area>.<Name>.Available
Puzzle.<Area>.<Name>.Solved
Artifact.<Name>.Collected
Journal.<Category>.<Name>.Unlocked
Ending.Score.Restore                 (int store — IncrementInt only)
Ending.Score.Protect                 (int store)
Ending.Score.Release                 (int store)
NPC.<Tag>.LastNodeID                 (int store — Yarn dialogue node ID)
```

All GameplayTag names are declared in `Config/DefaultGameplayTags.ini`.

---

## Consequences

**Positive:**
- `UGameInstanceSubsystem` is created when the GameInstance starts and destroyed when
  the game exits. It never resets between level transitions — exactly what narrative state requires.
- `FGameplayTag` provides editor autocompletion, typo-proofing, and hierarchy browsing.
  `FGameplayTagContainer` is GC-safe and UPROPERTY-compatible.
- Delegate broadcasting (`FOnFlagSet`, `FOnIntChanged`) gives all listening systems
  zero-cost event-driven reaction without polling on Tick().
- `ExportState() / ImportState()` contract is minimal and well-defined for the save system.

**Negative / Mitigations:**
- `UGameInstanceSubsystem` requires `GetGameInstance()->GetSubsystem<UNarrativeStateSubsystem>()`
  from actors. **Mitigation:** Provide a `UNarrativeStateBlueprintLibrary` with static
  helpers (`GetNSM(UObject* WorldContext)`) to reduce boilerplate.
- Flag name discipline is critical — typos produce silent misses. **Mitigation:** All flag
  constants declared in a shared header `NarrativeTags.h` using
  `UE_DECLARE_GAMEPLAY_TAG_EXTERN`; no string literals in gameplay code.
- `TMap` is unordered — iteration order of IntStore is undefined. **Mitigation:** Not
  a problem; the map is only read by key, never iterated for display logic.

---

## ADR Dependencies

- None. This ADR has no upstream architectural dependencies; all other ADRs depend on it.

---

## Engine Compatibility

| API | UE Version Introduced | Risk | Notes |
|-----|----------------------|------|-------|
| `UGameInstanceSubsystem` | UE 4.26 | ✅ LOW | Stable; unchanged in 5.7 |
| `FGameplayTag`, `FGameplayTagContainer` | UE 4.15 | ✅ LOW | Stable; GameplayTags plugin default in UE5 |
| `TMap<FGameplayTag, int32>` | UE 4.x | ✅ LOW | Standard container; no changes |
| `DECLARE_MULTICAST_DELEGATE_*` | UE 4.x | ✅ LOW | Stable |
| `AsyncSaveGameToSlot` | UE 4.14 | ✅ LOW | Used by SaveLoadSubsystem downstream |

**Verified against:** `docs/engine-reference/unreal/VERSION.md` — no breaking changes
in UE 5.4–5.7 affect these APIs.

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-nsm-001 | UGameInstanceSubsystem as NSM host (persists across levels) |
| TR-nsm-002 | FGameplayTagContainer for FlagStore |
| TR-nsm-003 | TMap<FGameplayTag,int32> for IntStore |
| TR-nsm-004 | Delegates FOnFlagSet + FOnIntChanged broadcast per-write |
| TR-nsm-005 | ExportState()/ImportState(FSaveData) for save integration |
| TR-nsm-006 | Fragment 4-state flags |
| TR-nsm-007 | Ending score tags with argmax resolution |
