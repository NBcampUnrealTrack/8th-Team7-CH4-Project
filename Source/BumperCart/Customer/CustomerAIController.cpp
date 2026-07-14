#include "Customer/CustomerAIController.h"

#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

ACustomerAIController::ACustomerAIController()
{
    BehaviorTree = nullptr;
    BlackboardDataAsset = nullptr;
}

void ACustomerAIController::BeginPlay()
{
    Super::BeginPlay();
}

void ACustomerAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ACustomerAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (BehaviorTree)
    {
        RunBehaviorTree(BehaviorTree);
    }
}
