// MainPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MainPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStatsChanged);

UCLASS()
class BUMPERCART_API AMainPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
    AMainPlayerState();

    // 플레이어 수치가 변할 시 호출됨
    UPROPERTY(BlueprintAssignable, Category = "PlayerStats")
    FOnPlayerStatsChanged OnPlayerStatsChanged;

    //점수 추가
    void AddScore(float AddScore);

    float GetScore() const;

    //플레이어 순위
    void SetRank(int32 NewRank);

    int32 GetRank() const;

    //계산대 이용 횟수
    void AddCheckoutCount(int32 Count);
    int32 GetCheckoutCount() const;

    //카트끼리 충돌 횟수
    void AddCartBumpCount(int32 Count);
    int32 GetCartBumpCount() const;

    //카트에서 상품 낙하 횟수
    void AddDroppedItemCount(int32 Count);
    int32 GetDroppedItemCount() const;

    //라운드 시작 시 통계 초기화
    void ResetStats();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_PlayerScore();

    UFUNCTION()
    void OnRep_Rank();

    UFUNCTION()
    void OnRep_CheckoutCount();

    UFUNCTION()
    void OnRep_CartBumpCount();

    UFUNCTION()
    void OnRep_DroppedItemCount();

    UPROPERTY(ReplicatedUsing = OnRep_PlayerScore, VisibleAnywhere,  Category = "PlayerStats")
    int32 PlayerScore = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Rank, VisibleAnywhere,  Category = "PlayerStats")
    int32 Rank = 0;

    UPROPERTY(ReplicatedUsing = OnRep_CheckoutCount, VisibleAnywhere,  Category = "PlayerStats")
    int32 CheckoutCount = 0;

    UPROPERTY(ReplicatedUsing = OnRep_CartBumpCount, VisibleAnywhere,  Category = "PlayerStats")
    int32 BumpCartCount = 0;

    UPROPERTY(ReplicatedUsing = OnRep_DroppedItemCount, VisibleAnywhere,  Category = "PlayerStats")
    int32 DroppedItemCount = 0;

};
