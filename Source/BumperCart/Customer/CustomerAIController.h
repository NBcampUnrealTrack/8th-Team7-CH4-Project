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

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnPossess(APawn* InPawn) override;

    

private:
    UPROPERTY(EditDefaultsOnly, Category = "AI | Setting")
    TObjectPtr<UBehaviorTree> BehaviorTree;

    UPROPERTY(EditDefaultsOnly, Category = "AI | Setting")
    TObjectPtr<UBlackboardData> BlackboardDataAsset;

    UPROPERTY(EditAnywhere, Category = "AI | Setting")
    float PatrolRadius = 500.0f;

public:
    
};
