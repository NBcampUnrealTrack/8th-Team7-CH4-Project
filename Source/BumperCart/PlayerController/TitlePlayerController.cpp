// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/TitlePlayerController.h"
#include "Blueprint/UserWidget.h"

void ATitlePlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() == false)
    {
        return;
    }

    if (IsValid(UIWidgetClass) == true)
    {
        UIWidgetInstance = CreateWidget<UUserWidget>(this, UIWidgetClass);
        if (IsValid(UIWidgetInstance) == true)
        {
            UIWidgetInstance->AddToViewport();

            FInputModeUIOnly Mode;
            Mode.SetWidgetToFocus(UIWidgetInstance->GetCachedWidget());
            SetInputMode(Mode);

            bShowMouseCursor = true;
        }
    }
}
