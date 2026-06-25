// MainGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MainGameState.generated.h"

UENUM(BlueprintType)
enum class ERoundPhase : uint8
{
    None                  UMETA(DisplayName = "None"),
    RoundStart            UMETA(DisplayName = "0:00 - 라운드 시작"),
    RandomOpenTwo         UMETA(DisplayName = "0:30 - 랜덤 오픈 시작"),
    SaleEvent             UMETA(DisplayName = "1:00 - 세일 상품 이벤트"),
    PremiumRespawn        UMETA(DisplayName = "1:30 - 중앙 고급 상품 리스폰"),
    FinalWarningOneOpen   UMETA(DisplayName = "2:30 - 막판 30초 경고"),
    RoundEnd              UMETA(DisplayName = "3:00 - 라운드 종료")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundPhaseChanged);

UCLASS()
class BUMPERCART_API AMainGameState : public AGameState
{
	GENERATED_BODY()

public:
    // Phase가 바뀔 때마다 호출
    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnRoundPhaseChanged OnRoundPhaseChanged;

    //라운드 시간(3분)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round")
    float RoundDurationSeconds = 180.f;

    // 서버에서만 호출
    //라운드 페이즈 지정 및 라운드 시작 시간 지정
    void SetRoundPhase(ERoundPhase NewPhase);
    void SetRoundStartTime();

    //현재 페이즈 가져오기
    UFUNCTION(BlueprintPure, Category = "Round")
    ERoundPhase GetCurrentPhase() const;

    // 서버-클라이언트 동기화된 시계 기준으로 남은 시간을 직접 계산 (틱 복제 불필요)
    UFUNCTION(BlueprintPure, Category = "Round")
    float GetRemainingTime() const;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_CurrentPhase();

    UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
    ERoundPhase CurrentPhase = ERoundPhase::None;

    UPROPERTY(Replicated)
    float RoundStartServerTime = 0.f;

};
