// MainGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameState/MainGameState.h"
#include "MainGameMode.generated.h"

enum class ETitleType : uint8;
class ACheckoutManager;
class UProductShelfManagerConfig;
class USaleEventConfig;
class ULimitedEventConfig;
class AMapGimmickManager;
class AItemSpawnManager;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundResultsReady);

UCLASS()
class BUMPERCART_API AMainGameMode : public AGameMode
{
	GENERATED_BODY()

public:
    AMainGameMode();

    UFUNCTION()
    void UpdateAllPlayerRanks();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRoundResultsReady OnRoundResultsReady;

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

    //virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:

    TArray<float> SortedTriggerTimes;
    int32 NextPhaseIndex = 0;

    FTimerHandle Timer_RoundTick;

    const float TickInterval = 0.25f;

    // 모든 플레이어 로비로 복귀
    void ReturnAllPlayersToLobby();

    // 플레이어에게 칭호 부여
    void ApplyTitles();


    // 게임 시작 연출을 위한 대기 시간
    UPROPERTY(EditDefaultsOnly, Category = "Round Schedule")
    float StartDelay;

    // 결과창 노출 시간
    UPROPERTY(EditDefaultsOnly, Category = "Round Schedule")
    float ResultScreenDuration;

    FTimerHandle Timer_StartDelay;
    FTimerHandle Timer_ReturnToLobby;

#pragma region Managers
private:

    UPROPERTY(EditAnywhere, Category = "GameMode | Managers")
    AMapGimmickManager* MapGimmickManager;

    UPROPERTY()
    ACheckoutManager* CheckoutManagerRef;

    UPROPERTY(EditAnywhere, Category = "GameMode | Managers")
    AItemSpawnManager* ItemSpawnManager;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<UProductShelfManagerConfig> ProductShelfManagerConfig;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<USaleEventConfig> SaleEventConfig;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<ULimitedEventConfig> LimitedEventConfig;



#pragma endregion

#pragma region PlayLoadCheck

private:
    void CheckAllPlayersLoaded();


    FTimerHandle Timer_PlayerLoadCheck;


    // 모든 플레이어가 접속했는지 확인하는 주기
    float PlayerLoadCheckInterval;
    // 모든 플레이어가 접속하지 않았더라도 강제 시작하는 시간
    float PlayerLoadWaitTimeout;
    // 실제 기다린 시간
    float PlayerLoadWait;
#pragma endregion
};

