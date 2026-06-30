// Fill out your copyright notice in the Description page of Project Settings.


#include "Product/ProductBase.h"

#include "ProductDataAsset.h"
#include "TimerManager.h"
#include "Product/DataAsset/ProductDropConfig.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

AProductBase::AProductBase()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);

    // 컴포넌트 설정
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionProfileName(TEXT("ProductPhysics"));
    Mesh->SetSimulatePhysics(false);
    Mesh->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Mesh);

    GrabCollision = CreateDefaultSubobject<USphereComponent>(TEXT("GrabCollision"));
    GrabCollision->SetupAttachment(Mesh);
    GrabCollision->SetSphereRadius(65.f);
    GrabCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GrabCollision->SetCollisionProfileName(TEXT("ProductGrab"));

    bOnSale = false;
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
    SetProductState(EProductState::Display);
}

void AProductBase::Initialize(const FVector& SpawnLocation)
{
    SetActorLocation(SpawnLocation);
    SetProductState(EProductState::Display);
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
        SetNetUpdateFrequency(20.f);
        SetReplicateMovement(true);

        Mesh->SetSimulatePhysics(true);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;

    case EProductState::Grabbed:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetSimulatePhysics(false);
        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::Loaded:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetSimulatePhysics(false);
        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::Falling:
        SetActorHiddenInGame(false);
        SetNetUpdateFrequency(30.f);
        SetReplicateMovement(true);

        Mesh->SetSimulatePhysics(true);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // 떨어지는 중에도 회수 가능하면 QueryOnly, 아니면 NoCollision
        GrabCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;

    case EProductState::Paid:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetSimulatePhysics(false);
        Mesh->SetCollisionProfileName(TEXT("NoCollision"));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        GrabCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::None:   // Fall Through
    default:
        SetActorHiddenInGame(true);
        SetNetUpdateFrequency(1.f);
        SetReplicateMovement(false);

        Mesh->SetSimulatePhysics(false);
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

    // 카트에서 떨어뜨림
    //DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // 물리/충돌 잠깐 해제
    Mesh->SetSimulatePhysics(false);
    Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 위치 잡기
    FVector Offset = FVector(
        FMath::RandRange(-DropConfig->HorizontalOffset, DropConfig->HorizontalOffset),
        FMath::RandRange(-DropConfig->HorizontalOffset, DropConfig->HorizontalOffset),
        0.f);
    FVector DropLocation = CartActor->GetActorLocation() + Offset + FVector(0.f, 0.f, DropConfig->HeightOffset);
    SetActorLocation(DropLocation, false, nullptr, ETeleportType::TeleportPhysics);

    // 위쪽 방향 기준으로 랜덤 원뿔 위치 방향에 줄 Impulse 계산
    float SpreadRadian = FMath::DegreesToRadians(DropConfig->HalfAngle);
    float ImpulseVal = FMath::RandRange(DropConfig->MinImpulse, DropConfig->MaxImpulse);
    FVector ImpulseDirection = FMath::VRandCone(FVector::UpVector, SpreadRadian) * ImpulseVal;

    ProductState.DropLocation = DropLocation;

    // Falling 상태로 변환
    SetProductState(EProductState::Falling);

    // 물리 활성화
    Mesh->WakeAllRigidBodies();

    Mesh->AddImpulse(ImpulseDirection, NAME_None, true);

    // 강제로 위치 업데이트
    ForceNetUpdate();

    // 일정 시간뒤 진열 상태로 전환
    GetWorldTimerManager().SetTimer(
        ReturnDisplayTimer,
        this,
        &ThisClass::HandleReturnDisplay,
        DropConfig->FallingDuration,
        false
    );
}

void AProductBase::OnRep_ProductState()
{
    // Falling의 경우 물리를 먼저 끄고 위치를 조정한다음 상태 반영
    if (ProductState.State == EProductState::Falling)
    {
        Mesh->SetSimulatePhysics(false);
        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

        SetActorLocation(ProductState.DropLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }

    ApplyProductState();
}

void AProductBase::HandleReturnDisplay()
{
    // 떨어지는 중에 아이템을 먹으면 Loaded이므로 제한함
    // Falling 일때만 Display로 바꿈
    if (ProductState.State == EProductState::Falling)
    {
        SetProductState(EProductState::Display);
    }
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
