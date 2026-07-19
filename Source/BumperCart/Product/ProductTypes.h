#pragma once

#include "CoreMinimal.h"
#include "ProductTypes.generated.h"

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
    Spawning,
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

    // 서버가 계산한 포물선 운동 시작 지점
    UPROPERTY(BlueprintReadOnly)
    FVector_NetQuantize10 LaunchStartLocation = FVector::ZeroVector;

    // 서버가 계산한 포물선 운동 도착 지점
    UPROPERTY(BlueprintReadOnly)
    FVector_NetQuantize10 LaunchEndLocation = FVector::ZeroVector;

    // 서버가 계산한 실제 보여지는 위치
    UPROPERTY(BlueprintReadOnly)
    FVector_NetQuantize10 DisplayLocation = FVector::ZeroVector;

    // 서버가 계산한 튀어오르는 높이
    UPROPERTY(BlueprintReadOnly)
    float LaunchHeight = 0.f;

    // 포물선 운동을 하는 지속시간
    UPROPERTY(BlueprintReadOnly)
    float LaunchDuration = 1.f;

    // 서버에서 연출을 시작한 시간
    UPROPERTY(BlueprintReadOnly)
    float LaunchServerStartTime = 0.f;
};

USTRUCT(BlueprintType)
struct FProductValueGradeRule
{
    GENERATED_BODY()

public:
    // 적용할 외곽선 + 오라 색상
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor OverlayColor = FLinearColor::White;

    // 오라 강도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AuraStrength = 0.f;

    // 가치
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Value = 0;

    // 확률
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Chance = 0;
};
