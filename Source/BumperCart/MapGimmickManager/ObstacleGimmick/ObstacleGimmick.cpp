// Fill out your copyright notice in the Description page of Project Settings.


#include "MapGimmickManager/ObstacleGimmick/ObstacleGimmick.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AObstacleGimmick::AObstacleGimmick()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterHoleMesh"));
    RootComponent = ObstacleMesh;

}

void AObstacleGimmick::BeginPlay()
{
	Super::BeginPlay();

}

