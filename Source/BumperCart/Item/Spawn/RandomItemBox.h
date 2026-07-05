#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomItemBox.generated.h"

class UBoxComponent;
class UItemDataAsset;

UCLASS()
class BUMPERCART_API ARandomItemBox : public AActor
{
	GENERATED_BODY()
	
public:	
	ARandomItemBox();

protected:
	virtual void BeginPlay() override;

public:
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

// ------------------------------------------------------------
// Overlap
// ------------------------------------------------------------
private:
    UFUNCTION()
    void OnPickupTriggerBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------
private:
    UPROPERTY(EditAnywhere, Category = "ItemBox|Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> ItemBoxMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> PickupTrigger;

// ------------------------------------------------------------
// 아이템 활성화 여부
// ------------------------------------------------------------
private:
    void SetItemBoxActive(bool bIsActive);

    void ApplyItemBoxActiveState();

    UFUNCTION()
    void OnRep_IsItemBoxActive();

private:
    // 아이템 박스 활성화 여부
    // 복제 데이터 
    UPROPERTY(ReplicatedUsing = OnRep_IsItemBoxActive, VisibleInstanceOnly, Category = "ItemBox|State")
    bool bIsItemBoxActive = true;

// ------------------------------------------------------------
// 아이템 지급
// ------------------------------------------------------------
private:
    UItemDataAsset* SelectRandomItem() const;

private:
    // 이 배열에 들어간 DataAsset 중 하나를 랜덤 지급
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemBox|Item", meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<UItemDataAsset>> RandomItemList;

};
