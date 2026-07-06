#pragma once

#include "CoreMinimal.h"
#include "Item/Action/ItemAction.h"
#include "TomatoItemAction.generated.h"

class ATomatoProjectile;

UCLASS()
class BUMPERCART_API UTomatoItemAction : public UItemAction
{
	GENERATED_BODY()

public:
    // 아이템 사용 가능한지 검사
    virtual bool CanExecute(ACartPawn* PlayerCharacter) const override;
    // 아이템 사용
    virtual bool Execute(ACartPawn* PlayerCharacter) override;

// ------------------------------------------------------------
// 토마토 투사체
// ------------------------------------------------------------
private:
    // 토마토 투사체 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Projectile", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<ATomatoProjectile> TomatoProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Projectile", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float SpawnForwardOffset = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Projectile", meta = (AllowPrivateAccess = "true"))
    float SpawnHeightOffset = 100.0f;
};
