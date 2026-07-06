// MainGameInstance.cpp


#include "GameInstance/MainGameInstance.h"


// 플레이어가 선택한 캐릭터 저장
void UMainGameInstance::SetPlayerCharacter(const FUniqueNetIdRepl& PlayerId, int32 CharacterIndex)
{
    if (PlayerId.IsValid() && CharacterIndex != INDEX_NONE)
    {
        PlayerCharacterSelections.Add(PlayerId, CharacterIndex);
    }
}

// 플레이어가 선택한 캐릭터 조회
int32 UMainGameInstance::GetPlayerCharacter(const FUniqueNetIdRepl& PlayerId) const
{
    if (const int32* FoundedIndex = PlayerCharacterSelections.Find(PlayerId))
    {
        return *FoundedIndex;
    }
    return INDEX_NONE;
}
