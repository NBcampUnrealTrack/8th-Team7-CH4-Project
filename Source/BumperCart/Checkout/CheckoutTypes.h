#pragma once

#include "CoreMinimal.h"
#include "CheckoutTypes.generated.h"


UENUM(BlueprintType)
enum class ECheckoutZoneState : uint8
{
    None,
    Open,           // 계산대 열림
    ClosingSoon,    // 계산대 마감 임박
    Closed          // 계산대 닫힘
};

USTRUCT(BlueprintType)
struct FCheckoutScoreResult
{
    GENERATED_BODY()

public:
    // 기본 점수
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    int32 BaseScore = 0;

    // 세일 보너스
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    int32 SaleBonusScore = 0;

    // 마지막 계산 보너스
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    int32 LastCheckoutBonusScore = 0;

    // 최종 점수
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    int32 TotalScore = 0;

    // 정산한 상품 수
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    int32 CheckoutProductCount = 0;

    // 세일이 적용되었는지
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    bool bIsSaleBonusApplied = false;

    // 마지막에 계산한 상품인지
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    bool bIsLastCheckoutBonusApplied = false;

    // 정산 완료했는지
    UPROPERTY(BlueprintReadOnly, Category = "Checkout|Score")
    bool bIsCalculationCompleted = false;
};

USTRUCT(BlueprintType)
struct FCheckoutZoneVisualStyle
{
    GENERATED_BODY()

public:
    // 바깥쪽
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CheckoutZone")
    FLinearColor RingColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CheckoutZone", meta = (ClampMin = "0.0"))
    float RingEmissiveStrength = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CheckoutZone", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RingOpacity = 1.0f;

    // 안쪽
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CheckoutZone")
    FLinearColor FillColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CheckoutZone", meta = (ClampMin = "0.0"))

    float FillEmissiveStrength = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CheckoutZone", meta = (ClampMin = "0.0", ClampMax = "1.0"))

    float FillOpacity = 0.5f;
};
