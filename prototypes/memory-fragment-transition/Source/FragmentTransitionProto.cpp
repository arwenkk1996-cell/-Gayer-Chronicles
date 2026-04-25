// PROTOTYPE - NOT FOR PRODUCTION
// Question: Does the concurrent level-load / visual-transition synchronization
//           handle fast AND slow level loads without hitches or freezes?
// Date: 2026-04-25

#include "FragmentTransitionProto.h"
#include "TimerManager.h"

UFragmentTransitionProto::UFragmentTransitionProto()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Start disabled; only tick during transitions
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFragmentTransitionProto::BeginPlay()
{
    Super::BeginPlay();
}

// ─────────────────────────────────────────────────────────────────────────────
// PUBLIC API
// ─────────────────────────────────────────────────────────────────────────────

void UFragmentTransitionProto::BeginFragment()
{
    if (State != EFragmentStateProto::Idle)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[FragmentProto] BeginFragment() ignored — state is %d, not Idle"),
            (int32)State);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[FragmentProto] ── BeginFragment() ──────────────────"));
    UE_LOG(LogTemp, Log, TEXT("[FragmentProto]    TransitionInDuration  = %.2fs"), TransitionInDuration);
    UE_LOG(LogTemp, Log, TEXT("[FragmentProto]    SimulatedLoadTime     = %.2fs"), SimulatedLevelLoadTime);
    UE_LOG(LogTemp, Log, TEXT("[FragmentProto]    Expected hold at apex = %.2fs"),
        FMath::Max(0.f, SimulatedLevelLoadTime - TransitionInDuration));

    State = EFragmentStateProto::Transitioning_In;
    ElapsedTransitionTime = 0.f;
    bAnimationComplete = false;
    bLevelLoadComplete = false;
    TransitionAlpha = 0.f;

    // Start tick to drive the visual transition animation
    SetComponentTickEnabled(true);

    // ── Concurrent: kick off simulated async level load ───────────────────────
    // In production: ULevelStreamingDynamic::LoadLevelInstance() + OnLevelLoaded delegate
    // Here: a timer fires after SimulatedLevelLoadTime seconds
    GetWorld()->GetTimerManager().SetTimer(
        LevelLoadTimer,
        this,
        &UFragmentTransitionProto::OnSimulatedLevelLoaded,
        SimulatedLevelLoadTime,
        false   // not looping
    );

    OnTransitioningIn_Started();
}

void UFragmentTransitionProto::CompleteFragment()
{
    if (State != EFragmentStateProto::InFragment)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[FragmentProto] CompleteFragment() ignored — state is %d, not InFragment"),
            (int32)State);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[FragmentProto] ── CompleteFragment() ─────────────────"));
    BeginTransitionOut();
}

// ─────────────────────────────────────────────────────────────────────────────
// TICK — drives the SmoothStep animation
// ─────────────────────────────────────────────────────────────────────────────

void UFragmentTransitionProto::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (State == EFragmentStateProto::Transitioning_In)
    {
        // ── Phase 1: animate toward apex ─────────────────────────────────────
        if (!bAnimationComplete)
        {
            ElapsedTransitionTime += DeltaTime;
            float t = FMath::Clamp(ElapsedTransitionTime / TransitionInDuration, 0.f, 1.f);
            TransitionAlpha = FMath::SmoothStep(0.f, 1.f, t);

            if (t >= 1.f)
            {
                bAnimationComplete = true;
                TransitionAlpha = 1.f;
                UE_LOG(LogTemp, Log,
                    TEXT("[FragmentProto]    Animation complete at t=%.2fs. Level loaded: %s"),
                    ElapsedTransitionTime,
                    bLevelLoadComplete ? TEXT("YES → entering fragment") : TEXT("NO → holding at apex"));

                TryEnterFragment();
            }
        }
        // ── Phase 2: hold at apex, waiting for level load ─────────────────────
        // TransitionAlpha stays at 1.0; tick remains enabled.
        // TryEnterFragment() will fire when bLevelLoadComplete becomes true
        // (see OnSimulatedLevelLoaded below).
    }
    else if (State == EFragmentStateProto::Transitioning_Out)
    {
        ElapsedTransitionTime += DeltaTime;
        float t = FMath::Clamp(ElapsedTransitionTime / TransitionOutDuration, 0.f, 1.f);
        // Reverse: alpha goes from 1.0 → 0.0
        TransitionAlpha = FMath::SmoothStep(0.f, 1.f, 1.f - t);

        if (t >= 1.f)
        {
            FinishTransitionOut();
        }
    }
    else
    {
        // Should not be ticking in other states; disable
        SetComponentTickEnabled(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// INTERNAL HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void UFragmentTransitionProto::OnSimulatedLevelLoaded()
{
    // Fired by the simulated level load timer.
    // In production: fired by ULevelStreaming::OnLevelLoaded delegate.

    bLevelLoadComplete = true;
    UE_LOG(LogTemp, Log,
        TEXT("[FragmentProto]    Level load complete at t=%.2fs. Animation done: %s"),
        ElapsedTransitionTime,
        bAnimationComplete ? TEXT("YES → entering fragment") : TEXT("NO → will enter when animation completes"));

    TryEnterFragment();
}

void UFragmentTransitionProto::TryEnterFragment()
{
    // Gate: both conditions must be true.
    // This function is called from two places — whichever completes last will
    // trigger the transition into InFragment.
    if (!bAnimationComplete || !bLevelLoadComplete)
    {
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[FragmentProto] ── Entering InFragment (total elapsed: %.2fs) ──────────"),
        ElapsedTransitionTime);

    State = EFragmentStateProto::InFragment;
    TransitionAlpha = 1.f;     // stays fully "in the past" while InFragment
    SetComponentTickEnabled(false);

    // In production: APlayerController->Possess(AFragmentCharacter)
    OnFragmentEntered();
}

void UFragmentTransitionProto::BeginTransitionOut()
{
    State = EFragmentStateProto::Transitioning_Out;
    ElapsedTransitionTime = 0.f;
    TransitionAlpha = 1.f;

    SetComponentTickEnabled(true);

    // In production: begin level unload here in parallel with the animation
    // ULevelStreaming->SetShouldBeLoaded(false)

    UE_LOG(LogTemp, Log, TEXT("[FragmentProto]    TransitionOut started (%.2fs)"), TransitionOutDuration);
    OnTransitioningOut_Started();
}

void UFragmentTransitionProto::FinishTransitionOut()
{
    State = EFragmentStateProto::Idle;
    TransitionAlpha = 0.f;
    SetComponentTickEnabled(false);

    // In production: APlayerController->Possess(AGaiaCharacter); apply world state

    UE_LOG(LogTemp, Log,
        TEXT("[FragmentProto] ── Back in Idle. Fragment cycle complete. ──────────────"));

    OnFragmentExited();
}
