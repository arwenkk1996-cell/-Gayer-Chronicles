// PROTOTYPE - NOT FOR PRODUCTION
// Question: Does the concurrent level-load / visual-transition synchronization
//           handle fast AND slow level loads without hitches or freezes?
// Date: 2026-04-25
//
// STRIPPED DOWN — no NSM, no DataTable, no save guards, no character possession.
// Simulates async level load with a configurable FTimerHandle delay.
// One hardcoded transition: Present → OldWorld.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FragmentTransitionProto.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// State machine — same 5 states as production UMemoryFragmentSubsystem
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EFragmentStateProto : uint8
{
    Idle,
    Transitioning_In,   // animation playing + level loading concurrently
    InFragment,         // both complete; player "inside" fragment
    Transitioning_Out,  // reverse animation + level unloading
    Aborted             // error path
};

// ─────────────────────────────────────────────────────────────────────────────
// Core prototype component — attach to a persistent Actor (e.g. GameMode BP)
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(ClassGroup=(Prototype), meta=(BlueprintSpawnableComponent))
class UFragmentTransitionProto : public UActorComponent
{
    GENERATED_BODY()

public:
    UFragmentTransitionProto();

    // ── Tunable values (tweak these in the Blueprint Details panel) ───────────

    // Duration of the visual transition (present → old world), in seconds.
    // Production spec: 2.5s. Prototype range: 1.0–4.0s.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|Timing")
    float TransitionInDuration = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|Timing")
    float TransitionOutDuration = 2.5f;

    // Simulated async level load time, in seconds.
    // Set < TransitionInDuration to test "level arrives early" path.
    // Set > TransitionInDuration to test "level arrives late" path.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|Timing")
    float SimulatedLevelLoadTime = 1.5f;

    // ── Public API ────────────────────────────────────────────────────────────

    // Call this when Gaia touches the artifact.
    // Guards: only fires if State == Idle.
    UFUNCTION(BlueprintCallable, Category="Proto")
    void BeginFragment();

    // Call this when the fragment scene's completion condition is met.
    // Guards: only fires if State == InFragment.
    UFUNCTION(BlueprintCallable, Category="Proto")
    void CompleteFragment();

    // ── State read-out (Blueprint-readable) ───────────────────────────────────

    UPROPERTY(BlueprintReadOnly, Category="Proto")
    EFragmentStateProto State = EFragmentStateProto::Idle;

    // 0.0–1.0: current normalized position in the transition animation.
    // Used by VisualTransitionProto to drive BlendWeight and MPC params.
    UPROPERTY(BlueprintReadOnly, Category="Proto")
    float TransitionAlpha = 0.f;

    // ── Blueprint Events (implement in BP to observe state changes) ───────────

    UFUNCTION(BlueprintImplementableEvent, Category="Proto")
    void OnTransitioningIn_Started();

    UFUNCTION(BlueprintImplementableEvent, Category="Proto")
    void OnFragmentEntered();       // both animation AND level load complete

    UFUNCTION(BlueprintImplementableEvent, Category="Proto")
    void OnTransitioningOut_Started();

    UFUNCTION(BlueprintImplementableEvent, Category="Proto")
    void OnFragmentExited();        // fully back in present world

    // ── UActorComponent overrides ─────────────────────────────────────────────

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

private:
    // ── Internal timing state ─────────────────────────────────────────────────

    float ElapsedTransitionTime = 0.f;

    // Two independent completion flags for the IN transition.
    // Both must be true before we enter InFragment.
    bool bAnimationComplete = false;    // visual transition reached t=1.0
    bool bLevelLoadComplete = false;    // simulated level loaded

    // Timer that fires when the simulated level load completes.
    FTimerHandle LevelLoadTimer;

    // ── Internal helpers ──────────────────────────────────────────────────────

    void TryEnterFragment();   // fires only when both flags are true
    void BeginTransitionOut();
    void FinishTransitionOut();

    UFUNCTION()
    void OnSimulatedLevelLoaded();  // fired by LevelLoadTimer
};
