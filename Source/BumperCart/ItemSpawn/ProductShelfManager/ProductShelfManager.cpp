#include "ItemSpawn/ProductShelfManager/ProductShelfManager.h"
#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Kismet/GameplayStatics.h"

AProductShelfManager::AProductShelfManager()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AProductShelfManager::BeginPlay()
{
	Super::BeginPlay();

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductShelf::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        AProductShelf* Shelf = Cast<AProductShelf>(Actor);
        if (Shelf)
        {
            AllProductShelfs.Add(Shelf);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("모든 가판대 등록 - 등록 갯수 : %d"), AllProductShelfs.Num());

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AProductShelfManager::DistributeItemsToShelves, RespawnDelay, true);
}

void AProductShelfManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    // 테스트용 후 삭제
    SetAllShelvesOpen(bToggleOn);
}

void AProductShelfManager::SetAllShelvesOpen(bool bToggle)
{
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
    if (!HasAuthority() || MasterProductList.Num() == 0) return;

    for (AProductShelf* Shelf : AllProductShelfs)
    {
        if (Shelf)
        {
            int32 RandomCount = FMath::RandRange(1, MaxSpawnCount);

            for (int32 i = 0; i < RandomCount; i++)
            {
                if (SpawnedItems.Num() >= MaxItemCount)
                {
                    UE_LOG(LogTemp, Warning, TEXT("아이템이 최대로 스폰되었습니다."));
                    return;
                }

                // 아이템 리스트에서 랜덤으로 하나 뽑기
                int32 RandomIndex = FMath::RandRange(0, MasterProductList.Num() - 1);
                TSubclassOf<APickUpProduct> ChosenItem = MasterProductList[RandomIndex];

                SpawnedItems.Add(ChosenItem);

                // 스폰 명령
                Shelf->SpawnSpecificItem(ChosenItem);
            }
        }
    }
}

