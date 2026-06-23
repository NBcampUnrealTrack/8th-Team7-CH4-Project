#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapGimmickManager.generated.h"

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
    TArray<class ATargetPoint*> GimmickSpawnPointList;

    //UPROPERTY()
    //TArray<TSubclassOf<ATargetPoint>> GimmickSpawnPointList;

#pragma endregion

#pragma region Water Hole
protected:
    UPROPERTY(EditAnywhere, Category = "Gimmick | WaterHole")
    TSubclassOf<class AWaterHoleGimmick> WaterHoleClass;

    // 물 웅덩이 수
    UPROPERTY(EditAnywhere, Category = "Gimmick | WaterHole")
    int32 TotalWaterHoleSpawnCount = 3;
public:
    // 물 웅덩이 스폰
    void SpawnWaterHole();
#pragma endregion



};
