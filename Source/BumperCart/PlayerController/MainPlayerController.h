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

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void NotifyPlayerStateReady();

    UFUNCTION()
    void HandleRoundPhaseChanged();

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controller|Main", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> UIWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Controller|Main", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UUserWidget> UIWidgetInstance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Controller|Main|Sound", meta = (AllowPrivateAccess = "true"))
    float FeverPitchMultiplier = 1.05f;

#pragma region PlayerLoadingWait
private:
    // 플레이어 접속 대기 중 띄울 위젯
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controller|Main|Loading", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> WaitingWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Controller|Main|Loading", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UUserWidget> WaitingWidgetInstance;

    UFUNCTION()
    void HandleWaitingForPlayersChanged(bool bIsWaiting);

    // BeginPlay 시점에 GameState가 아직 준비 안 됐을 경우 재시도용
    void TryBindWaitingDelegate();

    FTimerHandle Timer_BindWaitingDelegate;
#pragma endregion
};
