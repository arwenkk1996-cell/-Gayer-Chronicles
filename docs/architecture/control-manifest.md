# Control Manifest

> **Engine**: Unreal Engine 5.7
> **Last Updated**: 2026-04-25
> **Manifest Version**: 2026-04-25
> **ADRs Covered**: ADR-001, ADR-002, ADR-003, ADR-004, ADR-005, ADR-006, ADR-007, ADR-008
> **Status**: Active — regenerate with `/create-control-manifest` when ADRs change

`Manifest Version` is the date this manifest was generated. Story files embed this date.
`/story-readiness` compares a story's embedded version to this field to detect stories
written against stale rules. For the *reasoning* behind each rule, see the referenced ADR.

---

## Foundation Layer Rules

*Applies to: Narrative State Machine, Save/Load System, Level Streaming*

### Required Patterns

- **Use `UGameInstanceSubsystem` as the host for `UNarrativeStateSubsystem`.** It persists across level transitions and is auto-lifecycle managed by the GameInstance. — source: ADR-001
- **Use `FGameplayTagContainer` for the NSM FlagStore and `TMap<FGameplayTag, int32>` for the IntStore.** No other data structures for narrative state. — source: ADR-001
- **Declare all GameplayTag names in `Config/DefaultGameplayTags.ini` and in `Source/GaiaChronicles/Public/NarrativeTags.h`** using `UE_DECLARE_GAMEPLAY_TAG_EXTERN`. Never type tag strings inline in gameplay code. — source: ADR-001
- **Provide `UNarrativeStateBlueprintLibrary::GetNSM(UObject* WorldContext)` as a static helper** to reduce subsystem lookup boilerplate. — source: ADR-001
- **Use `AsyncSaveGameToSlot("GaiaSlot_0", 0, Callback)` for all saves.** Single slot, auto-save only. — source: ADR-004
- **Guard `TrySave()` with three conditions before executing:** `bIsSaving == false`, `FragmentState != InProgress`, `FragmentState != Transitioning_In/Out`. — source: ADR-004
- **Store fragment-tag-to-level-path mapping in `DT_FragmentLevels` DataTable.** Never hardcode level paths in C++. — source: ADR-003
- **Use `ULevelStreamingDynamic::LoadLevelInstance` for all Memory Fragment scene loading.** Each fragment is a standalone `.umap` in `Content/Levels/Fragments/`. — source: ADR-003

### Forbidden Approaches

- **Never cache NSM state locally in any feature system.** Always call `NSM.HasFlag()` / `NSM.GetInt()` live. A cached bool is a stale lie. — source: ADR-001
- **Never use `UGameplayStatics::LoadStreamLevel()`.** It is deprecated in UE5. Use `ULevelStreamingDynamic::LoadLevelInstance` instead. — source: ADR-003, deprecated-apis.md
- **Never use World Partition Data Layers for Memory Fragment scene loading.** Data Layers are designed for streaming world regions, not discrete enter/exit scene transitions. — source: ADR-003
- **Never trigger `TrySave()` while a Memory Fragment is InProgress or Transitioning.** Fragment InProgress state is not persisted — saving mid-fragment creates an unresolvable reload state. — source: ADR-004
- **Never expose a manual save UI to the player.** Auto-save only. No "Save Game" menu item. — source: ADR-004

### Performance Guardrails

- **Save I/O is always async.** `AsyncSaveGameToSlot` only — never block the game thread on save. — source: ADR-004

---

## Core Layer Rules

*Applies to: Character Controller, Camera System, Visual Transition System*

### Required Patterns

- **Use Enhanced Input (`UEnhancedInputComponent`, `UEnhancedInputLocalPlayerSubsystem`) for all player input.** Create `UInputAction` assets and `UInputMappingContext` assets. — source: ADR-006, deprecated-apis.md
- **Use `USpringArmComponent` + `UCameraComponent` attached to `AGaiaCharacter` for all camera logic.** Managed by a thin `UGaiaCameraComponent` wrapper class. — source: ADR-006
- **Camera has exactly five states: `Default`, `Narrow`, `Examine`, `Dialogue`, `Cinematic`.** All state transitions use `FMath::SmoothStep` to interpolate arm length and FOV. — source: ADR-006
- **Use `APlayerController::SetViewTargetWithBlend(Actor, 0.2f)` for dialogue camera transitions.** Never use hard-cut `SetViewTarget`. — source: ADR-006
- **Use four `APostProcessVolume` actors (one per `ETimeLayer`, all `bUnbound = true`) for time-layer visual profiles.** — source: ADR-007
- **Use one `UMaterialParameterCollection` asset (`MPC_TimeLayer`) with parameters `Saturation`, `ColorTint`, `GrainAmount` to drive material-level visual changes.** All Substrate environment materials must sample `MPC_TimeLayer`. — source: ADR-007
- **Interpolate `APostProcessVolume.BlendWeight` and `MPC_TimeLayer` parameters using `FMath::SmoothStep` in `UVisualTransitionSubsystem::TickComponent()`.** — source: ADR-007

### Forbidden Approaches

- **Never enable or use the `GameplayCameras` plugin (`UGameplayCameraComponent`).** It is experimental in UE 5.5–5.7 with documented API instability. This project uses `USpringArmComponent` only. — source: ADR-006
- **Never use legacy input bindings** (`InputComponent->BindAction("Jump", IE_Pressed, ...)`). They are deprecated in UE5.7 and replaced by Enhanced Input. — source: deprecated-apis.md
- **Never use `APlayerCameraManager` post-process settings as the sole time-layer visual driver.** Camera manager post-process does not propagate to material parameters. Always also update `MPC_TimeLayer`. — source: ADR-007
- **Never use Cascade particle systems.** All VFX must use Niagara. — source: deprecated-apis.md

### Performance Guardrails

- **PC target: 60fps (16.6ms frame budget).** Core layer systems must not exceed 4ms combined. — source: technical-preferences.md
- **Console target: 30fps (33.3ms frame budget), 4 GB VRAM.** Visual Transition post-process profiles must be validated on console target before Alpha milestone. — source: technical-preferences.md

---

## Feature Layer Rules

*Applies to: Memory Fragment System, Exploration/Interaction System, Dialogue System, Puzzle System*

### Required Patterns

- **Verify Yarn Spinner for Unreal UE5.7 compatibility before beginning the Dialogue System implementation sprint.** (Clone repo → build against UE5.7 → test custom command registration.) — source: ADR-002
- **Disable Yarn's internal variable storage entirely.** All dialogue state (node progress, flag writes, score changes) must flow through `UNarrativeStateSubsystem`. — source: ADR-002
- **Use `TWeakObjectPtr<UNarrativeStateSubsystem>` captured in lambdas for all Yarn custom command handlers.** Always call `.IsValid()` before dereferencing. — source: ADR-008
- **Convert Yarn command string arguments to `FGameplayTag` using `FGameplayTag::RequestGameplayTag(FName(*ArgString))`.** Log `UE_LOG(Warning)` if the tag is not found in the registry — do not crash. — source: ADR-008
- **All `IInteractable` puzzle elements must implement all six interface methods** (`GetInteractPriority`, `GetInteractHintText`, `OnInteract`, `OnExamine`, `OnFocusBegin`, `OnFocusEnd`). — source: architecture.md

### Forbidden Approaches

- **Never call `BeginFragment()` when `UMemoryFragmentSubsystem::GetState() != Idle`.** Only one fragment may be active at a time. — source: ADR-003, architecture.md
- **Never store dialogue state in Yarn's built-in variable storage.** Dual source of truth between Yarn and NSM will cause save/load desync. — source: ADR-002
- **Never use a raw `UNarrativeStateSubsystem*` pointer in Yarn command lambdas.** Use `TWeakObjectPtr` — raw UObject pointers can become dangling. — source: ADR-008
- **Fragment `InProgress` state is never persisted.** If the game saves during InProgress, it must be blocked. On load, InProgress reverts to `Available`. — source: ADR-004

### Performance Guardrails

- **Fragment sub-level scenes must be designed to stay within the present-world VRAM budget.** Both the present world and the fragment scene are loaded simultaneously during the visual transition. Keep fragment geometry small (1–2 room scale). — source: ADR-003

---

## Presentation Layer Rules

*Applies to: HUD/UI System, Audio System, VFX*

### Required Patterns

- **Use `UAudioComponent::FadeIn(Duration)` / `FadeOut(Duration, 0.f)` for all music stem transitions.** Linear interpolation for audio volume (not SmoothStep). — source: audio-system.md GDD
- **Use `UGameplayStatics::PushSoundMixModifier` / `PopSoundMixModifier` for Ducking.** Dialogue start pushes the dialogue duck modifier; dialogue end pops it. — source: audio-system.md GDD
- **Use `MetaSounds` for all procedural SFX** (footstep randomization, ambient event timing, fragment resonance synthesis). Sound Cue is permitted only for simple one-shot sounds. — source: ADR-005 engine ref
- **Use `UUserWidgetPool` for short-lived HUD notification elements** (interact hints, journal unlock notifications). Do not `CreateWidget` + `RemoveFromParent` every cycle. — source: hud-ui-system.md GDD
- **All player-visible text must go through `FText` and the localization pipeline.** No `FString` or string literal in any widget. — source: technical-preferences.md

### Forbidden Approaches

- **Never use Sound Cue nodes for complex audio logic.** Use MetaSounds. Sound Cue is legacy for simple cases only. — source: deprecated-apis.md
- **Never use `UGroomComponent` (Groom hair) until it has been benchmarked on console target hardware** (4 GB VRAM). Hair cards are the baseline through Alpha milestone. — source: ADR-005
- **Never use `APlayerCameraManager::PostProcessSettings` as the sole visual grade path.** (See Core layer rule — also applies to Presentation.) — source: ADR-007

### Performance Guardrails

- **Audio: maximum 32 active Sound Sources simultaneously on PC; 16 on console.** Enforce via Sound Concurrency assets on all frequently-played sounds. — source: audio-system.md GDD
- **VRAM ceiling: 8 GB (PC high), 4 GB (PC low / console target).** Groom hair is prohibited until the 4 GB budget is confirmed safe. — source: ADR-005, technical-preferences.md

---

## Global Rules (All Layers)

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Actor classes | `A` prefix + PascalCase | `AGaiaCharacter`, `AMemoryArtifact` |
| UObject classes | `U` prefix + PascalCase | `UNarrativeStateSubsystem`, `UInteractionComponent` |
| Structs | `F` prefix + PascalCase | `FJournalEntry`, `FFragmentResult` |
| Enums | `E` prefix + PascalCase | `ETimeLayer`, `EFragmentState` |
| Interfaces | `I` prefix + PascalCase | `IInteractable` |
| Boolean variables | `b` prefix + PascalCase | `bIsSaving`, `bHasReveal` |
| Non-bool variables | PascalCase | `MoveSpeed`, `CurrentArmLength` |
| Functions | PascalCase | `BeginFragment()`, `TrySave()` |
| Blueprint assets | `BP_` prefix | `BP_GaiaCharacter` |
| DataTable assets | `DT_` prefix | `DT_JournalEntries`, `DT_FragmentLevels` |
| Material assets | `M_` / `MI_` / `MPC_` prefix | `M_RuinWall`, `MPC_TimeLayer` |
| C++ header constants | `UPPER_SNAKE_CASE` | `DEFAULT_ARM_LENGTH`, `HINT_TRIGGER_SECONDS` |

### Performance Budgets

| Target | Value | Source |
|--------|-------|--------|
| PC framerate | 60 fps | technical-preferences.md |
| PC frame budget | 16.6 ms | technical-preferences.md |
| Console framerate | 30 fps | technical-preferences.md |
| Console frame budget | 33.3 ms | technical-preferences.md |
| Draw calls / frame | < 2000 | technical-preferences.md |
| VRAM (PC high) | ≤ 8 GB | technical-preferences.md |
| VRAM (PC low / console) | ≤ 4 GB | technical-preferences.md |
| Active Sound Sources (PC) | ≤ 32 | audio-system.md GDD |
| Active Sound Sources (console) | ≤ 16 | audio-system.md GDD |

### Approved Libraries / Addons

| Library | Purpose | Condition |
|---------|---------|-----------|
| Yarn Spinner for Unreal | Dialogue script parsing and execution | **Conditional** — requires UE5.7 compatibility verification per ADR-002 before use |
| CommonUI plugin | Gamepad-aware UI input routing in HUD/dialogue widgets | Approved — production-ready in UE 5.7 |
| GameplayTags plugin | Type-safe narrative flag identifiers | Approved — enabled by default in UE5 |
| MetaSounds | Procedural audio | Approved — production-ready in UE 5.7 |
| Niagara | All particle/VFX | Approved — production-ready |

### Forbidden APIs (Unreal Engine 5.7)

These APIs are deprecated or rejected for this project:

| API | Reason | Replacement | Source |
|-----|--------|-------------|--------|
| `InputComponent->BindAction(FName, ...)` | Deprecated in UE5.7 | Enhanced Input `UEnhancedInputComponent::BindAction` | deprecated-apis.md |
| `InputComponent->BindAxis(FName, ...)` | Deprecated in UE5.7 | Enhanced Input | deprecated-apis.md |
| `APlayerController::GetInputAxisValue()` | Deprecated in UE5.7 | Enhanced Input Action Values | deprecated-apis.md |
| Legacy material nodes (Base Color/Metallic/Roughness graph) | Deprecated in UE5.7 | Substrate Slab / Substrate Blend nodes | deprecated-apis.md |
| `UGameplayStatics::LoadStreamLevel()` | Deprecated in UE5 | `ULevelStreamingDynamic::LoadLevelInstance` | deprecated-apis.md, ADR-003 |
| Cascade particle system (`UParticleSystemComponent`) | Fully deprecated | Niagara (`UNiagaraComponent`) | deprecated-apis.md |
| Sound Cue complex logic | Deprecated for complex use | MetaSounds | deprecated-apis.md |
| `TSharedPtr<T>` for UObjects | Unsafe GC interaction | `TObjectPtr<T>` (UPROPERTY) | deprecated-apis.md |
| UE4 World Composition | Deprecated | World Partition | deprecated-apis.md |
| Level Streaming Volumes | Deprecated | World Partition Data Layers | deprecated-apis.md |
| Old animation retargeting | Deprecated | IK Rig + IK Retargeter | deprecated-apis.md |
| `UGameplayCameraComponent` (Gameplay Camera System plugin) | **EXPERIMENTAL** — API instability in UE 5.5–5.7 | `USpringArmComponent` + `UCameraComponent` | ADR-006 |
| `UGroomComponent` hair (before benchmark) | VRAM risk on console target | Hair cards (static mesh + aniso shader) | ADR-005 |

### Cross-Cutting Constraints

- **No hardcoded narrative strings in C++.** Every player-visible string (dialogue, journal text, UI labels, interact hints) goes through `FText` and the localization pipeline. `TEXT("some text")` is only valid for logging, not UI display. — source: technical-preferences.md
- **No `TickComponent()` / `Tick()` for logic that can be event-driven.** Subscribe to delegates and use `FTimerHandle`. Reserve Tick for continuous interpolation (camera blend, visual transition) only. — source: technical-preferences.md
- **No Blueprint-only systems for core narrative state.** Every system that reads or writes `UNarrativeStateSubsystem` must have a C++ backing class. Blueprint may extend behavior, not own the state contract. — source: technical-preferences.md
