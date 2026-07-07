// Fill out your copyright notice in the Description page of Project Settings.


#include "Product/ProductBase.h"

#include "ProductDataAsset.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "ProductShelfSubsystem/ProductShelfSubsystem.h"

AProductBase::AProductBase()
{
 	PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    bReplicates = true;
    SetReplicateMovement(false);
    SetNetUpdateFrequency(1.f);

    // 컴포넌트 설정
    ProductCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
    ProductCollision->SetSphereRadius(40.f);
    ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProductCollision->SetGenerateOverlapEvents(false);
    ProductCollision->SetCollisionProfileName(TEXT("ProductCollision"));
    SetRootComponent(ProductCollision);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetupAttachment(ProductCollision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetSimulatePhysics(false);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetReceivesDecals(false);

    bOnSale = false;

    ElapsedTime = 0.f;
    HeightOffset = 120.f;
    BobbingAmplitude = 30.f;
    BobbingSpeed = 1.5f;
    RotationSpeed = 60.f;

    LaunchElapsedTime = 0.f;
    FallingMinHeight = 120.f;
    FallingMaxHeight = 200.f;
    FallingHorizontalOffset = 200.f;
    FallingDuration = 1.f;

    SpawningDuration = 0.7f;
    SpawningHeight = 160.f;
}

void AProductBase::Destroyed()
{
    UWorld* World = GetWorld();
    if (IsValid(World))
    {
        if (UProductShelfSubsystem* ShelfSubsystem = World->GetSubsystem<UProductShelfSubsystem>())
        {
            ShelfSubsystem->OnProductDestroyed();
        }
    }

    Super::Destroyed();
}

void AProductBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    switch (EndPlayReason)
    {
    case EEndPlayReason::Destroyed:
        UE_LOG(LogTemp, Warning, TEXT("%s 상품 파괴"), *GetName());
        break;

    case EEndPlayReason::RemovedFromWorld:
        UE_LOG(LogTemp, Warning, TEXT("%s 상품 월드에서 제거"), *GetName());
        break;
    }
}

void AProductBase::BeginPlay()
{
    Super::BeginPlay();

    ApplyDataAsset();

    FVector BobbingLocation = GetActorLocation();
    BobbingLocation.Z = HeightOffset;
    SetActorLocation(BobbingLocation, false, nullptr, ETeleportType::TeleportPhysics);

    // 랜덤한 위아래 움직임을 위해 시작 시간을 다르게 함
    // 연출이라 모든 클라가 같을 필요는 없음
    ElapsedTime = FMath::RandRange(0.f, 2.f);

    // 상대 위치, 회전값 저장
    BaseMeshLocation = Mesh->GetRelativeLocation();
    BaseMeshRotation = Mesh->GetRelativeRotation();

    SetProductState(EProductState::None);
}

void AProductBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (ProductState.State == EProductState::Display)
    {
        TickDisplay(DeltaTime);
    }
    else if (IsLaunchState())
    {
        TickLaunch(DeltaTime);
    }
}

void AProductBase::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ApplyDataAsset();
}

void AProductBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ProductState);
}

void AProductBase::ApplyDataAsset()
{
    if (!IsValid(ProductDataAsset)) return;

    if (IsValid(ProductDataAsset->ProductMesh))
    {
        Mesh->SetStaticMesh(ProductDataAsset->ProductMesh);
    }
}

void AProductBase::ApplyProductState()
{
    switch (ProductState.State)
    {
    case EProductState::Display:
        SetActorHiddenInGame(false);

        ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // 클라이언트면 둥둥 떠다니는 연출위해 Tick 켜기, 데이케이트 서버는 끄기
        SetActorTickEnabled(GetNetMode() != NM_DedicatedServer);
        break;

    case EProductState::Spawning:   // Fall Through
    case EProductState::Falling:
        SetActorHiddenInGame(false);

        ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        SetActorTickEnabled(true);
        break;

    case EProductState::Grabbed:    // Fall Through
    case EProductState::Loaded:
    case EProductState::Paid:   
    case EProductState::None: 
    default:
        SetActorHiddenInGame(true);

        ProductCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        SetActorTickEnabled(false);
        break;
    }
}

void AProductBase::StartSpawn(const FVector& StartLocation, const FVector& EndLocation, AActor* IgnoreActor)
{
    if (!HasAuthority()) return;
    StartLaunch(
        EProductState::Spawning,
        StartLocation,
        EndLocation,
        SpawningHeight,
        SpawningDuration,
        IgnoreActor
    );
}

void AProductBase::SetProductState(EProductState NewState)
{
    if (!HasAuthority()) return;

    if (ProductState.State == NewState) return;

    ProductState.State = NewState;

    // 서버 또한 State에 따른 변화를 적용해야 함
    ApplyProductState();
}

bool AProductBase::TrySetLoaded()
{
    if (!HasAuthority()) return false;

    if (!CanLoad()) return false;

    SetProductState(EProductState::Loaded);
    ForceNetUpdate();
    return true;
}

bool AProductBase::TrySetGrabbed()
{
    if (!HasAuthority()) return false;

    if (!CanGrab()) return false;

    SetProductState(EProductState::Grabbed);
    ForceNetUpdate();
    return true;
}

void AProductBase::DropFromCart(AActor* CartActor)
{
    if (!HasAuthority()) return;
    if (!IsValid(CartActor)) return;

    // Falling 오프셋 결정
    FVector Offset = FVector(
        FMath::RandRange(-FallingHorizontalOffset, FallingHorizontalOffset),
        FMath::RandRange(-FallingHorizontalOffset, FallingHorizontalOffset),
        0.f);

    float RandomHeight = FMath::RandRange(FallingMinHeight, FallingMaxHeight);

    // Falling 시작 위치 잡기
    FVector StartLocation = CartActor->GetActorLocation();
    StartLocation.Z = HeightOffset;

    // Falling 목표 위치 잡기, 시간 0 으로 설정
    FVector EndLocation = StartLocation + Offset;
    LaunchElapsedTime = 0.f;

    StartLaunch(
        EProductState::Falling,
        StartLocation,
        EndLocation,
        RandomHeight,
        FallingDuration,
        CartActor
    );
}

void AProductBase::OnRep_ProductState()
{
    // Falling or Spawn으로 변할땐 포물선 운동 값 설정
    if (IsLaunchState())
    {
        LaunchElapsedTime = 0.f;

        SetActorLocation(ProductState.LaunchStartLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
    else if (ProductState.State == EProductState::Display && ProductState.bHasDisplayLocation)
    {
        // Falling/Spawning -> Display로 변할땐 위치 지정
        FVector ServerLocation = ProductState.DisplayLocation;
        ServerLocation.Z = HeightOffset;

        SetActorLocation(ServerLocation, false, nullptr, ETeleportType::TeleportPhysics);

        if (IsValid(Mesh))
        {
            Mesh->SetRelativeLocation(BaseMeshLocation);
            Mesh->SetRelativeRotation(BaseMeshRotation);
        }
    }

    ApplyProductState();
}

int32 AProductBase::GetWeight() const
{
    if (!ProductDataAsset) return 0;

    return ProductDataAsset->ProductData.Weight;
}

int32 AProductBase::GetValue() const
{
    if (!ProductDataAsset) return 0;

    return ProductDataAsset->ProductData.Value;
}

EProductState AProductBase::GetProductState() const
{
    return ProductState.State;
}

FLoadedProductInfo AProductBase::GetLoadedProductInfo() const
{
    FLoadedProductInfo Info;

    if (IsValid(ProductDataAsset))
    {
        Info.ProductId = ProductDataAsset->ProductId;
        Info.Value = ProductDataAsset->ProductData.Value;
    }
    Info.bOnSale = bOnSale;

    return Info;
}

UStaticMesh* AProductBase::GetProductMesh() const
{
    return IsValid(Mesh) ? Mesh->GetStaticMesh() : nullptr;
}

void AProductBase::SetOnSale(bool NewValue)
{
    bOnSale = NewValue;
}

bool AProductBase::IsOnSale() const
{
    return bOnSale;
}

bool AProductBase::CanLoad() const
{
    return ProductState.State == EProductState::Grabbed;
}

bool AProductBase::CanGrab() const
{
    return ProductState.State == EProductState::Display;
}

void AProductBase::TickDisplay(float DeltaTime)
{
    if (!IsValid(Mesh)) return;

    ElapsedTime += DeltaTime;

    // 메시만 SIn 함수 따라 상대 좌표 위아래로 이동
    float BobZ = FMath::Sin(ElapsedTime * BobbingSpeed) * BobbingAmplitude;
    Mesh->SetRelativeLocation(BaseMeshLocation + FVector(0.f, 0.f, BobZ));

    // 회전값 적용
    FRotator Rotation(0.f, RotationSpeed * ElapsedTime, 0.f);
    Mesh->SetRelativeRotation(BaseMeshRotation + Rotation);
}

void AProductBase::StartLaunch(EProductState State, const FVector& StartLocation, const FVector& EndLocation, float InHeight, float InDuration, AActor* IgnoreActor)
{
    if (!HasAuthority()) return;

    FVector FixedStart = StartLocation;
    FixedStart.Z = HeightOffset;

    FVector FixedEnd = EndLocation;
    FixedEnd.Z = HeightOffset;

    // 목표 지점을 벽/가판대 안으로 들어가지 않게 조정
    FVector SafeEnd = GetSafeLocation(FixedStart, FixedEnd, IgnoreActor);

    LaunchIgnoredActor = IgnoreActor;

    // 충돌체와 무시할 액터가 유효하면 해당 액터는 무시하고 이동하게 함
    //  -> 가판대 or 카트에 껴서 못나오는 문제 해결하기 위함
    if (IsValid(ProductCollision) && IsValid(IgnoreActor))
    {
        ProductCollision->IgnoreActorWhenMoving(IgnoreActor, true);
    }

    // 복제되는 구조체 한번에 설정하고 대입
    FProductRepState NewRepState;
    NewRepState.State = State;
    NewRepState.LaunchStartLocation = FixedStart;
    NewRepState.LaunchEndLocation = SafeEnd;
    NewRepState.LaunchHeight = InHeight;
    NewRepState.LaunchDuration = InDuration;
    NewRepState.bHasDisplayLocation = false;

    ProductState = NewRepState;

    LaunchElapsedTime = 0.f;

    // 우선 시작지점으로 이동
    SetActorLocation(ProductState.LaunchStartLocation, false, nullptr, ETeleportType::TeleportPhysics);

    ApplyProductState();
    ForceNetUpdate();
}

void AProductBase::TickLaunch(float DeltaTime)
{
    LaunchElapsedTime += DeltaTime;

    float Alpha = FMath::Clamp(
        LaunchElapsedTime / FMath::Max(ProductState.LaunchDuration, KINDA_SMALL_NUMBER),
        0.f,
        1.f
    );

    // 실제 움직임은 XY만
    FVector BaseLocation = FMath::Lerp(
        FVector(ProductState.LaunchStartLocation),
        FVector(ProductState.LaunchEndLocation),
        Alpha
    );
    BaseLocation.Z = HeightOffset;

    if (HasAuthority())
    {
        SetActorLocation(BaseLocation, true);
    }
    else
    {
        SetActorLocation(BaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
    

    if (IsValid(Mesh))
    {
        // 보여지는 메시만 위로 튀는 연출
        // Sin함수 PI까지만하면 0 ~ 1, 1 ~ 0 처리
        float CurrentZ = FMath::Sin(Alpha * PI) * ProductState.LaunchHeight;
        Mesh->SetRelativeLocation(BaseMeshLocation + FVector(0.f, 0.f, CurrentZ));
    }

    if (Alpha >= 1.f && HasAuthority())
    {
        SetActorLocation(ProductState.LaunchEndLocation, true);

        // 서버에서 상품이 실제로 위치한 좌표 저장
        ProductState.DisplayLocation = GetActorLocation();
        ProductState.DisplayLocation.Z = HeightOffset;
        ProductState.bHasDisplayLocation = true;

        // 시작할때 무시하도록 설정했던 값 되돌리기
        if (IsValid(ProductCollision) && LaunchIgnoredActor.IsValid())
        {
            ProductCollision->IgnoreActorWhenMoving(LaunchIgnoredActor.Get(), false);
        }
        LaunchIgnoredActor.Reset();

        if (IsValid(Mesh))
        {
            Mesh->SetRelativeLocation(BaseMeshLocation);
            Mesh->SetRelativeRotation(BaseMeshRotation);
        }

        SetProductState(EProductState::Display);
        ForceNetUpdate();
    }
}

bool AProductBase::IsLaunchState() const
{
    return ProductState.State == EProductState::Spawning ||
        ProductState.State == EProductState::Falling;
}

FVector AProductBase::GetSafeLocation(const FVector& Start, const FVector& End, AActor* IgnoreActor)
{
    if (!IsValid(ProductCollision)) return End;

    UWorld* World = GetWorld();
    if (!IsValid(World)) return End;

    FVector SafeStart = Start;
    SafeStart.Z = HeightOffset;

    FVector SafeEnd = End;
    SafeEnd.Z = HeightOffset;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ProductSafeLocation), false);
    Params.AddIgnoredActor(this);
    if (IsValid(IgnoreActor))
    {
        Params.AddIgnoredActor(IgnoreActor);
    }

    FCollisionShape Shape = FCollisionShape::MakeSphere(ProductCollision->GetScaledSphereRadius());

    FHitResult Hit;
    bool bHit = World->SweepSingleByProfile(
        Hit,
        SafeStart,
        SafeEnd,
        FQuat::Identity,
        ProductCollision->GetCollisionProfileName(),
        Shape,
        Params
    );

    // Sweep 으로 충돌 안했으면 안전한 위치임
    if (!bHit)
    {
        return SafeEnd;
    }

    // 시작지점부터 겹쳐있었다면 시작지점이 타겟
    if (Hit.bStartPenetrating)
    {
        return SafeStart;
    }

    // 충돌 지점을 구해서 반환
    FVector SafeLocation = Hit.Location;
    SafeLocation.Z = HeightOffset;
    return SafeLocation;
}
