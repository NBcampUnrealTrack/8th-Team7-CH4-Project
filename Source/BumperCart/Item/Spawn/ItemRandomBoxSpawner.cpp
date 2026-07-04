#include "Item/Spawn/ItemRandomBoxSpawner.h"

#include "Item/Spawn/ItemRandomBox.h"
#include "GameState/MainGameState.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"

AItemRandomBoxSpawner::AItemRandomBoxSpawner()
{
 	PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void AItemRandomBoxSpawner::BeginPlay()
{
	Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    AMainGameState* MainGameState = GetWorld()->GetGameState<AMainGameState>();
    if (IsValid(MainGameState))
    {

    }
}

void AItemRandomBoxSpawner::SpawnItemRandomBoxes()
{
    if (!HasAuthority())
    {
        return;
    }

    // BP_ItemRandomBox 지정
    if (!ItemRandomBoxClass)
    {
        return;
    }

    // Destroy 된 박스 정리
    ClearInvalidSpawnedItemBoxes();

    // 모든 슬롯 순회하면서 비어있는 위치에만 아이템 박스 스폰
    for (FItemRandomBoxSpawnSlot& SpawnSlot : ItemBoxSpawnSlots)
    {
        SpawnItemRandomBoxAtSlot(SpawnSlot);
    }
}

void AItemRandomBoxSpawner::ClearInvalidSpawnedItemBoxes()
{
    for (FItemRandomBoxSpawnSlot& SpawnSlot : ItemBoxSpawnSlots)
    {
        // 박스가 Destroy되면 Invalid가 false가 됨
        // 슬롯 재사용 가능하도록 nullptr 사용
        if (!IsValid(SpawnSlot.SpawnedItemBox))
        {
            SpawnSlot.SpawnedItemBox = nullptr;
        }
    }
}

void AItemRandomBoxSpawner::SpawnItemRandomBoxAtSlot(FItemRandomBoxSpawnSlot& SpawnSlot)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(SpawnSlot.SpawnPoint))
    {
        UE_LOG(LogTemp, Warning, TEXT("SapwnPoint 스폰하세요"));
        return;
    }

    // 아이템 박스 중복 생성 X
    if (IsValid(SpawnSlot.SpawnedItemBox))
    {
        return;
    }

    // 생성할 Actor 구조체
    const FTransform SpawnTransform = SpawnSlot.SpawnPoint->GetActorTransform();
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // ItemRandomBoxClass에 지정된 BP_ItemRandomBox를 SapwnPoint 위치에 생성함
    AItemRandomBox* SpawnedItemBox = GetWorld()->SpawnActor<AItemRandomBox>(
        ItemRandomBoxClass,
        SpawnTransform,
        SpawnParams
    );

    if (!IsValid(SpawnedItemBox))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 아이템 박스 스폰 실패"), *GetNameSafe(SpawnSlot.SpawnPoint));
        return;
    }

    SpawnSlot.SpawnedItemBox = SpawnedItemBox;

    UE_LOG(LogTemp, Warning, TEXT("%s: 아이템 박스 스폰 성공"), *GetNameSafe(SpawnSlot.SpawnPoint));
}


void AItemRandomBoxSpawner::ClearSpawnedItemRandomBoxes()
{
    if (!HasAuthority())
    {
        return;
    }

    for (FItemRandomBoxSpawnSlot& SpawnSlot : ItemBoxSpawnSlots)
    {
        // 라운드 종료 시 남아 있는 박스 제거
        if (IsValid(SpawnSlot.SpawnedItemBox))
        {
            SpawnSlot.SpawnedItemBox->Destroy();
        }

        SpawnSlot.SpawnedItemBox = nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("모든 아이템 박스 제거"));
}

