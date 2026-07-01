// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "GameMode/LobbyGameMode.h"
#include "GameState/LobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/LobbyPlayerState.h"


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

    if (ReadyButton)
    {
        ReadyButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnReadyButtonClicked);
    }

    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->OnLobbyPlayersChanged.AddDynamic(this, &ULobbyWidget::HandleLobbyPlayersChanged);
    }

    RefreshRoomTitle();
    UpdateReadyButtonVisibility();
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

void ULobbyWidget::OnReadyButtonClicked()
{
    ALobbyPlayerState* PS = GetOwningPlayerState<ALobbyPlayerState>();
    if (!PS) return;

    const bool bNewReady = !PS->IsReady();

    // 준비 상태 요청
    PS->SetReady(bNewReady);
    UE_LOG(LogTemp, Warning, TEXT("준비 완료"));
    UpdateReadyButtonLabel(bNewReady);
}

void ULobbyWidget::RefreshRoomTitle()
{
    if (!RoomTitleText) return;

    FString RoomTitle;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMainGameInstanceSubsystem* Sub = GI->GetSubsystem<UMainGameInstanceSubsystem>())
        {
            RoomTitle = Sub->GetRoomName();
        }
    }

    RoomTitleText->SetText(FText::FromString(
        RoomTitle.IsEmpty() ? TEXT("방 제목 없음") : RoomTitle
    ));
}

void ULobbyWidget::UpdateReadyButtonVisibility()
{
    if (!ReadyButton) return;

    APlayerController* PC = GetOwningPlayer();
    const bool bIsHost = PC && PC->HasAuthority();

    ReadyButton->SetVisibility(bIsHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

    if (!bIsHost)
    {
        if (const ALobbyPlayerState* MyPS = GetOwningPlayerState<ALobbyPlayerState>())
        {
            UpdateReadyButtonLabel(MyPS->IsReady());
        }
    }
}
\
void ULobbyWidget::UpdateReadyButtonLabel(bool bIsReady)
{
    if (ReadyButtonText)
    {
        ReadyButtonText->SetText(FText::FromString(bIsReady ? TEXT("준비 완료 (취소하기)") : TEXT("준비하기")));
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

    for (const FLobbyPlayerInfo& Info : GS->GetReplicatedPlayerInfos())
    {
        FString StatusLabel;
        if (Info.bIsHost)
        {
            StatusLabel = TEXT("방장");
        }
        else
        {
            StatusLabel = Info.bIsReady ? TEXT("준비완료") : TEXT("대기중");
        }

        UTextBlock* NameText = NewObject<UTextBlock>(this);
        if (NameText)
        {
            NameText->SetText(FText::FromString(
                FString::Printf(TEXT("%s  [%s]"), *Info.PlayerName, *StatusLabel)
            ));
            PlayerListBox->AddChildToVerticalBox(NameText);
        }
    }

    if (PlayerCountText)
    {
        PlayerCountText->SetText(FText::FromString(
            FString::Printf(TEXT("접속 인원: %d명"), GS->GetReplicatedPlayerInfos().Num())
        ));
    }

    // 만약을 대비 해 본인 버튼 텍스트 다시 갱신
    if (const ALobbyPlayerState* MyPS = GetOwningPlayerState<ALobbyPlayerState>())
    {
        UpdateReadyButtonLabel(MyPS->IsReady());
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
    const int32 CurrentPlayerCount = GS ? GS->GetReplicatedPlayerInfos().Num() : 0;
    const bool bHasEnoughPlayers = CurrentPlayerCount >= MinPlayersToStart;
    const bool bAllReady = GS && GS->bIsAllPlayersReady();

    StartGameButton->SetVisibility(
        (bIsHost && bHasEnoughPlayers && bAllReady) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
    );
}

