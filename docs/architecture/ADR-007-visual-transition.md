# ADR-007: Visual Transition — Per-Layer Post-Process Volume + Material Parameter Collection

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

The Memory Fragment system transitions between four distinct time-layer visual styles:
- **Present**: Full Lumen color, warm orange tones, saturation 1.0
- **OldWorld**: Desaturated sepia, 4200K, film grain 0.3, saturation 0.45
- **Exodus**: Near-monochrome, cold blue, vignette heavy, saturation 0.15
- **Founding**: Warm yellow-gold, soft vignette, saturation 0.7

Each layer must:
1. Apply a distinct post-process look (color grading, vignette, film grain, depth of field)
2. Affect material appearance (desaturation should propagate to environment materials, not just a grade)
3. Transition smoothly between layers over 2–4 seconds via SmoothStep BlendWeight

Three implementation strategies were evaluated:

| Option | Mechanism | Post-Process | Materials | Notes |
|--------|-----------|-------------|-----------|-------|
| A | Per-layer `APostProcessVolume` (unbounded) + `UMaterialParameterCollection` | ✅ Via PPV settings | ✅ Via MPC scalar/vector params | Both systems stable; full art control |
| B | `APlayerCameraManager` post-process settings override | ✅ | ❌ Does not affect materials | Camera manager alone cannot push params to materials |
| C | Single `APostProcessVolume` with dynamic BlendWeight, material params via MPC | ✅ | ✅ | Same as Option A but fewer PPV actors in scene |

---

## Decision

**Use Option A: one `APostProcessVolume` per time layer (4 total, all Unbound = true) +
one `UMaterialParameterCollection` (`MPC_TimeLayer`)** that drives material-level
desaturation and color shift.

**Runtime mechanism:**
```
ETimeLayer changes →
  UVisualTransitionSubsystem::BeginTransitionTo(Target, Duration)
    ├─ All 4 PPVs fade out (BlendWeight → 0) via SmoothStep over Duration
    ├─ Target PPV fades in (BlendWeight 0 → 1) via SmoothStep
    └─ MPC_TimeLayer scalar "Saturation" and vector "ColorTint" interpolate toward
       target-layer values simultaneously
```

**PPV blend weight control:**
```cpp
// In UVisualTransitionSubsystem::Tick():
float Alpha = FMath::SmoothStep(0.f, 1.f, ElapsedTime / TransitionDuration);
for (APostProcessVolume* PPV : AllLayerPPVs) {
    PPV->BlendWeight = (PPV == TargetPPV) ? Alpha : (1.f - Alpha) * SourceLayerWeight;
}
// MPC update:
MPC->SetScalarParameterValue(TEXT("Saturation"), FMath::Lerp(SourceSat, TargetSat, Alpha));
MPC->SetVectorParameterValue(TEXT("ColorTint"), FMath::Lerp(SourceTint, TargetTint, Alpha));
```

**MPC_TimeLayer parameters (starting values per layer):**

| Layer | Saturation | ColorTint (RGB) | GrainAmount |
|-------|-----------|----------------|-------------|
| Present | 1.0 | (1.0, 0.95, 0.88) | 0.0 |
| OldWorld | 0.45 | (1.0, 0.92, 0.78) | 0.3 |
| Exodus | 0.15 | (0.85, 0.90, 1.0) | 0.5 |
| Founding | 0.7 | (1.0, 0.95, 0.80) | 0.0 |

All Substrate materials in fragment and present-world scenes sample `MPC_TimeLayer`
for their desaturation and tint, ensuring material-level visual coherence with the
post-process grade.

**Substrate note:** Substrate material nodes natively support scalar and vector
parameter sampling from an MPC. The `Substrate Slab` node accepts a "Desaturation"
input that directly drives color desaturation at the material level.
⚠️ **Verify exact Substrate MPC sampling node name** against UE 5.7 material editor
documentation at implementation time (Substrate is production-ready in 5.7 but
completely post-cutoff).

---

## Consequences

**Positive:**
- `APostProcessVolume.BlendWeight` is the canonical UE5 pattern for runtime post-process
  blending. No custom rendering code required.
- `UMaterialParameterCollection` propagates changes to all materials that reference it
  in a single call — no per-material-instance iteration.
- Art team can adjust each layer's PPV settings in editor without code changes.
- SmoothStep interpolation produces the "memory dissolving" visual feel described in the GDD.

**Negative / Mitigations:**
- 4 unbounded `APostProcessVolume` actors exist simultaneously. Only one has BlendWeight > 0
  at any given time (during the transition, two briefly overlap). **Mitigation:** UE5 PPV
  blending is GPU-side; multiple PPVs with low/zero weight have negligible cost.
- Substrate MPC sampling API is post-cutoff (HIGH risk domain). **Mitigation:**
  Implementation note — verify node name in live editor; fallback is Dynamic Material
  Instance parameter setting if MPC→Substrate integration differs from expectation.

---

## ADR Dependencies

- ADR-001 (NSM): `ETimeLayer` transitions triggered by `UMemoryFragmentSubsystem`, which
  reads NSM fragment tags to determine which layer to transition to.

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| `APostProcessVolume.BlendWeight` | UE 4.x | ✅ LOW | Stable |
| `UMaterialParameterCollection::SetScalarParameterValue` | UE 4.x | ✅ LOW | Stable |
| `UMaterialParameterCollection::SetVectorParameterValue` | UE 4.x | ✅ LOW | Stable |
| Substrate material MPC sampling | ⚠️ UE 5.7 | HIGH | Production-ready 5.7; verify exact node API at implementation |
| `FMath::SmoothStep` | UE 4.x | ✅ LOW | Stable |

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-vis-001 | 4 ETimeLayer post-process profiles |
| TR-vis-002 | BeginTransitionTo(ETimeLayer, float Duration) API |
| TR-vis-003 | SmoothStep BlendWeight interpolation over Duration |
| TR-vis-004 | APostProcessVolume + UMaterialParameterCollection for per-layer |
| TR-vis-005 | Queue: one pending, replace-on-overflow |
