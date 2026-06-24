#include "PlayerState/LobbyPlayerState.h"

#include "GameState/LobbyGameState.h"
#include "Net/UnrealNetwork.h"

ALobbyPlayerState::ALobbyPlayerState()
{
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALobbyPlayerState, bIsReady);
}

void ALobbyPlayerState::SetReady(bool bInReady)
{
    // 서버에서만 값을 바꿀 수 있게 제한
    if (!HasAuthority())
    {
        return;
    }

    if (bIsReady == bInReady)
    {
        return;
    }

    bIsReady = bInReady;

    // 서버 자신(호스트)은 OnRep이 안 불리므로 직접 GameState 갱신 트리거
    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->RefreshPlayerNames();
    }
}

void ALobbyPlayerState::OnRep_IsReady()
{
    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->OnLobbyPlayersChanged.Broadcast();
    }
}
