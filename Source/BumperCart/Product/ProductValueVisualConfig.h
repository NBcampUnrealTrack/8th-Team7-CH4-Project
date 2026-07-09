// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProductTypes.h"
#include "ProductValueVisualConfig.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API UProductValueVisualConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    // 적용할 규칙들
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FProductValueVisualRule> Rules;

    // 적용할 수 있는 규칙이 없다면 적용할 디폴트 값
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor DefaultColor = FLinearColor::White;
};
