# Prototype: Memory Fragment Transition

**Date**: 2026-04-25
**Status**: PROTOTYPE — NOT FOR PRODUCTION
**Question**: Does the concurrent level-load / visual-transition synchronization
handle fast AND slow level loads correctly, and does the SmoothStep PPV+MPC blend
produce the intended "world dissolving" feel at 2.5s?

---

## Files

| File | Purpose |
|------|---------|
| `Source/FragmentTransitionProto.h/.cpp` | State machine (5 states) + simulated async level load |
| `Source/VisualTransitionProto.h/.cpp` | SmoothStep PostProcess + MPC blend driver |
| `Source/TestArtifactProto.h/.cpp` | Overlap trigger → BeginFragment() actor |
| `REPORT.md` | Prototype findings and recommendation |

---

## Setup (5-10 minutes)

1. Create a new blank UE5 test level (`Proto_FragmentTest.umap`)

2. Drop these 4 files into your project's `Source/[ProjectName]/` folder.
   Add them to your `.Build.cs` if they don't compile automatically.

3. In the level, place:
   - `BP_TestArtifactProto` (from ATestArtifactProto) — the walkable trigger
   - `APostProcessVolume` × 2 (Unbound = true):
     - **PPV_Present**: Color Grading → Saturation = 1.0; no vignette; no grain
     - **PPV_OldWorld**: Color Grading → Saturation = 0.45; slight warm tint;
       Film Grain Intensity = 0.3; Vignette Intensity = 0.4
     - Set both starting BlendWeights: PPV_Present = 1.0, PPV_OldWorld = 0.0

4. Create a Blueprint subclass of `ATestArtifactProto`:
   - Add a `UVisualTransitionProto` component
   - In Event Graph, on `OnTransitioningIn_Started` (and every tick while Transitioning),
     set `VisualTransitionProto.AlphaInput = FragmentComponent.TransitionAlpha`
   - Assign `PPV_Present`, `PPV_OldWorld`, and `MPC_TimeLayer` in the Details panel

5. Create `MPC_TimeLayer` asset in Content Browser with parameters:
   - `Saturation` (scalar, default 1.0)
   - `ColorTint` (vector, default white)
   - `GrainAmount` (scalar, default 0.0)

6. **Test scenarios** (adjust values in BP Details panel):

   | Scenario | SimulatedLevelLoadTime | TransitionInDuration | Expected behavior |
   |----------|----------------------|---------------------|-------------------|
   | Level arrives early | 0.8s | 2.5s | No hold at apex; transition completes normally |
   | Level arrives at apex | 2.5s | 2.5s | Brief hold (≤1 frame) at apex |
   | Level arrives late | 4.0s | 2.5s | Holds at apex 1.5s; enters fragment when load completes |
   | Very fast transition | 1.0s | 1.0s | Test whether 1.0s feels too abrupt |
   | Slow transition | 4.0s | 4.0s | Test whether 4.0s feels too long |

7. Walk into the artifact's trigger sphere and observe.
   Check the Output Log for timing printouts from `[FragmentProto]`.
   Press E (bound to ExitFragment in your input mapping) to complete the fragment.

---

## What to Observe

**State machine correctness:**
- [ ] Does `BeginFragment()` fire exactly once per overlap?
- [ ] Does the state machine hold at the visual apex if the level is slow to load?
- [ ] Does `OnFragmentEntered()` fire exactly once (after both animation AND load complete)?
- [ ] Does the reverse transition (TransitioningOut) run cleanly back to Idle?

**Visual feel:**
- [ ] At 2.5s: does the SmoothStep ease-in/ease-out feel like "dissolving" or "fading"?
- [ ] Is there a noticeable "hold" at the visual apex when the level is late?
- [ ] Does the MPC Saturation change feel visually meaningful (1.0 → 0.45)?
- [ ] Does the GrainAmount (0.0 → 0.3) add texture without being distracting?

**Timing gut-check:**
After testing 2.5s, also try 2.0s and 3.5s and write your subjective response in REPORT.md.
