// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/BCSettingsSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

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
