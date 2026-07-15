#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProductShelfManagerConfig.generated.h"

class UNiagaraSystem;
class USoundBase;

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

    // 한 번에 최대 스폰 가능한 수
    UPROPERTY(EditAnywhere, Category = "Spawn Rules")
    int32 MaxSpawnLimit = 3;

    // 리스폰 딜레이
    UPROPERTY(EditAnywhere, Category = "Spawn Rules")
    float RespawnDelay = 5.0f;

    // 스폰시 사용할 이펙트
    UPROPERTY(EditAnywhere, Category = "FX")
    TObjectPtr<UNiagaraSystem> SpawnFX;

    // 스폰 소리
    UPROPERTY(EditAnywhere, Category = "FX")
    TObjectPtr<USoundBase> SpawnSound;
};
