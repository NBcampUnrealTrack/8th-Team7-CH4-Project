// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/LobbyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "GameState/LobbyGameState.h"
#include "Audio/BGMSubsystem.h"

class UMainGameInstanceSubsystem;

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() == false)
    {
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UBGMSubsystem* BGMSubsystem = GameInstance->GetSubsystem<UBGMSubsystem>())
        {
            BGMSubsystem->PlayBGM(EBGMScene::Lobby);
        }
    }

    if (LobbyWidgetClass)
    {
        LobbyWidgetInstance = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
        if (LobbyWidgetInstance)
        {
            LobbyWidgetInstance->AddToViewport();
            bShowMouseCursor = true;
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

void ALobbyPlayerController::ServerRPCSetDisplayName_Implementation(const FString& DisplayName)
{
    if (PlayerState)
    {
        PlayerState->SetPlayerName(DisplayName);

        // 접속 직후 이름 갱신이 안되어 있을 수도 있으니 실제 세팅 이후 다시 갱신
        if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
        {
            GS->RefreshPlayerInfos();
        }
    }
}

void ALobbyPlayerController::Client_NotifyHostIsLeaving_Implementation()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMainGameInstanceSubsystem* Subsystem = GI->GetSubsystem<UMainGameInstanceSubsystem>())
        {
            Subsystem->NotifyLeaveRequestedByHost();
        }
    }
}

void ALobbyPlayerController::Server_AckLeftSession_Implementation()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMainGameInstanceSubsystem* Subsystem = GI->GetSubsystem<UMainGameInstanceSubsystem>())
        {
            Subsystem->OnClientAckLeftSession(this);
        }
    }
}
