// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/MainPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "PlayerState/MainPlayerState.h"
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
}

void AMainPlayerController::NotifyPlayerStateReady()
{
    if (!IsLocalController()) return;

    if (AMainPlayerState* PS = GetPlayerState<AMainPlayerState>())
    {
        OnPlayerStateReady.Broadcast(PS);
    }
}
