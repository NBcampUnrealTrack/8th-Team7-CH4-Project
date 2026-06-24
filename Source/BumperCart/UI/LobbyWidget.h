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

    void RefreshPlayerList();

    void UpdateStartButtonVisibility();

    int32 MinPlayersToStart;


    UFUNCTION()
    void HandleLobbyPlayersChanged();   // 델리게이트가 호출할 함수

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> StartGameButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UVerticalBox> PlayerListBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LobbyWidget", Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> PlayerCountText;
};
