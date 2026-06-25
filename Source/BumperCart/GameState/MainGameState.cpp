// MainGameState.cpp


#include "GameState/MainGameState.h"

#include "Net/UnrealNetwork.h"

void AMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMainGameState, CurrentPhase);
    DOREPLIFETIME(AMainGameState, RoundStartServerTime);
}

void AMainGameState::SetRoundPhase(ERoundPhase NewPhase)
{
    if (!HasAuthority())
    {
        return;
    }

    if (CurrentPhase == NewPhase)
    {
        return;
    }

    CurrentPhase = NewPhase;

    OnRoundPhaseChanged.Broadcast();
}

void AMainGameState::SetRoundStartTime()
{
    if (!HasAuthority())
    {
        return;
    }

    RoundStartServerTime = GetServerWorldTimeSeconds();
}

ERoundPhase AMainGameState::GetCurrentPhase() const
{
    return CurrentPhase;
}

float AMainGameState::GetRemainingTime() const
{
    const float Elapsed = GetServerWorldTimeSeconds() - RoundStartServerTime;
    return FMath::Max(0.f, RoundDurationSeconds - Elapsed);
}
void AMainGameState::OnRep_CurrentPhase()
{
    OnRoundPhaseChanged.Broadcast();
}
