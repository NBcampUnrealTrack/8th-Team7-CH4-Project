#include "Customer/BTTask_RandomNavLocation.h"

#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Customer/CustomerAI.h"
#include "Customer/CustomerAIController.h"

UBTTask_RandomNavLocation::UBTTask_RandomNavLocation()
{
    NodeName = "Random Nav Location";
}

EBTNodeResult::Type UBTTask_RandomNavLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::ExecuteTask(OwnerComp, NodeMemory);

    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (!BBComp) return EBTNodeResult::Failed;

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    ACustomerAI* Customer = Cast<ACustomerAI>(AIController->GetPawn());
    if (!Customer) return EBTNodeResult::Failed;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation RandomLocation;
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(Customer->GetActorLocation(), 1000.f, RandomLocation))
    {
        BBComp->SetValueAsVector(FName("TargetLocation"), RandomLocation.Location);
        return EBTNodeResult::Succeeded;
    }
    return EBTNodeResult::Failed;
}
