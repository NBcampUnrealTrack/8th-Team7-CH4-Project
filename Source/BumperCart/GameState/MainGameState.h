// MainGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MainGameState.generated.h"

UENUM(BlueprintType)
enum class ERoundPhase : uint8
{
    WaitingToStart        UMETA(DisplayName = "게임 시작 전 대기"),
    RoundStart            UMETA(DisplayName = "0:00 - 라운드 시작"),
    RandomOpenTwo         UMETA(DisplayName = "0:30 - 랜덤 오픈 시작"),
    SaleEvent             UMETA(DisplayName = "1:00 - 세일 상품 이벤트"),
    PremiumRespawn        UMETA(DisplayName = "1:30 - 중앙 고급 상품 리스폰"),
    FinalWarningOneOpen   UMETA(DisplayName = "2:30 - 막판 30초 경고"),
    RoundEnd              UMETA(DisplayName = "3:00 - 라운드 종료")
};

class AMainPlayerState;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundPhaseChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMainPlayerStateChanged, AMainPlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaitingForPlayersChanged, bool, bIsWaiting);


UCLASS()
class BUMPERCART_API AMainGameState : public AGameState
{
	GENERATED_BODY()

public:
    // Phase가 바뀔 때마다 호출
    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnRoundPhaseChanged OnRoundPhaseChanged;

    // 플레이어가 추가될 때마다 호출
    UPROPERTY(BlueprintAssignable, Category = "Player")
    FOnMainPlayerStateChanged OnMainPlayerStateAdded;

    // 플레이어가 제거될 때마다 호출
    UPROPERTY(BlueprintAssignable, Category = "Player")
    FOnMainPlayerStateChanged OnMainPlayerStateRemoved;

    // 라운드 시간(3분)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round")
    float RoundDurationSeconds = 180.f;

    // 서버에서만 호출
    // 라운드 페이즈 지정 및 라운드 시작 시간 지정
    void SetRoundPhase(ERoundPhase NewPhase);
    void SetRoundStartTime();

    //최종 1등 명단 저장
    void SetFinalWinners(const TArray<FString>& Winners);

    // 현재 페이즈 가져오기
    UFUNCTION(BlueprintPure, Category = "Round")
    ERoundPhase GetCurrentPhase() const;

    // 서버-클라이언트 동기화된 시계 기준으로 남은 시간을 직접 계산
    UFUNCTION(BlueprintPure, Category = "Round")
    float GetRemainingTime() const;

    //최종 1등(공동1등 포함) 조회
    UFUNCTION()
    TArray<FString> GetFinalWinners() const;

    // 라운드 시작 전인지 확인
    UFUNCTION()
    bool bCanPlayerMove() const;

    // 플레이어 추가하는 함수
    virtual void AddPlayerState(APlayerState* PlayerState) override;

    // 플레이어 제거하는 함수
    virtual void RemovePlayerState(APlayerState* PlayerState) override;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_CurrentPhase();

    UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
    ERoundPhase CurrentPhase = ERoundPhase::WaitingToStart;

    UPROPERTY(Replicated)
    float RoundStartServerTime = 0.f;

    UPROPERTY(Replicated)
    TArray<FString> FinalWinnerNames;


#pragma region PlayerLoadingWait
public:
    UPROPERTY(BlueprintAssignable, Category = "Loading")
    FOnWaitingForPlayersChanged OnWaitingForPlayersChanged;

    void SetWaitingForPlayers(bool bInWaiting);

    UFUNCTION(BlueprintPure, Category = "Loading")
    bool IsWaitingForPlayers() const { return bWaitingForPlayers; }

private:
    UPROPERTY(ReplicatedUsing = OnRep_bWaitingForPlayers)
    bool bWaitingForPlayers = true;

    UFUNCTION()
    void OnRep_bWaitingForPlayers();
#pragma endregion
};
