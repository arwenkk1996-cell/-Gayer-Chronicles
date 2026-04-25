# ADR-003: Memory Fragment Level Loading — Level Instance via ULevelStreamingDynamic

**Status:** Superseded — 引擎迁移至 Godot 4 (2026-04-25)
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Each Memory Fragment is a fully separate scene set in a different time period, with its own:
- Visual environment (different geometry, lighting, post-process profile)
- Playable character (`AFragmentCharacter`) with its own controller
- Puzzle or observational tasks
- NPC actors unique to that historical moment

When the player touches an artifact in the present-day world, the fragment scene must load,
the player must be transferred into it, the fragment plays to completion, and then the scene
must unload cleanly while returning the player to the present world — all without a full
level reload or visible hitch.

Three loading strategies were evaluated:

| Option | Mechanism | Pros | Cons |
|--------|-----------|------|------|
| A | `ULevelStreamingDynamic::LoadLevelInstance` — each fragment is a separate sub-level | Clean isolation; fully async; stable API; complete per-fragment art/lighting control | Each fragment is a separate .umap file; requires level naming discipline |
| B | World Partition Data Layers — fragment content lives in the same map, toggled by Data Layer | No separate files; cleaner memory picture | Data Layers are designed for streaming world regions, not discrete "enter/exit" scenes; Possess/Unpossess lifecycle is awkward; harder to author per-fragment lighting |
| C | Full level transition (`OpenLevel`) — fragment is a fully independent level | Maximum isolation | Destroys GameInstance? No — `UGameInstanceSubsystem` survives level transitions in UE5; but transition overhead is high; no smooth visual transition during cross-fade |

---

## Decision

**Use Option A: `ULevelStreamingDynamic::LoadLevelInstance`** for each fragment scene.

Each fragment scene is authored as a standalone `.umap` sub-level file:
```
Content/Levels/Fragments/
  Fragment_Sourceholm_WaterGate_OldWorld.umap
  Fragment_Sourceholm_WaterGate_Founding.umap
  Fragment_MirrorCoast_WalkMarriage_Road.umap
  ...
```

The `UMemoryFragmentSubsystem` owns a `TMap<FGameplayTag, FSoftObjectPath>` mapping
fragment tags to level paths (configured in a `DT_FragmentLevels` DataTable).

**Loading flow:**
```cpp
// In UMemoryFragmentSubsystem::BeginFragment_Internal():
FSoftObjectPath LevelPath = FragmentLevelTable->FindRow<FFragmentLevelRow>(Tag)->LevelPath;
ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(
    GetWorld(),
    LevelPath.ToString(),
    FVector::ZeroVector,   // offset — fragment scenes have their own camera; world position irrelevant
    FRotator::ZeroRotator,
    bOutSuccess
);
StreamingLevel->OnLevelLoaded.AddDynamic(this, &UMemoryFragmentSubsystem::OnFragmentLevelLoaded);
```

**Unloading:**
```cpp
// In UMemoryFragmentSubsystem::CompleteFragment_Internal():
StreamingLevel->SetShouldBeLoaded(false);
StreamingLevel->SetShouldBeVisible(false);
// UE handles GC of the streaming level on next frame
```

The fragment scene runs at an arbitrary world offset (e.g., Z+50000cm) or at origin with
a separate sky/environment — isolating it from the present-day world visually.

---

## Consequences

**Positive:**
- `ULevelStreamingDynamic::LoadLevelInstance` is a stable, well-understood pattern used
  in production UE5 titles (documented in UE5.7 official docs).
- Complete visual isolation: fragment scene has its own `ADirectionalLight`, `ASkyAtmosphere`,
  `APostProcessVolume`, and ambient actors — no interference with present-world lighting.
- Per-fragment art team authorship: level designers work on each fragment .umap independently.
- `UNarrativeStateSubsystem` survives the sub-level load/unload (it lives on the GameInstance,
  not in the level). Fragment results write cleanly to NSM on completion.
- Async loading: visual transition animation plays during load, hiding any hitch.

**Negative / Mitigations:**
- Fragment scenes are hidden but the present-world level remains loaded in memory during
  the fragment. **Mitigation:** Fragment scenes are small (1-2 room-scale environments);
  memory impact is acceptable within the 8 GB VRAM budget.
- Level naming must be strictly followed to avoid DataTable mismatches.
  **Mitigation:** `DT_FragmentLevels` DataTable is the single source of tag→path mapping;
  `UMemoryFragmentSubsystem` validates the path exists on load and logs an error if not.
- The deprecated API note in `deprecated-apis.md`: `UGameplayStatics::LoadStreamLevel()`
  is deprecated; we use `ULevelStreamingDynamic::LoadLevelInstance` directly (the
  recommended replacement). ✅ No deprecated API used.

---

## ADR Dependencies

- ADR-001 (NSM): Fragment state written to NSM on complete; fragment tag lookup uses NSM flags.

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| `ULevelStreamingDynamic::LoadLevelInstance` | UE 5.0+ | ✅ LOW–MEDIUM | Replaces deprecated `UGameplayStatics::LoadStreamLevel`; confirmed stable in UE5.7 docs |
| `ULevelStreaming::OnLevelLoaded` | UE 4.x | ✅ LOW | Delegate; unchanged |
| `ULevelStreamingDynamic::SetShouldBeLoaded` | UE 4.x | ✅ LOW | Standard streaming API |
| `UDataTable` / `FindRow<T>` | UE 4.x | ✅ LOW | Stable |

⚠️ **OQ-ARCH-02 noted:** The exact `LoadLevelInstance` function signature may have changed
between UE 5.3 and UE 5.7. Verify the parameter list against live UE 5.7 source/docs at
implementation time. Fallback: use `FLatentActionInfo` + `UGameplayStatics` streaming
APIs if `LoadLevelInstance` signature differs.

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-frag-001 | UMemoryFragmentSubsystem for fragment lifecycle |
| TR-frag-002 | AMemoryArtifact: IInteractable initiates fragment |
| TR-frag-003 | Async Level Instance load/unload for each fragment scene |
| TR-frag-004 | Possess AFragmentCharacter; restore Gaia on complete |
| TR-frag-006 | Aborted state on error or force-quit |
| TR-frag-007 | World state changes applied 0.5s after return |
