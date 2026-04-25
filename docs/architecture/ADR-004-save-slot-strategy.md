# ADR-004: Save Slot Strategy — Single Auto-Save Slot with Version Guard

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Gaia Chronicles is a single-player narrative game. Save state consists of:
- All NSM flags and ints (the complete narrative progression)
- Gaia's world position and rotation
- Current level name
- Save timestamp and version number

The design question is: **how many save slots, what triggers them, and how is version
compatibility handled across game updates?**

Three strategies were evaluated:

| Option | Slots | Manual? | Pros | Cons |
|--------|-------|---------|------|------|
| A | 1 auto-save only | No | Maximum simplicity; no "load wrong save" confusion; aligns with narrative game conventions (Firewatch, Tacoma, What Remains) | Player cannot manually save at will; no multiple playthroughs without clearing data |
| B | 3 manual slots + 1 auto | Yes | Multiple playthroughs; player agency | More UI work; narrative games rarely benefit from save scumming; contradicts "consequences matter" design pillar |
| C | 1 auto-save + 1 "chapter start" checkpoint | No | Allows retry without full restart | Adds checkpoint logic complexity; chapter boundaries need definition |

---

## Decision

**Use Option A: single auto-save slot `"GaiaSlot_0"` with three auto-save triggers.**

A "New Game" function exists that clears this slot and calls `NSM.ResetAllState()`.
No manual save UI is exposed to the player.

**Auto-save triggers (from save-load GDD):**
1. `Fragment.X.Complete` flag written to NSM → `TrySave()` immediately
2. Gaia crosses an `AAreaTransitionTrigger` volume → `TrySave()`
3. `UDialogueSubsystem::EndDialogue(Complete)` → `TrySave()`

**Save guard — conditions that block `TrySave()`:**
- `bIsSaving == true` (async save already in progress)
- `UMemoryFragmentSubsystem::GetState() == InProgress` (mid-fragment; InProgress not persisted)
- `UMemoryFragmentSubsystem::GetState() == Transitioning_In/Out` (mid-transition)

**Save data structure:**
```cpp
UCLASS()
class UGaiasSaveData : public USaveGame {
    UPROPERTY() FGameplayTagContainer FlagStore;    // mirror of NSM FlagStore
    UPROPERTY() TMap<FGameplayTag, int32> IntStore; // mirror of NSM IntStore
    UPROPERTY() FVector GaiaLocation;
    UPROPERTY() FRotator GaiaRotation;
    UPROPERTY() FString CurrentLevelName;
    UPROPERTY() FDateTime SaveTimestamp;
    UPROPERTY() int32 SaveVersion;                  // starts at 1; increment on breaking changes
};
```

**Version compatibility rule:**
```
IsCompatible = (SaveVersion >= MinSupportedVersion) AND (SaveVersion <= CurrentVersion)
```
Initial values: `CurrentVersion = 1`, `MinSupportedVersion = 1`.
When a new release adds flags that older saves lack: if the flag absence is handled
gracefully by `HasFlag()` returning false (which it does), version can remain
compatible without incrementing. Only increment when old saves would produce
invalid/corrupt game states.

---

## Consequences

**Positive:**
- `USaveGame` + `AsyncSaveGameToSlot("GaiaSlot_0", 0, Callback)` is the canonical
  UE5 save pattern. No custom serialization; UE handles the binary encoding.
- Single slot means zero UI complexity for save management.
- Auto-save after every meaningful narrative event means players never lose progress.
- `bIsSaving` guard prevents concurrent save attempts from corrupting the slot.

**Negative / Mitigations:**
- No multiple playthroughs without manually clearing the slot from the main menu.
  **Mitigation:** Main menu has a "New Game / Erase Data" confirmation flow. This is
  standard for narrative games and is not a design regression.
- Save file corruption (e.g., mid-write crash) produces an unplayable slot.
  **Mitigation:** `AsyncLoadGameFromSlot` failure → `ImportState(EmptySaveData)` →
  start from scratch; display "Save data could not be loaded" message.
- `TMap<FGameplayTag, int32>` serialization via `UPROPERTY()` relies on UE's standard
  property serialization. **Mitigation:** Confirmed stable for `TMap` of `FGameplayTag`
  keys in UE5.x.

---

## ADR Dependencies

- ADR-001 (NSM): Save mirrors NSM state via `ExportState()/ImportState()`.

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| `USaveGame` | UE 4.x | ✅ LOW | Stable |
| `AsyncSaveGameToSlot` | UE 4.14 | ✅ LOW | Unchanged in 5.7 |
| `AsyncLoadGameFromSlot` | UE 4.14 | ✅ LOW | Unchanged in 5.7 |
| `TMap<FGameplayTag, int32>` UPROPERTY serialization | UE 5.x | ✅ LOW | Standard property serialization |

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-save-001 | USaveGame subclass FGaiasSaveData mirroring NSM state |
| TR-save-002 | AsyncSaveGameToSlot / AsyncLoadGameFromSlot (non-blocking) |
| TR-save-003 | Auto-save on: fragment complete, area transition, dialogue end |
| TR-save-004 | Version compatibility: SaveVersion in [MinSupported, Current] |
| TR-save-005 | GaiaLocation + CurrentLevelName persisted in save |
| TR-save-006 | Fragment InProgress state blocks save trigger |
