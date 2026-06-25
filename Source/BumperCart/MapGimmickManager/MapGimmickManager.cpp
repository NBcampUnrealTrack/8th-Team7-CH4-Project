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

    // 맵에 있는 'GimmickPoint' 태그가 붙어있는 타겟포인트 가져오기
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


    // 게임 모드에서 호출시 삭제 예정
    StartGimmickSpawning();
}

void AMapGimmickManager::StartGimmickSpawning()
{
    RespawnWaterHole();

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AMapGimmickManager::RespawnWaterHole, WaterHoleRespawnInterval, true);
}

void AMapGimmickManager::RespawnWaterHole()
{
    if (!HasAuthority()) return;

    ClearAllWaterHole();

    SpawnWaterHole();
}

void AMapGimmickManager::ClearAllWaterHole()
{
    if (!HasAuthority()) return;

    for (AWaterHoleGimmick* WaterHole : SpawnedWaterHoles)
    {
        if (IsValid(WaterHole))
        {
            WaterHole->Destroy();
        }
    }

    SpawnedWaterHoles.Empty();

    UE_LOG(LogTemp, Warning, TEXT("[GimmickManager] 기존 물 웅덩이 정리완료"));
}

void AMapGimmickManager::SpawnWaterHole()
{
    if (!HasAuthority()) return;

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
                SpawnedWaterHoles.Add(SpawnedWaterHole);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GimmickManager] 물 웅덩이 %d개 스폰"), FinalSpawnCount);

}

void AMapGimmickManager::EndSpawnWaterHole()
{
    ClearAllWaterHole();
}

