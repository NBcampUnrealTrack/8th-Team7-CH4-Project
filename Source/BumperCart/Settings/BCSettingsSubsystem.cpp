// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/BCSettingsSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

//오디오 설정을 저장할 ini 섹션. 비디오는 UGameUserSettings가 알아서 저장하므로 오디오만 직접 관리
static const TCHAR* AudioSettingsSection = TEXT("/Script/BumperCart.BCAudioSettings");

//설정 화면에서 쓰는 사운드 클래스/믹스 경로
static const TCHAR* SettingsSoundMixPath = TEXT("/Game/BumperCart/Audio/SMIX_BumperCart.SMIX_BumperCart");
static const TCHAR* MasterSoundClassPath = TEXT("/Game/BumperCart/Audio/SC_Master.SC_Master");
static const TCHAR* BGMSoundClassPath = TEXT("/Game/BumperCart/Audio/SC_BGM.SC_BGM");
static const TCHAR* SFXSoundClassPath = TEXT("/Game/BumperCart/Audio/SC_SFX.SC_SFX");

//UI 창 enum을 언리얼 엔진 창 모드 타입으로 변환
static EWindowMode::Type ToEngineWindowMode(EBCDisplayMode DisplayMode)
{
    switch (DisplayMode)
    {
    case EBCDisplayMode::Fullscreen:
        return EWindowMode::Fullscreen;

    case EBCDisplayMode::Windowed:
        return EWindowMode::Windowed;

    case EBCDisplayMode::Borderless:
        return EWindowMode::WindowedFullscreen;

    default:
        return EWindowMode::Fullscreen;
    }
}

//언리얼 창 모드 타입을 UI enum으로 변환
static EBCDisplayMode ToBCDisplayMode(EWindowMode::Type WindowMode)
{
    switch (WindowMode)
    {
    case EWindowMode::Fullscreen:
        return EBCDisplayMode::Fullscreen;

    case EWindowMode::Windowed:
        return EBCDisplayMode::Windowed;

    case EWindowMode::WindowedFullscreen:
        return EBCDisplayMode::Borderless;

    default:
        return EBCDisplayMode::Fullscreen;
    }
}

void UBCSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //설정 화면에서만 쓰는 에셋이라 시작할 때 한 번만 로드해두고 재사용
    SettingsSoundMix = LoadObject<USoundMix>(nullptr, SettingsSoundMixPath);
    MasterSoundClass = LoadObject<USoundClass>(nullptr, MasterSoundClassPath);
    BGMSoundClass = LoadObject<USoundClass>(nullptr, BGMSoundClassPath);
    SFXSoundClass = LoadObject<USoundClass>(nullptr, SFXSoundClassPath);

    //저장해둔 음량을 게임 시작 시점에 바로 반영
    CurrentAudioSettings = LoadAudioSettingsFromConfig();
    ApplyVolumeToSoundClasses(CurrentAudioSettings);
}

FBCVideoSettings UBCSettingsSubsystem::GetCurrentVideoSettings() const
{
    FBCVideoSettings Result;

    if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        const FIntPoint Resolution = UserSettings->GetScreenResolution();

        Result.DisplayMode = ToBCDisplayMode(UserSettings->GetFullscreenMode());
        Result.ResolutionX = Resolution.X;
        Result.ResolutionY = Resolution.Y;
        Result.GraphicsQuality = UserSettings->GetOverallScalabilityLevel();
        Result.bVSync = UserSettings->IsVSyncEnabled();
    }

    return Result;
}

void UBCSettingsSubsystem::ApplyVideoSettings(const FBCVideoSettings& Settings, bool bSaveSettings)
{
    if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        UserSettings->SetFullscreenMode(ToEngineWindowMode(Settings.DisplayMode));
        UserSettings->SetScreenResolution(FIntPoint(Settings.ResolutionX, Settings.ResolutionY));
        UserSettings->SetOverallScalabilityLevel(Settings.GraphicsQuality);
        UserSettings->SetVSyncEnabled(Settings.bVSync);

        UserSettings->ApplySettings(false);

        if (bSaveSettings)
        {
            UserSettings->SaveSettings();
        }
    }
}

void UBCSettingsSubsystem::RevertVideoSettings()
{
    if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        UserSettings->LoadSettings(false);
        UserSettings->ApplySettings(false);
    }
}


FBCVideoSettings UBCSettingsSubsystem::GetDefaultVideoSettings() const
{
    FBCVideoSettings Result;
    Result.DisplayMode = EBCDisplayMode::Fullscreen;
    Result.ResolutionX = 1920;
    Result.ResolutionY = 1080;
    Result.GraphicsQuality = 2;
    Result.bVSync = false;
    return Result;
}

TArray<FIntPoint> UBCSettingsSubsystem::GetSupportedResolutions() const
{
    TArray<FIntPoint> Result;

    //현재 지원하는 전체화면 해상도 가져옴
    UKismetSystemLibrary::GetSupportedFullscreenResolutions(Result);

    //일부 환경에서 해상도 목록을 못 가져올 수 있으므로 기본 목록을 제공
    if (Result.Num() == 0)
    {
        Result.Add(FIntPoint(1280, 720));
        Result.Add(FIntPoint(1366, 768));
        Result.Add(FIntPoint(1600, 900));
        Result.Add(FIntPoint(1920, 1080));
        Result.Add(FIntPoint(2560, 1440));
        Result.Add(FIntPoint(3840, 2160));
    }

    //16:10 모니터처럼 1920x1080이 모드 목록에 없는 환경이 있다.
    //목록에 없으면 UI가 그 해상도를 고를 수 없으므로, 기본값과 현재 적용된 해상도는 항상 넣어준다.
    const FBCVideoSettings DefaultSettings = GetDefaultVideoSettings();
    Result.AddUnique(FIntPoint(DefaultSettings.ResolutionX, DefaultSettings.ResolutionY));

    if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        Result.AddUnique(UserSettings->GetScreenResolution());
    }

    //작은 해상도부터 순서대로
    Result.Sort([](const FIntPoint& A, const FIntPoint& B)
        {
            return (A.X * A.Y) < (B.X * B.Y);
        });

    return Result;
}

FString UBCSettingsSubsystem::ResolutionToString(const FIntPoint& Resolution) const
{
    return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}

FIntPoint UBCSettingsSubsystem::MakeResolution(int32 X, int32 Y) const
{
    return FIntPoint(X, Y);
}

FBCAudioSettings UBCSettingsSubsystem::GetCurrentAudioSettings() const
{
    return CurrentAudioSettings;
}

void UBCSettingsSubsystem::ApplyAudioSettings(const FBCAudioSettings& Settings, bool bSaveSettings)
{
    CurrentAudioSettings = Settings;

    ApplyVolumeToSoundClasses(CurrentAudioSettings);

    if (bSaveSettings)
    {
        SaveAudioSettingsToConfig(CurrentAudioSettings);
    }
}

void UBCSettingsSubsystem::RevertAudioSettings()
{
    CurrentAudioSettings = LoadAudioSettingsFromConfig();

    ApplyVolumeToSoundClasses(CurrentAudioSettings);
}

FBCAudioSettings UBCSettingsSubsystem::GetDefaultAudioSettings() const
{
    FBCAudioSettings Result;
    Result.MasterVolume = 0.5f;
    Result.BGMVolume = 0.5f;
    Result.SFXVolume = 0.5f;
    return Result;
}

FString UBCSettingsSubsystem::VolumeToString(float Volume) const
{
    return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(FMath::Clamp(Volume, 0.0f, 1.0f) * 100.0f));
}

void UBCSettingsSubsystem::ApplyVolumeToSoundClasses(const FBCAudioSettings& Settings)
{
    if (!SettingsSoundMix)
    {
        return;
    }

    const float Master = FMath::Clamp(Settings.MasterVolume, 0.0f, 1.0f);
    const float BGM = FMath::Clamp(Settings.BGMVolume, 0.0f, 1.0f);
    const float SFX = FMath::Clamp(Settings.SFXVolume, 0.0f, 1.0f);

    //배경음/효과음 클래스는 마스터의 자식이라 마스터 볼륨이 한 번 더 곱해짐
    if (MasterSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(this, SettingsSoundMix, MasterSoundClass, Master, 1.0f, 0.0f, true);
    }

    if (BGMSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(this, SettingsSoundMix, BGMSoundClass, BGM, 1.0f, 0.0f, true);
    }

    if (SFXSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(this, SettingsSoundMix, SFXSoundClass, SFX, 1.0f, 0.0f, true);
    }

    //믹스를 밀어 넣어야 위에서 덮어쓴 볼륨이 실제 출력에 반영됨
    UGameplayStatics::PushSoundMixModifier(this, SettingsSoundMix);
}

FBCAudioSettings UBCSettingsSubsystem::LoadAudioSettingsFromConfig() const
{
    FBCAudioSettings Result = GetDefaultAudioSettings();

    if (GConfig)
    {
        GConfig->GetFloat(AudioSettingsSection, TEXT("MasterVolume"), Result.MasterVolume, GGameUserSettingsIni);
        GConfig->GetFloat(AudioSettingsSection, TEXT("BGMVolume"), Result.BGMVolume, GGameUserSettingsIni);
        GConfig->GetFloat(AudioSettingsSection, TEXT("SFXVolume"), Result.SFXVolume, GGameUserSettingsIni);
    }

    Result.MasterVolume = FMath::Clamp(Result.MasterVolume, 0.0f, 1.0f);
    Result.BGMVolume = FMath::Clamp(Result.BGMVolume, 0.0f, 1.0f);
    Result.SFXVolume = FMath::Clamp(Result.SFXVolume, 0.0f, 1.0f);

    return Result;
}

void UBCSettingsSubsystem::SaveAudioSettingsToConfig(const FBCAudioSettings& Settings) const
{
    if (!GConfig)
    {
        return;
    }

    GConfig->SetFloat(AudioSettingsSection, TEXT("MasterVolume"), Settings.MasterVolume, GGameUserSettingsIni);
    GConfig->SetFloat(AudioSettingsSection, TEXT("BGMVolume"), Settings.BGMVolume, GGameUserSettingsIni);
    GConfig->SetFloat(AudioSettingsSection, TEXT("SFXVolume"), Settings.SFXVolume, GGameUserSettingsIni);

    GConfig->Flush(false, GGameUserSettingsIni);
}
