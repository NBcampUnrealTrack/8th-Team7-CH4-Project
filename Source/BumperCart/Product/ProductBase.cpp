// Fill out your copyright notice in the Description page of Project Settings.


#include "Product/ProductBase.h"

#include "ProductDataAsset.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AProductBase::AProductBase()
{
 	PrimaryActorTick.bCanEverTick = false;

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


    bReplicates = true;
}

void AProductBase::Initialize(const FVector& SpawnLocation)
{
    SetActorLocation(SpawnLocation);
    ProductState = EProductState::Display;
}

void AProductBase::BeginPlay()
{
	Super::BeginPlay();

    ApplyDataAsset();
    ProductState = EProductState::Display;
}

void AProductBase::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ApplyDataAsset();
}

void AProductBase::OnBeginOverlapCart(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 태그로 확인하는데 이후에 Interface를 사용할지 컴포넌트를 사용할지 생각해볼 것
    if (IsValid(OtherActor) && OtherActor->ActorHasTag(TEXT("Player")))
    {
        // 진열된 상태가 아니면 충돌 X
        if (ProductState != EProductState::Display) return;

        ProcessBeginOverlap(OtherActor);
    }
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
