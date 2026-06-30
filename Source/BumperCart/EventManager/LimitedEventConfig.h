#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Product/ProductBase.h"
#include "LimitedEventConfig.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API ULimitedEventConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    // 한정 제품 목록
    UPROPERTY(EditAnywhere, Category = "Limited Event | Product List")
    TArray<TSubclassOf<AProductBase>> LimitedProductList;
};
