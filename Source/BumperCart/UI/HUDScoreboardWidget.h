// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDScoreboardWidget.generated.h"

class UHUDScoreboardRowWidget;
class AMainPlayerState;
class AMainGameState;
class UPanelWidget;
class FViewport;

UCLASS()
class BUMPERCART_API UHUDScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    // 점수판 갱신
    UFUNCTION(BlueprintCallable)
    void RefreshScoreboard();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHUDScoreboardRowWidget> Row_1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHUDScoreboardRowWidget> Row_2;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHUDScoreboardRowWidget> Row_3;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHUDScoreboardRowWidget> Row_4;

    // Row_1~4 가 배치된 부모
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> Vb_Root;

    // 이동 시간
    UPROPERTY(EditDefaultsOnly, Category = "Scoreboard", meta = (ClampMin = "0.0"))
    float MoveDuration = 0.25f;

    // 이동 타이머 호출 간격
    UPROPERTY(EditDefaultsOnly, Category = "Scoreboard", meta = (ClampMin = "0.001"))
    float MoveInterval = 0.016f;

private:
    // 플레이어 점수가 변경되면 호출하는 함수
    UFUNCTION()
    void HandlePlayerScoreChanged();

    // 플레이어 추가되면 호출하는 함수
    UFUNCTION()
    void HandlePlayerStateAdded(AMainPlayerState* PlayerState);

    // 플레이어 제거되면 호출하는 함수
    UFUNCTION()
    void HandlePlayerStateRemoved(AMainPlayerState* PlayerState);

    /* GameStae, PlayerState 관련 */

    void TryCacheGameState();
    void SyncPlayerStates();

    void AddPlayerRow(AMainPlayerState* PlayerState);
    void RemovePlayerRow(AMainPlayerState* PlayerState);

    // 사용 가능한 행 찾기
    UHUDScoreboardRowWidget* FindAvailableRow() const;

    // 다음 틱에 행 재배치하는 함수
    void QueueRefresh();

    /* 레이아웃 사이즈 관련 처리 */

    void QueueCacheRowLayout();
    void CacheRowLayout();
    void HandleViewportResized(FViewport* Viewport, uint32 Unused);


    /* 이동 애니메이션 처리 */

    void StartMoveAnimation();
    void UpdateMoveAnimation();
    void StopMoveAnimation();

private:
    UPROPERTY(Transient)
    TObjectPtr<AMainGameState> CachedGameState;

    // HUDScoreboardRowWidget를 저장하는 배열
    // 항상 4개의 열을 관리, 사용하지 않는것만 Hidden 해야 함
    UPROPERTY(Transient)
    TArray<TObjectPtr<UHUDScoreboardRowWidget>> Rows;

    // MainPlayerState, HUDScoreboardRowWidget을 저장하는 Map
    // 실제 플레이어 수만큼 관리
    UPROPERTY(Transient)
    TMap<TObjectPtr<AMainPlayerState>, TObjectPtr<UHUDScoreboardRowWidget>> PlayerRows;

    // VB_Root 좌표 기준 각 행의 Y 위치
    TArray<float> SlotPositionsY;

    FTimerHandle GameStateCacheTimerHandle;
    FTimerHandle LayoutCacheTimerHandle;
    FTimerHandle MoveTimerHandle;

    FDelegateHandle ViewportResizedHandle;

    double MoveStartTime = 0.0;

    bool bLayoutReady = false;
    bool bLayoutCacheQueued = false;
    bool bHasInitialized = false;
    bool bRefreshQueued = false;
};
