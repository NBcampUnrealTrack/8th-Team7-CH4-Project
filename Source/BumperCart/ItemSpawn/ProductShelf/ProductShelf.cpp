#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/PrimitiveComponent.h"

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
    if (!TestActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("가판대에 TestActorClass가 지정되지 않았습니다!"));
        return;
    }

    for (int i = 0; i < SpawnCount; i++)
    {
        FVector SpawnLocation = LaunchPoints->GetComponentLocation();
        FRotator SpawnRotation = UKismetMathLibrary::RandomRotator();

        AActor* TestItem = GetWorld()->SpawnActor<AActor>(TestActorClass, SpawnLocation, SpawnRotation);

        if (TestItem)
        {
            UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(TestItem->GetRootComponent());
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
}

