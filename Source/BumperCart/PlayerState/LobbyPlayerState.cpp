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

//준비 완료 호출 받기
void ALobbyPlayerState::SetReady(bool bInReady)
{

    if (HasAuthority())
    {
        ApplyReady(bInReady);
    }
    else
    {
        Server_SetReady(bInReady);
    }

}

//클라이언트의 준비완료 호출 처리
void ALobbyPlayerState::Server_SetReady_Implementation(bool bInReady)
{
    ApplyReady(bInReady);
}

//준비완료 적용
void ALobbyPlayerState::ApplyReady(bool bInReady)
{
    if (!HasAuthority()) return;

    if (bIsReady == bInReady) return;

    bIsReady = bInReady;

    //서버가 직접 호출하면 OnRep이 호출되지 않으로 직접 GameState 갱신
    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->RefreshPlayerInfos();
    }
}

//준비완료 시 자동 호출
void ALobbyPlayerState::OnRep_IsReady()
{
    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->OnLobbyPlayersChanged.Broadcast();
    }
}
