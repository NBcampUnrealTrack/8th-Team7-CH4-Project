#include "Customer/CustomerAI.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ACustomerAI::ACustomerAI()
{
	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->bUseRVOAvoidance = true;
        MoveComp->MaxWalkSpeed = 300.0f;
        MoveComp->bOrientRotationToMovement = true;
    }

    CurrentState = ECustomerState::Patrolling;
    MinShoppingTime = 5.0f;
    MaxShoppingTime = 15.0f;
}

void ACustomerAI::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACustomerAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, CurrentState);
}

void ACustomerAI::OnRep_CustomerState()
{
    switch (CurrentState)
    {
    case ECustomerState::Patrolling:
        break;

    case ECustomerState::Shopping:
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->StopMovementImmediately();
        }
        break;

    case ECustomerState::KnockedOut:
        break;
    }
}

void ACustomerAI::StartShoppingAtShelf()
{
    if (!HasAuthority()) return;

    if (CurrentState == ECustomerState::Patrolling)
    {
        CurrentState = ECustomerState::Shopping;
        OnRep_CustomerState();
    }
}

void ACustomerAI::ResumePatrol()
{
    if (!HasAuthority()) return;

    if (CurrentState == ECustomerState::Shopping)
    {
        CurrentState = ECustomerState::Patrolling;

        if (GetCharacterMovement())
        {
            GetCharacterMovement()->MaxWalkSpeed = 300.0f;
        }

        OnRep_CustomerState();
    }
}

void ACustomerAI::SetKnockedOut()
{
    if (!HasAuthority()) return;

    CurrentState = ECustomerState::KnockedOut;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }

    OnRep_CustomerState();
}
