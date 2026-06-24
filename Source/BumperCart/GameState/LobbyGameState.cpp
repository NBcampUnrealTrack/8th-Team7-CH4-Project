#include "GameState/LobbyGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"


void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ReplicatedPlayerNames);
}

void ALobbyGameState::RefreshPlayerNames()
{
    if (!HasAuthority()) return;

    ReplicatedPlayerNames.Reset();

    for (APlayerState* PS : PlayerArray)
    {
        if (!PS)
        {
            continue;
        }

        FString DisplayName = PS->GetPlayerName();
        if (DisplayName.IsEmpty())
        {
            DisplayName = TEXT("(접속 중...)");
        }

        ReplicatedPlayerNames.Add(DisplayName);
    }

    // 서버 자신(호스트)은 OnRep이 호출되지 않으므로 직접 브로드캐스트
    OnLobbyPlayersChanged.Broadcast();
}

void ALobbyGameState::OnRep_PlayerNames()
{
    // 클라이언트에서 배열이 갱신되어 도착했을 때 호출됨
    OnLobbyPlayersChanged.Broadcast();
}
const TArray<FString>& ALobbyGameState::GetReplicatedPlayerNames() const
{ return ReplicatedPlayerNames; }
