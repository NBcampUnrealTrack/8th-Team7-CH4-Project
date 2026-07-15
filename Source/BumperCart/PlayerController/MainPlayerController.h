// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

class AMainPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateReady, AMainPlayerState*, PlayerState);

UCLASS()
class BUMPERCART_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    virtual void InitPlayerState() override;

    virtual void OnRep_PlayerState() override;


public:
    UPROPERTY(BlueprintAssignable)
    FOnPlayerStateReady OnPlayerStateReady;

protected:
    virtual void BeginPlay() override;

private:
    void NotifyPlayerStateReady();

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controller|Main", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> UIWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Controller|Main", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UUserWidget> UIWidgetInstance;
};
