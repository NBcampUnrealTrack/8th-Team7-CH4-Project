#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartItemInventoryComponent.generated.h"

class ACartPawn;
class UItemDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartItemInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCartItemInventoryComponent();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    
// ------------------------------------------------------------
// 아이템 정보
// ------------------------------------------------------------
private:
    // 현재 보유 중인 아이템
    UPROPERTY(ReplicatedUsing = OnRep_CurrentItemData)
    TObjectPtr<UItemDataAsset> CurrentItemData;

// ------------------------------------------------------------
// 아이템 획득
// ------------------------------------------------------------
public:
    // 서버에서 아이템 획득
    // 기존 아이템이 있을 경우 새 아이템으로 교체
    bool AcquireItem(UItemDataAsset* NewItemData);

// ------------------------------------------------------------
// 아이템 사용
// ------------------------------------------------------------
public:
    // 로컬 플레이어가 아이템 사용 요청
    void RequestUseItem();

private:
    // 서버 RPC
    // 클라이언틔의 아이템 사용을 서버로 전달
    UFUNCTION(Server, Reliable)
    void ServerUseItem();

    // 서버에서 아이템의 Action을 실행
    void UseItem();

    // 서버에서 현재 보유 아이템 제거
    void ClearItem();

// ------------------------------------------------------------
// Getter
// ------------------------------------------------------------
public:
    // 아이템 보유 여부
    UFUNCTION(BlueprintPure, Category = "Item")
    bool HasItem() const;

    // 현재 아이템 데이터 반환
    UFUNCTION(BlueprintPure, Category = "Item")
    UItemDataAsset* GetCurrentItemData() const;

// ------------------------------------------------------------
// 아이템 변경 
// ------------------------------------------------------------
private:
    // 아이템 변경 후 처리
    void HandleItemChanged();

    // 아이템 복제 시 호출
    UFUNCTION()
    void OnRep_CurrentItemData();
};
