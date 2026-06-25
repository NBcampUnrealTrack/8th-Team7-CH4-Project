// MainGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameState/MainGameState.h"
#include "MainGameMode.generated.h"

class ACheckoutManager;

UCLASS()
class BUMPERCART_API AMainGameMode : public AGameMode
{
	GENERATED_BODY()

public:
    AMainGameMode();

protected:
    // 매치가 InProgress로 전환될 때 엔진이 자동 호출
    virtual void HandleMatchHasStarted() override;

    void StartRound();

    // 0.25초마다 호출되어 다음 Phase에 도달했는지 체크
    void TickRoundSchedule();

    // 실제로 특정 Phase에 진입했을 때 처리 (팀원 함수 호출 지점)
    void EnterPhase(ERoundPhase NewPhase);


    UPROPERTY(EditDefaultsOnly, Category = "Cart")
    TSubclassOf<APawn> CartPawnClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Schedule")
    TMap<float, ERoundPhase> PhaseScheduleMap;

private:

    TArray<float> SortedTriggerTimes;
    int32 NextPhaseIndex = 0;

    FTimerHandle Timer_RoundTick;

    static constexpr float TickInterval = 0.25f;

    UPROPERTY()
    ACheckoutManager* CheckoutManagerRef;

};
