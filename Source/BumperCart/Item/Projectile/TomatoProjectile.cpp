#include "Item/Projectile/TomatoProjectile.h"

#include "Cart/CartPawn.h"
#include "Cart/Component/CartScreenFXComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ATomatoProjectile::ATomatoProjectile()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    SetRootComponent(CollisionComponent);

    // 충돌 설정
    CollisionComponent->InitSphereRadius(25.0f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->SetGenerateOverlapEvents(true);

    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

    TomatoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TomatoMesh"));
    TomatoMesh->SetupAttachment(CollisionComponent);
    TomatoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TomatoMesh->SetGenerateOverlapEvents(false);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComponent;
    ProjectileMovement->InitialSpeed = ProjectileSpeed;
    ProjectileMovement->MaxSpeed = ProjectileSpeed;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
}

void ATomatoProjectile::BeginPlay()
{
	Super::BeginPlay();

    if (IsValid(CollisionComponent))
    {
        CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnProjectileBeginOverlap);

        // 플레이어와의 충돌 방지
        if (IsValid(GetOwner()))
        {
            CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
        }
    }

    SetLifeSpan(MaxLifeTime);
}

// ------------------------------------------------------------
// 충돌 판정
// ------------------------------------------------------------

void ATomatoProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(OtherActor))
    {
        return;
    }

    // 플레이어 자신은 충돌 무시
    if (OtherActor == GetOwner())
    {
        return;
    }

    // 다른 플레이어와 충돌 시
    ACartPawn* HitPlayer = Cast<ACartPawn>(OtherActor);
    if (IsValid(HitPlayer))
    {
        HandleHitCart(HitPlayer);
        return;
    }

    // 그 외 액터와 충돌 시
    HandleHitOtherActor(OtherActor);
}


// ------------------------------------------------------------
// 투사체 발사
// ------------------------------------------------------------

void ATomatoProjectile::FireInDirection(const FVector& Direction)
{
    if (!IsValid(ProjectileMovement))
    {
        return;
    }

    const FVector FireDirection = Direction.GetSafeNormal();

    if (FireDirection.IsNearlyZero())
    {
        return;
    }

    ProjectileMovement->Velocity = FireDirection * ProjectileSpeed;
}

void ATomatoProjectile::HandleHitCart(ACartPawn* HitPlayer)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(HitPlayer))
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("투사체 플레이어 타격"));

    //HitPlayer->ClientApplyTomatoScreenBlock(ScreenBlockDuration);

    Destroy();
}

void ATomatoProjectile::HandleHitOtherActor(AActor* HitActor)
{
    if (!HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("다른 액터와 충돌"));

    Destroy();
}

