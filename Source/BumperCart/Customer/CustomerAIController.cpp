#include "Customer/CustomerAIController.h"

#include "BehaviorTree/BehaviorTree.h"

ACustomerAIController::ACustomerAIController()
{
    BehaviorTreeAsset = nullptr;
}

void ACustomerAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);
    }


}
