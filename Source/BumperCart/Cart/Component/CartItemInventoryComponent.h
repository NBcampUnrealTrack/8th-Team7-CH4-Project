#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartItemInventoryComponent.generated.h"

class ACartPawn;
class UItemDataAsset;
class UInputAction;
class UInputMappingContext;

struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnItemInventoryChanged,
    UCartItemInventoryComponent*, ItemInventoryComponent,
    UItemDataAsset*, CurrentItemData,
    int32, CurrentItemCount
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartItemInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCartItemInventoryComponent();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

// ------------------------------------------------------------
// 입력
// ------------------------------------------------------------
public:
    // 아이템 사용 입력 초기화
    void SetupInput();

    // 아이템 사용 입력 처리
    void HandleUseItemInput(const FInputActionValue& InputValue);

protected:
    // 아이템 입력 매핑
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    TObjectPtr<UInputMappingContext> UseItemInputMappingContext;

    // 아이템 사용 입력
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    TObjectPtr<UInputAction> UseItemInputAction;

    // 다른 IMC와 충돌 우선 순위
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    int32 MappingPriority = 1;

// ------------------------------------------------------------
// 아이템 정보
// ------------------------------------------------------------
public:
    UPROPERTY(BlueprintAssignable, Category = "Item")
    FOnItemInventoryChanged OnItemInventoryChanged;

private:
    // 현재 보유 중인 아이템
    UPROPERTY(ReplicatedUsing = OnRep_ItemInventory)
    TObjectPtr<UItemDataAsset> CurrentItemData = nullptr;

    // 현재 보유 중인 아이템 개수
    UPROPERTY(ReplicatedUsing = OnRep_ItemInventory)
    int32 CurrentItemCount = 0;

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

    // 현재 아이템 개수 반환
    UFUNCTION(BlueprintPure, Category = "Item")
    int32 GetCurrentItemCount() const;

    // 현재 보유 중인 아이템의 MaxStackCount
    UFUNCTION(BlueprintPure, Category = "Item")
    int32 GetCurrentItemMaxStackCount() const;

// ------------------------------------------------------------
// 아이템 변경 
// ------------------------------------------------------------
private:
    // 아이템 변경 후 처리
    void HandleItemChanged();

    // 아이템 복제 시 호출
    UFUNCTION()
    void OnRep_ItemInventory();
};
