// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameMode/LobbyGameMode.h"
#include "GameState/LobbyGameState.h"
#include "Kismet/GameplayStatics.h"


void ULobbyWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    MinPlayersToStart = 2;

    APlayerController* PC = GetOwningPlayer();

    if (StartGameButton)
    {
        StartGameButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnStartGameClicked);

        const bool bIsHost = PC && PC->HasAuthority();
        StartGameButton->SetVisibility((bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed));
    }

    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->OnLobbyPlayersChanged.AddDynamic(this, &ULobbyWidget::HandleLobbyPlayersChanged);
    }

    RefreshPlayerList();
    UpdateStartButtonVisibility();

}

void ULobbyWidget::NativeDestruct()
{
    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->OnLobbyPlayersChanged.RemoveDynamic(this, &ULobbyWidget::HandleLobbyPlayersChanged);
    }


    Super::NativeDestruct();
}

void ULobbyWidget::HandleLobbyPlayersChanged()
{
    RefreshPlayerList();
    UpdateStartButtonVisibility();
}

void ULobbyWidget::OnStartGameClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    if (ALobbyGameMode* GM = Cast<ALobbyGameMode>(PC->GetWorld()->GetAuthGameMode()))
    {
        GM->StartGame();
    }
}

void ULobbyWidget::RefreshPlayerList()
{
    ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
    if (!GS || !PlayerListBox)
    {
        return;
    }

    PlayerListBox->ClearChildren();

    for (const FString& Name : GS->GetReplicatedPlayerNames())
    {
        UTextBlock* NameText = NewObject<UTextBlock>(this);
        if (NameText)
        {
            NameText->SetText(FText::FromString(Name));
            PlayerListBox->AddChildToVerticalBox(NameText);
        }
    }

    if (PlayerCountText)
    {
        PlayerCountText->SetText(FText::FromString(
            FString::Printf(TEXT("접속 인원: %d명"), GS->GetReplicatedPlayerNames().Num())
        ));
    }
}

void ULobbyWidget::UpdateStartButtonVisibility()
{
    if (!StartGameButton)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    const bool bIsHost = PC && PC->HasAuthority();

    ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
    const int32 CurrentPlayerCount = GS ? GS->GetReplicatedPlayerNames().Num() : 0;
    const bool bHasEnoughPlayers = CurrentPlayerCount >= MinPlayersToStart;

    StartGameButton->SetVisibility(
        (bIsHost && bHasEnoughPlayers) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
    );
}

