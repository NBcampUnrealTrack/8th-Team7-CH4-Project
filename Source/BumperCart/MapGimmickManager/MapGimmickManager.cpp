#include "MapGimmickManager/MapGimmickManager.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "WaterHoleGimmick/WaterHoleGimmick.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "MapGimmickManager/NPCRushGimmick/NPCRushGimmick.h"

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
        // 맵에 있는 'GimmickPoint' 태그가 붙어있는 타겟포인트 가져오기
        if (Actor && Actor->ActorHasTag(FName("GimmickPoint")))
        {
            ATargetPoint* SpawnPoint = Cast<ATargetPoint>(Actor);
            if (SpawnPoint)
            {
                GimmickSpawnPointList.Add(SpawnPoint);
            }
        }
        // 맵에 있는 'NPCRushPoint' 태그가 붙어있는 타겟포인트 가져오기
        else if (Actor && Actor->ActorHasTag(FName("NPCRushPoint")))
        {
            ATargetPoint* SpawnPoint = Cast<ATargetPoint>(Actor);
            if (SpawnPoint)
            {
                NPCRushStartPointList.Add(SpawnPoint);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MapGimmickManager] 총 타겟 포인트 갯수 : %d "), GimmickSpawnPointList.Num());


    // 테스트용 - 게임 모드에서 호출시 삭제 예정
    //StartGimmickSpawning();
    //StartNPCRush();
}

void AMapGimmickManager::StartGimmickSpawning()
{
    RespawnObstacles();

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AMapGimmickManager::RespawnObstacles, ObstacleRespawnInterval, true);
}

void AMapGimmickManager::SpawnObstacles()
{
    if (!HasAuthority()) return;

    if (!GetWorld() || GimmickSpawnPointList.Num() == 0) return;

    for (int32 i = GimmickSpawnPointList.Num() - 1; i > 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        if (i != RandomIndex)
        {
            GimmickSpawnPointList.Swap(i, RandomIndex);
        }
    }

    int32 CurrentTargetPointIndex = 0;

    for (const FObstacleSpawnInfo& Info : ObstacleSpawnList)
    {
        if (!IsValid(Info.ObstacleClass)) continue;

        for (int32 i = 0; i < Info.SpawnCount; ++i)
        {
            ATargetPoint* TargetPoint = GimmickSpawnPointList[CurrentTargetPointIndex];

            if (IsValid(TargetPoint))
            {
                FVector SpawnLocation = TargetPoint->GetActorLocation();
                FRotator SpawnRotation = TargetPoint->GetActorRotation();

                AActor* SpawnedGimmick = GetWorld()->SpawnActor<AActor>(Info.ObstacleClass, SpawnLocation, SpawnRotation);

                if (IsValid(SpawnedGimmick))
                {
                    CurrentTargetPointIndex++;

                    SpawnedObstacleList.Add(SpawnedGimmick);
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("[MapGimmickManager] %s %d개 스폰 완료"), *Info.ObstacleName.ToString(), Info.SpawnCount);
    }
}

void AMapGimmickManager::ClearAllObstacles()
{
    if (!HasAuthority()) return;

    for (AActor* Obstacle : SpawnedObstacleList)
    {
        if (IsValid(Obstacle))
        {
            Obstacle->Destroy();
        }
    }

    SpawnedObstacleList.Empty();

    UE_LOG(LogTemp, Warning, TEXT("[GimmickManager] 기존 장애물들 정리완료"));
}

void AMapGimmickManager::RespawnObstacles()
{
    if (!HasAuthority()) return;

    ClearAllObstacles();

    SpawnObstacles();

    // 테스트용
    //StartNPCRush();
}

void AMapGimmickManager::EndSpawnObstacle()
{
    ClearAllObstacles();
}

void AMapGimmickManager::StartNPCRush()
{
    for (int32 i = NPCRushStartPointList.Num() - 1; i > 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        if (i != RandomIndex)
        {
            NPCRushStartPointList.Swap(i, RandomIndex);
        }
    }

    int32 RandomPoint = FMath::RandRange(0, NPCRushStartPointList.Num() - 1);
    ATargetPoint* TargetPoint = NPCRushStartPointList[RandomPoint];

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (IsValid(TargetPoint))
    {
        if (UWorld* World = GetWorld())
        {
            FVector SpawnLocation = TargetPoint->GetActorLocation();
            FRotator SpawnRotation = TargetPoint->GetActorRotation();

            ANPCRushGimmick* SpawnedGimmick = GetWorld()->SpawnActor<ANPCRushGimmick>(NPCRushGimmick, SpawnLocation, SpawnRotation, SpawnParams);

            if (IsValid(SpawnedGimmick))
            {
                UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] NPCRush 시작"))
            }
        }
    }
}
