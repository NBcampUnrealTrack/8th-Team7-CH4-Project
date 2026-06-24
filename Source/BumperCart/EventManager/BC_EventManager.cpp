#include "EventManager/BC_EventManager.h"

#include "ItemSpawn/ProductShelfManager/ProductShelfManager.h"
#include "Kismet/GameplayStatics.h"

ABC_EventManager::ABC_EventManager()
{
	PrimaryActorTick.bCanEverTick = false;

    bNetLoadOnClient = false;
}

void ABC_EventManager::BeginPlay()
{
	Super::BeginPlay();

    if (!HasAuthority())   return;

    if (!ProductShelfManager)
    {
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반 매니저 등록 안됨 -> 등록"));

        // 제품선반 매니저 등록을 안했을 시 맵에서 찾아서 등록
        ProductShelfManager = Cast<AProductShelfManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AProductShelfManager::StaticClass()));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반 매니저 등록되어있음"));
    }

    GetWorld()->GetTimerManager().SetTimer(SaleEventTimerHandle, this, &ABC_EventManager::StartSaleEvent, 20.0f,false);
    GetWorld()->GetTimerManager().SetTimer(TestLimitedEventTimerHandle, this, &ABC_EventManager::StartLimitedEvent, 30.0f, false);
}

TSubclassOf<AProductBase> ABC_EventManager::SaleProductSelection()
{
    if (!HasAuthority() || !ProductShelfManager)   return nullptr;

    if (SaleProductList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반매니저에 등록된 제품 목록이 비어있습니다."));
        return nullptr;
    }

    // 세일 제품 랜덤 선택
    int32 RandomIndex = FMath::RandRange(0, SaleProductList.Num()-1);
    TSubclassOf<AProductBase> SaleProduct = SaleProductList[RandomIndex];

    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 세일 제품 : %s"), *SaleProduct->GetName());

    return SaleProduct;
}

void ABC_EventManager::StartSaleEvent()
{
    if (!HasAuthority() || !ProductShelfManager)   return;

    GetWorldTimerManager().ClearTimer(SaleEventTimerHandle);
    GetWorldTimerManager().ClearTimer(SaleProductSpawnTimerHandle);
    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 세일 이벤트 시작"));

    // 세일 제품 저장
    CurrentSaleProduct = SaleProductSelection();

    if (CurrentSaleProduct)
    {
        GetWorld()->GetTimerManager().SetTimer(SaleProductSpawnTimerHandle, this, &ABC_EventManager::ExecuteRepeatSpawn, SaleProductSpawnInterval, true, 0.0f);
    }

    GetWorld()->GetTimerManager().SetTimer(SaleEventTimerHandle, this, &ABC_EventManager::StopSaleEvent, SaleEventTime, false);
}

void ABC_EventManager::StopSaleEvent()
{
    if (!HasAuthority())   return;

    GetWorldTimerManager().ClearTimer(SaleEventTimerHandle);
    GetWorldTimerManager().ClearTimer(SaleProductSpawnTimerHandle);

    CurrentSaleProduct = nullptr;

    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 세일 이벤트 종료, "));

}

void ABC_EventManager::ExecuteRepeatSpawn()
{
    if (!HasAuthority() || !ProductShelfManager || !CurrentSaleProduct)   return;

    ProductShelfManager->SaleProductSpawn(CurrentSaleProduct);
    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반 매니저 호출 -> 세일제품 스폰 반복 호출"));
}

TSubclassOf<AProductBase> ABC_EventManager::LimitedProductSelection()
{
    if (!HasAuthority() || !ProductShelfManager)   return nullptr;

    if (LimitedProductList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반매니저에 등록된 한정판 제품 목록이 비어있습니다."));
        return nullptr;
    }

    // 한정 제품 랜덤 선택
    int32 RandomIndex = FMath::RandRange(0, LimitedProductList.Num() - 1);
    TSubclassOf<AProductBase> LimitedProduct = LimitedProductList[RandomIndex];

    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 한정판 제품 : %s"), *LimitedProduct->GetName());

    return LimitedProduct;
}

void ABC_EventManager::StartLimitedEvent()
{
    if (!HasAuthority() || !ProductShelfManager)   return;

    GetWorldTimerManager().ClearTimer(TestLimitedEventTimerHandle);

    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 한정판 이벤트 시작."));

    TSubclassOf<AProductBase> LimitedProduct = LimitedProductSelection();

    if (LimitedProduct)
    {
        ProductShelfManager->LimitedProductSpawn(LimitedProduct);
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반 매니저 호출 -> 한정판 제품 스폰 호출."));
    }
}

