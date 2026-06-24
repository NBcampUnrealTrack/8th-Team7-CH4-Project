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
    // 카트 위쪽 방향으로 퍼지는 원뿔 각도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float HalfAngle = 20.f;

    // 최소 충격량
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float MinImpulse = 600.f;

    // 최대 충격량
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float MaxImpulse = 1600.f;

    // Falling 상태 지속 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float FallingDuration = 3.f;

    // 시작 높이 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float HeightOffset = 60.f;

    // 수평 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    float HorizontalOffset = 50.f;
};
