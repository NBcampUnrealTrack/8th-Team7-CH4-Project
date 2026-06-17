// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableProduct.h"

#include "Components/SphereComponent.h"


AInteractableProduct::AInteractableProduct()
{
    // 상호작용하는 상품은 EndOverlap 또한 처리해줘야 함
    SphereCollision->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlapCart);
}

void AInteractableProduct::ProcessBeginOverlap(AActor* OtherActor)
{
    EnableInteraction(OtherActor);
}

void AInteractableProduct::OnEndOverlapCart(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (IsValid(OtherActor) && OtherActor->ActorHasTag(TEXT("Player")))
    {
        // 진열된 상태가 아니면 충돌 X
        if (ProductState != EProductState::Display) return;

        DisableInteraction(OtherActor);
    }
}

void AInteractableProduct::EnableInteraction(AActor* OtherActor)
{
    // Overlap Count 증가시켜야 함
    UE_LOG(LogTemp, Warning, TEXT("%s 상품에 %s 플레이어 들어옴"), *this->GetName(), *OtherActor->GetName());
    OnBeginOverlapInteractableProduct.Broadcast(this, OtherActor);
}

void AInteractableProduct::DisableInteraction(AActor* OtherActor)
{
    // Overlap Count 감소시켜야 함
    // Player의 Overlap Count가 0이될 때 상호작용 비활성화
    UE_LOG(LogTemp, Warning, TEXT("%s 상품에서 %s 플레이어 벗어남"), *this->GetName(), *OtherActor->GetName());
    OnEndOverlapInteractableProduct.Broadcast(this, OtherActor);
}
