#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Product/ProductBase.h"
#include "ProductShelfManager.generated.h"

UCLASS()
class BUMPERCART_API AProductShelfManager : public AActor
{
	GENERATED_BODY()
	
public:	
    AProductShelfManager();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region Product Shelf
private:
    // 모든 일반 제품 선반 목록
    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Shelf")
    TArray<AProductShelf*> AllProductShelfs;

    FTimerHandle RespawnTimerHandle;

#pragma endregion

#pragma region Product Spawn
private:
    // 맵에 최대 아이템 갯수 제한
    UPROPERTY(EditAnywhere, Category = "Manager | Product Spawn")
    int32 MaxItemCount = 150;

    // 한 번에 스폰되는 최대 갯수
    UPROPERTY(EditAnywhere, Category = "Manager | Product Spawn")
    int32 MaxSpawnCount = 5;

    // 아이템 리스폰 시간
    UPROPERTY(EditAnywhere, Category = "Manager | Product Spawn")
    float RespawnDelay = 10.0f;

    UPROPERTY(VisibleAnywhere, Category = "Manager | Product Spawn")
    int32 CurrentProductCount = 0;

    // 스폰된 아이템 목록
    UPROPERTY()
    TArray<TSubclassOf<AProductBase>> SpawnedItems;

public:
    // 제품 스폰 호출
    void ProductSpawnCall();

    int32 GetCurrentProductCount() const { return CurrentProductCount; }

    void OnProductDestroyed();

#pragma endregion

#pragma region SaleProduct Spawn
private:
    // 중앙 세일 선반
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    AProductShelf* CenterSaleShelf;

public:
    // 세일 제품 스폰
    void SaleProductSpawn(TSubclassOf<AProductBase> SaleProduct);

#pragma endregion

#pragma region Limited Product
private:
    // 한정 제품 선반
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    AProductShelf* LimitedProductShelf;

    // 한정 제품 리스트
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    TArray<TSubclassOf<AProductBase>> MasterLimitedProductList;

public:
    // 한정 제품 스폰
    void LimitedProductSpawn(TSubclassOf<AProductBase> LimitedProduct);
#pragma endregion

};
