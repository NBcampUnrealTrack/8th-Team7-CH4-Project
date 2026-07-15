#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CustomerAIController.generated.h"

class UBehaviorTree;
class UBlackboardData;

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

protected:
    UPROPERTY(EditDefaultsOnly, Category = "AI | Setting")
    TObjectPtr<UBehaviorTree> CustomerBehaviorTree;

};
