// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BCSettingsTypes.h"
#include "BCSettingsSubsystem.generated.h"

UCLASS(BlueprintType)
class BUMPERCART_API UBCSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    //현재 적용된 비디오 설정 읽기
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    FBCVideoSettings GetCurrentVideoSettings() const;

    //선택한 비디오 설정 적용
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    void ApplyVideoSettings(const FBCVideoSettings& Settings, bool bSaveSettings = true);

    //저장되어 있던 설정 다시 불러와 적용
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    void RevertVideoSettings();

    //기본 비디오 설정값
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    FBCVideoSettings GetDefaultVideoSettings() const;

    //현재 모니터에서 지원하는 전체화면 해상도 목록을 가져옴
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    TArray<FIntPoint> GetSupportedResolutions() const;

    //1920,1080 같은 값을 "1920 x 1080" 텍스트로 바꿔 UI에 표시
    UFUNCTION(BlueprintPure, Category = "BumperCart|Settings")
    FString ResolutionToString(const FIntPoint& Resolution) const;

    //블루프린트에서 IntPoint 해상도 값을 만들기 위한 보조 함수
    UFUNCTION(BlueprintPure, Category = "BumperCart|Settings")
    FIntPoint MakeResolution(int32 X, int32 Y) const;

};
