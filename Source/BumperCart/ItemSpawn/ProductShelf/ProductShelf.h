#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "ProductShelf.generated.h"

UENUM(BlueprintType)
enum class EShelfType : uint8
{
    Common = 0,
    Rare = 1,
    Unique = 2,
};

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
    // 가판대 등급
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    EShelfType ShelfType;

    // 스폰되는 아이템 갯수
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    int32 SpawnCount = 5;

    // 아이템 리스폰 시간
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    float RespawnDelay = 5.0f;

    // 아이템 발사 세기
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    float LaunchForce = 500.0f;

    // 테스트 아이템
    UPROPERTY(EditAnywhere, Category = "Spawning Config")
    TSubclassOf<AActor> TestActorClass;

    // 아이템 데이터
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf Config")
    //TSubclassOf<AProductBase> ProductBaseClass;

private:
    FTimerHandle RespawnTimerHandle;

    void SpawnRandomItems();

    //TSubclassOf<AActor> GetRandomValidProductClass();
#pragma endregion

};
