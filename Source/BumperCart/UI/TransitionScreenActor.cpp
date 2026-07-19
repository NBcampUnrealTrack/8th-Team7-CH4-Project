// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TransitionScreenActor.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"

ATransitionScreenActor::ATransitionScreenActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ATransitionScreenActor::BeginPlay()
{
    Super::BeginPlay();

    if (LoadingWidgetClass && GEngine && GEngine->GameViewport)
    {
        LoadingWidget = CreateWidget<UUserWidget>(GetWorld(), LoadingWidgetClass);
        if (LoadingWidget)
        {
            LoadingWidget->AddToViewport(1000);
        }
    }
}

