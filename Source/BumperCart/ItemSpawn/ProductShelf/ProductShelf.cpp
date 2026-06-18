#include "ItemSpawn/ProductShelf/ProductShelf.h"

#include "Kismet/KismetMathLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Product/PickUpProduct.h"

AProductShelf::AProductShelf()
{
	PrimaryActorTick.bCanEverTick = false;

    ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
    RootComponent = ShelfMesh;

    LaunchPoints = CreateDefaultSubobject<USceneComponent>(TEXT("LaunchPoint"));
    LaunchPoints->SetupAttachment(RootComponent);

    ShelfType = EShelfType::Common;

}

void AProductShelf::BeginPlay()
{
	Super::BeginPlay();

    SpawnRandomItems();

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AProductShelf::SpawnRandomItems, RespawnDelay, true);
	
}

void AProductShelf::SpawnRandomItems()
{
    if (ProductBlueprintClasses.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] 가판대에 등록된 아이템 블루프린트가 없습니다!"), *GetName());
        return;
    }

    if (!bSpawning)
    {
        UE_LOG(LogTemp, Warning, TEXT("아이템 스폰 불가"));
        return;
    }

    for (int i = 0; i < SpawnCount; i++)
    {

        if (SpawnedItems.Num() >= SpawnMaxCount)
        {
            UE_LOG(LogTemp, Warning, TEXT("아이템 스폰 제한"));
            break;
        }

        // 아이템 목록에서 랜덤 선택
        int32 RandomIndex = FMath::RandRange(0, ProductBlueprintClasses.Num() - 1);
        TSubclassOf<APickUpProduct> ChosenClass = ProductBlueprintClasses[RandomIndex];

        if (!ChosenClass) continue;

        FVector SpawnLocation = LaunchPoints->GetComponentLocation();
        FRotator SpawnRotation = UKismetMathLibrary::RandomRotator();

        AActor* SpawnItem = GetWorld()->SpawnActor<AActor>(ChosenClass, SpawnLocation, SpawnRotation);

        if (SpawnItem)
        {
            // 스폰된 아이템 배열에 저장
            SpawnedItems.Add(SpawnItem);

            UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(SpawnItem->GetRootComponent());
            if (PhysComp)
            {
                // 가판대 정면 방향을 기준으로 좌우/상하로 퍼지는 무작위 벡터 계산
                FVector ForwardDir = LaunchPoints->GetForwardVector();
                FVector RandomDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(ForwardDir, 45.0f); // 45도 원뿔 범위로 퍼짐

                // 위쪽으로 살짝 뜨게 만들기 위해 Z축 힘 추가
                RandomDir.Z += 0.5f;
                RandomDir.Normalize();

                // 힘을 가해 튕겨내기
                PhysComp->AddImpulse(RandomDir * LaunchForce, NAME_None, true);
            }
        }
    }

    // 사라진 아이템 처리
    SpawnedItems.RemoveAll([](AActor* Item)
    {
        return !IsValid(Item);
    });

    UE_LOG(LogTemp, Log, TEXT("현재 가판대가 관리 중인 아이템 개수: %d개"), SpawnedItems.Num());
}

bool AProductShelf::SetSpawnToggle(bool bToggle)
{
    return bSpawning = bToggle;
}

