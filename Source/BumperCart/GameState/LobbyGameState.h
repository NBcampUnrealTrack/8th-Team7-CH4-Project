#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "LobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);

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
    void RefreshPlayerInfos();

    //시작전 모든 플레이어 준비 완료 확인
    bool bIsAllPlayersReady() const;

    //복사된 플레이어 정보 조회
    UPROPERTY(BlueprintReadOnly, Category = "Lobby|PlayerInfos ")
    const TArray<FLobbyPlayerInfo>& GetReplicatedPlayerInfos() const;


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
