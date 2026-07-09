#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "LobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);

class CharacterSelectionConfig;

USTRUCT(BlueprintType)
struct FLobbyPlayerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    FString PlayerName;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    bool bIsReady = false;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    bool bIsHost = false;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    int32 SelectedCharacterIndex = INDEX_NONE;
};

UCLASS()
class BUMPERCART_API ALobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
    // 이름 목록이 바뀔 때마다 호출됨
    UPROPERTY(BlueprintAssignable, Category = "Lobby")
    FOnLobbyPlayersChanged OnLobbyPlayersChanged;

    //플레이어 정보 갱신
    void RefreshPlayerInfos(APlayerState* ExcludedPlayerState = nullptr);

    //시작전 모든 플레이어 준비 완료 확인
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool bIsAllPlayersReady() const;

    //복사된 플레이어 정보 조회
    UFUNCTION(BlueprintPure, Category = "Lobby ")
    const TArray<FLobbyPlayerInfo>& GetReplicatedPlayerInfos() const;

    // 선택 가능한 캐릭터 조회
    UFUNCTION(BlueprintPure, Category = "Lobby|Character")
    const TArray<FCharacterData>& GetAvailableCharacters() const;

    // 다른 플레이어가 선택한 캐릭터인지 확인
    bool IsCharacterIndexSelectedByOtherPlayer(int32 CharacterIndex, const APlayerState* PS) const;

    // 배정되지 않은 캐릭터 중 가장 앞선 index 조회
    int32 GetNextAvailableCharacterIndex() const;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:
    UFUNCTION()
    void OnRep_PlayerInfos();

    UPROPERTY(ReplicatedUsing = OnRep_PlayerInfos)
    TArray<FLobbyPlayerInfo> ReplicatedPlayerInfos;

    //호스트인지 확인
    bool IsHostPlayerState(const APlayerState* PS) const;


};
