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

USTRUCT(BlueprintType)
struct FLoadedProductInfo
{
    GENERATED_BODY();

public:
    // 상품 식별용 ID
    UPROPERTY(BlueprintReadOnly)
    FName ProductId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int32 Value = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bOnSale = false;
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

USTRUCT(BlueprintType)
struct FProductRepState
{
    GENERATED_BODY()

public:
    // 상품 상태
    UPROPERTY(BlueprintReadOnly)
    EProductState State = EProductState::None;

    // 서버가 계산한 드롭 시작하는 위치
    UPROPERTY(BlueprintReadOnly)
    FVector_NetQuantize10 DropLocation = FVector::ZeroVector;
};
