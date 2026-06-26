#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapGimmickManager.generated.h"

class AWaterHoleGimmick;
class AObstacleGimmick;

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

#pragma endregion

#pragma region Gimmick
private:
    UPROPERTY()
    TArray<TWeakObjectPtr<class AObstacleGimmick>> SpawnedObstacles;

    // 스폰될 장애물 목록
    UPROPERTY(EditAnywhere, Category = "Gimmick | Settings")
    TArray<FObstacleSpawnInfo> ObstacleSpawnList;

    TArray<AActor*> SpawnedObstacleList;

public:
    // 게임 시작시 호출 - 게임 모드에서 호출
    void StartGimmickSpawning();
#pragma endregion


#pragma region Water Hole
protected:
    // 스폰할 물 웅덩이
    UPROPERTY(EditAnywhere, Category = "Gimmick | WaterHole")
    TSubclassOf<AWaterHoleGimmick> WaterHoleClass;

    // 스폰될 물 웅덩이 수
    UPROPERTY(EditAnywhere, Category = "Gimmick | WaterHole")
    int32 TotalWaterHoleSpawnCount = 3;

    // 물 웅덩이 리스폰 시간
    UPROPERTY(EditAnywhere, Category = "Gimmick | WaterHole")
    float WaterHoleRespawnInterval = 20.0f;

private:
    // 스폰된 물 웅덩이 저장
    UPROPERTY()
    TArray<TObjectPtr<class AWaterHoleGimmick>> SpawnedWaterHoles;

    FTimerHandle RespawnTimerHandle;

    // 물 웅덩이 리스폰
    void RespawnWaterHole();

    // 스폰된 물 웅덩이 정리
    void ClearAllWaterHole();

    // 물 웅덩이 스폰
    void SpawnWaterHole();

    // 라운드 끝 정리용
    void EndSpawnWaterHole();

#pragma endregion

#pragma region Obstacle
protected:
    // 스폰할 장애물
    UPROPERTY(EditAnywhere, Category = "Gimmick | Obstacle")
    TArray<TSubclassOf<AObstacleGimmick>> ObstacleList;

    UPROPERTY(EditAnywhere, Category = "Gimmick | Obstacle")
    int32 TotalObstacleSpawnCount = 5;

    void SpawnObstacles();

    void ClearAllObstacles();

    void RespawnObstacles();

    void EndSpawnObstacle();

#pragma endregion



};
