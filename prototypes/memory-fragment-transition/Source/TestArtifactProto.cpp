// PROTOTYPE - NOT FOR PRODUCTION
// Question: Does the overlap-trigger → BeginFragment() path fire correctly?
// Date: 2026-04-25

#include "TestArtifactProto.h"
#include "GameFramework/Character.h"

ATestArtifactProto::ATestArtifactProto()
{
    PrimaryActorTick.bCanEverTick = false;

    ArtifactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArtifactMesh"));
    RootComponent = ArtifactMesh;

    TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
    TriggerSphere->SetupAttachment(RootComponent);
    TriggerSphere->SetSphereRadius(180.f);   // 180cm — matches production interaction radius
    TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));

    FragmentComponent = CreateDefaultSubobject<UFragmentTransitionProto>(TEXT("FragmentComponent"));
}

void ATestArtifactProto::BeginPlay()
{
    Super::BeginPlay();
    TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &ATestArtifactProto::OnOverlapBegin);
}

void ATestArtifactProto::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // Only fire for the player character
    if (!OtherActor || !OtherActor->IsA<ACharacter>()) { return; }

    UE_LOG(LogTemp, Log,
        TEXT("[TestArtifact] Gaia overlapped artifact — calling BeginFragment()"));

    FragmentComponent->BeginFragment();
}
