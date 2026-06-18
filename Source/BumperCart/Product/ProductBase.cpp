// Fill out your copyright notice in the Description page of Project Settings.

 
#include "Product/ProductBase.h"

#include "ProductDataAsset.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

AProductBase::AProductBase()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    // 컴포넌트 설정
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetCollisionProfileName(TEXT("NoCollision"));
    SetRootComponent(Mesh);

    SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
    SphereCollision->SetupAttachment(Mesh);
    SphereCollision->SetSphereRadius(150.f);
    SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlapCart);

    // 기본 변수 설정
    ProductState = EProductState::None;
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

void AProductBase::OnBeginOverlapCart(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;
    if (!IsValid(OtherActor)) return;
    if (ProductState != EProductState::Display) return;

    ProcessBeginOverlap(OtherActor);
}

void AProductBase::ProcessBeginOverlap(AActor* OtherActor)
{
}

void AProductBase::ApplyDataAsset()
{
    if (!IsValid(ProductDataAsset)) return;

    ProductData = ProductDataAsset->ProductData;

    if (IsValid(ProductDataAsset->ProductMesh))
    {
        Mesh->SetStaticMesh(ProductDataAsset->ProductMesh);
    }
}

void AProductBase::ApplyProductState()
{
    switch (ProductState)
    {
    case EProductState::Display:
        SetActorHiddenInGame(false);
        SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;

    case EProductState::Loaded:
        //SetActorHiddenInGame(true);
        SetActorHiddenInGame(false);
        SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::Falling:
        SetActorHiddenInGame(false);
        SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::Paid:
        SetActorHiddenInGame(true);
        SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;

    case EProductState::None:   // Fall Through
    default:
        SetActorHiddenInGame(true);
        SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;
    }
}

void AProductBase::SetProductState(EProductState NewState)
{
    if (!HasAuthority()) return;

    if (ProductState == NewState) return;

    ProductState = NewState;

    // 서버 또한 State에 따른 변화를 적용해야 함
    ApplyProductState();
}

bool AProductBase::TrySetLoaded()
{
    if (!HasAuthority()) return false;

    if (ProductState != EProductState::Display) return false;

    SetProductState(EProductState::Loaded);
    return true;
}

void AProductBase::DropFromCart(AActor* CartActor)
{
    // 카트에서 떨어뜨림
    // Falling 상태로 변환
    // 카트 주변에 흩트리기
}

void AProductBase::OnRep_ProductState()
{
    ApplyProductState();
}

int32 AProductBase::GetWeight() const
{
    return ProductData.Weight;
}

int32 AProductBase::GetValue() const
{
    return ProductData.Value;
}

EProductState AProductBase::GetProductState() const
{
    return ProductState;
}
