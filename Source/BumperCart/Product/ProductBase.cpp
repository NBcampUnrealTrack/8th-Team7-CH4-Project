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
#include "Components/SceneComponent.h"

AProductBase::AProductBase()
{
 	PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    bReplicates = true;
    SetReplicateMovement(false);

    // 컴포넌트 설정
    Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    SetRootComponent(Scene);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetupAttachment(Scene);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionProfileName(TEXT("ProductPhysics"));
    Mesh->SetSimulatePhysics(false);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetReceivesDecals(false);

    GrabCollision = CreateDefaultSubobject<USphereComponent>(TEXT("GrabCollision"));
    GrabCollision->SetupAttachment(Scene);
    GrabCollision->SetSphereRadius(40.f);
    GrabCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GrabCollision->SetCollisionProfileName(TEXT("ProductGrab"));

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
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // 클라이언트면 둥둥 떠다니는 연출위해 Tick 켜기
        if (GetNetMode() != NM_DedicatedServer)
        {
            SetActorTickEnabled(true);
        }
        break;

    case EProductState::Grabbed:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::Loaded:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::Falling:
        SetActorHiddenInGame(false);
        SetNetUpdateFrequency(30.f);
        SetReplicateMovement(false);

        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        SetActorTickEnabled(true);
        break;

    case EProductState::Paid:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetCollisionProfileName(TEXT("NoCollision"));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::None:   // Fall Through
    default:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

    // 충돌 잠깐 끄기
    Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
    FallingEndLocation = FallingStartLocation + Offset;
    FallingElapsedTime = 0.f;

    ProductState.FallingStartLocation = FallingStartLocation;
    ProductState.FallingEndLocation = FallingEndLocation;
    ProductState.FallingHeight = FallingHeight;

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
    return ProductState.State == EProductState::Display ||
        ProductState.State == EProductState::Falling;
}

void AProductBase::TickDisplay(float DeltaTime)
{
    if (!IsValid(Mesh)) return;

    ElapsedTime += DeltaTime;

    // 메시만 SIn 함수 따라 상대 좌표 위아래로 이동
    float BobZ = FMath::Sin(ElapsedTime * BobbingSpeed) * BobbingAmplitude;
    Mesh->SetRelativeLocation(FVector(0.f, 0.f, BobZ));

    // 회전값 적용
    FRotator Rotation = Mesh->GetRelativeRotation();
    Rotation.Yaw = RotationSpeed * ElapsedTime;
    Mesh->SetRelativeRotation(Rotation);
}

void AProductBase::TickFalling(float DeltaTime)
{
    if (ProductState.State != EProductState::Falling) return;

    FallingElapsedTime += DeltaTime;

    float Alpha = FallingElapsedTime / FMath::Max(DropConfig->FallingDuration, KINDA_SMALL_NUMBER);
    FVector Location = GetFallLocation(Alpha);

    SetActorLocation(Location, true);

    if (Alpha >= 1.f)
    {
        SetActorLocation(FallingEndLocation, true);

        if (HasAuthority())
        {
            SetProductState(EProductState::Display);
            ForceNetUpdate();
        }
    }
}

void AProductBase::SetBaseLocation(const FVector& Location)
{
}

FVector AProductBase::GetFallLocation(float Alpha) const
{
    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

    FVector Location = FMath::Lerp(FallingStartLocation, FallingEndLocation, Alpha);

    // PI 까지만 Sin 함수 이용하면 위로 올라갔다가 내려옴
    float Z = FMath::Sin(Alpha * PI) * FallingHeight;
    Location.Z += Z;

    return Location;
}
