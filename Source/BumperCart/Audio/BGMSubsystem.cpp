// Fill out your copyright notice in the Description page of Project Settings.


#include "Audio/BGMSubsystem.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameInstance/MainGameInstance.h"
#include "BGMConfig.h"

void UBGMSubsystem::PlayBGM(EBGMScene Scene)
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer)
    {
        StopBGM();
        return;
    }

    const UMainGameInstance* GI = World->GetGameInstance<UMainGameInstance>();
    const UBGMConfig* Config = GI ? GI->BGMConfig : nullptr;
    const FBGMSceneInfo* Info = Config ? Config->FindBGMInfo(Scene) : nullptr;

    // 적용할 사운드가 없다면 종료
    if (!Info || !IsValid(Info->Sound))
    {
        StopBGM();
        return;
    }

    // 같은 BGM이라면 재생 위치 유지
    if (IsValid(AudioComponent)
        && AudioComponent->IsPlaying()
        && AudioComponent->GetSound() == Info->Sound)
    {
        CurrentScene = Scene;
        return;
    }

    // 기존 BGM은 종료, 이전 음악의 설정으로 FadeOut
    if (IsValid(AudioComponent))
    {
        float FadeOutDuration = 1.f;

        if (const FBGMSceneInfo* CurrentInfo = Config->FindBGMInfo(CurrentScene))
        {
            FadeOutDuration = CurrentInfo->FadeOutDuration;
        }

        AudioComponent->FadeOut(FadeOutDuration, 0.f);
    }

    AudioComponent = UGameplayStatics::CreateSound2D(
        World,
        Info->Sound,
        Info->Volume,
        1.f,
        0.f,
        nullptr,
        true,   // 레벨 전환 시 유지
        true    // 정지 후 자동 제거
    );

    if (IsValid(AudioComponent))
    {
        AudioComponent->FadeIn(Info->FadeInDuration);
        CurrentScene = Scene;
    }
    else
    {
        CurrentScene = EBGMScene::None;
    }
}

void UBGMSubsystem::StopBGM(float FadeOutDuration)
{
    if (IsValid(AudioComponent))
    {
        AudioComponent->FadeOut(FadeOutDuration, 0.f);
    }

    AudioComponent = nullptr;
    CurrentScene = EBGMScene::None;
}
