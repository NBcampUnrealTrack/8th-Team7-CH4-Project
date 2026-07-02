#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapGimmickManager.generated.h"

class AWaterHoleGimmick;
class AObstacleGimmick;
class UNiagaraSystem;
class ANPCRushGimmick;

USTRUCT(BlueprintType)
struct FObstacleSpawnInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gimmick")
    FName ObstacleName = TEXT("None"); // 장애물 이름

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gimmick")
    TSubclassOf<AActor> ObstacleClass = nullptr; // 스폰할 장애물 블루프린트 클래스

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gimmick")
    int32 SpawnCount = 3; // 해당 장애물 스폰 개수
};

UCLASS()
class BUMPERCART_API AMapGimmickManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AMapGimmickManager();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region Target Point
private:
    // 기믹 스폰될 위치 목록
    UPROPERTY()
    TArray<TObjectPtr<class ATargetPoint>> GimmickSpawnPointList;

    UPROPERTY()
    TArray<TObjectPtr<class ATargetPoint>> NPCRushStartPointList;

#pragma endregion

#pragma region Gimmick
private:
    UPROPERTY()
    TArray<TWeakObjectPtr<class AObstacleGimmick>> SpawnedObstacles;

    // 스폰될 장애물 목록
    UPROPERTY(EditAnywhere, Category = "Gimmick | Settings")
    TArray<FObstacleSpawnInfo> ObstacleSpawnList;

    TArray<AActor*> SpawnedObstacleList;

    FTimerHandle RespawnTimerHandle;

protected:
    // 장애물 리스폰 시간
    UPROPERTY(EditAnywhere, Category = "Gimmick | WaterHole")
    float ObstacleRespawnInterval = 20.0f;

public:
    // 게임 시작시 호출 - 게임 모드에서 호출
    void StartGimmickSpawning();

    // 장애물 스폰
    void SpawnObstacles();

    // 장애물 정리
    void ClearAllObstacles();

    // 장애물 리스폰
    void RespawnObstacles();

    // 라운드/게임 종료시 정리
    void EndSpawnObstacle();

#pragma endregion

#pragma region NPC Rush
private:
    UPROPERTY(EditAnywhere, Category = "Gimmick | NPC Rush")
    TSubclassOf<ANPCRushGimmick> NPCRushGimmick;

public:
    void StartNPCRush();

#pragma endregion


};
