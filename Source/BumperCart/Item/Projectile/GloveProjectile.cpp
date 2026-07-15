// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Projectile/GloveProjectile.h"
#include "Cart/CartPawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Cart/Component/CartLoadComponent.h"
#include "NiagaraFunctionLibrary.h"


AGloveProjectile::AGloveProjectile()
{
    Strength = 500.f;
}

void AGloveProjectile::OnHitCart(ACartPawn* HitPlayer)
{
    // HitPlayer가 아이템 떨어뜨리도록
    if (!IsValid(HitPlayer)) return;

    FVector MoveVelocity = ProjectileMovement->Velocity;
    FVector Direction = MoveVelocity.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;

    HitPlayer->ApplyExternalKnockback(Direction, Strength);
    HitPlayer->ClientPlayCameraShake(nullptr, 1.f);

    UCartLoadComponent* LoadComp = HitPlayer->FindComponentByClass<UCartLoadComponent>();
    if (IsValid(LoadComp))
    {
        LoadComp->DropProducts(Strength, EDropCollisionRole::GloveAttack);
    }

    Multicast_PlayHitEffect(GetActorLocation());
}

void AGloveProjectile::Multicast_PlayHitEffect_Implementation(FVector_NetQuantize100 Location)
{
    if (GetNetMode() == NM_DedicatedServer) return;

    if (HitEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            HitEffect,
            Location,
            FRotator::ZeroRotator,
            FVector::OneVector,
            true
        );
    }
}
