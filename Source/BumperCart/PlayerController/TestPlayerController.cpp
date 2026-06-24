// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/TestPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameInstanceSubsystem/MainGameInstanceSubsystem.h"

class UMainGameInstanceSubsystem;

void ATestPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() == false)
    {
        return;
    }

    if (LobbyWidgetClass)
    {
        LobbyWidgetInstance = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
        if (LobbyWidgetInstance)
        {
            LobbyWidgetInstance->AddToViewport();
        }
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMainGameInstanceSubsystem* Sub = GI->GetSubsystem<UMainGameInstanceSubsystem>())
        {
            ServerRPCSetDisplayName(Sub->CachedDisplayName);
        }
    }
}

void ATestPlayerController::ServerRPCSetDisplayName_Implementation(const FString& DisplayName)
{
    if (PlayerState)
    {
        PlayerState->SetPlayerName(DisplayName);
    }
}
