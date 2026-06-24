#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Checkout/CheckoutTypes.h"
#include "Product/ProductTypes.h"
#include "CheckoutScoreCalculator.generated.h"


UCLASS()
class BUMPERCART_API UCheckoutScoreCalculator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    // 상품의 기본 점수 합산
    UFUNCTION(BlueprintPure, Category = "Checkout|Score")
    static int32 CalculateBaseScore(const TArray<FLoadedProductInfo>& Products);

    // 세일 상품 보너스 점수 계산
    UFUNCTION(BlueprintPure, Category = "Checkout|Score")
    static int32 CalculateSaleBonusScore(const TArray<FLoadedProductInfo>& Products,float SaleMultiplier);

    // 마지막 계산 시간대 보너스 계산
    UFUNCTION(BlueprintPure, Category = "Checkout|Score")
    static int32 CalculateLastCheckoutBonusScore(int32 BaseScore, bool bIsApplyLastCheckoutBonus, float LastCheckoutBonusMultiplier);

    // 모든 점수를 합산해 최종 점수 반환
    UFUNCTION(BlueprintPure, Category = "Checkout|Score")
    static int32 CalculateTotalScore(int32 BaseScore, int32 SaleBonusScore, int32 LastCheckoutBonusScore);

    // 전체 계산 결과 구조체 반환
    UFUNCTION(BlueprintPure, Category = "Checkout|Score")
    static FCheckoutScoreResult CalculateCheckoutScore(
        const TArray<FLoadedProductInfo>& Products,
        float SaleMultiplier,
        bool bApplyLastCheckoutBonus,
        float LastCheckoutBonusMultiplier
    );
};
