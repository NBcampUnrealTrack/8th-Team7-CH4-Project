// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TestPlayerController.generated.h"

class UUserWidget;

UCLASS()
class BUMPERCART_API ATestPlayerController : public APlayerController
{
	GENERATED_BODY()


public:
    virtual void BeginPlay() override;

    UFUNCTION(Server, Reliable)
    void ServerRPCSetDisplayName(const FString& DisplayName);

protected:
    // 에디터에서 어떤 위젯 블루프린트를 띄울지 지정
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|UI")
    TSubclassOf<UUserWidget> LobbyWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<UUserWidget> LobbyWidgetInstance;

};
