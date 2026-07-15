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

    // 쇼핑 할 확률
    UPROPERTY(EditAnywhere, Category = "Shopping Logic")
    float ShoppingIntentChance;

    // 이동할 위치
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    // 쇼핑 행동 확인
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsShoppingKey;


};
