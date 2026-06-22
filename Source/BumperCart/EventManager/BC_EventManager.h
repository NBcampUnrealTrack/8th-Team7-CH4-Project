#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BC_EventManager.generated.h"

class AProductShelfManager;
class APickUpProduct;

UCLASS()
class BUMPERCART_API ABC_EventManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ABC_EventManager();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region ProductShelfManager
private:
    // 맵에 있는 제품선반관리 매니저
    UPROPERTY(EditAnywhere, Category = "Manager")
    class AProductShelfManager* ProductShelfManager;
#pragma endregion

#pragma region SaleEvent
private:
    // 세일 이벤트 종료용
    FTimerHandle SaleEventTimerHandle;

    // 세일 이벤트 제한시간
    UPROPERTY(EditAnywhere, Category = "Event Manager | Sale")
    float SaleEventTime = 15.0f;

    UPROPERTY(VisibleAnywhere, Category = "Event Manager | Sale")
    bool bSaleEvent = false;

    // 세일 이벤트 중 연속 스폰용
    FTimerHandle SaleProductSpawnTimerHandle;

    // 세일 제품 스폰 주기
    UPROPERTY(EditAnywhere, Category = "Event Manager | Sale")
    float SaleProductSpawnInterval = 3.0f;

    UPROPERTY()
    TSubclassOf<class APickUpProduct> CurrentSaleProduct;

public:
    // 세일 제품 선택
    TSubclassOf<APickUpProduct> SaleProductSelection();

    // 세일 이벤트 시작
    void StartSaleEvent();

    // 세일 이벤트 종료
    void StopSaleEvent();

    // 세일 이벤트중 반복 호출될 스폰 함수
    void ExecuteRepeatSpawn();
#pragma endregion

#pragma region Limited Event
private:
    // 한정 제품 선택
    TSubclassOf<APickUpProduct> LimitedProductSelection();

    FTimerHandle TestLimitedEventTimerHandle;

public:
    // 한정 이벤트 시작
    void StartLimitedEvent();
#pragma endregion

};
