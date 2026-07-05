#include "Item/Spawn/RandomItemBox.h"

#include "Cart/CartPawn.h"
#include "Cart/Component/CartItemInventoryComponent.h"
#include "Item/ItemDataAsset.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

ARandomItemBox::ARandomItemBox()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    ItemBoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemBoxMesh"));
    ItemBoxMesh->SetupAttachment(SceneRoot);

    // 아이템 박스는 충돌 X
    ItemBoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ItemBoxMesh->SetGenerateOverlapEvents(false);

    PickupTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupTrigger"));
    PickupTrigger->SetupAttachment(SceneRoot);

    // 물리 충돌 X, overlap
    PickupTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupTrigger->SetGenerateOverlapEvents(true);
    PickupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ARandomItemBox::BeginPlay()
{
	Super::BeginPlay();

    if (IsValid(PickupTrigger))
    {
        PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPickupTriggerBeginOverlap);
    }

    ApplyItemBoxActiveState();
}

void ARandomItemBox::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARandomItemBox, bIsItemBoxActive);
}

// ------------------------------------------------------------
// Overlap
// ------------------------------------------------------------

void ARandomItemBox::OnPickupTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 서버에서만 처리
    if (!HasAuthority())
    {
        return;
    }

    // 아이템 박스 활성화 여부
    if (!bIsItemBoxActive)
    {
        return;
    }

    ACartPawn* PlayerCharacter = Cast<ACartPawn>(OtherActor);
    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    UCartItemInventoryComponent* ItemInventory = PlayerCharacter->FindComponentByClass<UCartItemInventoryComponent>();
    if (!IsValid(ItemInventory))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 인벤토리 없음"), *GetNameSafe(PlayerCharacter));
        return;
    }

    UItemDataAsset* SelectedItem = SelectRandomItem();
    if (!IsValid(SelectedItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("랜덤 선택된 아이템 없음"));
        return;
    }

    if (ItemInventory->AcquireItem(SelectedItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: %s 아이템 획득"), *GetNameSafe(PlayerCharacter), *GetNameSafe(SelectedItem));
        Destroy();
    }
}

// ------------------------------------------------------------
// 아이템 활성화 여부
// ------------------------------------------------------------

void ARandomItemBox::SetItemBoxActive(bool bIsActive)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bIsItemBoxActive == bIsActive)
    {
        return;
    }

    bIsItemBoxActive = bIsActive;

    ApplyItemBoxActiveState();

    ForceNetUpdate();
}

void ARandomItemBox::ApplyItemBoxActiveState()
{
    if(IsValid(ItemBoxMesh))
    {
        ItemBoxMesh->SetVisibility(bIsItemBoxActive, true);
    }

    if (IsValid(PickupTrigger))
    {
        PickupTrigger->SetCollisionEnabled(
            bIsItemBoxActive
            ? ECollisionEnabled::QueryOnly
            : ECollisionEnabled::NoCollision
        );
        PickupTrigger->SetGenerateOverlapEvents(bIsItemBoxActive);
    }
}

void ARandomItemBox::OnRep_IsItemBoxActive()
{
    ApplyItemBoxActiveState();
}

// ------------------------------------------------------------
// 아이템 지급
// ------------------------------------------------------------

UItemDataAsset* ARandomItemBox::SelectRandomItem() const
{
    TArray<UItemDataAsset*> ValidItemList;

    // 존재하는 아이템만 뽑기
    for (UItemDataAsset* Item : RandomItemList)
    {
        if (IsValid(Item))
        {
            ValidItemList.Add(Item);
        }
    }

    if (ValidItemList.Num() <= 0)
    {
        return nullptr;
    }

    const int32 RandomIndex = FMath::RandRange(0, ValidItemList.Num() - 1);

    return ValidItemList[RandomIndex];
}
