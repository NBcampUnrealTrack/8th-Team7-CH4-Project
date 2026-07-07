#include "ItemSpawn/ProductShelf/ProductShelf.h"

#include "Kismet/KismetMathLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Product/ProductBase.h"
#include "Kismet/GameplayStatics.h"
#include "ProductShelfSubsystem/ProductShelfSubsystem.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"

AProductShelf::AProductShelf()
{
	PrimaryActorTick.bCanEverTick = false;

    ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
    RootComponent = ShelfMesh;

    LaunchPoints = CreateDefaultSubobject<USceneComponent>(TEXT("LaunchPoint"));
    LaunchPoints->SetupAttachment(RootComponent);

    SpawnAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    SpawnAreaBox->SetupAttachment(RootComponent);

}

void AProductShelf::BeginPlay()
{
	Super::BeginPlay();

    // 제품선반 서브시스템일 경우 (WorldSubsystem)
    if (UWorld* World = GetWorld())
    {
        if (auto* ProductShelfSubsystem = World->GetSubsystem<UProductShelfSubsystem>())
        {
            ProductShelfSubsystem->RegisterShelf(this, ShelfType);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[선반] 선반 서브시스템을 찾지 못했습니다."));
        }
    }
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

    // 처음 스폰되는 곳 (선반의 중심)
    FVector SpawnStartLocation = FVector::ZeroVector;

    if (ShelfMesh)
    {
        // 선반 메쉬의 진짜 3D 바운딩 박스(크기) 정보를 가져옵니다.
        FBoxSphereBounds MeshBounds = ShelfMesh->Bounds;

        // 피벗 위치와 상관없는 '진짜 기하학적 중심점'
        SpawnStartLocation = MeshBounds.Origin;
    }

    // 스폰시 충돌 처리 규칙
    // 강제 스폰
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 제품 스폰
    AProductBase* SpawnedProduct = GetWorld()->SpawnActor<AProductBase>(ItemClass, SpawnStartLocation, FRotator::ZeroRotator, SpawnParams);

    // 제품 스폰 성공
    if (SpawnedProduct)
    {
        // 스폰 후 날아가는 위치
        FVector SpawnLocation = GetRandomPointInVolume();
        FRotator SpawnRotation = UKismetMathLibrary::RandomRotator();

        // 제품 베이스의 StartSpawn()으로 날아가는 느낌 표현
        SpawnedProduct->StartSpawn(SpawnStartLocation, SpawnLocation, this);

        // 나이아가라
        if (UWorld* World = GetWorld())
        {
            if (auto* ProductShelfSubsystem = World->GetSubsystem<UProductShelfSubsystem>())
            {
                UNiagaraSystem* SpawnFX = ProductShelfSubsystem->GetSpawnFX();

                if (IsValid(SpawnFX))
                {
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                        World,
                        SpawnFX,
                        SpawnStartLocation,
                        SpawnRotation,
                        FVector(1.0f),
                        true,
                        true
                    );
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("[선반] %s 제품 스폰."), *SpawnedProduct->GetName());

        return SpawnedProduct;
    }

    return nullptr;
}

FVector AProductShelf::GetRandomPointInVolume() const
{
    // 박스 x/y/z 의 반지름
    FVector BoxExtent = SpawnAreaBox->GetScaledBoxExtent();
    // 박스 중심
    FVector BoxOrigin = SpawnAreaBox->GetComponentLocation();

    return BoxOrigin + FVector(
        FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
        FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
        FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z));
}

