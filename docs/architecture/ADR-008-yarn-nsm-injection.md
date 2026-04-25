# ADR-008: Yarn Spinner → NSM Injection Pattern

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Yarn Spinner custom commands (`<<set_flag X>>`, `<<check_flag X>>`, `<<add_ending_score X N>>`)
must call into `UNarrativeStateSubsystem`. The problem:

- Yarn's command handler callbacks are typically plain C++ functions or lambdas
- `UNarrativeStateSubsystem` is a `UGameInstanceSubsystem` — a UObject that requires a
  `UWorld` context to retrieve via `GetGameInstance()->GetSubsystem<>()`
- If the custom command handler doesn't have a `UWorld*` reference, it cannot safely
  call into the subsystem
- We must avoid circular dependency: `UDialogueSubsystem` (which owns the Yarn runner)
  depends on NSM already; NSM must not depend on Dialogue

The naive approach — storing a raw pointer to NSM in a Yarn command class — is unsafe
(GC could collect the subsystem reference; raw pointers to UObjects are dangerous).

---

## Decision

**Use `UWorld*` context injection into the Yarn command dispatcher at Dialogue startup.**

When `UDialogueSubsystem::BeginDialogue()` is called, it passes `GetWorld()` into the
Yarn runner's command registration call. All custom commands are registered as lambdas
that capture `TWeakObjectPtr<UNarrativeStateSubsystem>`.

**Pattern:**

```cpp
// In UDialogueSubsystem::BeginPlay() or Initialize():

TWeakObjectPtr<UNarrativeStateSubsystem> WeakNSM =
    GetGameInstance()->GetSubsystem<UNarrativeStateSubsystem>();

// Register custom commands with the Yarn runner
YarnRunner->RegisterCommand(TEXT("set_flag"),
    [WeakNSM](TArray<FString> Args) {
        if (WeakNSM.IsValid()) {
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Args[0]));
            WeakNSM->SetFlag(Tag);
        }
    });

YarnRunner->RegisterCommand(TEXT("check_flag"),
    [WeakNSM](TArray<FString> Args) -> bool {
        if (WeakNSM.IsValid()) {
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Args[0]));
            return WeakNSM->HasFlag(Tag);
        }
        return false;
    });

YarnRunner->RegisterCommand(TEXT("add_ending_score"),
    [WeakNSM](TArray<FString> Args) {
        if (WeakNSM.IsValid()) {
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Args[0]));
            int32 Delta = FCString::Atoi(*Args[1]);
            WeakNSM->IncrementInt(Tag, Delta);
        }
    });
```

**Key properties of this pattern:**
- `TWeakObjectPtr<>` is safe against GC — if the subsystem is somehow destroyed, the
  `IsValid()` check prevents a crash (this should never happen in practice since
  `UGameInstanceSubsystem` outlives all actors/subsystems, but defensive code is correct)
- No circular dependency: `UDialogueSubsystem` holds a weak ref to NSM (already a
  declared dependency); NSM holds no reference to Dialogue
- Yarn's own variable storage is bypassed entirely — all state flows through NSM
- `FGameplayTag::RequestGameplayTag(FName)` converts the string argument from the `.yarn`
  script to a type-safe tag. **Precondition:** the tag must be declared in
  `DefaultGameplayTags.ini`; if not, `RequestGameplayTag` returns `FGameplayTag::EmptyTag`
  and the command silently no-ops with a `UE_LOG(Warning)`.

---

## Consequences

**Positive:**
- Clean separation: Yarn scripts contain only string tag names; the injection layer
  validates and converts them to typed `FGameplayTag` at runtime
- `TWeakObjectPtr` pattern is standard UE5 idiom for safe cross-subsystem references
- All NSM writes during dialogue are synchronous and immediate — no timing ambiguity

**Negative / Mitigations:**
- If a `.yarn` script references an undeclared tag, the command silently fails.
  **Mitigation:** Add a validation pass in `UDialogueSubsystem::BeginDialogue()` that
  iterates the yarn script's command list and verifies each referenced tag exists in
  the GameplayTag registry before executing. Log a warning if any are missing.
- The exact Yarn Spinner for Unreal command registration API is unverified (plugin
  verification per ADR-002). **Mitigation:** This injection pattern is architecturally
  correct regardless of the specific Yarn API shape; adapt the registration call syntax
  at implementation time.

---

## ADR Dependencies

- ADR-001 (NSM): injection targets NSM's public API
- ADR-002 (Yarn Spinner): this ADR assumes Yarn Spinner passes UE5.7 compat verification

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| `TWeakObjectPtr<T>` | UE 4.x | ✅ LOW | Standard safe pointer pattern |
| `FGameplayTag::RequestGameplayTag(FName)` | UE 4.x | ✅ LOW | Stable; returns EmptyTag for unknown names (graceful) |
| Lambda capture in UE5 C++ | UE 5.x | ✅ LOW | C++20, confirmed in UE 5.7 |
| Yarn Runner command registration | Yarn Spinner plugin | ⚠️ MEDIUM | Verify exact API at plugin integration time |

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-dlg-002 | Custom Yarn commands: set_flag, check_flag, add_ending_score wiring to NSM |
