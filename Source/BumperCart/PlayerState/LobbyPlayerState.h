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

    // 나중에 "준비 완료" 버튼 같은 기능을 붙이고 싶을 때를 위한 예시 필드
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetReady(bool bInReady);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsReady() const { return bIsReady; }

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(ReplicatedUsing = OnRep_IsReady)
    bool bIsReady = false;

    UFUNCTION()
    void OnRep_IsReady();

};
