#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawnManger.generated.h"

UCLASS()
class BUMPERCART_API AItemSpawnManger : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemSpawnManger();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
