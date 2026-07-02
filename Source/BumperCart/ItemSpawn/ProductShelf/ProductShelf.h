#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Product/ProductBase.h"
#include "ItemSpawn/ProductShelfManager/ProductShelfTypes.h"
#include "ProductShelf.generated.h"

class AProductBase;
class UNiagaraSystem;

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

#pragma region Shelf Info
private:
    // 선반 유형
    UPROPERTY(EditAnywhere, Category = "Product Shelf | Shelf Info")
    EShelfType ShelfType = EShelfType::Normal;

    // 각 선반의 제품 목록
    UPROPERTY(EditAnyWhere, Category = "Product Shelf | Product List")
    TArray<TSubclassOf<AProductBase>> ProductList;

public:
    TArray<TSubclassOf<AProductBase>> GetProductList() const { return ProductList; }
#pragma endregion


#pragma region Spawning
protected:
    // 아이템 발사 세기
    UPROPERTY(EditAnywhere, Category = "Spawning")
    float LaunchForce = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UNiagaraSystem> SpawnFX;

public:
    // 제품 목록중 랜덤 선택후 스폰 호출, 제품 반환
    AProductBase* SpawnRandomProduct();

    // 제품 스폰
    AProductBase* SpawnSpecificItem(TSubclassOf<AProductBase> ItemClass);

#pragma endregion

};
