

#pragma once

#include "CoreMinimal.h"
#include "CartLoadTypes.generated.h"


USTRUCT(BlueprintType)
struct FLoadInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentLoadedCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentWeight = 0;
};

USTRUCT(BlueprintType)
struct FDropCountRule
{
    GENERATED_BODY()

public:
    // 규칙을 적용할 최소 충격량
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cart|Drop")
    float RequiredImpulse = 0.f;

    // 떨어뜨릴 최소 개수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cart|Drop")
    int32 MinDropCount = 0;

    // 떨어뜨릴 최대 개수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cart|Drop")
    int32 MaxDropCount = 1;

    // 규칙이 적용될 확률
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cart|Drop")
    float DropChance = 1.f;
};

UENUM(BlueprintType)
enum class EDropCollisionRole : uint8
{
    Normal              UMETA(DisplayName = "Normal Collision"),
    BoosterInstigator   UMETA(DisplayName = "Booster Instigator"),
    BoostedTarget       UMETA(DisplayName = "Boosted Target"),
    GloveAttack         UMETA(DisplayName = "Glove Attacked")
};
