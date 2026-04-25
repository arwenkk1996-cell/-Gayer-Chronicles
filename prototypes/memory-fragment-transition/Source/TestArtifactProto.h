// PROTOTYPE - NOT FOR PRODUCTION
// Question: Does the overlap-trigger → BeginFragment() path fire correctly?
// Date: 2026-04-25
//
// Drop a BP_TestArtifactProto in the test level.
// Walk into its overlap sphere → triggers BeginFragment().
// Press E (or call CompleteFragment() from Blueprint) to exit.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FragmentTransitionProto.h"
#include "TestArtifactProto.generated.h"

UCLASS()
class ATestArtifactProto : public AActor
{
    GENERATED_BODY()

public:
    ATestArtifactProto();

    // The fragment state machine component — lives on this Actor for the prototype.
    // In production it lives on UGameInstanceSubsystem.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Proto")
    UFragmentTransitionProto* FragmentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Proto")
    USphereComponent* TriggerSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Proto")
    UStaticMeshComponent* ArtifactMesh;

    // Call from input binding (e.g., E key BP input) to exit the fragment
    UFUNCTION(BlueprintCallable, Category="Proto")
    void ExitFragment() { FragmentComponent->CompleteFragment(); }

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);
};
