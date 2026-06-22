#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapGimmickManager.generated.h"

class ATargetPoint;

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

#pragma region Water Hole
private:
    // 물 웅덩이 스폰될 위치 목록
    UPROPERTY(EditAnywhere, Category = "Water Hole | Spawn")
    TArray<TObjectPtr<ATargetPoint>> GimmickSpawnPointList;

public:
    // 물 웅덩이 스폰
    void SpawnWaterHole();
#pragma endregion



};
