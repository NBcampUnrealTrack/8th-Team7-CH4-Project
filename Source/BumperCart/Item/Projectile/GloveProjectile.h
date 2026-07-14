// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Projectile/ItemProjectile.h"
#include "GloveProjectile.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API AGloveProjectile : public AItemProjectile
{
	GENERATED_BODY()

public:
    AGloveProjectile();

protected:
    virtual void OnHitCart(ACartPawn* HitPlayer) override;

private:
    // 충격 강도
    UPROPERTY(EditAnywhere, Category = "Projectile|Glove|Bump")
    float Impulse;
};
