// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProductBase.h"
#include "InteractableProduct.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBeginOverlapInteractableProduct, AProductBase*, Product, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEndOverlapInteractableProduct, AProductBase*, Product, AActor*, Interactor);

UCLASS()
class BUMPERCART_API AInteractableProduct : public AProductBase
{
    GENERATED_BODY()

public:
    AInteractableProduct();

public:
    UPROPERTY(BlueprintAssignable)
    FOnBeginOverlapInteractableProduct OnBeginOverlapInteractableProduct;

    UPROPERTY(BlueprintAssignable)
    FOnBeginOverlapInteractableProduct OnEndOverlapInteractableProduct;

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
    // 상호작용 활성화 하는 함수, BeginOverlap 에서 호출함
    void EnableInteraction(AActor* OtherActor);

    // 상호작용 비활성화 하는 함수, EndOverlap과 습득시 호출함
    void DisableInteraction(AActor* OtherActor);
};
