// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TitleGameMode.h"

#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"

void ATitleGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (UMainGameInstanceSubsystem* MainSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>() : nullptr)
    {
        MainSubsystem->StartAutoLoginRetry(2.0f);
    }
}
