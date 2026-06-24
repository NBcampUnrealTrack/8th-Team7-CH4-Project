#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "LobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);

UCLASS()
class BUMPERCART_API ALobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
    // 클라이언트(위젯)가 구독할 델리게이트.
    // 이름 목록이 바뀔 때마다 호출됨 (서버/클라이언트 양쪽에서 호출됨)
    UPROPERTY(BlueprintAssignable, Category = "Lobby")
    FOnLobbyPlayersChanged OnLobbyPlayersChanged;

    // 서버에서만 호출: PlayerArray를 읽어서 ReplicatedPlayerNames를 다시 채움
    void RefreshPlayerNames();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:
    UFUNCTION()
    void OnRep_PlayerNames();

    UPROPERTY(ReplicatedUsing = OnRep_PlayerNames)
    TArray<FString> ReplicatedPlayerNames;

public:
    const TArray<FString>& GetReplicatedPlayerNames() const;
};
