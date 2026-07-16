// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDScoreboardRowWidget.generated.h"


class AMainPlayerState;

UCLASS()
class BUMPERCART_API UHUDScoreboardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure)
    int32 GetBaseIndex() const;

    UFUNCTION(BlueprintCallable)
    void SetBaseIndex(int32 NewIndex);

    // 타겟 위치를 지정해서 움직일 준비를 하는 함수
    UFUNCTION(BlueprintCallable)
    bool PrepareMove(float NewTargetY);

    // Alpha에 따라 Lerp로 이동하는 함수
    UFUNCTION(BlueprintCallable)
    void ApplyMove(float Alpha);

    // 이동이 끝나면 정확한 위치로 맞추는 함수
    UFUNCTION(BlueprintCallable)
    void SnapToTarget();

    // Blueprint에서 구현
    UFUNCTION(BlueprintImplementableEvent)
    void UpdateRow(AMainPlayerState* PlayerState, int32 DisplayRank);

private:
    int32 BaseIndex = INDEX_NONE;
    float MoveStartY = 0.f;
    float MoveTargetY = 0.f;
};
