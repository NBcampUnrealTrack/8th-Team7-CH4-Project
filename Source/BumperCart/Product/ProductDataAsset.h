// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Product/ProductTypes.h"
#include "ProductDataAsset.generated.h"

class UStaticMesh;

UCLASS()
class BUMPERCART_API UProductDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    // 상품이 사용할 메시
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    TObjectPtr<UStaticMesh> ProductMesh;

    // 구분용 ID 값
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    FName ProductId;

    // 표시할 상품 이름
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    FText DisplayName;

    // 상품의 무게
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    int32 Weight;
};
