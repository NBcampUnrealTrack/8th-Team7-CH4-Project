#include "MapGimmickManager/MapGimmickManager.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "WaterHoleGimmick/WaterHoleGimmick.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "MapGimmickManager/NPCRushGimmick/NPCRushGimmick.h"
#include "Sound/SoundBase.h"
#include "Components/DecalComponent.h"
#include "MapGimmickManager/NPCRushGimmick/NPCRushWarningArea.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavigationSystem.h"
#include "Customer/CustomerAI.h"

AMapGimmickManager::AMapGimmickManager()
{
	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

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

    UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] 총 타겟 포인트 갯수 : %d "), GimmickSpawnPointList.Num());


    // 테스트용 - 게임 모드에서 호출시 삭제 예정
    //StartGimmickSpawning();
}

void AMapGimmickManager::StartGimmickSpawning()
{
    if (!HasAuthority()) return;

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AMapGimmickManager::RespawnObstacles, ObstacleRespawnInterval, true);
}

void AMapGimmickManager::SpawnObstacles()
{
    if (!HasAuthority()) return;

    if (!GetWorld() || GimmickSpawnPointList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] 기믹 스폰 포인트가 없습니다."));
        return;
    }

    for (int32 i = GimmickSpawnPointList.Num() - 1; i >= 0; --i)
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

        UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] %s %d개 스폰 완료"), *Info.ObstacleName.ToString(), Info.SpawnCount);
    }
}

void AMapGimmickManager::ClearAllObstacles()
{
    if (!HasAuthority()) return;

    for (int32 i = SpawnedObstacleList.Num() - 1; i >=0; --i)
    {
        if (SpawnedObstacleList[i].IsValid())
        {
            AActor* Obstacle = SpawnedObstacleList[i].Get();
            Obstacle->Destroy();
        }
    }

    SpawnedObstacleList.Empty();

    UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] 기존 장애물들 정리완료"));
}

void AMapGimmickManager::RespawnObstacles()
{
    if (!HasAuthority()) return;

    ClearAllObstacles();

    SpawnObstacles();

    StartNPCRush();
}

void AMapGimmickManager::EndSpawnObstacle()
{
    if (!HasAuthority()) return;

    ClearAllObstacles();
}

void AMapGimmickManager::StartNPCRush()
{
    if (!HasAuthority()) return;

    if (NPCRushStartPointList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] NPCRush 포인트가 없습니다."));
        return;
    }

    // 타겟 포인트 목록 섞고 하나 랜덤 선택
    for (int32 i = NPCRushStartPointList.Num() - 1; i >= 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        if (i != RandomIndex)
        {
            NPCRushStartPointList.Swap(i, RandomIndex);
        }
    }

    int32 RandomPoint = FMath::RandRange(0, NPCRushStartPointList.Num() - 1);
    CurrentTargetPoint = NPCRushStartPointList[RandomPoint];

    CalculateTraceDimensionsFromTarget(CurrentTargetPoint, 5000.0f);
}

void AMapGimmickManager::SpawnNPCRush()
{
    if (!HasAuthority())   return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (IsValid(CurrentTargetPoint))
    {
        if (UWorld* World = GetWorld())
        {
            FVector SpawnLocation = CurrentTargetPoint->GetActorLocation();
            FRotator SpawnRotation = CurrentTargetPoint->GetActorRotation();

            ANPCRushGimmick* SpawnedGimmick = GetWorld()->SpawnActor<ANPCRushGimmick>(NPCRushGimmick, SpawnLocation, SpawnRotation, SpawnParams);

            if (IsValid(SpawnedGimmick))
            {
                UE_LOG(LogTemp, Log, TEXT("[맵기믹 매니저] NPCRush 시작"));
                SpawnedGimmick->SetLifeSpan(7.0f);
            }
        }
    }
}

void AMapGimmickManager::WaitingNpcRush()
{
    if (!HasAuthority())   return;

    // 경고 시간 뒤 거대 카트 돌진
    FTimerHandle SpawnTimerHandle;
    GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AMapGimmickManager::SpawnNPCRush, 5.0f, false);
}

void AMapGimmickManager::CalculateTraceDimensionsFromTarget(ATargetPoint* TargetPoint, float MaxTraceDistance)
{
    if (!HasAuthority())   return;

    if (!TargetPoint || !GetWorld()) return;

    // 라인 트레이스 시작점과 정면 방향 설정
    FVector StartPos = TargetPoint->GetActorLocation();
    // 플레이어 위로 쏴지게
    StartPos.Z += 100.0f;
    FVector ForwardDir = TargetPoint->GetActorForwardVector();

    // 기본적으로 벽에 안 부딪혔을 때를 대비한 가상의 끝점
    FVector EndPos = StartPos + (ForwardDir * MaxTraceDistance);
    EndPos.Z += 100.0f;

    // 라인 트레이스
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(TargetPoint); // 타겟 포인트 자신은 무시

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartPos,
        EndPos,
        ECC_Visibility,
        QueryParams
    );

    // 벽에 부딪혔다면 실제 충돌 지점을 끝점으로 갱신
    if (bHit)
    {
        EndPos = HitResult.ImpactPoint;
    }

    // 중심점(Center) 벡터 구하기
    FVector CenterVector = (StartPos + EndPos) * 0.5f;
    CenterVector.Z = 10.0f;

    // 반지름(Radius) 구하기
    // 시작점과 끝점 사이의 총 거리를 구한 뒤 반으로 나누기
    float TotalDistance = FVector::Dist(StartPos, EndPos);
    float Radius = TotalDistance * 0.5f;

    SpawnWarningDecal(CenterVector, Radius);
}

void AMapGimmickManager::SpawnWarningDecal(FVector CenterPoint, float Radius)
{
    if (WarningSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), WarningSound);
    }

    // 데칼 스폰
    if (NPCRushWarningArea)
    {
        FRotator DecalRotation = FRotationMatrix::MakeFromXZ(CenterPoint.ForwardVector, FVector::UpVector).Rotator();

        SpawnNPCRushWarningArea = GetWorld()->SpawnActor<ANPCRushWarningArea>(
            NPCRushWarningArea,
            CenterPoint,
            DecalRotation
        );
    }

    // 스폰된 데칼 사이즈 변경
    if (SpawnNPCRushWarningArea)
    {
        FVector TargetExtent = FVector(20.f, 100.0f, Radius);
        FVector DefaultDecalSize = FVector(128.f, 128.f, 128.f); // 엔진 기본값

        FVector NewScale = FVector(
            TargetExtent.X / DefaultDecalSize.X,
            TargetExtent.Y / DefaultDecalSize.Y,
            TargetExtent.Z / DefaultDecalSize.Z
        );

        SpawnNPCRushWarningArea->SetActorScale3D(NewScale);

        SpawnNPCRushWarningArea->InitWarningDecal(5.0f);
    }

    WaitingNpcRush();
}

void AMapGimmickManager::SpawnCustomerAI()
{
    if (!HasAuthority())    return;

    UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());

    FVector CentorLocation = GetActorLocation();
    float SearchRadius = 5000.0f;
    FNavLocation RandomNavLocation;

    for (int32 i = 0; i < SpawnCustomerAICount; i++)
    {
        int32 RandomIndex = FMath::RandRange(0, CustomerAIList.Num() - 1);
        TSubclassOf<ACustomerAI> CustomerAI = CustomerAIList[RandomIndex];


        if (NavigationSystem->GetRandomPointInNavigableRadius(CentorLocation, SearchRadius, RandomNavLocation))
        {
            FVector SpawnLocation = RandomNavLocation.Location;

            FRotator SpawnRotation = FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            ACustomerAI* SpawnedCustomer = GetWorld()->SpawnActor<ACustomerAI>(CustomerAI, SpawnLocation, SpawnRotation, SpawnParams);
        }
    }
    
}
