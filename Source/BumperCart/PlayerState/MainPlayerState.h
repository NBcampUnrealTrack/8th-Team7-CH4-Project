// MainPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MainPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStatsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerScoreChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDisplayInfoChanged);

UENUM(BlueprintType)
enum class ETitleType : uint8
{
    Default UMETA(DisplayName = "오늘도 장봤다"),
    MartKing UMETA(DisplayName = "마트 지배자"),
    BumpKing UMETA(DisplayName = "통로의 재앙"),
    DestroyKing UMETA(DisplayName = "파괴왕"),
    ReceiptCollector UMETA(DisplayName = "영수증 컬렉터")
};

UCLASS()
class BUMPERCART_API AMainPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
    AMainPlayerState();

    // 플레이어 수치가 변할 시 호출됨
    UPROPERTY(BlueprintAssignable, Category = "PlayerStats")
    FOnPlayerStatsChanged OnPlayerStatsChanged;

    // 플레이어 점수가 변할 시 호출됨
    UPROPERTY(BlueprintAssignable, Category = "PlayerStats")
    FOnPlayerScoreChanged OnPlayerScoreChanged;

    // 플레이어 표시 정보 변할 시 호출됨
    UPROPERTY(BlueprintAssignable, Category = "PlayerStats")
    FOnPlayerDisplayInfoChanged OnPlayerDisplayInfoChanged;

    //점수 추가
    void AddPlayerScore(int32 AddScore);
    UFUNCTION(BlueprintPure, Category = "PlayerStats")
    int32 GetPlayerScore() const;

    //플레이어 순위
    void SetRank(int32 NewRank);
    UFUNCTION(BlueprintPure, Category = "PlayerStats")
    int32 GetRank() const;

    //계산대 이용 횟수
    void AddCheckoutCount(int32 Count);
    UFUNCTION(BlueprintPure, Category = "PlayerStats")
    int32 GetCheckoutCount() const;

    //카트끼리 충돌 횟수
    void AddCartBumpCount(int32 Count);
    UFUNCTION(BlueprintPure, Category = "PlayerStats")
    int32 GetCartBumpCount() const;

    //카트에서 상품 낙하 횟수
    void AddDroppedItemCount(int32 Count);
    UFUNCTION(BlueprintPure, Category = "PlayerStats")
    int32 GetDroppedItemCount() const;

    // 플레이어 칭호 조회
    UFUNCTION(BlueprintPure, Category = "PlayerStats")
    ETitleType GetTitle() const;

    void SetTitle(ETitleType NewTitle);

    //라운드 시작 시 통계 초기화
    void ResetStats();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnRep_PlayerName() override;

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

    UFUNCTION()
    void OnRep_Title();

    UPROPERTY(ReplicatedUsing = OnRep_PlayerScore, VisibleAnywhere,  Category = "PlayerStats")
    int32 PlayerScore;

    UPROPERTY(ReplicatedUsing = OnRep_Rank, VisibleAnywhere,  Category = "PlayerStats")
    int32 Rank;

    UPROPERTY(ReplicatedUsing = OnRep_CheckoutCount, VisibleAnywhere,  Category = "PlayerStats")
    int32 CheckoutCount;

    UPROPERTY(ReplicatedUsing = OnRep_CartBumpCount, VisibleAnywhere,  Category = "PlayerStats")
    int32 BumpCartCount;

    UPROPERTY(ReplicatedUsing = OnRep_DroppedItemCount, VisibleAnywhere,  Category = "PlayerStats")
    int32 DroppedItemCount;

    UPROPERTY(ReplicatedUsing = OnRep_Title, VisibleAnywhere, Category = "PlayerStats")
    ETitleType Title;

    //

};
