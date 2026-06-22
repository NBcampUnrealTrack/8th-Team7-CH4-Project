#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterHoleGimmick.generated.h"

UCLASS()
class BUMPERCART_API AWaterHoleGimmick : public AActor
{
	GENERATED_BODY()
	
public:	
    AWaterHoleGimmick();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion


};
