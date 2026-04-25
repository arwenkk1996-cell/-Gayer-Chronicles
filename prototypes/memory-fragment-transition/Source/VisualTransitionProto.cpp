// PROTOTYPE - NOT FOR PRODUCTION
// Question: Does SmoothStep PostProcess + MPC blend feel like "world dissolving"?
// Date: 2026-04-25

#include "VisualTransitionProto.h"
#include "Kismet/GameplayStatics.h"

UVisualTransitionProto::UVisualTransitionProto()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Tick every frame — this drives visual output, so it must always run
}

void UVisualTransitionProto::BeginPlay()
{
    Super::BeginPlay();

    if (!PPV_Present || !PPV_OldWorld)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[VisualProto] PPV_Present or PPV_OldWorld not assigned! Visual blend will not work."));
    }

    if (!MPC_TimeLayer)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[VisualProto] MPC_TimeLayer not assigned! Material desaturation will not work."));
    }

    // Initialize to fully-Present state
    ApplyBlend(0.f);
}

void UVisualTransitionProto::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // AlphaInput is set each tick by Blueprint (reads from UFragmentTransitionProto.TransitionAlpha)
    ApplyBlend(AlphaInput);
}

// ─────────────────────────────────────────────────────────────────────────────
// CORE BLEND LOGIC
//
// NOTE: AlphaInput is ALREADY SmoothStep-adjusted by UFragmentTransitionProto.
// We apply it directly to BlendWeight — do NOT SmoothStep here again.
//
// PostProcess blend:
//   Present PPV:  BlendWeight = 1.0 - Alpha   (fades OUT as we enter the past)
//   OldWorld PPV: BlendWeight = Alpha          (fades IN as we enter the past)
//
// MPC parameters:
//   Saturation = Lerp(Present=1.0, OldWorld=0.45, Alpha)
//   ColorTint  = Lerp(Present, OldWorld, Alpha)
// ─────────────────────────────────────────────────────────────────────────────

void UVisualTransitionProto::ApplyBlend(float Alpha)
{
    // ── 1. PostProcess volume blend weights ───────────────────────────────────
    if (PPV_Present)
    {
        PPV_Present->BlendWeight = FMath::Clamp(1.f - Alpha, 0.f, 1.f);
    }
    if (PPV_OldWorld)
    {
        PPV_OldWorld->BlendWeight = FMath::Clamp(Alpha, 0.f, 1.f);
    }

    // ── 2. MPC parameter interpolation ───────────────────────────────────────
    // Drives material-level desaturation on all Substrate environment materials
    // that sample MPC_TimeLayer.

    UMaterialParameterCollectionInstance* MPCInst = GetMPCInstance();
    if (MPCInst)
    {
        float Saturation = FMath::Lerp(Saturation_Present, Saturation_OldWorld, Alpha);
        FLinearColor Tint = FMath::Lerp(ColorTint_Present, ColorTint_OldWorld, Alpha);

        MPCInst->SetScalarParameterValue(FName("Saturation"), Saturation);
        MPCInst->SetVectorParameterValue(FName("ColorTint"), Tint);

        // GrainAmount: 0.0 (Present) → 0.3 (OldWorld)
        MPCInst->SetScalarParameterValue(FName("GrainAmount"),
            FMath::Lerp(0.f, 0.3f, Alpha));
    }
}

UMaterialParameterCollectionInstance* UVisualTransitionProto::GetMPCInstance() const
{
    if (!MPC_TimeLayer || !GetWorld()) { return nullptr; }
    return GetWorld()->GetParameterCollectionInstance(MPC_TimeLayer);
}
