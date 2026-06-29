// UW_TitleLayout.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_TitleLayout.generated.h"

class UComboBoxString;
class UButton;
class UEditableText;
class UTextBlock;
class UWidgetSwitcher;
class UVerticalBox;

UCLASS()
class BUMPERCART_API UUW_TitleLayout : public UUserWidget
{
	GENERATED_BODY()

public:
    UUW_TitleLayout(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
    virtual void NativeOnInitialized() override;

    UFUNCTION()
    void OnHostClicked();

    UFUNCTION()
    void OnJoinClicked();

    UFUNCTION()
    void OnFindClicked();

    UFUNCTION()
    void OnLoginClicked();

    UFUNCTION()
    void OnSessionsFoundHandler(int32 FoundCount);


    UFUNCTION()
    void HandleLoginResult(bool bWasSuccessful, const FString& Message);


    UFUNCTION()
    void OnCreateRoomConfirmClicked();


    UFUNCTION()
    void OnCreateRoomBackClicked();


    void ShowCreateRoomPanel(bool bShow);

    void RefreshSessionListUI(int32 FoundCount);



protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> LoginButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> HostButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> JoinButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> FindButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString>  PlayerSelectCombo;

    // 메인 메뉴 / 방 생성 패널 전환용 (0번: 메인 메뉴, 1번: 방 생성 패널)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UWidgetSwitcher> MenuSwitcher;

    // 방 제목 입력
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UEditableText> RoomNameInputText;

    // 방 비밀번호 입력
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UEditableText> RoomPasswordInputText;

    //  생성 버튼
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> CreateRoomConfirmButton;

    // 뒤로가기 버튼
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UButton> CreateRoomBackButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UTextBlock> CreateRoomWarningText;

    // 참가 시 입력할 비밀번호
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UEditableText> JoinPasswordInputText;

    //검색된 방 목록
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USTitleWidget, Meta = (AllowPrivateAccess, BindWidget))
    TObjectPtr<UVerticalBox> SessionListBox;

};
