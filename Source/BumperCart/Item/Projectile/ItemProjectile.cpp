// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Projectile/ItemProjectile.h"

#include "Cart/CartPawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Util/BCCollisionChannels.h"

AItemProjectile::AItemProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    SetRootComponent(CollisionComponent);

    // 충돌 설정
    CollisionComponent->InitSphereRadius(25.0f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollisionComponent->SetGenerateOverlapEvents(false);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Overlap);

    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(CollisionComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMesh->SetGenerateOverlapEvents(false);

    // 투사체 궤적
    TrailNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagaraComponent"));
    TrailNiagaraComponent->SetupAttachment(CollisionComponent);
    TrailNiagaraComponent->SetAutoActivate(false);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComponent;
    ProjectileMovement->InitialSpeed = ProjectileSpeed;
    ProjectileMovement->MaxSpeed = ProjectileSpeed;
    ProjectileMovement->ProjectileGravityScale = ProjectileGravityScale;    // 중력 적용
    ProjectileMovement->bRotationFollowsVelocity = true;
}

void AItemProjectile::BeginPlay()
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

    if (IsValid(ProjectileMovement))
    {
        ProjectileMovement->InitialSpeed = ProjectileSpeed;
        ProjectileMovement->MaxSpeed = ProjectileSpeed;
        ProjectileMovement->ProjectileGravityScale = ProjectileGravityScale;
    }

    // 궤적 이펙트 적용
    if (IsValid(TrailNiagaraComponent) && IsValid(TrailNiagaraSystem))
    {
        TrailNiagaraComponent->SetAsset(TrailNiagaraSystem);
        TrailNiagaraComponent->Activate(true);
    }

    // 사운드 적용
    PlayThrowProjectileSound();

    SetLifeSpan(MaxLifeTime);
}

// ------------------------------------------------------------
// 충돌 판정
// ------------------------------------------------------------

void AItemProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bHasHit)
    {
        return;
    }

    if (!IsValid(OtherActor))
    {
        return;
    }

    // 플레이어 자신은 충돌 무시
    if (OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

    bHasHit = true;

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

void AItemProjectile::FireInDirection(const FVector& Direction)
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

    // SpawnActor가 완료된 후 충돌 활성화
    CollisionComponent->SetGenerateOverlapEvents(true);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->UpdateOverlaps();
}

void AItemProjectile::OnHitCart(ACartPawn* HitPlayer)
{
    UE_LOG(LogTemp, Error, TEXT("OnHitCart 함수 재정의 필요!"));
}

void AItemProjectile::HandleHitCart(ACartPawn* HitPlayer)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(HitPlayer))
    {
        return;
    }

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning, TEXT("투사체 플레이어 타격"));
#endif

    // 주변 플레이어 충돌 이펙트 재생
    MulticastPlayHitProjectileEffect(GetActorLocation());

    // 주변 플레이어 사운드 재생
    MulticastPlayHitProjectileSound(GetActorLocation());

    // 플레이어 타격 시 처리할 로직 호출
    OnHitCart(HitPlayer);

    Destroy();
}

void AItemProjectile::HandleHitOtherActor(AActor* HitActor)
{
    if (!HasAuthority())
    {
        return;
    }

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning, TEXT("다른 액터와 충돌"));
#endif

    MulticastPlayHitProjectileEffect(GetActorLocation());

    MulticastPlayHitProjectileSound(GetActorLocation());

    Destroy();
}

// ------------------------------------------------------------
// 이펙트
// ------------------------------------------------------------

void AItemProjectile::MulticastPlayHitProjectileEffect_Implementation(const FVector& EffectLocation)
{
    if (!IsValid(HitNiagaraSystem))
    {
        return;
    }

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        HitNiagaraSystem,
        EffectLocation,
        FRotator::ZeroRotator,
        FVector::OneVector,
        true,
        true
    );
}

// ------------------------------------------------------------
// 사운드
// ------------------------------------------------------------

void AItemProjectile::PlayThrowProjectileSound() const
{
    if (!IsValid(ThrowProjectileSound))
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, ThrowProjectileSound, GetActorLocation(), 1.0f, 1.0f);
}

void AItemProjectile::MulticastPlayHitProjectileSound_Implementation(const FVector& SoundLocation)
{
    if (!IsValid(HitProjectileSound))
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, HitProjectileSound, SoundLocation, 1.0f, 1.0f, 0.0f, HitSoundAttenuation);
}

