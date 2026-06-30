#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Product/ProductBase.h"
#include "SaleEventConfig.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API USaleEventConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    // 세일 제품 목록
    UPROPERTY(EditAnywhere, Category = "Sale Event | Product List")
    TArray<TSubclassOf<AProductBase>> SaleProductList;

    // 세일 이벤트 지속시간
    UPROPERTY(EditAnywhere, Category = "Sale Event | Info")
    float SaleEventTime = 15.0f;

    // 세일 제품 스폰 주기
    UPROPERTY(EditAnywhere, Category = "Sale Event | Info")
    float SaleProductSpawnInterval = 3.0f;

};
