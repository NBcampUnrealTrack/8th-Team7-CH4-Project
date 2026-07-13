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

protected:
    UPROPERTY(EditAnywhere, Category = "WarningArea | Sound")
    USoundBase* WarningSound;

    virtual void BeginPlay() override;

public:
    void InitWarningDecal(float Duration);
};
