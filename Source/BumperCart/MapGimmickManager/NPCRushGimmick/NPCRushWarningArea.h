#pragma once

#include "CoreMinimal.h"
#include "Engine/DecalActor.h"
#include "NPCRushWarningArea.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API ANPCRushWarningArea : public ADecalActor
{
	GENERATED_BODY()

public:

    ANPCRushWarningArea();

    void InitWarningDecal(float Duration);
};
