// Fill out your copyright notice in the Description page of Project Settings.


#include "PickUpProduct.h"

#include "Cart/Component/CartLoadComponent.h"


void APickUpProduct::ProcessBeginOverlap(AActor* OtherActor)
{
    UCartLoadComponent* LoadedComponent = OtherActor->GetComponentByClass<UCartLoadComponent>();
    if (!IsValid(LoadedComponent)) return;

    // 적재 실패시 반환
    if (!LoadedComponent->TryAddProduct(this)) return;
}
