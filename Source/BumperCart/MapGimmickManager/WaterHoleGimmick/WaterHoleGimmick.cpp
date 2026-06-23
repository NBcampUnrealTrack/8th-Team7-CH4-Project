#include "MapGimmickManager/WaterHoleGimmick/WaterHoleGimmick.h"

#include "Components/BoxComponent.h"

AWaterHoleGimmick::AWaterHoleGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(GetRootComponent());

    WaterHoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterHoleMesh"));
    WaterHoleMesh->SetupAttachment(BoxCollision);

}

void AWaterHoleGimmick::BeginPlay()
{
	Super::BeginPlay();
	
}
