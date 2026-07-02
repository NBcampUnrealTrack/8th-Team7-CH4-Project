// MainGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"


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
};
