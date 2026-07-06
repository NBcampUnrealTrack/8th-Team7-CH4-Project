// Fill out your copyright notice in the Description page of Project Settings.


#include "Product/ProductBase.h"

#include "ProductDataAsset.h"
#include "TimerManager.h"
#include "Product/DataAsset/ProductDropConfig.h"
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

    FallingStartLocation = FVector::ZeroVector;
    FallingEndLocation = FVector::ZeroVector;
    FallingElapsedTime = 0.f;
    FallingMinHeight = 120.f;
    FallingMaxHeight = 200.f;
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

    SetProductState(EProductState::Display);
}

void AProductBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (ProductState.State == EProductState::Display)
    {
        TickDisplay(DeltaTime);
    }
    else if (ProductState.State == EProductState::Falling)
    {
        TickFalling(DeltaTime);
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
    if (!IsValid(CartActor) || !IsValid(DropConfig)) return;

    // Faling 오프셋 결정
    FVector Offset = FVector(
        FMath::RandRange(-DropConfig->HorizontalOffset, DropConfig->HorizontalOffset),
        FMath::RandRange(-DropConfig->HorizontalOffset, DropConfig->HorizontalOffset),
        0.f);

    FallingHeight = FMath::RandRange(FallingMinHeight, FallingMaxHeight);

    // Falling 시작 위치 잡기
    FallingStartLocation = CartActor->GetActorLocation();
    FallingStartLocation.Z = HeightOffset;

    // Falling 목표 위치 잡기, 시간 0 으로 설정
    FallingEndLocation = GetSafeLocation(FallingStartLocation, FallingStartLocation + Offset, CartActor);
    FallingElapsedTime = 0.f;

    ProductState.FallingStartLocation = FallingStartLocation;
    ProductState.FallingEndLocation = FallingEndLocation;
    ProductState.FallingHeight = FallingHeight;
    ProductState.bIsFell = false;

    // Falling 시작 위치로 일단 이동
    SetActorLocation(FallingStartLocation, false, nullptr, ETeleportType::TeleportPhysics);

    // Falling 상태로 변환
    SetProductState(EProductState::Falling);

    // 강제로 위치 업데이트
    ForceNetUpdate();
}

void AProductBase::OnRep_ProductState()
{
    // Falling으로 변할땐 위치 지정
    if (ProductState.State == EProductState::Falling)
    {
        FallingStartLocation = ProductState.FallingStartLocation;
        FallingEndLocation = ProductState.FallingEndLocation;
        FallingHeight = ProductState.FallingHeight;
        FallingElapsedTime = 0.f;

        SetActorLocation(ProductState.FallingStartLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
    else if (ProductState.State == EProductState::Display && ProductState.bIsFell)
    {
        ProductState.bIsFell = false;

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
    return ProductDataAsset->ProductData.Weight;
}

int32 AProductBase::GetValue() const
{
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

void AProductBase::TickFalling(float DeltaTime)
{
    if (ProductState.State != EProductState::Falling) return;
    if (!IsValid(DropConfig)) return;

    FallingElapsedTime += DeltaTime;

    float Alpha = FMath::Clamp(
        FallingElapsedTime / FMath::Max(DropConfig->FallingDuration, KINDA_SMALL_NUMBER),
        0.f,
        1.f
    );

    // 실제 움직임은 XY만
    FVector BaseLocation = FMath::Lerp(FallingStartLocation, FallingEndLocation, Alpha);
    BaseLocation.Z = HeightOffset;
    SetActorLocation(BaseLocation, true);

    if (IsValid(Mesh))
    {
        // 보여지는 메시만 위로 튀는 연출
        // Sin함수 PI까지만하면 0 ~ 1, 1 ~ 0 처리
        float CurrentZ = FMath::Sin(Alpha * PI) * FallingHeight;
        Mesh->SetRelativeLocation(BaseMeshLocation + FVector(0.f, 0.f, CurrentZ));
    }

    if (Alpha >= 1.f && HasAuthority())
    {
        SetActorLocation(FallingEndLocation, true);

        // 서버에서 상품이 실제로 위치한 좌표 저장
        ProductState.DisplayLocation = GetActorLocation();
        ProductState.DisplayLocation.Z = HeightOffset;
        ProductState.bIsFell = true;

        if (IsValid(Mesh))
        {
            Mesh->SetRelativeLocation(BaseMeshLocation);
            Mesh->SetRelativeRotation(BaseMeshRotation);
        }

        SetProductState(EProductState::Display);
        ForceNetUpdate();
    }
}

FVector AProductBase::GetSafeLocation(const FVector& Start, const FVector& End, AActor* IgnoreActor)
{
    if (!IsValid(ProductCollision)) return End;

    FVector SafeStart = Start;
    SafeStart.Z = HeightOffset;

    FVector SafeEnd = End;
    SafeEnd.Z = HeightOffset;

    // 이전 콜리전Enabled 저장
    ECollisionEnabled::Type PrevCollisionEnabled = ProductCollision->GetCollisionEnabled();
    ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    // 충돌 무시할 액터 설정
    if (IsValid(IgnoreActor))
    {
        ProductCollision->IgnoreActorWhenMoving(IgnoreActor, true);
    }

    // 시작점으로 이동했다가 목표 지점으로 sweep 을 킨 상태로 이동
    SetActorLocation(SafeStart, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorLocation(SafeEnd, true);

    // 안전한 위치 저장
    FVector SafeLocation = GetActorLocation();
    SafeLocation.Z = HeightOffset;

    // 위치 저장했으니 원래 위치로 복귀
    SetActorLocation(SafeStart, false, nullptr, ETeleportType::TeleportPhysics);

    // 충돌 무시할 액터 설정 되돌리기
    if (IsValid(IgnoreActor))
    {
        ProductCollision->IgnoreActorWhenMoving(IgnoreActor, false);
    }

    ProductCollision->SetCollisionEnabled(PrevCollisionEnabled);

    return SafeLocation;
}
