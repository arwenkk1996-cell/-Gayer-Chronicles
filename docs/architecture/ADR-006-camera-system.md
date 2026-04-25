# ADR-006: Camera System — USpringArmComponent + UCameraComponent

**Status:** Superseded — 引擎迁移至 Godot 4 (2026-04-25)
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Gaia Chronicles is a third-person narrative game. The camera system must support:
- Default exploration (spring arm follow cam)
- Narrow passage auto-adjust (shorter arm)
- Examine mode (pull back, slight FOV change)
- Dialogue composition (soft-cut to NPC framing)
- Fragment scenes (each fragment may have custom camera configured by level designers)
- Cinematic sequences (Sequencer takeover)

Two candidates were evaluated:

| Option | System | Status in UE5.7 | Notes |
|--------|--------|----------------|-------|
| A | `USpringArmComponent` + `UCameraComponent` | ✅ STABLE — UE4.x, unchanged | Battle-tested in thousands of shipped titles |
| B | `UGameplayCameraComponent` (Gameplay Camera System plugin) | ⚠️ EXPERIMENTAL — UE 5.5+ | New in UE 5.5; "Expect API changes in future versions" per engine reference; Blueprint support still improving |

**Engine reference verdict on Option B (from `plugins/gameplay-camera-system.md`):**
> "Limitations (Experimental Status): API Instability — Expect breaking changes in UE 5.8+.
> Limited Documentation — Official docs still evolving. Production Risk — Test thoroughly
> before shipping."

Our camera requirements — a follow cam with 5 discrete states and smooth blend between
them — are well within what `USpringArmComponent` handles natively. There is no feature
gap that requires the Gameplay Camera System.

---

## Decision

**Use Option A: `USpringArmComponent` (spring arm) + `UCameraComponent` on `AGaiaCharacter`,**
managed by a thin `UGaiaCameraComponent` helper class that owns the state machine and
arm/FOV interpolation logic.

The `UGameplayCameraComponent` plugin is **not enabled** for this project.

**Camera state machine:**

```cpp
UENUM(BlueprintType)
enum class EGaiaCameraState : uint8 {
    Default,     // arm=350cm, FOV=75
    Narrow,      // arm=180cm, FOV=75  (auto — triggered by passage geometry)
    Examine,     // arm=280cm, FOV=65  (triggered by hold-examine input)
    Dialogue,    // arm=level-configured, FOV=75, soft-cut to NPC side-view
    Cinematic,   // arm=N/A — Sequencer takeover; GaiaCameraComponent hands off
};
```

**Transition interpolation:**
```cpp
// Each Tick: blend arm length and FOV toward target state values
CurrentArmLength = FMath::SmoothStep(0.f, 1.f, BlendAlpha) * (TargetArm - SourceArm) + SourceArm;
CurrentFOV       = FMath::SmoothStep(0.f, 1.f, BlendAlpha) * (TargetFOV  - SourceFOV) + SourceFOV;
```

**Dialogue camera:**
- `BeginDialogueCam(NPCTag)` — soft-cut (0s) to pre-configured camera bone/socket on
  NPC actor. Camera stays behind Gaia's head, NPC at 3/4 angle. Implementation:
  `APlayerController::SetViewTargetWithBlend(NPCDialogueCamActor, 0.2f, VTBlend_Linear)`

**Cinematic takeover:**
- `ALevelSequenceActor::GetSequencePlayer()->Play()` — Sequencer moves the camera directly.
  `UGaiaCameraComponent` detects Sequencer activation and suppresses its own updates.

---

## Consequences

**Positive:**
- `USpringArmComponent::bDoCollisionTest = true` provides automatic arm shortening in
  tight spaces — the Narrow state triggers automatically via collision, no custom code.
- Spring arm lag (`bEnableCameraLag = true`, `CameraLagSpeed`) provides natural follow
  smoothing for free.
- Complete engine documentation; no knowledge gap risk.
- `UGaiaCameraComponent` is a thin wrapper (~200 lines) — straightforward to test and reason about.

**Negative / Mitigations:**
- Spring arm can clip in complex geometry. **Mitigation:** `bDoCollisionTest = true`,
  `ProbeSize = 12cm`, `bUsePawnControlRotation = false`. Known UE behaviour; handled
  via standard spring arm settings.
- Dialogue camera uses a hard-coded soft-cut rather than a smooth blend. **Mitigation:**
  0.2s `SetViewTargetWithBlend` provides sufficient transition feel per the GDD.

---

## ADR Dependencies

- None. Camera system has no cross-system state dependencies beyond reading GaiaCharacter transform.

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| `USpringArmComponent` | UE 4.x | ✅ LOW | Stable; unchanged in 5.7 |
| `UCameraComponent` | UE 4.x | ✅ LOW | Stable |
| `APlayerController::SetViewTargetWithBlend` | UE 4.x | ✅ LOW | Stable |
| `ALevelSequenceActor` / `ULevelSequencePlayer` | UE 4.x | ✅ LOW | Stable in 5.7 |
| `UGameplayCameraComponent` | ⚠️ UE 5.5 EXPERIMENTAL | ❌ REJECTED | Explicitly not used |

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-cam-001 | USpringArmComponent (350cm default) + UCameraComponent |
| TR-cam-002 | 5 camera states: Default / Narrow / Examine / Dialogue / Cinematic |
| TR-cam-003 | SmoothStep arm length + FOV transition per state change |
| TR-cam-004 | Dialogue camera soft-cut to NPC composition |
