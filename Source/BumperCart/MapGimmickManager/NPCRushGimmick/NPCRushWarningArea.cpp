#include "MapGimmickManager/NPCRushGimmick/NPCRushWarningArea.h"

#include "Components/DecalComponent.h"

ANPCRushWarningArea::ANPCRushWarningArea()
{
    bReplicates = true;
}

void ANPCRushWarningArea::InitWarningDecal(float Duration)
{
    UDecalComponent* TargetDecal = GetDecal();
    if (TargetDecal)
    {
        SetLifeSpan(Duration);
    }
}
