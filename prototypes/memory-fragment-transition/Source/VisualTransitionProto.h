// PROTOTYPE - NOT FOR PRODUCTION
// Question: Does SmoothStep PostProcess + MPC blend feel like "world dissolving"
//           at the 2.5s production spec duration?
// Date: 2026-04-25
//
// Reads TransitionAlpha from UFragmentTransitionProto each tick and applies it to:
//   1. APostProcessVolume.BlendWeight (two volumes: Present + OldWorld)
//   2. UMaterialParameterCollection MPC_TimeLayer (Saturation + ColorTint)
//
// Wire up in Blueprint: assign the two PPV references and the MPC asset.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "VisualTransitionProto.generated.h"

UCLASS(ClassGroup=(Prototype), meta=(BlueprintSpawnableComponent))
class UVisualTransitionProto : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisualTransitionProto();

    // ── Wire-up: set these in the Blueprint Details panel ────────────────────

    // PostProcessVolume configured for the Present time layer (Saturation=1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|PostProcess")
    APostProcessVolume* PPV_Present = nullptr;

    // PostProcessVolume configured for the OldWorld time layer (Saturation=0.45)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|PostProcess")
    APostProcessVolume* PPV_OldWorld = nullptr;

    // The MPC_TimeLayer asset from Content/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|MPC")
    UMaterialParameterCollection* MPC_TimeLayer = nullptr;

    // ── MPC parameter values (hardcoded for Present → OldWorld) ──────────────
    // Present:  Saturation=1.0, ColorTint=(1.0, 0.95, 0.88)
    // OldWorld: Saturation=0.45, ColorTint=(1.0, 0.92, 0.78)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|MPC")
    float Saturation_Present = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|MPC")
    float Saturation_OldWorld = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|MPC")
    FLinearColor ColorTint_Present  = FLinearColor(1.0f, 0.95f, 0.88f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proto|MPC")
    FLinearColor ColorTint_OldWorld = FLinearColor(1.0f, 0.92f, 0.78f, 1.f);

    // ── Driver: read this each tick from UFragmentTransitionProto ─────────────
    // 0.0 = fully Present, 1.0 = fully OldWorld
    UPROPERTY(BlueprintReadWrite, Category="Proto")
    float AlphaInput = 0.f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

private:
    void ApplyBlend(float Alpha);
    UMaterialParameterCollectionInstance* GetMPCInstance() const;
};
