// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProductBase.h"
#include "PickUpProduct.generated.h"

UCLASS()
class BUMPERCART_API APickUpProduct : public AProductBase
{
    GENERATED_BODY()

protected:
    // 충돌 시 적재하는 함수
    virtual void ProcessBeginOverlap(AActor* OtherActor) override;
};
