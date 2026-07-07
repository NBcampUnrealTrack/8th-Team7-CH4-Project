#include "MapGimmickManager/NPCRushGimmick/NPCRushWarningArea.h"

#include "Components/DecalComponent.h"

void ANPCRushWarningArea::InitWarningDecal(float Duration)
{
    UDecalComponent* TargetDecal = GetDecal();
    if (TargetDecal)
    {

        SetLifeSpan(Duration);
    }
}
