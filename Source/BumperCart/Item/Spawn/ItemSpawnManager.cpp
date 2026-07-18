#include "Item/Spawn/ItemSpawnManager.h"

#include "Item/Spawn/RandomItemBox.h"
#include "GameState/MainGameState.h"

#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AItemSpawnManager::AItemSpawnManager()
{
 	PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void AItemSpawnManager::BeginPlay()
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

void AItemSpawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopSpawning();

    Super::EndPlay(EndPlayReason);
}
// ------------------------------------------------------------
// 타이머
// ------------------------------------------------------------

void AItemSpawnManager::StartSpawning()
{
    if (!HasAuthority())
    {
        return;
    }

    if (SpawnInterval <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    if (!RandomItemBoxClass)
    {
        return;
    }

    StopSpawning();

    // 게임 시작 시 바로 아이템 박스 스폰
    if (bSpawnImmediatelyOnStart)
    {
        SpawnRandomItemBoxes();
    }

    GetWorldTimerManager().SetTimer(
        RandomItemBoxSpawnTimerHandle,
        this,
        &ThisClass::HandleSpawnIntervalElapsed,
        SpawnInterval,
        true
    );
}

void AItemSpawnManager::StopSpawning()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!GetWorldTimerManager().IsTimerActive(RandomItemBoxSpawnTimerHandle))
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(RandomItemBoxSpawnTimerHandle);
}

bool AItemSpawnManager::IsSpawning() const
{
    if (!HasAuthority())
    {
        return false;
    }

    return GetWorldTimerManager().IsTimerActive(RandomItemBoxSpawnTimerHandle);
}

void AItemSpawnManager::HandleSpawnIntervalElapsed()
{
    if (!HasAuthority())
    {
        return;
    }

    // 아이템 스폰
    SpawnRandomItemBoxes();
}

// ------------------------------------------------------------
// 아이템 박스 스폰
// ------------------------------------------------------------

void AItemSpawnManager::SpawnRandomItemBoxes()
{
    if (!HasAuthority())
    {
        return;
    }

    // BP_RandomItemBox 지정
    if (!RandomItemBoxClass)
    {
        return;
    }

    // Destroy 된 박스 정리
    ClearInvalidSpawnedItemBoxes();

    // 모든 슬롯 순회하면서 비어있는 위치에만 아이템 박스 스폰
    for (FRandomItemBoxSpawnSlot& SpawnSlot : ItemBoxSpawnSlots)
    {
        SpawnRandomItemBoxAtSlot(SpawnSlot);
    }
}

void AItemSpawnManager::ClearInvalidSpawnedItemBoxes()
{
    for (FRandomItemBoxSpawnSlot& SpawnSlot : ItemBoxSpawnSlots)
    {
        // 박스가 Destroy되면 Invalid가 false가 됨
        // 슬롯 재사용 가능하도록 nullptr 사용
        if (!IsValid(SpawnSlot.SpawnedItemBox))
        {
            SpawnSlot.SpawnedItemBox = nullptr;
        }
    }
}

void AItemSpawnManager::SpawnRandomItemBoxAtSlot(FRandomItemBoxSpawnSlot& SpawnSlot)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(SpawnSlot.SpawnPoint))
    {
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

    // RandomItemBoxClass에 지정된 BP_RandomItemBox를 SapwnPoint 위치에 생성함
    ARandomItemBox* SpawnedItemBox = GetWorld()->SpawnActor<ARandomItemBox>(
        RandomItemBoxClass,
        SpawnTransform,
        SpawnParams
    );

    if (!IsValid(SpawnedItemBox))
    {
        return;
    }

    SpawnSlot.SpawnedItemBox = SpawnedItemBox;
}


void AItemSpawnManager::ClearSpawnedRandomItemBoxes()
{
    if (!HasAuthority())
    {
        return;
    }

    for (FRandomItemBoxSpawnSlot& SpawnSlot : ItemBoxSpawnSlots)
    {
        // 라운드 종료 시 남아 있는 박스 제거
        if (IsValid(SpawnSlot.SpawnedItemBox))
        {
            SpawnSlot.SpawnedItemBox->Destroy();
        }

        SpawnSlot.SpawnedItemBox = nullptr;
    }
}

