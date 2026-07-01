// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BCSettingsTypes.generated.h"

UENUM(BlueprintType)
enum class EBCDisplayMode : uint8
{
    Fullscreen UMETA(DisplayName = "전체 화면"),
    Windowed UMETA(DisplayName = "창 모드"),
    Borderless UMETA(DisplayName = "테두리 없는 창")
};

USTRUCT(BlueprintType)
struct FBCVideoSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    EBCDisplayMode DisplayMode = EBCDisplayMode::Fullscreen;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 ResolutionX = 1920;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 ResolutionY = 1080;

    // 0 낮음, 1 보통, 2 높음, 3 최상
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 GraphicsQuality = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bVSync = false;

};
