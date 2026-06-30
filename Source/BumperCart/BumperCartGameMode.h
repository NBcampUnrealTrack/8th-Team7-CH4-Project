// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BumperCartGameMode.generated.h"

class UProductShelfManagerConfig;
class USaleEventConfig;
class ULimitedEventConfig;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ABumperCartGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ABumperCartGameMode();

    virtual void BeginPlay() override;

private:
    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<UProductShelfManagerConfig> ProductShelfManagerConfig;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<USaleEventConfig> SaleEventConfig;

    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<ULimitedEventConfig> LimitedEventConfig;
};



