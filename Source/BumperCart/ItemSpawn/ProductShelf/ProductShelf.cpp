#include "ItemSpawn/ProductShelf/ProductShelf.h"

#include "Kismet/KismetMathLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Product/ProductBase.h"
#include "Kismet/GameplayStatics.h"
#include "ProductShelfSubsystem/ProductShelfSubsystem.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

AProductShelf::AProductShelf()
{
	PrimaryActorTick.bCanEverTick = false;

    ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
    ShelfMesh->SetMobility(EComponentMobility::Static);
    ShelfMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    RootComponent = ShelfMesh;

    LaunchPoints = CreateDefaultSubobject<USceneComponent>(TEXT("LaunchPoint"));
    LaunchPoints->SetupAttachment(RootComponent);

    SpawnAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    SpawnAreaBox->SetupAttachment(RootComponent);
    SpawnAreaBox->SetCollisionProfileName(TEXT("NoCollision"));
    SpawnAreaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnAreaBox->SetGenerateOverlapEvents(false);
    SpawnAreaBox->SetMobility(EComponentMobility::Static);
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

AProductBase* AProductShelf::SpawnRandomProduct(bool bOnSale)
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

    return SpawnSpecificItem(RandomProduct, bOnSale);
}

AProductBase* AProductShelf::SpawnSpecificItem(TSubclassOf<AProductBase> ItemClass, bool bOnSale)
{
    if (!HasAuthority() || !ItemClass) return nullptr;

    if (FMath::FRand() > SpawnProbability)  return nullptr;

    // 처음 스폰되는 곳
    FVector SpawnStartLocation = LaunchPoints->GetComponentLocation();
    FRotator SpawnStartRotation = LaunchPoints->GetComponentRotation();    

    FTransform SpawnTransform(SpawnStartRotation, SpawnStartLocation);

    // 스폰시 충돌 처리 규칙
    // 강제 스폰
    // 제품 스폰
    AProductBase* SpawnedProduct = GetWorld()->SpawnActorDeferred<AProductBase>(ItemClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    // 제품 스폰 성공
    if (SpawnedProduct)
    {
        // OnSale를 먼저 지정하고 스폰 완료 처리
        SpawnedProduct->SetOnSale(bOnSale);

        SpawnedProduct->FinishSpawning(SpawnTransform);


        // 스폰 후 날아가는 위치
        FVector SpawnEndLocation = GetRandomPointInVolume();

        // 제품 베이스의 StartSpawn()으로 날아가는 느낌 표현
        SpawnedProduct->StartSpawn(SpawnStartLocation, SpawnEndLocation, this);

        // 이펙트, 사운드
        Multicast_ProductSpawnEffect(SpawnStartLocation, SpawnStartRotation);

        //UE_LOG(LogTemp, Log, TEXT("[선반] %s 제품 스폰."), *SpawnedProduct->GetName());

        return SpawnedProduct;
    }

    return nullptr;
}

FVector AProductShelf::GetRandomPointInVolume() const
{
    if (!SpawnAreaBox) return GetActorLocation();

    // 박스 x/y/z 의 반지름
    FVector BoxExtent = SpawnAreaBox->GetUnscaledBoxExtent();
    // 박스 중심
    FVector BoxOrigin = SpawnAreaBox->GetRelativeLocation();

    // 상자 크기를 기준으로 로컬 랜덤 좌표
    FVector LocalRandomOffset = FVector(
        FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
        FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
        FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z));

    // 박스의 배치 시작점에 랜덤 오프셋을 더해 '선반 기준의 로컬 좌표'
    FVector ShelfLocalLocation = BoxOrigin + LocalRandomOffset;

    // 선반 액터 자체의 월드 트랜스폼(위치/회전)을 가져옵니다.
    FTransform ShelfTransform = GetActorTransform();

    // TransformPosition이 이 로컬 좌표를 선반이 회전한 각도만큼 돌림.
    return ShelfTransform.TransformPosition(ShelfLocalLocation);
}

void AProductShelf::Multicast_ProductSpawnEffect_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
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
                    SpawnLocation,
                    SpawnRotation,
                    FVector(1.0f),
                    true,
                    true
                );
            }

            USoundBase* SpawnSound = ProductShelfSubsystem->GetSpawnSound();

            if (IsValid(SpawnSound))
            {
                UGameplayStatics::PlaySoundAtLocation(
                    World,
                    SpawnSound,
                    SpawnLocation,
                    FRotator::ZeroRotator
                );
            }

        }
    }
}

