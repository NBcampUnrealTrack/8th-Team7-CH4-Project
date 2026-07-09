#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_DetermineShoppingIntent.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API UBTTask_DetermineShoppingIntent : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_DetermineShoppingIntent();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    UPROPERTY(EditAnywhere, Category = "Shopping Logic")
    float ShoppingIntentChance;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsTargetPointKey;


};
