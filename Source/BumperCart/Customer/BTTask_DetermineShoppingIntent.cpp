#include "Customer/BTTask_DetermineShoppingIntent.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Customer/CustomerAI.h"
#include "Customer/CustomerAIController.h"
#include "Engine/TargetPoint.h"

UBTTask_DetermineShoppingIntent::UBTTask_DetermineShoppingIntent()
{
    NodeName = "Determine Shopping Intent";
    ShoppingIntentChance = 0.5f;
}

EBTNodeResult::Type UBTTask_DetermineShoppingIntent::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::ExecuteTask(OwnerComp, NodeMemory);

    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (!BBComp) return EBTNodeResult::Failed;

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    ACustomerAI* Customer = Cast<ACustomerAI>(AIController->GetPawn());
    if (!Customer) return EBTNodeResult::Failed;

    if (!Customer->HasAuthority()) return EBTNodeResult::Failed;

    const TArray<TWeakObjectPtr<ATargetPoint>>& Points = Customer->GetCachedTargetPoints();

    if (Points.Num() == 0)   return EBTNodeResult::Failed;

    if (FMath::FRand() >= ShoppingIntentChance)
    {
        Customer->SetIsShoppingState(false);
        BBComp->SetValueAsBool(FName("IsShopping"), false);

        return EBTNodeResult::Succeeded;
    }

    int32 RandomIndex = FMath::RandRange(0, Points.Num() - 1);

    if (Points[RandomIndex].IsValid())
    {
        Customer->SetIsShoppingState(true);

        ATargetPoint* ChosenPoint = Points[RandomIndex].Get();
        FVector TargetLoc = ChosenPoint->GetActorLocation();

        BBComp->SetValueAsVector(FName("TargetLocation"), TargetLoc);
        BBComp->SetValueAsBool(FName("IsShopping"), true);
        BBComp->SetValueAsObject(FName("TargetPoint"), ChosenPoint);

        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}
