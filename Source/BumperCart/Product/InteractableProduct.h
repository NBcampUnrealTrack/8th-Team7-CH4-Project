// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProductBase.h"
#include "InteractableProduct.generated.h"

UCLASS()
class BUMPERCART_API AInteractableProduct : public AProductBase
{
    GENERATED_BODY()

public:
    AInteractableProduct();

    // 누군가가 카트에 적재할 경우 호출하는 함수, 기존에 접촉했던 플레이어에게 알려야 함
    UFUNCTION()
    void OnLoaded();

protected:

    // 범위 내에 들어오면 상호작용 활성화 하는 함수
    virtual void ProcessBeginOverlap(AActor* OtherActor) override;

    UFUNCTION()
    void OnEndOverlapCart(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

private:
    // 상호작용 비활성화 하는 함수, EndOverlap과 습득시 호출함
    void DisableInteraction(AActor* OtherActor);

    UPROPERTY()
    TSet<AActor*> OverlappedActors;
};
