#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Product/PickUpProduct.h"
#include "ProductShelf.generated.h"

class APickUpProduct;

UCLASS()
class BUMPERCART_API AProductShelf : public AActor
{
	GENERATED_BODY()
	
public:	
	AProductShelf();

#pragma region Component
protected:
    // 선반 메쉬
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> ShelfMesh;

    // 제품 발사 포인트
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<USceneComponent> LaunchPoints;

#pragma endregion

#pragma region Override
protected:
	virtual void BeginPlay() override;

#pragma endregion

#pragma region Product List
private:
    // 각 선반의 제품 목록 - 수정된 기획 내용
    UPROPERTY(EditAnyWhere, Category = "Product Shelf | Product List")
    TArray<TSubclassOf<APickUpProduct>> ProductList;

public:
    TArray<TSubclassOf<APickUpProduct>> GetProductList() const { return ProductList; }
#pragma endregion


#pragma region Spawning
protected:
    // 아이템 발사 세기
    UPROPERTY(EditAnywhere, Category = "Spawning")
    float LaunchForce = 500.0f;

    // 아이템 스폰가능 확인
    UPROPERTY(EditAnywhere, Category = "Spawning")
    bool bSpawning = false;

public:
    // 제품 목록중 랜덤 선택후 스폰 호출, 제품 반환 - 수정된 기획 내용
    APickUpProduct* SpawnRandomProduct();

    // 제품 스폰
    APickUpProduct* SpawnSpecificItem(TSubclassOf<APickUpProduct> ItemClass);

    bool SetSpawnToggle(bool bToggle) { return bSpawning = bToggle; };

#pragma endregion

};
