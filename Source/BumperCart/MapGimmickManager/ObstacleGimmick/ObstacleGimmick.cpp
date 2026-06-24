// Fill out your copyright notice in the Description page of Project Settings.


#include "MapGimmickManager/ObstacleGimmick/ObstacleGimmick.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AObstacleGimmick::AObstacleGimmick()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(GetRootComponent());

    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterHoleMesh"));
    ObstacleMesh->SetupAttachment(BoxCollision);

}

void AObstacleGimmick::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority())
    {

    }
}

