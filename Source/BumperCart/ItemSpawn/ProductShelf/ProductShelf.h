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
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> ShelfMesh;

    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<USceneComponent> LaunchPoints;

#pragma endregion

#pragma region Override
protected:
	virtual void BeginPlay() override;

#pragma endregion

#pragma region Spawning
protected:
    // 아이템 발사 세기
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    float LaunchForce = 500.0f;

    // 아이템 스폰가능 확인
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    bool bSpawning = false;

    // 테스트 아이템
    //UPROPERTY(EditAnywhere, Category = "Spawning Config")
    //TSubclassOf<AActor> TestActorClass;

private:
    // 기존 가판대가 아이템 목록을 가지고있을 때 - 테스트용으로 남김
    //void SpawnRandomItems();

public:
    UFUNCTION(BlueprintCallable)
    bool SetSpawnToggle(bool bToggle) { return bSpawning = bToggle; };

    // 매니저에서 호출 제품 스폰
    APickUpProduct* SpawnSpecificItem(TSubclassOf<APickUpProduct> ItemClass);

#pragma endregion

};
