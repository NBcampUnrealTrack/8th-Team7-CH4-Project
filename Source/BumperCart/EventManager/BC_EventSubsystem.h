#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EventManager/SaleEventConfig.h"
#include "EventManager/LimitedEventConfig.h"
#include "BC_EventSubsystem.generated.h"

class AProductShelfManager;
class AProductBase;

/**
 * 
 */
UCLASS()
class BUMPERCART_API UBC_EventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

#pragma region Override
protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void Deinitialize() override;

#pragma endregion

#pragma region SaleEvent
private:
    // 데이터 에셋 - 월드 서브시스템으로 변경 후 게임 모드로 옮겨야함
    UPROPERTY(EditAnywhere, Category = "Sale Event | Config")
    TObjectPtr<USaleEventConfig> SaleEventConfig;

    // USaleEventConfig에 정보가 들어있고 설정 확인용
    // 세일 제품 목록
    UPROPERTY(VisibleAnywhere, Category = "Sale Event | Product List")
    TArray<TSubclassOf<AProductBase>> SaleProductList;

    // 세일 이벤트 제한시간
    UPROPERTY(VisibleAnywhere, Category = "Sale Event | Info")
    float SaleEventTime = 15.0f;

    // 세일 제품 스폰 주기
    UPROPERTY(VisibleAnywhere, Category = "Sale Event | Info")
    float SaleProductSpawnInterval = 3.0f;

    // 세일 이벤트 종료용
    FTimerHandle SaleEventTimerHandle;

    // 세일 이벤트 중 연속 스폰용
    FTimerHandle SaleProductSpawnTimerHandle;

    // 현재 세일 제품
    UPROPERTY()
    TSubclassOf<AProductBase> CurrentSaleProduct;

    // 세일 제품 선택
    TSubclassOf<AProductBase> SaleProductSelection();

public:
    // 게임 모드에서 호출하여 세일 이벤트 데이터 에셋 적용
    void InitializeSaleEventConfig(USaleEventConfig* InSaleEventConfig);

    // 세일 이벤트 시작 - 게임 모드에서 호출
    void StartSaleEvent();

    // 세일 이벤트 종료
    void StopSaleEvent();

    // 세일 이벤트중 반복 호출될 스폰 함수
    void ExecuteRepeatSpawn();

    FORCEINLINE TSubclassOf<AProductBase> GetCurrentSaleProduct() const { return CurrentSaleProduct; }
#pragma endregion

#pragma region Limited Event
private:
    // 데이터 에셋 - 월드 서브시스템으로 변경 후 게임 모드로 옮겨야함
    UPROPERTY(EditAnywhere, Category = "Limited Event | Config")
    TObjectPtr<ULimitedEventConfig> LimitedEventConfig;

    // 한정판 제품 목록
    UPROPERTY(EditAnywhere, Category = "Limited Event | Product List")
    TArray<TSubclassOf<AProductBase>> LimitedProductList;

    // 한정 제품 선택
    TSubclassOf<AProductBase> LimitedProductSelection();

    FTimerHandle TestLimitedEventTimerHandle;

public:
    // 게임 모드에서 호출하여 한정판 이벤트 데이터 에셋 적용
    void InitializeLimitedEventConfig(ULimitedEventConfig* InLimitedEventConfig);

    // 한정 이벤트 시작 - 게임 모드에서 호출
    void StartLimitedEvent();

#pragma endregion
};
