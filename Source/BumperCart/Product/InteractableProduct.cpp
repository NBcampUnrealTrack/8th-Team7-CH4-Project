// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableProduct.h"

#include "Cart/Component/CartLoadComponent.h"
#include "Components/SphereComponent.h"


AInteractableProduct::AInteractableProduct()
{
    // 상호작용하는 상품은 EndOverlap 또한 처리해줘야 함
    SphereCollision->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlapCart);
}

void AInteractableProduct::OnLoaded()
{
    if (!HasAuthority()) return;

    SetProductState(EProductState::Loaded);

    for (AActor* OverlappedActor : OverlappedActors)
    {
        if (IsValid(OverlappedActor))
        {
            DisableInteraction(OverlappedActor);
        }
    }
    OverlappedActors.Empty();
}

void AInteractableProduct::ProcessBeginOverlap(AActor* OtherActor)
{
    // Overlap Count 증가시켜야 함
    UE_LOG(LogTemp, Warning, TEXT("%s 상품에 %s 플레이어 들어옴"), *this->GetName(), *OtherActor->GetName());
    OverlappedActors.Add(OtherActor);

    // 특정 클라이언트의 함수 호출 필요, 인터페이스 or Cast
    // 특정 클라이언트가 이후에 이 상품을 습득하면 OnLoaded 호출해야 함
}

void AInteractableProduct::OnEndOverlapCart(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority()) return;
    if (!IsValid(OtherActor)) return;
    if (ProductState != EProductState::Display) return;


    // Overlap Count 감소시켜야 함
    // Player의 Overlap Count가 0이될 때 상호작용 비활성화
    UE_LOG(LogTemp, Warning, TEXT("%s 상품에서 %s 플레이어 벗어남"), *this->GetName(), *OtherActor->GetName());
    OverlappedActors.Remove(OtherActor);
    DisableInteraction(OtherActor);
}

void AInteractableProduct::DisableInteraction(AActor* OtherActor)
{
    // 특정 클라이언트의 함수 호출 필요, 인터페이스 or Cast
}
