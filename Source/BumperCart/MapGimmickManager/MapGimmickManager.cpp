#include "MapGimmickManager/MapGimmickManager.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "WaterHoleGimmick/WaterHoleGimmick.h"

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

    SpawnWaterHole();
}

void AMapGimmickManager::SpawnWaterHole()
{
    if (!IsValid(WaterHoleClass))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GimmickManager] 물 웅덩이 클래스가 없습니다"));
        return;
    }

    if (GimmickSpawnPointList.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GimmickManager] 저장된 타겟 포인트가 없습니다"));
        return;
    }

    for (int32 i = GimmickSpawnPointList.Num() - 1; i > 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        GimmickSpawnPointList.Swap(i, RandomIndex);
    }

    int32 FinalSpawnCount = FMath::Min(TotalWaterHoleSpawnCount, GimmickSpawnPointList.Num());

    for (int32 i = 0; i < FinalSpawnCount; i++)
    {
        ATargetPoint* TargetPoint = GimmickSpawnPointList[i];
        if (IsValid(TargetPoint))
        {
            FVector SpawnLocation = TargetPoint->GetActorLocation();
            FRotator SpawnRotation = TargetPoint->GetActorRotation();

            AWaterHoleGimmick* SpawnedWaterHole = GetWorld()->SpawnActor<AWaterHoleGimmick>(WaterHoleClass, SpawnLocation, SpawnRotation);
            if (IsValid(SpawnedWaterHole))
            {
                UE_LOG(LogTemp, Log, TEXT("[GimmickManager] %d 번쨰 물 웅덩이 스폰"), i + 1);
            }
        }
    }
}

