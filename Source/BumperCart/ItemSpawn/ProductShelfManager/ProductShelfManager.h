#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Product/PickUpProduct.h"
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

    // 스폰될 제품 블루프린트 목록
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    TArray<TSubclassOf<APickUpProduct>> MasterProductList;

    FTimerHandle RespawnTimerHandle;

    // 제품 선반 아이템 스폰 온오프
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf", meta = (AllowPrivateAccess = "true"))
    bool bToggleOn = false;

public:
    // 모든 제품 선반 온오프 세팅
    void SetAllShelvesOpen(bool bToggle);

    // 제푼 선반들을 순회하며 아이템을 하나씩 무작위로 배달하는 함수 - 기획 변경으로 미사용
    void DistributeItemsToShelves();

    UFUNCTION(BlueprintCallable)
    bool SetToggleOn(bool bToggle) { return bToggleOn = bToggle; }

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

    // 스폰된 아이템 목록
    UPROPERTY()
    TArray<TSubclassOf<APickUpProduct>> SpawnedItems;

public:
    // 제품 스폰 호출
    void ProductSpawnCall();

#pragma endregion

#pragma region SaleProduct Spawn
private:
    // 중앙 세일 선반
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    AProductShelf* CenterSaleShelf;

public:
    // 세일 제품 스폰
    void SaleProductSpawn(TSubclassOf<APickUpProduct> SaleProduct);

#pragma endregion

#pragma region Limited Product
private:
    // 한정 제품 선반
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    AProductShelf* LimitedProductShelf;

    // 한정 제품 리스트
    UPROPERTY(EditAnywhere, Category = "Manager | Product Shelf")
    TArray<TSubclassOf<APickUpProduct>> MasterLimitedProductList;

public:
    // 한정 제품 스폰
    void LimitedProductSpawn(TSubclassOf<APickUpProduct> LimitedProduct);

    TArray<TSubclassOf<APickUpProduct>> GetMasterLimitedProductList();
#pragma endregion

};
