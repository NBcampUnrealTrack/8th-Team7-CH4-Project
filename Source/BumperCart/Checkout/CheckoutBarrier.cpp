#include "Checkout/CheckoutBarrier.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

ACheckoutBarrier::ACheckoutBarrier()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
    BarrierMesh->SetupAttachment(SceneRoot);

    // 메시는 충돌 X
    BarrierMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BarrierMesh->SetGenerateOverlapEvents(false);

    BarrierCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BarrierCollision"));
    BarrierCollision->SetupAttachment(SceneRoot);

    // 충돌 설정
    BarrierCollision->SetGenerateOverlapEvents(false);
    BarrierCollision->SetCollisionObjectType(ECC_WorldStatic);
    BarrierCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    BarrierCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    BarrierCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}


void ACheckoutBarrier::SetBarrierEnabled(bool bIsEnabled)
{
    bIsBarrierEnabled = bIsEnabled;

    if (IsValid(BarrierMesh))
    {
        BarrierMesh->SetVisibility(bIsEnabled, true);
    }

    if (IsValid(BarrierCollision))
    {
        BarrierCollision->SetCollisionEnabled(
            bIsEnabled
            ? ECollisionEnabled::QueryAndPhysics
            : ECollisionEnabled::NoCollision
        );
    }
}

bool ACheckoutBarrier::IsBarrierEnabled() const
{
    return bIsBarrierEnabled;
}
