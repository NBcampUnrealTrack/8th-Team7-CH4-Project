#include "Customer/BTTask_DetermineShoppingIntent.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"

UBTTask_DetermineShoppingIntent::UBTTask_DetermineShoppingIntent()
{
    NodeName = "Determine Shopping Intent";
    ShoppingIntentChance = 0.3f;
}

EBTNodeResult::Type UBTTask_DetermineShoppingIntent::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (!BBComp) return EBTNodeResult::Failed;

    if (FMath::FRand() <= ShoppingIntentChance)
    {
        TArray<AActor*> FoundTargetPoints;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), FoundTargetPoints);

        if (FoundTargetPoints.Num() > 0)
        {
            int32 RandomIndex = FMath::RandRange(0, FoundTargetPoints.Num() - 1);
            AActor* SelectedPoint = FoundTargetPoints[RandomIndex];

            if (SelectedPoint)
            {
                BBComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, SelectedPoint->GetActorLocation());
                BBComp->SetValueAsBool(IsTargetPointKey.SelectedKeyName, true);

                return EBTNodeResult::Succeeded;
            }
        }

        BBComp->SetValueAsBool(IsTargetPointKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;

    }
    return EBTNodeResult::Type();
}
