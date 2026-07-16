#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Product/ProductBase.h"
#include "ItemSpawn/ProductShelfManager/ProductShelfTypes.h"
#include "ProductShelfSubsystem.generated.h"

class AProductShelf;
class UProductShelfManagerConfig;
class UNiagaraSystem;
class USoundBase;

/**
 * 
 */
UCLASS()
class BUMPERCART_API UProductShelfSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

#pragma region Override
protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void Deinitialize() override;

#pragma endregion

#pragma region Product Shelf
private:
    // 모든 일반 제품 선반 목록
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Shelf")
    TArray<AProductShelf*> AllProductShelfs;

    // 일반 제품 선반 목록
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Shelf")
    TArray<TObjectPtr<AProductShelf>> NormalProductShelfs;

    // 세일 제품 선반 목록
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Shelf")
    TArray<TObjectPtr<AProductShelf>> SaleProductShelfs;

    // 한정 제품 선반 목록
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Shelf")
    TArray<TObjectPtr<AProductShelf>> LimitedProductShelfs;

    FTimerHandle RespawnTimerHandle;

public:
    // 선반 액터에서 게임 시작시 호출하여 등록
    void RegisterShelf(AProductShelf* InShelf, EShelfType InType);

#pragma endregion

#pragma region Product Spawn
private:
    // 데이터 에셋
    UPROPERTY(VisibleAnywhere, Category = "Manager | Config")
    TObjectPtr<UProductShelfManagerConfig> SpawnConfig;

    // 현재 스폰된 제품 갯수
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Spawn")
    int32 CurrentProductCount = 0;

    // UProductShelfManagerConfig에 정보가 들어있고 설정 확인용
    // 맵에 최대 아이템 갯수 제한
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Spawn")
    int32 MaxSpawnCount = 150;

    // 한 번에 스폰되는 최대 갯수
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Spawn")
    int32 MaxSpawnLimit = 1;

    // 아이템 리스폰 시간
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Spawn")
    float RespawnDelay = 20.0f;

    // 스폰시 사용할 이펙트
    UPROPERTY(EditAnywhere, Category = "Manager | FX")
    TObjectPtr<UNiagaraSystem> SpawnFX;

    UPROPERTY(EditAnywhere, Category = "Manager | Spawn Sound")
    TObjectPtr<USoundBase> SpawnSound;

public:
    // 제품 스폰 호출
    void ProductSpawnCall();

    // 스폰된 아이템이 파괴되었을 떄
    void OnProductDestroyed();

    // 게임 시작시 호출
    void StartProductSpawning();

    // 테스트용 타이머
    FTimerHandle GameStartTimerHandle;

    // 데이터 에셋 적용
    void InitializeConfig(UProductShelfManagerConfig* InConfig);

    FORCEINLINE int32 GetCurrentProductCount() const { return CurrentProductCount; }
    FORCEINLINE void SetMaxSpawnCount(int32 SpawnCount) { MaxSpawnCount = SpawnCount; }
    FORCEINLINE UNiagaraSystem* GetSpawnFX() const { return SpawnFX; }
    FORCEINLINE USoundBase* GetSpawnSound() const { return SpawnSound; }

#pragma endregion

#pragma region SaleProduct Spawn
public:
    // 세일 제품 스폰
    void SaleProductSpawn();

#pragma endregion

#pragma region Limited Product
private:
    // 한정 제품 리스트 - 데이터 에셋으로 만들어야한다
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    TArray<TSubclassOf<AProductBase>> MasterLimitedProductList;

public:
    // 한정 제품 스폰
    void LimitedProductSpawn(TSubclassOf<AProductBase> LimitedProduct);

#pragma endregion

};
