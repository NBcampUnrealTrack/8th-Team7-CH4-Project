#include "Cart/Component/CartItemInventoryComponent.h"

#include "Cart/CartPawn.h"
#include "Item/ItemDataAsset.h"
#include "Item/Action/ItemAction.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UCartItemInventoryComponent::UCartItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
}

void UCartItemInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCartItemInventoryComponent, CurrentItemData);
}

void UCartItemInventoryComponent::SetupInput()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());

    if (!IsValid(OwnerPawn))
    {
        return;
    }

    // 서버에 존재하는 다른 플레이어 카트는 등록 X
    if (!OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    // 입력 바인딩
    if (APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // IMC 등록
            if (UseItemInputMappingContext)
            {
                Subsystem->AddMappingContext(UseItemInputMappingContext, MappingPriority);
            }

            // IA 등록
            if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
            {
                if (UseItemInputAction)
                {
                    EnhancedInputComponent->BindAction(
                        UseItemInputAction,
                        ETriggerEvent::Started,
                        this,
                        &ThisClass::HandleUseItemInput
                    );
                }
            }
        }
    }
}

void UCartItemInventoryComponent::HandleUseItemInput(const FInputActionValue& InputValue)
{
    UE_LOG(LogTemp, Warning, TEXT("아이템 사용 입력"));

    RequestUseItem();
}

// ------------------------------------------------------------
// 아이템 획득
// ------------------------------------------------------------

bool UCartItemInventoryComponent::AcquireItem(UItemDataAsset* NewItemData)
{
    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor))
    {
        return false;
    }

    // 아이템은 서버에서만 변경
    if (!OwnerActor->HasAuthority())
    {
        return false;
    }

    if (!IsValid(NewItemData))
    {
        return false;
    }

    // 기존 아이템 지우고, 새 아이템으로 교체
    CurrentItemData = NewItemData;

    HandleItemChanged();

    return true;
}

// ------------------------------------------------------------
// 아이템 사용
// ------------------------------------------------------------

void UCartItemInventoryComponent::RequestUseItem()
{
    // 아이템 보유 여부
    if (!HasItem())
    {
        return;
    }

    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor))
    {
        return;
    }

    // 서버일 경우 바로 아이템 사용
    if (OwnerActor->HasAuthority())
    {
        UseItem();
        return;
    }

    // 클라이언트일 경우 서버 RPC 호출
    ServerUseItem();
}

void UCartItemInventoryComponent::ServerUseItem_Implementation()
{
    UseItem();
}

void UCartItemInventoryComponent::UseItem()
{
    ACartPawn* PlayerCharacter = Cast<ACartPawn>(GetOwner());

    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    if (!PlayerCharacter->HasAuthority())
    {
        return;
    }

    // 아이템 보유 여부
    if (!CurrentItemData)
    {
        return;
    }

    // 실행할 Action 클래스가 존재하는지
    if (!CurrentItemData->ActionClass)
    {
        return;
    }

    // DataAsset에 지정된 Action 선택
    UItemAction* ItemAction = NewObject<UItemAction>(this, CurrentItemData->ActionClass);

    if (!IsValid(ItemAction))
    {
        return;
    }

    // Action의 사용 조건 확인
    if (!ItemAction->CanExecute(PlayerCharacter))
    {
        return;
    }

    // 아이템 사용
    const bool bExecuted = ItemAction->Execute(PlayerCharacter);

    // 아이템 사용 성공한 경우, 아이템 소모
    if (bExecuted)
    {
        ClearItem();
    }
}

void UCartItemInventoryComponent::ClearItem()
{
    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor))
    {
        return;
    }

    if (!OwnerActor->HasAuthority())
    {
        return;
    }

    CurrentItemData = nullptr;

    HandleItemChanged();
}


// ------------------------------------------------------------
// Getter
// ------------------------------------------------------------

bool UCartItemInventoryComponent::HasItem() const
{
    return IsValid(CurrentItemData);
}

UItemDataAsset* UCartItemInventoryComponent::GetCurrentItemData() const
{
    return CurrentItemData;
}

// ------------------------------------------------------------
// 아이템 변경 
// ------------------------------------------------------------

void UCartItemInventoryComponent::HandleItemChanged()
{

}

void UCartItemInventoryComponent::OnRep_CurrentItemData()
{
    HandleItemChanged();
}

