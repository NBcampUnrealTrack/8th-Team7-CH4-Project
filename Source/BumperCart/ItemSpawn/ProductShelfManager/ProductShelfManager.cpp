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

