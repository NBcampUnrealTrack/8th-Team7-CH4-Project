#include "Item/Action/TomatoItemAction.h"

#include "Cart/CartPawn.h"
#include "Item/Projectile/TomatoProjectile.h"

#include "Engine/World.h"

bool UTomatoItemAction::CanExecute(ACartPawn* PlayerCharacter) const
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    if (!PlayerCharacter->HasAuthority())
    {
        return false;
    }

    if (!TomatoProjectileClass)
    {
        return false;
    }

    return true;
}

bool UTomatoItemAction::Execute(ACartPawn* PlayerCharacter)
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
    // z축 X
    FVector FireDirection = PlayerCharacter->GetActorForwardVector();
    FireDirection.Z = 0.0f;
    FireDirection = FireDirection.GetSafeNormal();

    if (FireDirection.IsNearlyZero())
    {
        return false;
    }

    // 카트 위치 + 
    const FVector SpawnLocation =
        PlayerCharacter->GetActorLocation()
        + FireDirection * SpawnForwardOffset
        + FVector(0.0f, 0.0f, SpawnHeightOffset);

    // 투사체가 발사 방향을 바라보도록
    const FRotator SpawnRotation = FireDirection.Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = PlayerCharacter;    
    SpawnParams.Instigator = PlayerCharacter;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 서버에서 토마토 투사체 생성
    ATomatoProjectile* TomatoProjectile = World->SpawnActor<ATomatoProjectile>(
        TomatoProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (!IsValid(TomatoProjectile))
    {
        return false;
    }

    // 방향대로 투사체 발사
    TomatoProjectile->FireInDirection(FireDirection);

    UE_LOG(LogTemp, Warning, TEXT("%s: 토마토 아이템 사용"), *GetNameSafe(PlayerCharacter));

    return true;
}
