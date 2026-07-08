#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CustomerAIController.generated.h"

class  UBehaviorTree;

/**
 * 
 */
UCLASS()
class BUMPERCART_API ACustomerAIController : public AAIController
{
	GENERATED_BODY()

public:
    ACustomerAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;

    UPROPERTY(EditAnywhere, Category = "AI | Setup")
    UBehaviorTree* BehaviorTreeAsset;
};
