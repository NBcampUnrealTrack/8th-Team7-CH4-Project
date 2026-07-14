// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Action/ItemAction.h"
#include "ProjectileItemAction.generated.h"

class AItemProjectile;


UCLASS()
class BUMPERCART_API UProjectileItemAction : public UItemAction
{
	GENERATED_BODY()

public:
    // 아이템 사용 가능한지 검사
    virtual bool CanExecute(ACartPawn* PlayerCharacter) const override;
    // 아이템 사용
    virtual bool Execute(ACartPawn* PlayerCharacter) override;

// ------------------------------------------------------------
// 투사체
// ------------------------------------------------------------
protected:
    // 투사체 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
    TSubclassOf<AItemProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
    float SpawnForwardOffset = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
    float SpawnHeightOffset = 100.0f;
};
