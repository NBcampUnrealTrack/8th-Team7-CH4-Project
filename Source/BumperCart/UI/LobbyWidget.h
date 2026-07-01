#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

class UButton;
class UVerticalBox;
class UTextBlock;

UCLASS()
class BUMPERCART_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnReadyButtonClicked();

    void RefreshPlayerList();
    void UpdateStartButtonVisibility();
    void UpdateReadyButtonVisibility();
    void UpdateReadyButtonLabel(bool bIsReady);
    void RefreshRoomTitle();

    int32 MinPlayersToStart;


    UFUNCTION()
    void HandleLobbyPlayersChanged();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> StartGameButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UVerticalBox> PlayerListBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> PlayerCountText;

    // 방 제목 표시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> RoomTitleText;

    // 준비 버튼
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> ReadyButton;

    // 준비하기 / 준비 완료 토글
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> ReadyButtonText;
};
