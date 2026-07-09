// MainGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"


class UCharacterSelectionConfig;

UCLASS()
class BUMPERCART_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    // 레벨 경로
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    TSoftObjectPtr<UWorld> TitleLevel;
    UPROPERTY(EditDefaultsOnly,  Category = "Level")
    TSoftObjectPtr<UWorld> LobbyLevel;
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    TSoftObjectPtr<UWorld> GamePlayLevel;


    // 플레이 예정 인원 수
    int32 ExpectedPlayerCount = 0;
    // 플레이 예정 인원 수 설정
    void SetExpectedPlayerCount(int32 InCount);
    // 플레이 예정 인원 수 조회
    int32 GetExpectedPlayerCount() const;

#pragma region Character
public:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Character")
    TObjectPtr<UCharacterSelectionConfig> CharacterSelectionConfig;

    // 플레이어가 로비에서 준비 완료 시 확정한 캐릭터를 저장
    UFUNCTION(BlueprintCallable, Category = "Lobby|Character")
    void SetPlayerCharacter(const FUniqueNetIdRepl& PlayerId, int32 CharacterIndex);

    // 플레이어 캐릭터 조회
    UFUNCTION(BlueprintCallable, Category = "Lobby|Character")
    int32 GetPlayerCharacter(const FUniqueNetIdRepl& PlayerId) const;

private:
    // 플레이어 아이디 - 선택한 캐릭터
    UPROPERTY()
    TMap<FUniqueNetIdRepl, int32> PlayerCharacterSelections;
#pragma endregion
};
