// Copyright Epic Games, Inc. All Rights Reserved.

#include "BumperCartGameMode.h"
#include "ProductShelfSubsystem/ProductShelfSubsystem.h"
#include "EventManager/BC_EventSubsystem.h"

ABumperCartGameMode::ABumperCartGameMode()
{
	// stub
}

void ABumperCartGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (auto* ProductShelfSubsystem = GetWorld()->GetSubsystem<UProductShelfSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[게임모드] 선반 서브시스템 설정 초기화 실행"));
        ProductShelfSubsystem->InitializeConfig(ProductShelfManagerConfig);
    }

    if (auto* BC_EventSubsystem = GetWorld()->GetSubsystem<UBC_EventSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[게임모드] 세일 이벤트 설정 초기화 실행"));
        BC_EventSubsystem->InitializeSaleEventConfig(SaleEventConfig);
        
        UE_LOG(LogTemp, Log, TEXT("[게임모드] 한정 이벤트 설정 초기화 실행"));
        BC_EventSubsystem->InitializeLimitedEventConfig(LimitedEventConfig);
    }

}
