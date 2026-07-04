#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemRandomBoxSpawner.generated.h"

class AItemRandomBox;

USTRUCT(BlueprintType)
struct FItemRandomBoxSpawnSlot
{
    GENERATED_BODY()

public:
    // 아이템 박스 생성될 위치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn")
    TObjectPtr<AActor> SpawnPoint = nullptr;

    // SpawnPoint에 생성될 아이템 박스
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ItemBox|Spawn")
    TObjectPtr<AItemRandomBox> SpawnedItemBox = nullptr;
};

UCLASS()
class BUMPERCART_API AItemRandomBoxSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemRandomBoxSpawner();

protected:
	virtual void BeginPlay() override;

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------
private:
    UPROPERTY(VisibleAnywhere, Category = "ItemBox|Components")
    TObjectPtr<USceneComponent> SceneRoot;

// ------------------------------------------------------------
// 아이템 박스 스폰
// ------------------------------------------------------------
public:
    // 모든 SpawnPoint를 확인하고,
    // 비어 있는 곳에 아이템 박스 생성
    UFUNCTION(BlueprintCallable, Category = "ItemBox|Spawn")
    void SpawnItemRandomBoxes();

    // 라운드 종료 시 남아 있는 아이템 박스 모두 제거
    UFUNCTION(BlueprintCallable, Category = "ItemBox|Spawn")
    void ClearSpawnedItemRandomBoxes();

private:
    // 슬롯에 아이템 박스 생성
    void SpawnItemRandomBoxAtSlot(FItemRandomBoxSpawnSlot& SpawnSlot);

    // Destroy된 박스 정리
    void ClearInvalidSpawnedItemBoxes();

// ------------------------------------------------------------
// 스폰 데이터
// ------------------------------------------------------------
private:
    // 스폰할 아이템 랜덤 박스 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<AItemRandomBox> ItemRandomBoxClass;

    // 아이템 박스가 생성될 모든 위치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn", meta = (AllowPrivateAccess = "true"))
    TArray<FItemRandomBoxSpawnSlot> ItemBoxSpawnSlots;

};
