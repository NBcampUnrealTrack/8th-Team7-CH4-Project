// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BGMTypes.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EBGMScene : uint8
{
    None,
    Title,
    Lobby,
    InGame
};

USTRUCT(BlueprintType)
struct FBGMSceneInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<USoundBase> Sound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float Volume = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float FadeInDuration = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float FadeOutDuration = 1.f;
};
