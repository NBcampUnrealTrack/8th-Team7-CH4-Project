// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BCSettingsTypes.h"
#include "BCSettingsSubsystem.generated.h"

class USoundClass;
class USoundMix;

UCLASS(BlueprintType)
class BUMPERCART_API UBCSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

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

    //현재 적용된 오디오 설정 읽기
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    FBCAudioSettings GetCurrentAudioSettings() const;

    //선택한 오디오 설정 적용
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    void ApplyAudioSettings(const FBCAudioSettings& Settings, bool bSaveSettings = true);

    //저장되어 있던 오디오 설정 다시 불러와 적용
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    void RevertAudioSettings();

    //기본 오디오 설정값
    UFUNCTION(BlueprintCallable, Category = "BumperCart|Settings")
    FBCAudioSettings GetDefaultAudioSettings() const;

    //0.75 같은 값을 "75%" 텍스트로 바꿔 UI에 표시
    UFUNCTION(BlueprintPure, Category = "BumperCart|Settings")
    FString VolumeToString(float Volume) const;

private:
    //사운드 클래스별 볼륨을 덮어써 실제 출력에 반영
    void ApplyVolumeToSoundClasses(const FBCAudioSettings& Settings);

    //ini에서 오디오 설정을 읽고 씀 (비디오 설정은 UGameUserSettings가 담당)
    FBCAudioSettings LoadAudioSettingsFromConfig() const;
    void SaveAudioSettingsToConfig(const FBCAudioSettings& Settings) const;

    //현재 세션에서 적용 중인 오디오 설정 (슬라이더를 움직일 때마다 갱신)
    UPROPERTY(Transient)
    FBCAudioSettings CurrentAudioSettings;

    UPROPERTY(Transient)
    TObjectPtr<USoundMix> SettingsSoundMix;

    UPROPERTY(Transient)
    TObjectPtr<USoundClass> MasterSoundClass;

    UPROPERTY(Transient)
    TObjectPtr<USoundClass> BGMSoundClass;

    UPROPERTY(Transient)
    TObjectPtr<USoundClass> SFXSoundClass;

};
