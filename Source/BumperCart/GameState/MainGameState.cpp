// MainGameState.cpp


#include "GameState/MainGameState.h"

#include "Net/UnrealNetwork.h"

void AMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMainGameState, CurrentPhase);
    DOREPLIFETIME(AMainGameState, RoundStartServerTime);
    DOREPLIFETIME(AMainGameState, FinalWinnerNames)
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

//서버에서만 호출 - 최종 1등 명단 저장
void AMainGameState::SetFinalWinners(const TArray<FString>& InWinnerNames)
{
    if (!HasAuthority())
    {
        return;
    }

    FinalWinnerNames = InWinnerNames;
}

//최종 1등 명단 조회
TArray<FString> AMainGameState::GetFinalWinners() const
{
    return FinalWinnerNames;
}
