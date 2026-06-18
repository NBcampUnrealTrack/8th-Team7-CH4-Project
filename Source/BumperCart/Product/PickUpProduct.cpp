// Fill out your copyright notice in the Description page of Project Settings.


#include "PickUpProduct.h"


void APickUpProduct::ProcessBeginOverlap(AActor* OtherActor)
{
    SetProductState(EProductState::Loaded);
    UE_LOG(LogTemp, Warning, TEXT("상품 적재!!"));
}
