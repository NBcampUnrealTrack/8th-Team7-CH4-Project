#include "MapGimmickManager/WaterHoleGimmick/WaterHoleGimmick.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Cart/CartPawn.h"

AWaterHoleGimmick::AWaterHoleGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(GetRootComponent());

    WaterHoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterHoleMesh"));
    WaterHoleMesh->SetupAttachment(BoxCollision);

    SpinningCharacter = nullptr;
}

void AWaterHoleGimmick::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority())
    {
        BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AWaterHoleGimmick::OnWaterHoleBeginOverlap);
    }
}

void AWaterHoleGimmick::OnWaterHoleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if(!OtherActor || !OtherActor->Implements<USlideAffectable>())
    {
        return;
    }

    const float Yaw = FMath::RandBool() ? 60.f : -60.f;
    ISlideAffectable::Execute_ApplySlip(OtherActor, EffectDuration, Yaw);

    Destroy();
}

void AWaterHoleGimmick::OnWaterHoleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
