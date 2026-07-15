#include "GameState/LobbyGameState.h"

#include "VectorUtil.h"
#include "DataAsset/CharacterSelectionConfig.h"
#include "GameFramework/PlayerState.h"
#include "GameInstance/MainGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/LobbyPlayerState.h"


void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ReplicatedPlayerInfos);
}

// 플레이어 정보 갱신
void ALobbyGameState::RefreshPlayerInfos(APlayerState* ExcludedPlayerState)
{
    if (!HasAuthority()) return;

    ReplicatedPlayerInfos.Reset();

    UMainGameInstance* GI = GetGameInstance<UMainGameInstance>();

    TArray<APlayerState*> SortedPlayers = PlayerArray;
    SortedPlayers.Sort([GI](const APlayerState& A, const APlayerState& B)
    {
        const int32 IdxA = GI ? GI->GetPlayerIndex(A.GetUniqueId()) : INDEX_NONE;
        const int32 IdxB = GI ? GI->GetPlayerIndex(B.GetUniqueId()) : INDEX_NONE;
        return IdxA < IdxB;
    });


    for (APlayerState* PS : PlayerArray)
    {
        // 갱신 제외할 플레이어 정보가 있을 경우 패스
        if (!PS || PS == ExcludedPlayerState)
        {
            continue;
        }

        FLobbyPlayerInfo Info;

        FString DisplayName = PS->GetPlayerName();
        Info.PlayerName = DisplayName.IsEmpty() ? TEXT("(접속 중...)") : DisplayName;

        Info.bIsHost = IsHostPlayerState(PS);

        if (ALobbyPlayerState* LPS = Cast<ALobbyPlayerState>(PS))
        {
            LPS->RestoreSelectedCharacterFromGameInstance();

            Info.bIsReady = LPS->IsReady();
            Info.SelectedCharacterIndex = LPS->GetSelectedCharacterIndex();
        }


        ReplicatedPlayerInfos.Add(Info);
    }

    OnLobbyPlayersChanged.Broadcast();
}

// 모든 플레이어 준비 완료했는지 확인
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

// 플레이어의 정보가 바뀔 시 호출
void ALobbyGameState::OnRep_PlayerInfos()
{
    OnLobbyPlayersChanged.Broadcast();
}

// 플레이어 정보 제공
const TArray<FLobbyPlayerInfo>& ALobbyGameState::GetReplicatedPlayerInfos() const
{ return ReplicatedPlayerInfos; }

// 호스트인지 확인
bool ALobbyGameState::IsHostPlayerState(const APlayerState* PS) const
{
    if (!PS) return false;

    const APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
    return PC && PC->GetLocalPlayer() != nullptr;
}

const TArray<FCharacterData> ALobbyGameState::GetAvailableCharacters() const
{
    const TArray<FCharacterData> Empty;
    if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
    {
        if (GI->CharacterSelectionConfig)
        {
            return GI->CharacterSelectionConfig->CharacterDatas;
        }
    }
    return TArray<FCharacterData>();
}

// 플레이어가 선택한 캐릭터가 이미 다른 플레이어가 선택한 캐릭터인지 확인
bool ALobbyGameState::IsCharacterIndexSelectedByOtherPlayer(int32 CharacterIndex, const APlayerState* ReqeustPS) const
{
    if (CharacterIndex == INDEX_NONE) return false;

    for (APlayerState* PS : PlayerArray)
    {
        if (!PS || PS == ReqeustPS) continue;

        if (ALobbyPlayerState* LPS = Cast<ALobbyPlayerState>(PS))
        {
            if (LPS->GetSelectedCharacterIndex() == CharacterIndex)
            {
                return true;
            }
        }
    }
    return false;
}

int32 ALobbyGameState::GetNextAvailableCharacterIndex() const
{
    const TArray<FCharacterData>& Characters = GetAvailableCharacters();

    for (int32 index = 0; index < Characters.Num(); index++)
    {
        if (!IsCharacterIndexSelectedByOtherPlayer(index, nullptr))
        {
            return index;
        }
    }
    return INDEX_NONE;
}
