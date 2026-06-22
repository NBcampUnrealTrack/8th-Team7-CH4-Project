#include "MapGimmickManager/MapGimmickManager.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"

AMapGimmickManager::AMapGimmickManager()
{
	PrimaryActorTick.bCanEverTick = false;

    bNetLoadOnClient = false;
}

void AMapGimmickManager::BeginPlay()
{
	Super::BeginPlay();

    if (!HasAuthority()) return;

    TArray<AActor*> FoundActors;

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (Actor && Actor->ActorHasTag(FName("GimmickPoint")))
        {
            ATargetPoint* SpawnPoint = Cast<ATargetPoint>(Actor);
            if (SpawnPoint)
            {
                GimmickSpawnPointList.Add(SpawnPoint);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MapGimmickManager] 총 타겟 포인트 갯수 : %d "), GimmickSpawnPointList.Num());
}

void AMapGimmickManager::SpawnWaterHole()
{
}

