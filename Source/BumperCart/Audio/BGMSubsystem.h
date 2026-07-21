// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BGMTypes.h"
#include "BGMSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class BUMPERCART_API UBGMSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void PlayBGM(EBGMScene Scene);

    UFUNCTION(BlueprintCallable)
    void StopBGM(float FadeOutDuration = 1.f);

    UFUNCTION(BlueprintCallable)
    void SetBGMPitchMultiplier(float NewPitchMultiplier);

private:
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AudioComponent;

    EBGMScene CurrentScene = EBGMScene::None;

    float CurrentPitchMultiplier = 1.f;
};
