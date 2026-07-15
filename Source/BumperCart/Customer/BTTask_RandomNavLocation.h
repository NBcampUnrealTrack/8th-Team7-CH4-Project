#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RandomNavLocation.generated.h"

/**
 * 
 */
UCLASS()
class BUMPERCART_API UBTTask_RandomNavLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_RandomNavLocation();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 이동할 위치
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;
};
