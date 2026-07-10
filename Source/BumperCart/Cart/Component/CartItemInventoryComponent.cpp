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
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UCartItemInventoryComponent::UCartItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
}

void UCartItemInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCartItemInventoryComponent, CurrentItemData);
    DOREPLIFETIME(UCartItemInventoryComponent, CurrentItemCount);

    // 클라이언트만 복제
    DOREPLIFETIME_CONDITION(UCartItemInventoryComponent, ItemCooldownEndTime, COND_OwnerOnly);
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

    const bool bItemChanged = CurrentItemData != NewItemData;

    // 최대 보유 가능 아이템 수
    const int32 MaxStackCount = FMath::Max(1, NewItemData->MaxStackCount);

    // MaxStackCount만큼 아이템 지급
    CurrentItemData = NewItemData;
    CurrentItemCount = MaxStackCount;

    // 아이템이 교체될 경우 쿨타임 초기화
    if (bItemChanged)
    {
        ResetItemCooldown();
    }

    HandleItemChanged();
    OwnerActor->ForceNetUpdate();

    ClientPlayAcquireItemSound();

    return true;
}

// ------------------------------------------------------------
// 아이템 사용
// ------------------------------------------------------------

void UCartItemInventoryComponent::RequestUseItem()
{
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
    if (!HasItem())
    {
        return;
    }

    // 쿨타임 검사
    if (IsItemOnCooldown())
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
    if (bExecuted)
    {
        // 쿨타임 적용
        StartItemCooldown();

        // 수량 1개 소모
        CurrentItemCount = FMath::Max(0, CurrentItemCount - 1);

        // 아이템이 없을 경우 슬롯 창 지우기
        if (CurrentItemCount <= 0)
        {
            ClearItem();
            return;
        }

        HandleItemChanged();

        if (AActor* OwnerActor = GetOwner())
        {
            OwnerActor->ForceNetUpdate();
        }
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
    CurrentItemCount = 0;

    // 쿨타임 제거
    ResetItemCooldown();

    HandleItemChanged();
    OwnerActor->ForceNetUpdate();
}

// ------------------------------------------------------------
// 아이템 쿨타임
// ------------------------------------------------------------

bool UCartItemInventoryComponent::IsItemOnCooldown() const
{
    return GetItemCooldownRemaining() > KINDA_SMALL_NUMBER;
}

float UCartItemInventoryComponent::GetItemCooldownRemaining() const
{
    return FMath::Max(0.0f, ItemCooldownEndTime - GetCurrentTimeSeconds());
}

float UCartItemInventoryComponent::GetItemCooldownPercent() const
{
    if (!IsValid(CurrentItemData))
    {
        return 0.0f;
    }

    const float CooldownDuration = (0.0f, CurrentItemData->CooldownDuration);

    if (CooldownDuration <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    return FMath::Clamp(GetItemCooldownRemaining() / CooldownDuration, 0.0f, 1.0f);
}

void UCartItemInventoryComponent::StartItemCooldown()
{
    if (!IsValid(CurrentItemData))
    {
        return;
    }

    const float CooldownDuration =  FMath::Max(0.0f, CurrentItemData->CooldownDuration);

    ItemCooldownEndTime = GetCurrentTimeSeconds() + CooldownDuration;
}

void UCartItemInventoryComponent::ResetItemCooldown()
{
    ItemCooldownEndTime = 0.0f;
}

float UCartItemInventoryComponent::GetCurrentTimeSeconds() const
{
    const UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return 0.0f;
    }

    const AGameStateBase* GameState = World->GetGameState();

    if (IsValid(GameState))
    {
        return GameState->GetServerWorldTimeSeconds();
    }

    return World->GetTimeSeconds();
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

int32 UCartItemInventoryComponent::GetCurrentItemCount() const
{
    return CurrentItemCount;
}

int32 UCartItemInventoryComponent::GetCurrentItemMaxStackCount() const
{
    if (!IsValid(CurrentItemData))
    {
        return 0;
    }

    return FMath::Max(1, CurrentItemData->MaxStackCount);
}

// ------------------------------------------------------------
// 아이템 변경 
// ------------------------------------------------------------

void UCartItemInventoryComponent::HandleItemChanged()
{
    OnItemInventoryChanged.Broadcast(this, CurrentItemData, CurrentItemCount);
}

void UCartItemInventoryComponent::OnRep_ItemInventory()
{
    HandleItemChanged();
}

// ------------------------------------------------------------
// 사운드
// ------------------------------------------------------------

void UCartItemInventoryComponent::ClientPlayAcquireItemSound_Implementation()
{
    if (!IsValid(AcquireItemSound))
    {
        return;
    }

    UGameplayStatics::PlaySound2D(this, AcquireItemSound);
}

