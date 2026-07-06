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
    None,
    Display,
    Grabbed,
    Loaded,
    Falling,
    Paid,
};

USTRUCT(BlueprintType)
struct FProductRepState
{
    GENERATED_BODY()

public:
    // 상품 상태
    UPROPERTY(BlueprintReadOnly)
    EProductState State = EProductState::None;

    // 서버가 계산한 Falling 시작 지점
    UPROPERTY(BlueprintReadOnly)
    FVector_NetQuantize10 FallingStartLocation = FVector::ZeroVector;

    // 서버가 계산한 Falling 도착 지점
    UPROPERTY(BlueprintReadOnly)
    FVector_NetQuantize10 FallingEndLocation = FVector::ZeroVector;

    // 서버가 계산한 튀어오르는 높이
    UPROPERTY(BlueprintReadOnly)
    float FallingHeight = 0.f;
};
