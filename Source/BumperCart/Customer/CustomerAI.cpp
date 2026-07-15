#include "Customer/CustomerAI.h"

#include "Customer/CustomerAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"

ACustomerAI::ACustomerAI()
{
	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);

    AIControllerClass = ACustomerAIController::StaticClass();

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->bUseRVOAvoidance = true;
        MoveComp->MaxWalkSpeed = 300.0f;
        MoveComp->bOrientRotationToMovement = true;
    }
}

void ACustomerAI::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority()) return;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (Actor && Actor->ActorHasTag(FName("ShoppingPoint")))
        {
            if (ATargetPoint* TargetPoint = Cast<ATargetPoint>(Actor))
            {
                CachedTargetPoints.Add(TargetPoint);
            }
        }
    }
}

void ACustomerAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, bIsShoppingState);
}
