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
    // 스폰되는 아이템 갯수 - 테스트
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    int32 SpawnCount = 3;

    // 아이템 스폰 제한수 - 테스트
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    int32 SpawnMaxCount = 15;

    // 아이템 리스폰 시간 - 테스트
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    float RespawnDelay = 5.0f;

    // 아이템 발사 세기
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    float LaunchForce = 500.0f;

    // 아이템 스폰가능 확인
    UPROPERTY(VisibleAnywhere, Category = "Spawning Config")
    bool bSpawning = false;

    // 테스트 아이템
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    TSubclassOf<AActor> TestActorClass;

private:
    // 테스트용 타이머 -> 후에 매니저로 옮겨서 관리
    FTimerHandle RespawnTimerHandle;

    // 기존 가판대가 아이템 목록을 가지고있을 때 - 테스트용으로 남김
    void SpawnRandomItems();

public:
    bool SetSpawnToggle(bool bToggle);

    // 매니저에서 호출 제품 스폰
    void SpawnSpecificItem(TSubclassOf<APickUpProduct> ItemClass);

#pragma endregion


#pragma region Item
private:
    // 스폰된 아이템 목록 - 매니저에서 관리 예정
    UPROPERTY()
    TArray<AActor*> SpawnedItems;

#pragma endregion

};
