// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProductDropConfig.generated.h"


UCLASS()
class BUMPERCART_API UProductDropConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    // Falling 상태 지속 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float FallingDuration = 1.f;

    // 수평 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float HorizontalOffset = 200.f;
};
