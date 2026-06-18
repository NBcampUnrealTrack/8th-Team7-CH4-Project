#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestEventManager.generated.h"

UCLASS()
class BUMPERCART_API ATestEventManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ATestEventManager();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region Event
private:
    UPROPERTY(EditAnywhere, Category = "SaleEvent")
    float SaleEventTime = 30;

    FTimerHandle SaleEventTimerHandle;

public:
    void SaleEvent();
#pragma endregion

};
