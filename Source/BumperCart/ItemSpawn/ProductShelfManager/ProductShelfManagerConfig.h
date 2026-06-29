#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProductShelfManagerConfig.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API UProductShelfManagerConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    // 맵 최대 스폰 제한
    UPROPERTY(EditAnywhere, Category = "Spawn Rules")
    int32 MaxSpawnCount = 150;

    // 리스폰 딜레이
    UPROPERTY(EditAnywhere, Category = "Spawn Rules")
    float RespawnDelay = 10.0f;
};
