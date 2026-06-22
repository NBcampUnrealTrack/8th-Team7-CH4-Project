#include "ItemSpawn/ProductShelfManager/ProductShelfManager.h"

#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Kismet/GameplayStatics.h"

AProductShelfManager::AProductShelfManager()
{
	PrimaryActorTick.bCanEverTick = false;

    // 클라이언트로 복제 금지 - 서버에만 존재
    bNetLoadOnClient = false;
}

void AProductShelfManager::BeginPlay()
{
	Super::BeginPlay();

    if (!HasAuthority()) return;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductShelf::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        AProductShelf* Shelf = Cast<AProductShelf>(Actor);
        if (Shelf)
        {
            // 세일, 한정판 선반은 따로 관리
            if (Shelf == CenterSaleShelf)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 중앙 세일 선반은 따로 관리"));

                continue;
            }
            else if (Shelf == LimitedProductShelf)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 한정판 선반은 따로 관리"));

                continue;
            }

            AllProductShelfs.Add(Shelf);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 모든 일반 선반 등록 - 등록 갯수 : %d"), AllProductShelfs.Num());

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AProductShelfManager::ProductSpawnCall, RespawnDelay, true);
}

void AProductShelfManager::SetAllShelvesOpen(bool bToggle)
{
    if (!HasAuthority()) return;

    for (AProductShelf* Shelf : AllProductShelfs)
    {
        if (Shelf)
        {
            Shelf->SetSpawnToggle(bToggle);
        }
    }
}

void AProductShelfManager::DistributeItemsToShelves()
{
    if (!HasAuthority()) return;

    // 에디터에서 값이 변경이 안되서 일단 주석처리
    /*if (!bToggleOn)
    {
        UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 스폰 활성화 안됨"));
        return;
    }*/

    for (AProductShelf* Shelf : AllProductShelfs)
    {
        if (Shelf)
        {
            int32 RandomCount = FMath::RandRange(1, MaxSpawnCount);

            for (int32 i = 0; i < RandomCount; i++)
            {
                if (SpawnedItems.Num() >= MaxItemCount)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 아이템이 최대로 스폰되었습니다."));
                    return;
                }

                // 아이템 리스트에서 랜덤으로 하나 뽑기
                int32 RandomIndex = FMath::RandRange(0, MasterProductList.Num() - 1);
                TSubclassOf<APickUpProduct> SpawnedItem = MasterProductList[RandomIndex];
                
                // 스폰 명령
                if (Shelf->SpawnSpecificItem(SpawnedItem))
                {
                    SpawnedItems.Add(SpawnedItem);
                }
            }
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 스폰된 아이템 수 : %d"), SpawnedItems.Num());
}

void AProductShelfManager::ProductSpawnCall()
{
    if (!HasAuthority()) return;

    for (AProductShelf* Shelf : AllProductShelfs)
    {
        if (Shelf)
        {
            int32 RandomCount = FMath::RandRange(1, MaxSpawnCount);

            for (int32 i = 0; i < RandomCount; i++)
            {
                if (SpawnedItems.Num() >= MaxItemCount)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 맵의 제품이 최대로 스폰되었습니다."));
                    return;
                }

                APickUpProduct* SpawnedProduct = Shelf->SpawnRandomProduct();

                if (SpawnedProduct)
                {
                    SpawnedItems.Add(SpawnedProduct->GetClass());
                }
            }
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 스폰된 제품 수 : %d"), SpawnedItems.Num());
}

void AProductShelfManager::SaleProductSpawn(TSubclassOf<APickUpProduct> SaleProduct)
{
    if (!HasAuthority() || !SaleProduct) return;

    if (!CenterSaleShelf)
    {
        UE_LOG(LogTemp, Error, TEXT("[ProductShelfManager] 중앙 세일 선반(CenterSaleShelf)이 에디터에서 지정되지 않았습니다!"));
        return;
    }

    APickUpProduct* SpawnedProduct = CenterSaleShelf->SpawnSpecificItem(SaleProduct);

    if (SpawnedProduct != nullptr)
    {
        // 세일 아이템 체크 (변수, 함수 추가 요청)
        //SpawnedProduct->SetSaleItem(true);

        SpawnedItems.Add(SaleProduct);
        UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 세일 제품 스폰 및 체크 완료."));

        // 넷 멀티캐스트로 UI 알림 추가해야함
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 세일 제품 스폰 실패."));
    }

}

void AProductShelfManager::LimitedProductSpawn(TSubclassOf<APickUpProduct> LimitedProduct)
{
    if (!HasAuthority() || !LimitedProduct || !LimitedProductShelf) return;

    if (!LimitedProductShelf)
    {
        UE_LOG(LogTemp, Error, TEXT("[ProductShelfManager] 한정 제품 선반이 에디터에서 지정되지 않았습니다!"));
        return;
    }

    APickUpProduct* SpawnedProduct = LimitedProductShelf->SpawnSpecificItem(LimitedProduct);

    if (SpawnedProduct != nullptr)
    {
        // 한정 제품 체크 (변수, Set() 함수 추가 요청)
        // SpawnedProduct->SetLimitedProduct(true);

        SpawnedItems.Add(LimitedProduct);
        UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 한정 제품 스폰 및 체크 완료."));

        // 넷 멀티캐스트로 UI 알림 추가해야함
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 한정 제품 스폰 실패."));
    }
}

