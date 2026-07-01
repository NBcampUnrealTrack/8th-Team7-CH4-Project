// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

class UUserWidget;

UCLASS()
class BUMPERCART_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()
public:
    virtual void BeginPlay() override;


private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = ASUIPlayerController, Meta = (AllowPrivateAccess))
    TSubclassOf<UUserWidget> UIWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = ASUIPlayerController, Meta = (AllowPrivateAccess))
    TObjectPtr<UUserWidget> UIWidgetInstance;
};
