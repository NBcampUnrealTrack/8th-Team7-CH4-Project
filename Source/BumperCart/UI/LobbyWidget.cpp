// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "GameMode/LobbyGameMode.h"
#include "DataAsset/CharacterSelectionConfig.h"
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

    CharacterButtons = { Character1Button, Character2Button, Character3Button, Character4Button };
    CharacterStatusTexts = { Character1StatusText, Character2StatusText, Character3StatusText, Character4StatusText };

    if (Character1Button) Character1Button->OnClicked.AddDynamic(this, &ULobbyWidget::OnCharacter1Clicked);
    if (Character2Button) Character2Button->OnClicked.AddDynamic(this, &ULobbyWidget::OnCharacter2Clicked);
    if (Character3Button) Character3Button->OnClicked.AddDynamic(this, &ULobbyWidget::OnCharacter3Clicked);
    if (Character4Button) Character4Button->OnClicked.AddDynamic(this, &ULobbyWidget::OnCharacter4Clicked);

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

    ReadyButton->SetVisibility(ESlateVisibility::Visible);

    if (const ALobbyPlayerState* MyPS = GetOwningPlayerState<ALobbyPlayerState>())
    {
        UpdateReadyButtonLabel(MyPS->IsReady());
    }

}

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

void ULobbyWidget::OnCharacter1Clicked() { HandleCharacterButtonClicked(0); }
void ULobbyWidget::OnCharacter2Clicked() { HandleCharacterButtonClicked(1); }
void ULobbyWidget::OnCharacter3Clicked() { HandleCharacterButtonClicked(2); }
void ULobbyWidget::OnCharacter4Clicked() { HandleCharacterButtonClicked(3); }

void ULobbyWidget::HandleCharacterButtonClicked(int32 CharacterIndex)
{
    ALobbyPlayerState* PS = GetOwningPlayerState<ALobbyPlayerState>();
    if (!PS) return;

    if (PS->IsReady()) return;

    PS->SelectCharacter(CharacterIndex);

    RefreshCharacterSelectionUI();
}

void ULobbyWidget::RefreshCharacterSelectionUI()
{
    ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
    const ALobbyPlayerState* MyPS = GetOwningPlayerState<ALobbyPlayerState>();
    if (!GS) return;

    const TArray<FCharacterData>& Characters = GS->GetAvailableCharacters();
    const int32 MySelectedIndex = MyPS ? MyPS->GetSelectedCharacterIndex() : INDEX_NONE;
    const bool bAmIReady = MyPS && MyPS->IsReady();

    for (int32 i = 0; i < 4; ++i)
    {
        UButton* Button = CharacterButtons.IsValidIndex(i) ? CharacterButtons[i] : nullptr;
        UTextBlock* StatusText = CharacterStatusTexts.IsValidIndex(i) ? CharacterStatusTexts[i] : nullptr;
        if (!Button && !StatusText) continue;

        const FString CharacterName = Characters.IsValidIndex(i)
            ? Characters[i].DisplayName.ToString()
            : FString::Printf(TEXT("캐릭터 %d"), i + 1);

        // 나를 제외한 다른 플레이어가 이미 확정(준비완료)한 캐릭터인지 확인
        const bool bTakenByOther = MyPS && GS->IsCharacterIndexSelectedByOtherPlayer(i, MyPS);

        FString StatusLabel;
        if (bTakenByOther)
        {
            StatusLabel = TEXT("확정됨 (다른 플레이어)");
        }
        else if (MySelectedIndex == i)
        {
            StatusLabel = bAmIReady ? TEXT("확정됨 (나)") : TEXT("선택됨");
        }
        else
        {
            StatusLabel = TEXT("선택 가능");
        }

        if (StatusText)
        {
            StatusText->SetText(FText::FromString(
                FString::Printf(TEXT("%s\n[%s]"), *CharacterName, *StatusLabel)
            ));
        }

        if (Button)
        {
            // 다른 플레이어가 확정했거나, 내가 이미 준비완료 상태면 버튼 비활성화
            const bool bCanClick = !bTakenByOther && !bAmIReady;
            Button->SetIsEnabled(bCanClick);

            // 내가 선택 중인 캐릭터는 살짝 다른 스타일로 표시(선택 강조)
            FButtonStyle Style = Button->WidgetStyle;
            const FLinearColor NormalTint = (MySelectedIndex == i) ? FLinearColor(0.3f, 0.7f, 1.0f) : FLinearColor::White;
            Style.Normal.TintColor = FSlateColor(NormalTint);
            Style.Hovered.TintColor = FSlateColor(NormalTint);
            Button->SetStyle(Style);
        }
    }
}
