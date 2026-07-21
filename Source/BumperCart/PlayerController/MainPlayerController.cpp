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

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UBGMSubsystem* BGM = GI->GetSubsystem<UBGMSubsystem>())
        {
            BGM->PlayBGM(EBGMScene::InGame);
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

void AMainPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        EndPlay(EndPlayReason);
        return;
    }

    World->GetTimerManager().ClearTimer(Timer_BindWaitingDelegate);

    if (IsLocalController())
    {
        if (AMainGameState* GS = World->GetGameState<AMainGameState>())
        {
            GS->OnWaitingForPlayersChanged.RemoveDynamic(this, &ThisClass::HandleWaitingForPlayersChanged);
            GS->OnRoundPhaseChanged.RemoveDynamic(this, &ThisClass::HandleRoundPhaseChanged);
        }

        if (UGameInstance* GI = GetGameInstance())
        {
            if (UBGMSubsystem* BGM = GI->GetSubsystem<UBGMSubsystem>())
            {
                BGM->SetBGMPitchMultiplier(1.f);
            }
        }
    }

    Super::EndPlay(EndPlayReason);
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
        GetWorldTimerManager().SetTimer(Timer_BindWaitingDelegate, this, &ThisClass::TryBindWaitingDelegate, 0.1f, false);
        return;
    }

    GS->OnWaitingForPlayersChanged.AddUniqueDynamic(this, &ThisClass::HandleWaitingForPlayersChanged);
    GS->OnRoundPhaseChanged.AddUniqueDynamic(this, &ThisClass::HandleRoundPhaseChanged);

    // 바인딩이 늦어서 이미 브로드캐스트를 놓쳤을 경우를 대비해 현재 값 즉시 반영
    HandleWaitingForPlayersChanged(GS->IsWaitingForPlayers());
    HandleRoundPhaseChanged();
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

void AMainPlayerController::HandleRoundPhaseChanged()
{
    AMainGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMainGameState>() : nullptr;
    UBGMSubsystem* BGM = GetGameInstance() ? GetGameInstance()->GetSubsystem<UBGMSubsystem>() : nullptr;

    if (!GS || !BGM) return;

    bool bFever = GS->GetCurrentPhase() == ERoundPhase::FinalWarningOneOpen;

    BGM->SetBGMPitchMultiplier(bFever ? 1.2f : 1.f);
}
