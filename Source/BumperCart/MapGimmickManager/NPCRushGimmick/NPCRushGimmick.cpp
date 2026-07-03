#include "MapGimmickManager/NPCRushGimmick/NPCRushGimmick.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetStringLibrary.h"
#include "Cart/CartPawn.h"

ANPCRushGimmick::ANPCRushGimmick()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    SetRootComponent(BoxCollision);

    CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
    CartMesh->SetupAttachment(RootComponent);
    //CartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    NPCRushFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NPCRushFX->SetupAttachment(CartMesh);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

    if (IsValid(ProjectileMovement))
    {
        ProjectileMovement->InitialSpeed =1000.0f;
        ProjectileMovement->MaxSpeed = 1000.0f;
        ProjectileMovement->ProjectileGravityScale = 0.0f;
    }
    
}

void ANPCRushGimmick::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority() && BoxCollision)
    {
        BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ANPCRushGimmick::OnCartOverlap);
    }
}

void ANPCRushGimmick::OnCartOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;

    if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
    {
        UE_LOG(LogTemp, Log, TEXT("[맵 기믹 매니저] 거대 카트가 마트 벽에 부딪혀 박살났습니다!"));

        Destroy();
    }

    if (ACartPawn* HitPlayer = Cast<ACartPawn>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("[NPCRush] 플레이어 오버랩 감지"));
    }
}

