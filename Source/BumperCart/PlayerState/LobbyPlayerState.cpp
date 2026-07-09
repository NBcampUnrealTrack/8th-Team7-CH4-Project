#include "PlayerState/LobbyPlayerState.h"

#include "DataAsset/CharacterSelectionConfig.h"
#include "GameInstance/MainGameInstance.h"
#include "GameState/LobbyGameState.h"
#include "Net/UnrealNetwork.h"

ALobbyPlayerState::ALobbyPlayerState()
{
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALobbyPlayerState, bIsReady);
    DOREPLIFETIME(ALobbyPlayerState, SelectedCharacterIndex);
}

//준비 완료 호출 받기
void ALobbyPlayerState::SetReady(bool IsReady)
{
    UE_LOG(LogTemp, Warning, TEXT("플레이어 준비 상태"))
    if (HasAuthority())
    {
        ApplyReady(IsReady);
    }
    else
    {
        Server_SetReady(IsReady);
    }

}

//클라이언트의 준비완료 호출 처리
void ALobbyPlayerState::Server_SetReady_Implementation(bool IsReady)
{
    ApplyReady(IsReady);
}

//준비완료 적용
void ALobbyPlayerState::ApplyReady(bool IsReady)
{
    if (!HasAuthority()) return;

    if (bIsReady == IsReady) return;

    ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;


    bIsReady = IsReady;
    UE_LOG(LogTemp, Warning, TEXT("플레이어 준비 완료"))


    if (GS)
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


void ALobbyPlayerState::SelectCharacter(int32 CharacterIndex)
{
    if (HasAuthority())
    {
        ApplySelectCharacter(CharacterIndex);
    }
    else
    {
        Server_SelectCharacter(CharacterIndex);
    }
}

void ALobbyPlayerState::Server_SelectCharacter_Implementation(int32 CharacterIndex)
{
    ApplySelectCharacter(CharacterIndex);
}

void ALobbyPlayerState::ApplySelectCharacter(int32 CharacterIndex)
{
    //서버 플레이어만 호출 가능
    if (!HasAuthority()) return;
    //if (bIsReady) return;

    ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
    if (!GS) return;

    //다른 플레이어가 선택한 캐릭터인지 체크
    if (GS->IsCharacterIndexSelectedByOtherPlayer(CharacterIndex, this)) return;

    if (SelectedCharacterIndex == CharacterIndex) return;

    SelectedCharacterIndex = CharacterIndex;
    GS->RefreshPlayerInfos();

    if (UMainGameInstance* MainGI = GetWorld()->GetGameInstance<UMainGameInstance>())
    {
        MainGI->SetPlayerCharacter(GetUniqueId(), SelectedCharacterIndex);
    }

    // 정상작동 확인 로그
    FString CharacterName = TEXT("Unknown");
    if (const TArray<FCharacterData>& Characters = GS->GetAvailableCharacters();
        Characters.IsValidIndex(CharacterIndex))
    {
        CharacterName = Characters[CharacterIndex].DisplayName.ToString();
    }

    UE_LOG(LogTemp, Warning, TEXT("[캐릭터 선택] 플레이어: %s / 선택한 캐릭터: %s (Index: %d)"),
        *GetPlayerName(), *CharacterName, CharacterIndex);
}

void ALobbyPlayerState::OnRep_SelectedCharacter()
{
    if (ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr)
    {
        GS->OnLobbyPlayersChanged.Broadcast();
    }
}

int32 ALobbyPlayerState::GetSelectedCharacterIndex() const
{
    return SelectedCharacterIndex;
}
