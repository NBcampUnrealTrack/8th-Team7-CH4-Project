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
    void SetReady(bool IsReady);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsReady() const { return bIsReady; }


    // 플레이어가 캐릭터 선택
    UFUNCTION(BlueprintCallable, Category = "Lobby|Character")
    int32 SelectCharacter(int32 CharacterIndex);

    // 플레이어가 선택한 캐릭터 index 조회
    UFUNCTION(BlueprintCallable, Category = "Lobby|Character")
    int32 GetSelectedCharacterIndex() const;

    void RestoreSelectedCharacterFromGameInstance();


protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    //클라이언트의 준비완료 호출
    UFUNCTION(Server, Reliable)
    void Server_SetReady(bool IsReady);

    //서버에서 실제로 준비 완료 값을 바꾸는 함수
    void ApplyReady(bool IsReady);

    UPROPERTY(ReplicatedUsing = OnRep_IsReady)
    bool bIsReady = false;

    UFUNCTION()
    void OnRep_IsReady();

    UFUNCTION(Server, Reliable)
    void Server_SelectCharacter(int32 CharacterIndex);

    void ApplySelectCharacter(int32 CharacterIndex);

    UFUNCTION()
    void OnRep_SelectedCharacter();

    int32 GetSelecctedCharacterIndex() const;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacter)
    int32 SelectedCharacterIndex = INDEX_NONE;



};
