// MainGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameState/MainGameState.h"
#include "MainGameMode.generated.h"

class ACheckoutManager; // 계산대 매니저
class ABC_EventManager; // 이벤트 매니저
class AMapGimmickManager; // 맵 기믹 매니저
class AProductShelfManager; // 제품 선반 매니저

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

    //UFUNCTION(BlueprintCallable)
    //void GetFinalRank();


#pragma region EventManager
private:
    UPROPERTY(EditAnywhere, Category = "GameMode | Managers")
    TObjectPtr<ABC_EventManager> BC_EventManager = nullptr;

    UPROPERTY(EditAnywhere, Category = "GameMode | Managers")
    TObjectPtr<AMapGimmickManager> MapGimmickManager = nullptr;

    UPROPERTY(EditAnywhere, Category = "GameMode | Managers")
    TObjectPtr<AProductShelfManager> ProductShelfManager = nullptr;

public:
    // 게임모드를 통해 매니저를 찾을 수 있도록 Getter
    FORCEINLINE AProductShelfManager* GetProductShelfManager() const { return ProductShelfManager; }
    FORCEINLINE AMapGimmickManager* GetMapGimmickManager() const { return MapGimmickManager; }
    FORCEINLINE ABC_EventManager * GetBC_EventManager () const { return BC_EventManager ; }
#pragma endregion
};
