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

public:
    virtual void Tick(float DeltaTime) override;

#pragma endregion

#pragma region ProductShelf
private:
    // 모든 제품 선반 목록
    UPROPERTY(VisibleAnywhere)
    TArray<AProductShelf*> AllProductShelfs;

    // 스폰될 제품 블루프린트 목록
    UPROPERTY(EditAnywhere, Category = "Manager Config | Product Shelf")
    TArray<TSubclassOf<APickUpProduct>> MasterProductList;

    // 제품 선반 아이템 스폰 온오프
    UPROPERTY(EditAnywhere)
    bool bToggleOn = false;

    FTimerHandle RespawnTimerHandle;

public:
    // 모든 제품 선반 온오프 세팅
    void SetAllShelvesOpen(bool bToggle);

    // 제푼 선반들을 순회하며 아이템을 하나씩 무작위로 배달하는 함수
    void DistributeItemsToShelves();

#pragma endregion

#pragma region Item Spawn
private:
    // 맵에 최대 아이템 갯수 제한
    UPROPERTY(EditAnywhere, Category = "Manager Config | Item Spawn")
    int32 MaxItemCount = 150;

    UPROPERTY(EditAnywhere, Category = "Manager Config | Item Spawn")
    int32 MaxSpawnCount = 5;

    // 아이템 리스폰 시간
    UPROPERTY(EditAnywhere, Category = "Manager Config | Item Spawn")
    float RespawnDelay = 5.0f;

    // 스폰된 아이템 목록
    UPROPERTY()
    TArray<TSubclassOf<APickUpProduct>> SpawnedItems;

#pragma endregion

};
