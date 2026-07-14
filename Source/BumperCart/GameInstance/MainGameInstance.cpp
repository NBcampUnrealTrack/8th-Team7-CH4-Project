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

// 플레이 예정 인원 수 설정
void UMainGameInstance::SetExpectedPlayerCount(int32 InCount)
{
    ExpectedPlayerCount = InCount;
}

// 플레이 예정 인원 수 조회
int32 UMainGameInstance::GetExpectedPlayerCount() const
{
    return ExpectedPlayerCount;
}

void UMainGameInstance::SetPlayerIndex(const FUniqueNetIdRepl& PlayerId, int32 Index)
{
    if (PlayerId.IsValid() && Index != INDEX_NONE)
    {
        PlayerIndex.Add(PlayerId, Index);
    }
}

int32 UMainGameInstance::GetPlayerIndex(const FUniqueNetIdRepl& PlayerId) const
{
    if (const int32* FoundedIndex = PlayerIndex.Find(PlayerId))
    {
        return *FoundedIndex;
    }
    return INDEX_NONE;
}

int32 UMainGameInstance::GetNextLobbyIndex()
{
    return NextLobbyIndex++;
}

void UMainGameInstance::RemovePlayerIndex(const FUniqueNetIdRepl& PlayerId)
{
    const int32* FoundIndex = PlayerIndex.Find(PlayerId);
    if (!FoundIndex)
    {
        return;
    }

    const int32 RemovedIndex = *FoundIndex;
    PlayerIndex.Remove(PlayerId);

    // 나간 플레이어보다 뒤 순번이었던 사람들은 전부 1씩 당겨줌
    for (auto& Player : PlayerIndex)
    {
        if (Player.Value > RemovedIndex)
        {
            Player.Value -= 1;
        }
    }

    // 다음 발급 번호도 한 칸 당겨서 빈 자리부터 이어지도록 유지
    if (NextLobbyIndex > 0)
    {
        NextLobbyIndex -= 1;
    }
}


void UMainGameInstance::LogAllPlayerIndices() const
{
    UE_LOG(LogTemp, Warning, TEXT("===== [Lobby Index 현황] 총 %d명 (다음 발급 번호: %d) ====="),
        PlayerIndex.Num(), NextLobbyIndex);

    TArray<TPair<FUniqueNetIdRepl, int32>> SortedEntries;
    for (const auto& Pair : PlayerIndex)
    {
        SortedEntries.Add(Pair);
    }
    SortedEntries.Sort([](const TPair<FUniqueNetIdRepl, int32>& A, const TPair<FUniqueNetIdRepl, int32>& B)
    {
        return A.Value < B.Value;
    });

    for (const auto& Entry : SortedEntries)
    {
        // ToString() 대신 GetTypeHash로 식별 (모듈 링크 불필요)
        UE_LOG(LogTemp, Warning, TEXT("  - PlayerId Hash: %u / Index: %d"),
            GetTypeHash(Entry.Key), Entry.Value);
    }

    UE_LOG(LogTemp, Warning, TEXT("=========================================="));
}
