#include "Checkout/CheckoutScoreCalculator.h"

int32 UCheckoutScoreCalculator::CalculateCheckoutProductCount(const TArray<FLoadedProductInfo>& Products)
{
    int32 ProductCount = 0;

    for (const FLoadedProductInfo& ProductInfo : Products)
    {
        if (ProductInfo.Value <= 0)
        {
            continue;
        }

        ++ProductCount;
    }

    return ProductCount;
}

int32 UCheckoutScoreCalculator::CalculateBaseScore(const TArray<FLoadedProductInfo>& Products)
{
    int32 BaseScore = 0;

    for (const FLoadedProductInfo& ProductInfo : Products)
    {
        // 0점 이하일 경우 제외
        if (ProductInfo.Value <= 0)
        {
            continue;
        }

        BaseScore += ProductInfo.Value;
    }

    return BaseScore;
}

int32 UCheckoutScoreCalculator::CalculateSaleBonusScore(const TArray<FLoadedProductInfo>& Products, float SaleMultiplier)
{
    const float SafeSaleMultiplier = FMath::Max(SaleMultiplier, 1.0f);

    int32 SaleBonusScore = 0;

    for (const FLoadedProductInfo& ProductInfo : Products)
    {
        // 0점 이하일 경우 제외
        if (ProductInfo.Value <= 0)
        {
            continue;
        }

        // 세일 상품이 아닐 경우 제외
        if (!ProductInfo.bOnSale)
        {
            continue;
        }

        SaleBonusScore += ProductInfo.Value * (SafeSaleMultiplier - 1.0f);
    }

    return SaleBonusScore;
}

int32 UCheckoutScoreCalculator::CalculateLastCheckoutBonusScore(int32 BaseScore, bool bIsApplyLastCheckoutBonus, float LastCheckoutBonusMultiplier)
{
    // 마지막 계산 시간대가 아닐 경우 보너스 없음
    if (!bIsApplyLastCheckoutBonus)
    {
        return 0;
    }

    if (BaseScore <= 0)
    {
        return 0;
    }

    const float SafeMultiplier = FMath::Max(LastCheckoutBonusMultiplier, 1.0f);

    int32 LastCheckoutBonusScore = BaseScore * (SafeMultiplier - 1.0f);

    return LastCheckoutBonusScore;
}

int32 UCheckoutScoreCalculator::CalculateComboBonusScore(int32 BaseScore, int32 ProductCount, float& ComboMultiplier)
{
    ComboMultiplier = 1.0f;

    if (BaseScore <= 0)
    {
        return 0;
    }


    if (ProductCount < 10)
    {
        return 0;
    }

    // 10~14 -> 1.5
    // 15~19 -> 2.0
    // 5개 마다 0.5배 증가
    const int32 ComboStep = (ProductCount - 10) / 5;

    ComboMultiplier = 1.5f + ComboStep * 0.5f;

    int32 ComboBonusScore = FMath::RoundToInt(BaseScore * (ComboMultiplier - 1.0f));

    return ComboBonusScore;
}

int32 UCheckoutScoreCalculator::CalculateTotalScore(int32 BaseScore, int32 SaleBonusScore, int32 LastCheckoutBonusScore, int32 ComboBonusScore)
{
    const int32 TotalScore = BaseScore + SaleBonusScore + LastCheckoutBonusScore + ComboBonusScore;

    return TotalScore;
}

FCheckoutScoreResult UCheckoutScoreCalculator::CalculateCheckoutScore(const TArray<FLoadedProductInfo>& Products, float SaleBonusMultiplier, bool bIsLastCheckoutBonusApplied, float LastCheckoutBounusMultiplier)
{
    FCheckoutScoreResult Result;

    // 정산할 상품이 없다면,
    // 빈 배열 반환
    if (Products.IsEmpty())
    {
        return Result;
    }

    Result.CheckoutProductCount = CalculateCheckoutProductCount(Products);

    if (Result.CheckoutProductCount <= 0)
    {
        return Result;
    }

    // 최종 점수 계산
    Result.BaseScore = CalculateBaseScore(Products);
    Result.SaleBonusScore = CalculateSaleBonusScore(Products, SaleBonusMultiplier);
    Result.LastCheckoutBonusScore = CalculateLastCheckoutBonusScore(Result.BaseScore, bIsLastCheckoutBonusApplied, LastCheckoutBounusMultiplier);
    Result.ComboBonusScore = CalculateComboBonusScore(Result.BaseScore, Result.CheckoutProductCount, Result.ComboMultiplier);
    Result.TotalScore = CalculateTotalScore(Result.BaseScore, Result.SaleBonusScore, Result.LastCheckoutBonusScore, Result.ComboBonusScore);

    for (const FLoadedProductInfo& ProductInfo : Products)
    {
        // 정산한 상품 수
        if (ProductInfo.Value > 0)
        {
            ++Result.CheckoutProductCount;
        }
    }

    // 보너스 적용 및 정산 완료 여부
    Result.bIsSaleBonusApplied = Result.SaleBonusScore > 0;
    Result.bIsLastCheckoutBonusApplied = Result.LastCheckoutBonusScore > 0;
    Result.bIsComboBonusApplied = Result.ComboBonusScore > 0;

    Result.bIsCalculationCompleted = true;

    return Result;
}
