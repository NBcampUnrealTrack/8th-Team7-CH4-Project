#include "Checkout/CheckoutBarrier.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

ACheckoutBarrier::ACheckoutBarrier()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
    BarrierMesh->SetupAttachment(SceneRoot);

    // 에디터에서는 표시, 게임 시작 시 표시 X
    BarrierMesh->SetVisibility(true, true);
    BarrierMesh->SetHiddenInGame(true);

    // 메시 충돌 설정
    BarrierMesh->SetCollisionObjectType(ECC_WorldStatic);
    BarrierMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BarrierMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    BarrierMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BarrierMesh->SetGenerateOverlapEvents(false);
    BarrierMesh->CanCharacterStepUpOn = ECB_No; // 발판 방지
}

void ACheckoutBarrier::BeginPlay()
{
    Super::BeginPlay();

    SetBarrierEnabled(false);
}

void ACheckoutBarrier::SetBarrierEnabled(bool bIsEnabled)
{
    bIsBarrierEnabled = bIsEnabled;

    if (!IsValid(BarrierMesh))
    {
        return;
    }

    // 게임 화면 표시
    BarrierMesh->SetHiddenInGame(!bIsEnabled);

    // 충돌 설정
    BarrierMesh->SetCollisionEnabled(
        bIsEnabled
        ? ECollisionEnabled::QueryOnly
        : ECollisionEnabled::NoCollision
    );
}

bool ACheckoutBarrier::IsBarrierEnabled() const
{
    return bIsBarrierEnabled;
}
