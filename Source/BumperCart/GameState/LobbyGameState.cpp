#include "GameState/LobbyGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/LobbyPlayerState.h"


void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ReplicatedPlayerInfos);
}

//플레이어 정보 갱신
void ALobbyGameState::RefreshPlayerInfos()
{
    if (!HasAuthority()) return;

    ReplicatedPlayerInfos.Reset();

    for (APlayerState* PS : PlayerArray)
    {
        if (!PS)
        {
            continue;
        }

        FLobbyPlayerInfo Info;

        FString DisplayName = PS->GetPlayerName();
        Info.PlayerName = DisplayName.IsEmpty() ? TEXT("(접속 중...)") : DisplayName;

        Info.bIsHost = IsHostPlayerState(PS);

        if (const ALobbyPlayerState* LPS = Cast<ALobbyPlayerState>(PS))
        {
            Info.bIsReady = LPS->IsReady();
        }


        ReplicatedPlayerInfos.Add(Info);
    }

    OnLobbyPlayersChanged.Broadcast();
}

//모든 플레이어 준비 완료했는지 확인
bool ALobbyGameState::bIsAllPlayersReady() const
{
    for (APlayerState* PS : PlayerArray)
    {
        if (!PS) continue;

        if (IsHostPlayerState(PS)) continue;

        const ALobbyPlayerState* LPS = Cast<ALobbyPlayerState>(PS);
        if (!LPS || !LPS->IsReady()) return false;
    }
    return true;
}

//플레이어의 정보가 바뀔 시 호출
void ALobbyGameState::OnRep_PlayerInfos()
{
    OnLobbyPlayersChanged.Broadcast();
}

//플레이어 정보 제공(
const TArray<FLobbyPlayerInfo>& ALobbyGameState::GetReplicatedPlayerInfos() const
{ return ReplicatedPlayerInfos; }

//호스트인지 확인
bool ALobbyGameState::IsHostPlayerState(const APlayerState* PS) const
{
    if (!PS) return false;

    const APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
    return PC && PC->GetNetConnection() == nullptr;
}
