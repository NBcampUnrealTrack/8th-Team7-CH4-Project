#pragma once

#include "CoreMinimal.h"
#include "ProductTypes.generated.h"

USTRUCT(BlueprintType)
struct FProductData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    int32 Value = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    int32 Weight = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    bool bDamageable = false;
};

UENUM(BlueprintType)
enum class EProductState : uint8
{
    None        UMETA(DisplayName = "None"),
    Display     UMETA(DisplayName = "진열"),
    Loaded      UMETA(DisplayName = "적재"),
    Falling     UMETA(DisplayName = "낙하"),
    Paid        UMETA(DisplayName = "결제완료"),
};
