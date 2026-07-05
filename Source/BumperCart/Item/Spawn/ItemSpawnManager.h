#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawnManager.generated.h"

class ARandomItemBox;

USTRUCT(BlueprintType)
struct FRandomItemBoxSpawnSlot
{
    GENERATED_BODY()

public:
    // 아이템 박스 생성될 위치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn")
    TObjectPtr<AActor> SpawnPoint = nullptr;

    // SpawnPoint에 생성될 아이템 박스
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ItemBox|Spawn")
    TObjectPtr<ARandomItemBox> SpawnedItemBox = nullptr;
};

UCLASS()
class BUMPERCART_API AItemSpawnManager  : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemSpawnManager ();

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------
private:
    UPROPERTY(VisibleAnywhere, Category = "ItemBox|Components")
    TObjectPtr<USceneComponent> SceneRoot;

// ------------------------------------------------------------
// 타이머
// ------------------------------------------------------------
public:
    // 아이템 박스 스폰 시작
    UFUNCTION(BlueprintCallable, Category = "ItemBox|Spawn")
    void StartSpawning();

    // 아이템 박스 스폰 정지
    UFUNCTION(BlueprintCallable, Category = "ItemBox|Spawn")
    void StopSpawning();

    // 현재 스폰 타이머가 동작 중인지
    UFUNCTION(BlueprintPure, Category = "ItemBox|Spawn")
    bool IsSpawning() const;

private:
    // 스폰 주기 지나면 호출
    void HandleSpawnIntervalElapsed();

private:
    // 아이템 박스 스폰 타이머
    FTimerHandle RandomItemBoxSpawnTimerHandle;

    // 스폰 주기
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
    float SpawnInterval = 30.0f;

    // 게임 시작 즉시 한 번 스폰할지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn", meta = (AllowPrivateAccess = "true"))
    bool bSpawnImmediatelyOnStart = true;

// ------------------------------------------------------------
// 아이템 박스 스폰
// ------------------------------------------------------------
public:
    // 모든 SpawnPoint를 확인하고,
    // 비어 있는 곳에 아이템 박스 생성
    UFUNCTION(BlueprintCallable, Category = "ItemBox|Spawn")
    void SpawnRandomItemBoxes();

    // 라운드 종료 시 남아 있는 아이템 박스 모두 제거
    UFUNCTION(BlueprintCallable, Category = "ItemBox|Spawn")
    void ClearSpawnedRandomItemBoxes();

private:
    // 슬롯에 아이템 박스 생성
    void SpawnRandomItemBoxAtSlot(FRandomItemBoxSpawnSlot& SpawnSlot);

    // Destroy된 박스 정리
    void ClearInvalidSpawnedItemBoxes();

// ------------------------------------------------------------
// 스폰 데이터
// ------------------------------------------------------------
private:
    // 스폰할 아이템 랜덤 박스 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<ARandomItemBox> RandomItemBoxClass;

    // 아이템 박스가 생성될 모든 위치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Spawn", meta = (AllowPrivateAccess = "true"))
    TArray<FRandomItemBoxSpawnSlot> ItemBoxSpawnSlots;
};
