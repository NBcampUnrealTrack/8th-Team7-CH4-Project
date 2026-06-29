#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BC_EventManager.generated.h"

class AProductShelfManager;
class AProductBase;

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
    //UPROPERTY(EditAnywhere, Category = "Manager")
    //class AProductShelfManager* ProductShelfManager;

    // 등록받을 선반 매니저 변수
    UPROPERTY()
    TObjectPtr<AProductShelfManager> ProductShelfManager;

public:
    void RegisterProductShelfManager(AProductShelfManager* InProductShelfManager);

#pragma endregion

#pragma region SaleEvent
private:
    // 세일 제품 목록
    UPROPERTY(EditAnywhere, Category = "Sale Event | Product List")
    TArray<TSubclassOf<AProductBase>> SaleProductList;

    // 세일 이벤트 종료용
    FTimerHandle SaleEventTimerHandle;

    // 세일 이벤트 제한시간
    UPROPERTY(EditAnywhere, Category = "Sale Event | Info")
    float SaleEventTime = 15.0f;

    UPROPERTY(VisibleAnywhere, Category = "Sale Event | Info")
    bool bSaleEvent = false;

    // 세일 이벤트 중 연속 스폰용
    FTimerHandle SaleProductSpawnTimerHandle;

    // 세일 제품 스폰 주기
    UPROPERTY(EditAnywhere, Category = "Sale Event | Info")
    float SaleProductSpawnInterval = 3.0f;

    // 현재 세일 제품
    UPROPERTY()
    TSubclassOf<AProductBase> CurrentSaleProduct;

public:
    // 세일 제품 선택
    TSubclassOf<AProductBase> SaleProductSelection();

    // 세일 이벤트 시작 - 게임 모드에서 호출
    void StartSaleEvent();

    // 세일 이벤트 종료
    void StopSaleEvent();

    // 세일 이벤트중 반복 호출될 스폰 함수
    void ExecuteRepeatSpawn();
#pragma endregion

#pragma region Limited Event
private:
    // 한정판 제품 목록
    UPROPERTY(EditAnywhere, Category = "Limited Event | Product List")
    TArray<TSubclassOf<AProductBase>> LimitedProductList;

    // 한정 제품 선택
    TSubclassOf<AProductBase> LimitedProductSelection();

    FTimerHandle TestLimitedEventTimerHandle;

public:
    // 한정 이벤트 시작 - 게임 모드에서 호출
    void StartLimitedEvent();
#pragma endregion

};
