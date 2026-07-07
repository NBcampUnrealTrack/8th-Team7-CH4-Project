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

    // 캐릭터 선택 테스트
    UFUNCTION()
    void OnCharacter1Clicked();
    UFUNCTION()
    void OnCharacter2Clicked();
    UFUNCTION()
    void OnCharacter3Clicked();
    UFUNCTION()
    void OnCharacter4Clicked();
    // 실제 캐릭터 선택 처리
    void HandleCharacterButtonClicked(int32 CharacterIndex);

    // 캐릭터 선택 버튼들의 텍스트/활성화 상태 갱신
    void RefreshCharacterSelectionUI();

    void RefreshPlayerList();
    void UpdateStartButtonVisibility();
    void UpdateReadyButtonVisibility();
    void UpdateReadyButtonLabel(bool bIsReady);
    void RefreshRoomTitle();

    int32 MinPlayersToStart;


    TArray<UButton*> CharacterButtons;
    TArray<UTextBlock*> CharacterStatusTexts;


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


    // 캐릭터 선택 버튼 4개 (WBP에서 동일한 이름으로 위젯을 배치해야 함)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> Character1Button;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> Character2Button;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> Character3Button;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> Character4Button;

    // 각 캐릭터 버튼 위에 표시할 이름 / 선택-확정 상태 텍스트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> Character1StatusText;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> Character2StatusText;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> Character3StatusText;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget|Character", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> Character4StatusText;
};
