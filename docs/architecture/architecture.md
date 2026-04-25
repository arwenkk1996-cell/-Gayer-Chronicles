# Gaia Chronicles — Master Architecture

## Document Status

| Field | Value |
|-------|-------|
| **Version** | 1.0 |
| **Last Updated** | 2026-04-25 |
| **Engine** | Unreal Engine 5.7 |
| **GDDs Covered** | All 12 (7 MVP + 5 Vertical Slice) |
| **ADRs Referenced** | ADR-001 through ADR-008 (required — not yet written) |
| **Technical Director Sign-Off** | 2026-04-25 — APPROVED |
| **Lead Programmer Feasibility** | Lean mode — skipped |

---

## Engine Knowledge Gap Summary

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.7 |
| **LLM Training Covers** | up to approximately UE 5.3 |
| **Post-Cutoff Versions** | 5.4 (HIGH), 5.5 (HIGH), 5.6 (MEDIUM), 5.7 (HIGH) |

### HIGH RISK Domains

| Domain | Key Changes | GDD Systems Affected |
|--------|-------------|----------------------|
| **Substrate Materials** | Replaces legacy material system; new Substrate Slab/Blend nodes | Visual Transition System, Art Bible |
| **PCG Framework** | Production-ready, major API overhaul | Not currently used — skip |
| **Megalights** | New lighting system, millions of lights | All world environments |
| **Gameplay Camera System** | Experimental plugin (UE 5.5+), API unstable | Camera System |

### MEDIUM RISK Domains

| Domain | Key Changes | GDD Systems Affected |
|--------|-------------|----------------------|
| **Enhanced Input** | Now default; legacy bindings deprecated | Character Controller |
| **Nanite** | Now encouraged default for static meshes | All ruin environments |
| **World Partition** | UE4 Level Streaming deprecated | Level Streaming, Memory Fragment |

### LOW RISK Domains (stable, in training data)

- UGameInstanceSubsystem, UGameplayStatics, USaveGame — stable from 5.1+
- UUserWidget / UMG — stable; CommonUI available
- UAudioComponent, SoundMix, SoundClass — stable
- ACharacter, UCharacterMovementComponent — stable
- USpringArmComponent, UCameraComponent — stable
- IInterface pattern, FGameplayTag — stable (GameplayTags plugin)

### Critical Architectural Decision from Risk Analysis

> **⚠️ Gameplay Camera System is EXPERIMENTAL in UE 5.5–5.7** — "Expect API changes in
> future versions" (engine reference). Our camera requirements (4 states, spring arm
> distances, FOV changes) do not justify taking experimental API risk. Architecture
> mandates `USpringArmComponent + UCameraComponent` instead. See ADR-006.

---

## Technical Requirements Baseline

*Extracted from 12 GDDs — 72 total requirements*

| Req ID | GDD | System | Requirement | Domain |
|--------|-----|--------|-------------|--------|
| TR-nsm-001 | narrative-state-machine | NSM | UGameInstanceSubsystem as NSM host (persists across levels) | Foundation |
| TR-nsm-002 | narrative-state-machine | NSM | FGameplayTagContainer for FlagStore | Foundation |
| TR-nsm-003 | narrative-state-machine | NSM | TMap<FGameplayTag,int32> for IntStore | Foundation |
| TR-nsm-004 | narrative-state-machine | NSM | Delegates FOnFlagSet + FOnIntChanged broadcast per-write | Foundation |
| TR-nsm-005 | narrative-state-machine | NSM | ExportState()/ImportState(FSaveData) for save integration | Foundation |
| TR-nsm-006 | narrative-state-machine | NSM | Fragment 4-state flags: Undiscovered/Available/InProgress/Complete | Foundation |
| TR-nsm-007 | narrative-state-machine | NSM | Ending score tags Restore/Protect/Release with argmax resolution | Foundation |
| TR-exp-001 | exploration-interaction | Interaction | IInteractable C++ interface (abstract) | Core |
| TR-exp-002 | exploration-interaction | Interaction | UInteractionComponent on Gaia — sphere overlap 180cm radius | Core |
| TR-exp-003 | exploration-interaction | Interaction | FocusScore = 0.6×(1−NormAngle) + 0.4×(1−NormDist) | Core |
| TR-exp-004 | exploration-interaction | Interaction | Short press (<0.5s) vs hold (≥0.5s) input distinction | Core |
| TR-exp-005 | exploration-interaction | Interaction | Locked state during fragments blocks all interaction | Core |
| TR-frag-001 | memory-fragment | Fragment | UMemoryFragmentSubsystem for fragment lifecycle | Feature |
| TR-frag-002 | memory-fragment | Fragment | AMemoryArtifact: AStaticMeshActor + IInteractable | Feature |
| TR-frag-003 | memory-fragment | Fragment | Async Level Instance load/unload for each fragment scene | Feature |
| TR-frag-004 | memory-fragment | Fragment | Possess AFragmentCharacter; restore Gaia on complete | Feature |
| TR-frag-005 | memory-fragment | Fragment | FFragmentResult struct carries choice records back to NSM | Feature |
| TR-frag-006 | memory-fragment | Fragment | Aborted state on error or force-quit | Feature |
| TR-frag-007 | memory-fragment | Fragment | World state changes applied 0.5s after return to present | Feature |
| TR-vis-001 | visual-transition | VFX | 4 ETimeLayer post-process profiles | Core |
| TR-vis-002 | visual-transition | VFX | BeginTransitionTo(ETimeLayer, float Duration) API | Core |
| TR-vis-003 | visual-transition | VFX | SmoothStep BlendWeight interpolation over Duration | Core |
| TR-vis-004 | visual-transition | VFX | APostProcessVolume + UMaterialParameterCollection for per-layer | Core |
| TR-vis-005 | visual-transition | VFX | Queue: one pending, replace-on-overflow | Core |
| TR-ctrl-001 | character-controller | Character | ACharacter + UCharacterMovementComponent base | Core |
| TR-ctrl-002 | character-controller | Character | Enhanced Input System; no legacy bindings | Core |
| TR-ctrl-003 | character-controller | Character | 4 speeds: Walk(280) / Run(520) / ExamineWalk(160) / Vault(Montage) | Core |
| TR-ctrl-004 | character-controller | Character | Vault via AnimMontage + RootMotion (0.8s) | Core |
| TR-ctrl-005 | character-controller | Character | Speed = Lerp(Current, Target, AccelRate×DeltaTime) | Core |
| TR-ctrl-006 | character-controller | Character | Input suspended during fragments and dialogues | Core |
| TR-cam-001 | camera-system | Camera | USpringArmComponent (350cm default) + UCameraComponent | Core |
| TR-cam-002 | camera-system | Camera | 4 states: Default / Narrow / Examine / Fragment / Cinematic | Core |
| TR-cam-003 | camera-system | Camera | SmoothStep arm length transition per state change | Core |
| TR-cam-004 | camera-system | Camera | Dialogue camera soft-cut to NPC composition | Core |
| TR-save-001 | save-load | Persistence | USaveGame subclass FGaiasSaveData mirroring NSM state | Foundation |
| TR-save-002 | save-load | Persistence | AsyncSaveGameToSlot / AsyncLoadGameFromSlot (non-blocking) | Foundation |
| TR-save-003 | save-load | Persistence | Auto-save on: fragment complete, area transition, dialogue end | Foundation |
| TR-save-004 | save-load | Persistence | Version compatibility: SaveVersion in [MinSupported, Current] | Foundation |
| TR-save-005 | save-load | Persistence | GaiaLocation + CurrentLevelName persisted in save | Foundation |
| TR-save-006 | save-load | Persistence | Fragment InProgress state blocks save trigger | Foundation |
| TR-dlg-001 | dialogue-system | Dialogue | Yarn Spinner for Unreal plugin integration | Feature+ |
| TR-dlg-002 | dialogue-system | Dialogue | Custom Yarn commands: <<set_flag>>, <<check_flag>>, <<add_ending_score>> | Feature+ |
| TR-dlg-003 | dialogue-system | Dialogue | UDialogueSubsystem: BeginDialogue / EndDialogue lifecycle | Feature+ |
| TR-dlg-004 | dialogue-system | Dialogue | Max 4 options; unavailable options hidden (not greyed) | Feature+ |
| TR-dlg-005 | dialogue-system | Dialogue | NPC node persistence via NSM int store (NPC.Tag.LastNodeID) | Feature+ |
| TR-dlg-006 | dialogue-system | Dialogue | Camera state switch on dialogue begin/end | Feature+ |
| TR-dlg-007 | dialogue-system | Dialogue | Dialogue abort on fragment transition (writes .Partial flag) | Feature+ |
| TR-puzz-001 | puzzle-system | Puzzle | 4 puzzle types: observation/reconstruction/time-gated/contextual | Feature+ |
| TR-puzz-002 | puzzle-system | Puzzle | Per-puzzle state: Locked→Available→InProgress→Solved | Feature+ |
| TR-puzz-003 | puzzle-system | Puzzle | Puzzle.X.Solved tag written to NSM on completion | Feature+ |
| TR-puzz-004 | puzzle-system | Puzzle | All puzzle elements implement IInteractable | Feature+ |
| TR-puzz-005 | puzzle-system | Puzzle | Hint timer: 90s proximity, Gaia monologue via HUD | Feature+ |
| TR-puzz-006 | puzzle-system | Puzzle | InProgress state not persisted (resets to Available on load) | Feature+ |
| TR-jour-001 | journal-chronicle | Journal | FJournalEntry struct (7 fields) stored in DataTable | Feature+ |
| TR-jour-002 | journal-chronicle | Journal | Auto-unlock via FOnFlagSet + DataTable mapping | Feature+ |
| TR-jour-003 | journal-chronicle | Journal | RevealText trigger via second FOnFlagSet condition | Feature+ |
| TR-jour-004 | journal-chronicle | Journal | Double-page UMG widget (left: time axes, right: detail) | Feature+ |
| TR-jour-005 | journal-chronicle | Journal | Time axis position = (EntryIndex/TotalEntries) × AxisHeight | Feature+ |
| TR-jour-006 | journal-chronicle | Journal | 0.3s sequential unlock queue to prevent HUD stacking | Feature+ |
| TR-hud-001 | hud-ui | HUD | UUserWidget stack with ZOrder layering | Presentation |
| TR-hud-002 | hud-ui | HUD | Input mode arbitration: Cinematic > UIOnly > GameAndUI > GameOnly | Presentation |
| TR-hud-003 | hud-ui | HUD | All text via FText + localization pipeline (no hardcoded strings) | Presentation |
| TR-hud-004 | hud-ui | HUD | UUserWidgetPool for short-lived notification elements | Presentation |
| TR-hud-005 | hud-ui | HUD | Full HUD hide during fragment transitions + cinematics | Presentation |
| TR-hud-006 | hud-ui | HUD | APlayerController input device detection for icon switching | Presentation |
| TR-aud-001 | audio-system | Audio | 4-era layered Stem music, UAudioComponent per stem | Presentation |
| TR-aud-002 | audio-system | Audio | Stem crossfade 2.5s on ETimeLayer change | Presentation |
| TR-aud-003 | audio-system | Audio | SoundClass hierarchy: Master → Music / SFX / Voice | Presentation |
| TR-aud-004 | audio-system | Audio | SoundMix Modifier for Ducking (dialogue: Music.Exploration → 30%) | Presentation |
| TR-aud-005 | audio-system | Audio | MetaSounds for procedural SFX (footsteps, ambient events) | Presentation |
| TR-aud-006 | audio-system | Audio | UAudioSubsystem as audio state manager | Presentation |
| TR-aud-007 | audio-system | Audio | Sound Concurrency: ≤32 active sources (PC), ≤16 (console) | Presentation |

---

## System Layer Map

```
┌───────────────────────────────────────────────────────────────────────────┐
│  PRESENTATION LAYER                                                        │
│  ┌────────────────────────┐  ┌─────────────────────┐  ┌────────────────┐  │
│  │ HUD / UI System        │  │ Audio System         │  │ VFX (Niagara)  │  │
│  │ UUserWidget stack      │  │ UAudioSubsystem      │  │ Fragment VFX   │  │
│  │ CommonUI gamepad layer │  │ MetaSounds + SoundMix│  │ Puzzle solve   │  │
│  │ Input mode arbiter     │  │ 4-era Stem music     │  │ Transition     │  │
│  └────────────────────────┘  └─────────────────────┘  └────────────────┘  │
├───────────────────────────────────────────────────────────────────────────┤
│  FEATURE+ LAYER                                                            │
│  ┌──────────────────┐  ┌────────────────────┐  ┌──────────────────────┐   │
│  │ Dialogue System  │  │ Puzzle System       │  │ Journal/Chronicle    │   │
│  │ UDialogueSub     │  │ per-puzzle state    │  │ UJournalSubsystem    │   │
│  │ Yarn Spinner UE  │  │ IInteractable elems │  │ FJournalEntry + DT   │   │
│  │ Yarn→NSM cmds    │  │ hint timer          │  │ double-page UMG      │   │
│  └──────────────────┘  └────────────────────┘  └──────────────────────┘   │
├───────────────────────────────────────────────────────────────────────────┤
│  FEATURE LAYER                                                             │
│  ┌─────────────────────────────────┐  ┌──────────────────────────────┐    │
│  │ Memory Fragment System          │  │ Exploration / Interaction     │    │
│  │ UMemoryFragmentSubsystem        │  │ IInteractable interface       │    │
│  │ AMemoryArtifact                 │  │ UInteractionComponent (Gaia)  │    │
│  │ Level Instance async load       │  │ FocusScore selection          │    │
│  │ AFragmentCharacter possess      │  │ short-press vs hold-examine   │    │
│  └─────────────────────────────────┘  └──────────────────────────────┘    │
├───────────────────────────────────────────────────────────────────────────┤
│  CORE LAYER                                                                │
│  ┌─────────────────────┐  ┌───────────────────┐  ┌────────────────────┐   │
│  │ Character Controller│  │ Camera System      │  │ Visual Transition  │   │
│  │ AGaiaCharacter      │  │ USpringArmComp     │  │ UVisTransSubsystem │   │
│  │ CMC + Enhanced Input│  │ UCameraComponent   │  │ ETimeLayer enum    │   │
│  │ IMC context switch  │  │ 5 states + blending│  │ PPV + MPC blend    │   │
│  │ AnimMontage vault   │  │ Sequencer takeover │  │ SmoothStep interp  │   │
│  └─────────────────────┘  └───────────────────┘  └────────────────────┘   │
├───────────────────────────────────────────────────────────────────────────┤
│  FOUNDATION LAYER                                                          │
│  ┌──────────────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │ Narrative State Machine  │  │ Save / Load       │  │ Level Streaming │  │
│  │ UNarrativeStateSubsystem │  │ FSaveLoadSubsystem│  │ World Partition │  │
│  │ FlagStore + IntStore     │  │ FGaiasSaveData    │  │ Level Instances │  │
│  │ FOnFlagSet + FOnIntChgd  │  │ AsyncSave/Load    │  │ (fragment scenes│  │
│  │ ExportState/ImportState  │  │ auto-save triggers│  │  per-fragment)  │  │
│  └──────────────────────────┘  └──────────────────┘  └─────────────────┘  │
├───────────────────────────────────────────────────────────────────────────┤
│  PLATFORM LAYER                                                            │
│  UE 5.7: Lumen GI · Nanite Geometry · Substrate Materials                │
│  Enhanced Input · MetaSounds · Niagara VFX · World Partition              │
└───────────────────────────────────────────────────────────────────────────┘
```

### Layer Ownership Principles

1. **NSM is the only state truth.** Every system reads/writes through it. No system caches narrative state locally.
2. **Dependencies flow downward only.** A module may depend on any module in the same or lower layer. Never upward.
3. **Subsystems over singletons.** All manager classes extend `UGameInstanceSubsystem` (for narrative-critical systems) or `UWorldSubsystem` (for scene-scoped systems like audio).
4. **Interfaces at feature boundaries.** `IInteractable` and the Fragment state delegates are the only coupling points between Feature and Core layers.

---

## Module Ownership

### Foundation Layer

| Module | Class | Owns | Exposes | Consumes | Engine API (risk) |
|--------|-------|------|---------|----------|-------------------|
| Narrative State Machine | `UNarrativeStateSubsystem` : `UGameInstanceSubsystem` | FlagStore (`FGameplayTagContainer`), IntStore (`TMap<FGameplayTag,int32>`), fragment states, ending scores | `SetFlag`, `HasFlag`, `ClearFlag`, `IncrementInt`, `GetInt`, `FOnFlagSet`, `FOnIntChanged`, `ExportState`, `ImportState`, `ResetAllState` | Nothing — pure hub | `UGameInstanceSubsystem` (✅ LOW RISK), `FGameplayTag` (✅ plugin, stable) |
| Save / Load | `USaveLoadSubsystem` : `UGameInstanceSubsystem` | Save slot I/O, save version, bIsSaving flag | `TrySave(FName Slot)`, `TryLoad(FName Slot)`, `FOnSaveComplete` | `UNarrativeStateSubsystem::ExportState/ImportState`, `AGaiaCharacter` position | `USaveGame`, `AsyncSaveGameToSlot`, `AsyncLoadGameFromSlot` (✅ LOW RISK) |
| Level Streaming | `ULevelStreamingDynamic` (UE built-in) + per-fragment `ULevelStreamingDynamic*` managed by `UMemoryFragmentSubsystem` | Level lifecycle for fragment sub-levels | `LoadLevelInstance(FName LevelPath)`, `UnloadLevelInstance()`, `FOnLevelLoaded` | `UNarrativeStateSubsystem` (fragment tag → level path lookup via DataTable) | `ULevelStreamingDynamic::LoadLevelInstance` (✅ LOW–MEDIUM; ADR-003 confirms Level Instance vs World Partition) |

### Core Layer

| Module | Class | Owns | Exposes | Consumes | Engine API (risk) |
|--------|-------|------|---------|----------|-------------------|
| Character Controller | `AGaiaCharacter` : `ACharacter` | Movement state, active IMC, current animation | `SuspendInput()`, `ResumeInput()`, `GetMoveMode()`, `GetForwardVector()` | `UEnhancedInputLocalPlayerSubsystem` (IMC), `UCharacterMovementComponent` | `ACharacter`, `UCharacterMovementComponent`, `UEnhancedInputComponent` (✅ MEDIUM — now default in 5.7, confirmed stable) |
| Camera System | `UGaiaCameraComponent` (thin wrapper) + Spring Arm/Camera on `AGaiaCharacter` | Arm length, FOV, active camera state | `SetCameraState(EGaiaCameraState)`, `BeginDialogueCam(FGameplayTag NPCTag)`, `EndDialogueCam()` | `AGaiaCharacter` transform, `UDialogueSubsystem` events | `USpringArmComponent`, `UCameraComponent` (✅ LOW RISK — **explicitly chosen over experimental Gameplay Camera System**; see ADR-006) |
| Visual Transition | `UVisualTransitionSubsystem` : `UWorldSubsystem` | `ETimeLayer` current/pending, PPV blend weight | `BeginTransitionTo(ETimeLayer, float)`, `GetCurrentLayer()`, `FOnTimeLayerChanged` | Nothing — driven by `MemoryFragmentSubsystem` calls | `APostProcessVolume`, `UMaterialParameterCollection` (✅ LOW RISK), Substrate material parameters (⚠️ HIGH — Substrate production-ready 5.7, tested in Art Bible) |

### Feature Layer

| Module | Class | Owns | Exposes | Consumes | Engine API (risk) |
|--------|-------|------|---------|----------|-------------------|
| Interaction System | `UInteractionComponent` on `AGaiaCharacter` + `IInteractable` interface | Focused `IInteractable*`, overlap set | `OnFocusChanged(IInteractable*)` delegate, `GetCurrentFocus()` | `AGaiaCharacter` forward vector + position, `UNarrativeStateSubsystem` (locked check) | `USphereComponent` overlap (✅ LOW RISK) |
| Memory Fragment | `UMemoryFragmentSubsystem` : `UGameInstanceSubsystem`, `AMemoryArtifact`, `AFragmentCharacter` | `EFragmentState`, loaded level ref, active `AFragmentCharacter` | `BeginFragment(Tag)`, `CompleteFragment(Tag, FFragmentResult)`, `AbortFragment()`, `FOnFragmentStateChanged` | `UVisualTransitionSubsystem::BeginTransitionTo`, `ULevelStreamingDynamic` (load/unload), `UNarrativeStateSubsystem` (state write), `AGaiaCharacter::SuspendInput` | `APlayerController::Possess/UnPossess` (✅ LOW RISK), `ULevelStreamingDynamic` (MEDIUM — see ADR-003) |

### Feature+ Layer

| Module | Class | Owns | Exposes | Consumes | Engine API (risk) |
|--------|-------|------|---------|----------|-------------------|
| Dialogue System | `UDialogueSubsystem` : `UWorldSubsystem` | Active dialogue state, Yarn runner | `BeginDialogue(FGameplayTag NPC, FName NodeID)`, `EndDialogue(EDialogueEndReason)`, `FOnDialogueLineReady(FDialogueLineData)` | `UNarrativeStateSubsystem` (flag check/write), `UGaiaCameraComponent::BeginDialogueCam`, `AGaiaCharacter::SuspendInput` | Yarn Spinner for Unreal plugin (⚠️ MEDIUM — UE5.7 compat TBD; see ADR-002) |
| Puzzle System | `APuzzleActor` base class, per-puzzle level BP subclass | `EPuzzleState` per puzzle, hint timer | `GetPuzzleState(FGameplayTag)`, `SolvePuzzle(FGameplayTag)` (called by puzzle element BP) | `UNarrativeStateSubsystem` (prereq check, solved write), `IInteractable` on all elements | Standard actor/component (✅ LOW RISK) |
| Journal System | `UJournalSubsystem` : `UGameInstanceSubsystem` | `TArray<FJournalEntry>`, unlock queue | `OpenJournal()`, `CloseJournal()`, `GetEntriesForLayer(ETimeLayer)`, `FOnEntryUnlocked` | `UNarrativeStateSubsystem::FOnFlagSet`, DataTable `DT_JournalEntries` | `UDataTable`, `UUserWidget`, `SetGamePaused` (✅ LOW RISK) |
| Audio System | `UAudioSubsystem` : `UWorldSubsystem` | `EAudioGameState`, Stem `UAudioComponent[]`, SoundMix stack | `SetTimeLayer(ETimeLayer)`, `SetAudioState(EAudioGameState)`, `PlaySFX(FGameplayTag)` | `UVisualTransitionSubsystem::FOnTimeLayerChanged`, `UMemoryFragmentSubsystem::FOnFragmentStateChanged`, `UDialogueSubsystem::FOnDialogueStateChanged` | `UAudioComponent::FadeIn/FadeOut` (✅ LOW), `UGameplayStatics::PushSoundMixModifier` (✅ LOW), MetaSounds (✅ LOW — production-ready 5.7) |

### Presentation Layer

| Module | Class | Owns | Exposes | Consumes | Engine API (risk) |
|--------|-------|------|---------|----------|-------------------|
| HUD / UI System | `UHUDSubsystem` : `UWorldSubsystem` | Widget stack, active input mode, notification queue | `RequestInputMode(EHUDInputMode)`, `ShowInteractHint(FText)`, `HideInteractHint()`, `QueueNotification(FJournalEntry)`, `HideAll()` | Events from ALL feature systems via delegates; `APlayerController` device type | `UUserWidget`, `UUserWidgetPool` (✅ LOW), `APlayerController::SetInputMode` (✅ LOW), `CommonUI` (✅ MEDIUM — well-supported in 5.7) |

---

## Data Flow

### 1. Memory Fragment Entry Path

```
Player input (E key)
  │
  ▼
UInteractionComponent::Tick
  │  FocusScore selects AMemoryArtifact
  │
  ▼
AMemoryArtifact::OnInteract(Gaia)
  │
  ▼
UMemoryFragmentSubsystem::BeginFragment(FragmentTag)
  │  Checks NSM.HasFlag(Fragment.Tag.Available)
  │  Sets State = Transitioning_In
  │  → AGaiaCharacter::SuspendInput()
  │  → UHUDSubsystem::HideAll()                         ← event driven
  │  → UAudioSubsystem::SetAudioState(InFragment)        ← event driven
  │  → UVisualTransitionSubsystem::BeginTransitionTo(Layer, 2.5s)
  │
  ▼
ULevelStreamingDynamic::LoadLevelInstance(FragmentLevel)
  │  async — completion callback:
  │
  ▼
APlayerController::Possess(AFragmentCharacter)
  │  State = InFragment
  │
  ▼
[Player plays fragment scene]
  │
  ▼
UMemoryFragmentSubsystem::CompleteFragment(Tag, FFragmentResult)
  │  → NSM.SetFlag(Fragment.Tag.Complete)
  │  → NSM.IncrementInt(Ending.Score.X, delta)
  │  State = Transitioning_Out
  │  → UVisualTransitionSubsystem::BeginTransitionTo(Present, 2.0s)
  │
  ▼
APlayerController::Possess(AGaiaCharacter)
  │  → ULevelStreamingDynamic::UnloadLevelInstance()
  │  → AGaiaCharacter::ResumeInput()
  │  → UHUDSubsystem::RestoreHUD()
  │  → (0.5s timer) → ApplyWorldStateChanges(NSM flags)
  │
  ▼
State = Idle
  │
  ▼ [side-effects, driven by NSM FOnFlagSet]
UJournalSubsystem: hears Fragment.Tag.Complete → TryUnlockEntry
UHUDSubsystem: hears FOnEntryUnlocked → QueueNotification
USaveLoadSubsystem: hears Fragment.Tag.Complete → TrySave()
```

### 2. Dialogue Path

```
Player input (E on NPC)
  │
  ▼
ANPC::OnInteract() → UDialogueSubsystem::BeginDialogue(NPCTag, NodeID)
  │  NodeID = NSM.GetInt(NPC.Tag.LastNodeID) ?? DefaultNodeID
  │  → AGaiaCharacter::SuspendInput()
  │  → UGaiaCameraComponent::BeginDialogueCam(NPCTag)
  │  → UHUDSubsystem::RequestInputMode(GameAndUI)
  │  → UAudioSubsystem::SetAudioState(InDialogue)
  │
  ▼
Yarn Spinner runner parses .yarn file
  │  <<check_flag X>> → NSM.HasFlag(X)
  │  <<set_flag X>>   → NSM.SetFlag(X)
  │  <<add_ending_score X N>> → NSM.IncrementInt(Ending.Score.X, N)
  │
  ▼
UHUDSubsystem receives FOnDialogueLineReady(FDialogueLineData) → renders dialogue UI
  │
  ▼
Player selects option → Yarn advances → EndDialogue triggered
  │  → NSM.SetFlag(Dialogue.NPC.NodeID.Done)
  │  → NSM.SetInt(NPC.Tag.LastNodeID, nextNodeID)
  │  → UGaiaCameraComponent::EndDialogueCam()
  │  → AGaiaCharacter::ResumeInput()
  │  → UAudioSubsystem::SetAudioState(Exploring)
  │  → USaveLoadSubsystem: TrySave() (if auto-save trigger met)
```

### 3. Save Path

```
Trigger (one of three):
  - NSM FOnFlagSet where flag = Fragment.X.Complete
  - AGaiaCharacter enters area transition trigger volume
  - UDialogueSubsystem::EndDialogue()
  │
  ▼
USaveLoadSubsystem::TrySave()
  │  Guard: bIsSaving = false, NSM fragment state != InProgress
  │  bIsSaving = true
  │
  ▼
FGaiasSaveData data = NSM.ExportState()
  + GaiaLocation, GaiaRotation, CurrentLevelName, SaveTimestamp, SaveVersion
  │
  ▼
AsyncSaveGameToSlot("GaiaSlot_0", 0, Callback)
  │  non-blocking — Callback:
  │  bIsSaving = false
  │  FOnSaveComplete.Broadcast(bSuccess)
```

### 4. Initialisation Order

```
GameInstance::Init()
  ├─ UNarrativeStateSubsystem (auto-init as UGameInstanceSubsystem)
  ├─ USaveLoadSubsystem (auto-init)
  │   └─ AsyncLoadGameFromSlot → NSM.ImportState()
  └─ UJournalSubsystem (auto-init)

Level BeginPlay:
  ├─ UVisualTransitionSubsystem::Initialize() (UWorldSubsystem)
  ├─ UAudioSubsystem::Initialize() (UWorldSubsystem)
  ├─ UHUDSubsystem::Initialize() (UWorldSubsystem)
  │   └─ Creates widget stack, subscribes to all feature delegates
  ├─ AGaiaCharacter::BeginPlay()
  │   └─ UInteractionComponent starts sphere overlap
  │   └─ Enhanced Input IMC registered
  └─ Level actors (NPC, AMemoryArtifact, APuzzleActor) register IInteractable
```

### 5. Frame Update Path

```
Input (Enhanced Input) → AGaiaCharacter::Move(FInputActionValue)
  → CMC processes movement
  → UInteractionComponent::TickComponent()
      → sphere overlap query → FocusScore per candidate
      → FOnFocusChanged if winner changes
  → UHUDSubsystem hears FOnFocusChanged → update/hide interact hint
  → USpringArmComponent follows character (built-in)
  → UCameraComponent renders scene
```

---

## API Boundaries

All public contracts are defined here. These are the interfaces programmers code against.

```cpp
// ─── FOUNDATION ──────────────────────────────────────────────────────────────

// INVARIANT: NSM is the ONLY source of narrative truth.
// CALLER CONTRACT: Never cache NSM state locally; always query live.
UCLASS()
class UNarrativeStateSubsystem : public UGameInstanceSubsystem {
public:
    UFUNCTION(BlueprintCallable) void SetFlag(FGameplayTag Tag);
    UFUNCTION(BlueprintCallable) bool HasFlag(FGameplayTag Tag) const;
    UFUNCTION(BlueprintCallable) void ClearFlag(FGameplayTag Tag);
    UFUNCTION(BlueprintCallable) void IncrementInt(FGameplayTag Tag, int32 Delta);
    UFUNCTION(BlueprintCallable) int32 GetInt(FGameplayTag Tag) const;
    UFUNCTION(BlueprintCallable) void ResetAllState();

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnFlagSet, FGameplayTag);
    FOnFlagSet OnFlagSet;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIntChanged, FGameplayTag, int32);
    FOnIntChanged OnIntChanged;

    // Called only by USaveLoadSubsystem
    FSaveData ExportState() const;
    void ImportState(const FSaveData& Data);
};

// ─── INTERFACES ───────────────────────────────────────────────────────────────

// Every interactive object in the world MUST implement this.
// CALLER CONTRACT: Always call OnFocusEnd before calling OnFocusBegin on a new actor.
UINTERFACE(BlueprintType)
class IInteractable {
public:
    UFUNCTION(BlueprintCallable) virtual float GetInteractPriority() = 0;
    UFUNCTION(BlueprintCallable) virtual FText GetInteractHintText() = 0;
    UFUNCTION(BlueprintCallable) virtual void OnInteract(AGaiaCharacter* Gaia) = 0;
    UFUNCTION(BlueprintCallable) virtual void OnExamine(AGaiaCharacter* Gaia) = 0;
    UFUNCTION(BlueprintCallable) virtual void OnFocusBegin() = 0;
    UFUNCTION(BlueprintCallable) virtual void OnFocusEnd() = 0;
};

// ─── FEATURE ──────────────────────────────────────────────────────────────────

// INVARIANT: Only one fragment may be active (State != Idle) at any time.
// CALLER CONTRACT: Check GetState() == Idle before calling BeginFragment.
UCLASS()
class UMemoryFragmentSubsystem : public UGameInstanceSubsystem {
public:
    UFUNCTION(BlueprintCallable) void BeginFragment(FGameplayTag FragmentTag);
    UFUNCTION(BlueprintCallable) void CompleteFragment(FGameplayTag FragmentTag, FFragmentResult Result);
    UFUNCTION(BlueprintCallable) void AbortFragment();
    UFUNCTION(BlueprintCallable) EFragmentState GetState() const;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnFragmentStateChanged, EFragmentState);
    FOnFragmentStateChanged OnStateChanged;
};

// ─── FEATURE+ ─────────────────────────────────────────────────────────────────

// INVARIANT: Dialogue cannot begin while a fragment is active.
// CALLER CONTRACT: Only call BeginDialogue from IInteractable::OnInteract on NPC actors.
UCLASS()
class UDialogueSubsystem : public UWorldSubsystem {
public:
    UFUNCTION(BlueprintCallable)
    void BeginDialogue(FGameplayTag NPCTag, FName StartNodeID = NAME_None);
    // NAME_None = look up NSM.GetInt(NPC.Tag.LastNodeID) ?? DefaultStart

    UFUNCTION(BlueprintCallable)
    void EndDialogue(EDialogueEndReason Reason = EDialogueEndReason::Complete);

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueLineReady, FDialogueLineData);
    FOnDialogueLineReady OnDialogueLineReady;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueStateChanged, EDialogueState);
    FOnDialogueStateChanged OnStateChanged;
};

// ─── PRESENTATION ─────────────────────────────────────────────────────────────

// INVARIANT: Input mode is resolved by priority; highest active request wins.
// CALLER CONTRACT: Always call ReleaseInputMode when your system no longer needs it.
UCLASS()
class UHUDSubsystem : public UWorldSubsystem {
public:
    UFUNCTION(BlueprintCallable) void RequestInputMode(EHUDInputMode Mode);
    UFUNCTION(BlueprintCallable) void ReleaseInputMode(EHUDInputMode Mode);
    UFUNCTION(BlueprintCallable) void ShowInteractHint(FText HintText, UTexture2D* KeyIcon);
    UFUNCTION(BlueprintCallable) void HideInteractHint();
    UFUNCTION(BlueprintCallable) void QueueNotification(const FJournalEntry& Entry);
    UFUNCTION(BlueprintCallable) void HideAll();     // called by fragment system
    UFUNCTION(BlueprintCallable) void RestoreHUD();  // called after fragment return
};
```

---

## ADR Audit

No ADRs exist yet. All architecture decisions made in this document require formal ADRs before implementation begins.

| ADR | Engine Compat | Version Recorded | GDD Linkage | Valid |
|-----|--------------|-----------------|-------------|-------|
| (none) | — | — | — | — |

---

## Required ADRs

All decisions below must have written ADRs before any code touches the relevant layer.

### Foundation Layer (must write before any coding)

| Priority | ADR Title | TR IDs Covered | Key Decision |
|----------|-----------|----------------|--------------|
| 🔴 1 | ADR-001: NSM — UGameInstanceSubsystem + GameplayTags | TR-nsm-001..007 | Confirm subsystem choice, GameplayTags plugin, flag naming convention |
| 🔴 2 | ADR-003: Memory Fragment level loading — Level Instance vs World Partition | TR-frag-003 | `ULevelStreamingDynamic::LoadLevelInstance` vs World Partition Data Layer |
| 🔴 3 | ADR-004: Save slot strategy | TR-save-001..006 | Single slot "GaiaSlot_0" vs multi-slot, version numbering starting values |

### Core Layer (write before Core layer is built)

| Priority | ADR Title | TR IDs Covered | Key Decision |
|----------|-----------|----------------|--------------|
| 🟡 4 | ADR-006: Camera — SpringArm+Camera vs Gameplay Camera System | TR-cam-001..004 | Architecture mandates SpringArm (stable); ADR formally records rejection of experimental Gameplay Camera System |
| 🟡 5 | ADR-007: Visual Transition — Post-Process Volume blend technique | TR-vis-001..005 | Per-layer PPV with BlendWeight vs Material Parameter Collection as sole driver; Substrate material parameter strategy |

### Feature Layer (write before relevant system)

| Priority | ADR Title | TR IDs Covered | Key Decision |
|----------|-----------|----------------|--------------|
| 🟢 6 | ADR-002: Dialogue middleware — Yarn Spinner UE5.7 compatibility | TR-dlg-001..007 | Confirm Yarn Spinner for Unreal 5.7 compat (OQ-01 from dialogue GDD); fallback options |
| 🟢 7 | ADR-008: Yarn Spinner → NSM injection pattern | TR-dlg-002 | How custom Yarn Commands access UNarrativeStateSubsystem without circular dependency |
| 🟢 8 | ADR-005: Gaia hair — UE5 Groom vs static mesh hair cards | (Art Bible) | Performance vs fidelity; Groom performance on target hardware (Maine Coon long fur) |

---

## Architecture Principles

These five principles govern every technical decision in Gaia Chronicles:

1. **NSM is the single source of narrative truth.** No system stores narrative state locally. All story progress, fragment completion, and dialogue history lives exclusively in `UNarrativeStateSubsystem`. Programmers who want to check if something happened use `NSM.HasFlag()` — they do not maintain their own booleans.

2. **Events flow down, data flows up.** Feature systems broadcast events (fragments, dialogue ends, puzzle solves) upward to Presentation via delegates. Presentation systems never call down into Feature systems to push state. This keeps HUD/Audio as pure responders.

3. **InProgress is never persisted.** Fragment InProgress, Puzzle InProgress, and Dialogue InProgress are runtime-only states. If the game is saved mid-activity, those systems reset to their Available/Idle state on load. This eliminates a whole class of save corruption bugs.

4. **Stable engine APIs over experimental ones.** UE 5.7 ships an experimental Gameplay Camera System. This architecture explicitly chooses `USpringArmComponent + UCameraComponent` — thoroughly battle-tested — over the experimental system. The same principle applies to all engine choices: prototype on experimental features in `prototypes/`, ship on stable ones.

5. **All text through FText.** No hardcoded strings anywhere in C++. Every player-visible string (dialogue, journal text, interact hints, error messages) goes through the `FText` localization pipeline from day one. The Localization System (System 18) must find nothing to fix at Full Vision milestone.

---

## Open Questions / Deferred Decisions

| # | Question | Blocks | Resolve By |
|---|----------|--------|------------|
| OQ-ARCH-01 | Yarn Spinner for Unreal 5.7 compatibility — official confirmation needed. If incompatible: Articy Draft 3 or custom minimal dialogue tree. | ADR-002, TR-dlg-001 | Before Feature Layer build |
| OQ-ARCH-02 | `ULevelStreamingDynamic::LoadLevelInstance` API changed between 5.3→5.7 (deprecations table shows Level Streaming Volumes deprecated). Verify the sub-level loading pattern for fragment scenes on target engine. | ADR-003, TR-frag-003 | Before Feature Layer build |
| OQ-ARCH-03 | Gaia's Groom hair performance on console targets (4 GB VRAM budget). Must benchmark Groom vs hair cards on a representative scene before committing to Groom for production. | ADR-005 | Before Alpha |
| OQ-ARCH-04 | Are Journal entries stored in a DataTable asset (editor-configured) or loaded from JSON (external, writer-editable)? DataTable is simpler; JSON is more writer-friendly. | TR-jour-001..002 | Before Feature+ build |
| OQ-ARCH-05 | World Partition for main world vs standard persistent level + level streaming. The Sourceholm hub + 5 ruin areas could benefit from World Partition streaming, but adds complexity. | Level Streaming ADR-003 extended | Before Alpha level layout |

---

## Dependency Graph (Flat Reference)

```
NarrativeStateSubsystem ← read/write by: ALL systems
SaveLoadSubsystem        ← triggers: Fragment.Complete, AreaTransition, Dialogue.Done
                         ← serializes: NSM state + Gaia position
VisualTransitionSubsystem← driven by: MemoryFragmentSubsystem
                         ← signals: AudioSubsystem, HUDSubsystem (via events)
CharacterController      ← suspended by: MemoryFragmentSubsystem, DialogueSubsystem
InteractionSystem        ← locked by: MemoryFragmentSubsystem
MemoryFragmentSubsystem  ← triggers: VisualTransition, LevelStreaming, CharController suspend
                         ← completes: NSM writes, SaveTrigger, JournalUnlock
DialogueSubsystem        ← triggers: CharController suspend, CameraState, AudioDuck
                         ← completes: NSM writes, SaveTrigger
PuzzleSystem             ← reads: NSM prereqs; writes: NSM Puzzle.X.Solved
JournalSubsystem         ← reads: NSM via FOnFlagSet; writes: unlock queue → HUD
AudioSubsystem           ← listens: VisTransition, Fragment, Dialogue, Puzzle events
HUDSubsystem             ← listens: ALL feature layer events; owns: input mode arbitration
```
