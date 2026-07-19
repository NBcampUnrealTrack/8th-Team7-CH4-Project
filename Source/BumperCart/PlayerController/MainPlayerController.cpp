// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/MainPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "PlayerState/MainPlayerState.h"
#include "GameState/MainGameState.h"
#include "Audio/BGMSubsystem.h"

void AMainPlayerController::InitPlayerState()
{
    Super::InitPlayerState();
    NotifyPlayerStateReady();
}

void AMainPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    NotifyPlayerStateReady();
}

void AMainPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController()) return;

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UBGMSubsystem* BGMSubsystem = GameInstance->GetSubsystem<UBGMSubsystem>())
        {
            BGMSubsystem->PlayBGM(EBGMScene::InGame);
        }
    }

    if (IsValid(UIWidgetClass))
    {
        UIWidgetInstance = CreateWidget<UUserWidget>(this, UIWidgetClass);
        if (IsValid(UIWidgetInstance))
        {
            UIWidgetInstance->AddToViewport();

            SetInputMode(FInputModeGameOnly());
            bShowMouseCursor = false;
        }
    }

    TryBindWaitingDelegate();
}

void AMainPlayerController::NotifyPlayerStateReady()
{
    if (!IsLocalController()) return;

    if (AMainPlayerState* PS = GetPlayerState<AMainPlayerState>())
    {
        OnPlayerStateReady.Broadcast(PS);
    }
}

void AMainPlayerController::TryBindWaitingDelegate()
{
    AMainGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMainGameState>() : nullptr;
    if (!GS)
    {
        GetWorldTimerManager().SetTimer(Timer_BindWaitingDelegate, this, &AMainPlayerController::TryBindWaitingDelegate, 0.1f, false);
        return;
    }

    GS->OnWaitingForPlayersChanged.AddDynamic(this, &AMainPlayerController::HandleWaitingForPlayersChanged);

    // 바인딩이 늦어서 이미 브로드캐스트를 놓쳤을 경우를 대비해 현재 값 즉시 반영
    HandleWaitingForPlayersChanged(GS->IsWaitingForPlayers());
}

void AMainPlayerController::HandleWaitingForPlayersChanged(bool bIsWaiting)
{
    if (bIsWaiting)
    {
        if (!IsValid(WaitingWidgetInstance) && IsValid(WaitingWidgetClass))
        {
            WaitingWidgetInstance = CreateWidget<UUserWidget>(this, WaitingWidgetClass);
            if (IsValid(WaitingWidgetInstance))
            {
                WaitingWidgetInstance->AddToViewport(1000);
            }
        }
    }
    else if (IsValid(WaitingWidgetInstance))
    {
        WaitingWidgetInstance->RemoveFromParent();
        WaitingWidgetInstance = nullptr;
    }
}
