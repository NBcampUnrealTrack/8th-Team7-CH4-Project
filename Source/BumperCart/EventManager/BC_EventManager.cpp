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
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반 매니저 안됨 -> 등록"));

        // 제품선반 매니저 등록을 안했을 시 맵에서 찾아서 등록
        ProductShelfManager = Cast<AProductShelfManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AProductShelfManager::StaticClass()));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반 매니저 등록되어있음"));
    }

    GetWorld()->GetTimerManager().SetTimer(SaleEventTimerHandle, this, &ABC_EventManager::StartSaleEvent, 20.0f,false);
}

TSubclassOf<APickUpProduct> ABC_EventManager::SaleProductSelection()
{
    if (!HasAuthority() || !ProductShelfManager)   return nullptr;

    const TArray<TSubclassOf<APickUpProduct>>& ProductList = ProductShelfManager->GetMasterProductList();

    if (ProductList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 제품선반매니저에 등록된 제품리스트가 비어있습니다."));
    }

    // 세일 제품 랜덤 선택
    int32 RandomIndex = FMath::RandRange(0, ProductList.Num()-1);
    TSubclassOf<APickUpProduct> SaleProduct = ProductList[RandomIndex];

    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 세일 제품 : %s"), *SaleProduct->GetName());

    return SaleProduct;
}

void ABC_EventManager::StartSaleEvent()
{
    if (!HasAuthority() || !ProductShelfManager)   return;

    GetWorldTimerManager().ClearTimer(SaleEventTimerHandle);
    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 세일 이벤트 시작"));

    TSubclassOf<APickUpProduct> SelectedSaleProduct = SaleProductSelection();

    if (SelectedSaleProduct)
    {
        ProductShelfManager->SaleProductSpawn(SelectedSaleProduct);
    }

    GetWorld()->GetTimerManager().SetTimer(SaleEventTimerHandle, this, &ABC_EventManager::StopSaleEvent, SaleEventTime, false);
}

void ABC_EventManager::StopSaleEvent()
{
    if (!HasAuthority())   return;

    GetWorldTimerManager().ClearTimer(SaleEventTimerHandle);

    UE_LOG(LogTemp, Log, TEXT("[BC_EventManager] 세일 이벤트 종료"));

}

