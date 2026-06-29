#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

UCLASS()
class BUMPERCART_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
    ALobbyPlayerState();

    // 준비 완료
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetReady(bool bInReady);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsReady() const { return bIsReady; }

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    //클라이언트의 준비완료 호출
    UFUNCTION(Server, Reliable)
    void Server_SetReady(bool bInReady);

    //서버에서 실제로 준비 완료 값을 바꾸는 함수
    void ApplyReady(bool bInReady);

    UPROPERTY(ReplicatedUsing = OnRep_IsReady)
    bool bIsReady = false;

    UFUNCTION()
    void OnRep_IsReady();

};
