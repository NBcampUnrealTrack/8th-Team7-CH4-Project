#include "ItemSpawn/ProductShelf/ProductShelf.h"

#include "Kismet/KismetMathLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Product/ProductBase.h"

AProductShelf::AProductShelf()
{
	PrimaryActorTick.bCanEverTick = false;

    ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
    RootComponent = ShelfMesh;

    LaunchPoints = CreateDefaultSubobject<USceneComponent>(TEXT("LaunchPoint"));
    LaunchPoints->SetupAttachment(RootComponent);

}

void AProductShelf::BeginPlay()
{
	Super::BeginPlay();
}

AProductBase* AProductShelf::SpawnRandomProduct()
{
    if (!HasAuthority())  return nullptr;

    if (ProductList.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] 가판대에 스폰할 제품이 없습니다!"), *GetName());
        return nullptr;
    }

    // 제품 목록에서 랜덤 선택
    int32 RandomIndex = FMath::RandRange(0, ProductList.Num() - 1);
    TSubclassOf<AProductBase> RandomProduct = ProductList[RandomIndex];

    return SpawnSpecificItem(RandomProduct);
}

AProductBase* AProductShelf::SpawnSpecificItem(TSubclassOf<AProductBase> ItemClass)
{
    if (!HasAuthority() || !ItemClass) return nullptr;

    FVector SpawnLocation = LaunchPoints->GetComponentLocation();
    FRotator SpawnRotation = UKismetMathLibrary::RandomRotator();

    // 스폰시 충돌 처리 규칙
    // 가능하면 위치를 조정하고, 계속 부딪치면 스폰x
    // 강제 스폰
    FActorSpawnParameters SpawnParams;
    //SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AProductBase* SpawnedProduct = GetWorld()->SpawnActor<AProductBase>(ItemClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (SpawnedProduct)
    {
        UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(SpawnedProduct->GetRootComponent());
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

            SpawnedProduct->SetLifeSpan(30.0f);
        }

        UE_LOG(LogTemp, Log, TEXT("[%s] 제품 스폰."), *GetName());

        return SpawnedProduct;
    }

    return nullptr;
}

