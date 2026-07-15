#include "Customer/CustomerAIController.h"

#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

ACustomerAIController::ACustomerAIController()
{
    bReplicates = true;
}

void ACustomerAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!HasAuthority()) return;

    if (CustomerBehaviorTree)
    {
        RunBehaviorTree(CustomerBehaviorTree);
    }
}
