// MainGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameState/MainGameState.h"
#include "MainGameMode.generated.h"

class ACheckoutManager; // 계산대 매니저
class UProductShelfManagerConfig;
class USaleEventConfig;
class ULimitedEventConfig;
class AMapGimmickManager;

UCLASS()
class BUMPERCART_API AMainGameMode : public AGameMode
{
	GENERATED_BODY()

public:
    AMainGameMode();

    UFUNCTION()
    void UpdateAllPlayerRanks();

protected:
    // 매치가 InProgress로 전환될 때 엔진이 자동 호출
    virtual void HandleMatchHasStarted() override;

    virtual void BeginPlay() override;

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

    //라운드 시작
    void StartRound();

    // 0.25초마다 호출되어 다음 Phase에 도달했는지 체크
    void TickRoundSchedule();

    // 실제로 특정 Phase에 진입했을 때 처리 (팀원 함수 호출 지점)
    void EnterPhase(ERoundPhase NewPhase);


    UPROPERTY(EditDefaultsOnly, Category = "Cart")
    TSubclassOf<APawn> CartPawnClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Schedule")
    TMap<float, ERoundPhase> PhaseScheduleMap;

    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:

    TArray<float> SortedTriggerTimes;
    int32 NextPhaseIndex = 0;

    FTimerHandle Timer_RoundTick;

    const float TickInterval = 0.25f;

    // 모든 플레이어 로비로 복귀
    void ReturnAllPlayersToLobby();

    // 결과창 노출 시간
    UPROPERTY(EditDefaultsOnly, Category = "Round Schedule")
    float ResultScreenDuration = 5.f;

    FTimerHandle Timer_ReturnToLobby;

#pragma region Managers
private:

    UPROPERTY(EditAnywhere, Category = "GameMode | Managers")
    AMapGimmickManager* MapGimmickManager;

    UPROPERTY()
    ACheckoutManager* CheckoutManagerRef;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<UProductShelfManagerConfig> ProductShelfManagerConfig;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<USaleEventConfig> SaleEventConfig;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<ULimitedEventConfig> LimitedEventConfig;



#pragma endregion
};
