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

    // 같은 BGM이고, 실행중이라면 넘기기
    if (CurrentScene == Scene && IsValid(AudioComponent)
        && AudioComponent->IsPlaying())
    {
        return;
    }

    const UMainGameInstance* GI = World->GetGameInstance<UMainGameInstance>();
    const UBGMConfig* Config = GI ? GI->BGMConfig : nullptr;
    const FBGMSceneInfo* Info = Config ? Config->FindBGMInfo(Scene) : nullptr;


    if (!Info || !IsValid(Info->Sound))
    {
        StopBGM();
        return;
    }

    // 기존 BGM은 종료
    if (IsValid(AudioComponent))
    {
        AudioComponent->FadeOut(Info->FadeOutDuration, 0.f);
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
    }

    CurrentScene = Scene;
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
