#include "MapGimmickManager/NPCRushGimmick/NPCRushWarningArea.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"

ANPCRushWarningArea::ANPCRushWarningArea()
{
    bReplicates = true;
}

void ANPCRushWarningArea::BeginPlay()
{
    Super::BeginPlay();

    if (WarningSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), WarningSound);
    }
}

void ANPCRushWarningArea::InitWarningDecal(float Duration)
{
    UDecalComponent* TargetDecal = GetDecal();
    if (TargetDecal)
    {
        SetLifeSpan(Duration);
    }
}
