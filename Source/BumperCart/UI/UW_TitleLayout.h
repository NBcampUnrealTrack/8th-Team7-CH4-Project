// UW_TitleLayout.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_TitleLayout.generated.h"

class UComboBoxString;
class UButton;
class UEditableText;
class UTextBlock;

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


};
