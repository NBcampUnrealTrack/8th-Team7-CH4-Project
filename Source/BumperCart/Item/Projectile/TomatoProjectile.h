#pragma once

#include "CoreMinimal.h"
#include "ItemProjectile.h"
#include "TomatoProjectile.generated.h"

UCLASS()
class BUMPERCART_API ATomatoProjectile : public AItemProjectile
{
	GENERATED_BODY()
	
protected:
    virtual void OnHitCart(ACartPawn* HitPlayer) override;

// ------------------------------------------------------------
// 토마토 효과 적용
// ------------------------------------------------------------
private:
    // 화면 가림 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Tomato|Effect", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    float ScreenBlockDuration = 3.0f;

    // 토마토 피격 시 충격 강도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Tomato|Effect", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    float ProductDropStrength = 1500.0f;
};
