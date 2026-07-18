// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Action/ProjectileItemAction.h"

#include "Cart/CartPawn.h"
#include "Item/Projectile/ItemProjectile.h"

#include "Engine/World.h"

bool UProjectileItemAction::CanExecute(ACartPawn* PlayerCharacter) const
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    if (!PlayerCharacter->HasAuthority())
    {
        return false;
    }

    if (!ProjectileClass)
    {
        return false;
    }

    return true;
}

bool UProjectileItemAction::Execute(ACartPawn* PlayerCharacter)
{
    if (!CanExecute(PlayerCharacter))
    {
        return false;
    }

    UWorld* World = PlayerCharacter->GetWorld();
    if (!IsValid(World))
    {
        return false;
    }

    // 카트 방향을 기준으로 방향 생성
    FVector HorizontalDirection = PlayerCharacter->GetActorForwardVector();
    HorizontalDirection.Z = 0.0f;
    HorizontalDirection = HorizontalDirection.GetSafeNormal();

    if (HorizontalDirection.IsNearlyZero())
    {
        return false;
    }

    // 투사체 방향 계산
    // 살짝 떠올랐다가 떨어지는 포물선
    FVector FireDirection = HorizontalDirection + FVector::UpVector * 0.06f;
    FireDirection = FireDirection.GetSafeNormal();

    // 투사체 생성
    // 카트 위치 + 
    const FVector SpawnLocation =
        PlayerCharacter->GetActorLocation()
        + HorizontalDirection * SpawnForwardOffset
        + FVector(0.0f, 0.0f, SpawnHeightOffset);

    // 투사체가 발사 방향을 바라보도록
    const FRotator SpawnRotation = FireDirection.Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = PlayerCharacter;
    SpawnParams.Instigator = PlayerCharacter;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 서버에서 투사체 생성
    AItemProjectile* ItemProjectile = World->SpawnActor<AItemProjectile>(
        ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    // 투사체 생성 실패한 경우만 False
    if (!IsValid(ItemProjectile))
    {
        return false;
    }

    ItemProjectile->FireInDirection(FireDirection);

    return true;
}
