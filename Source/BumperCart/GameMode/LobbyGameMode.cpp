// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameState/LobbyGameState.h"
#include "PlayerState/LobbyPlayerState.h"

ALobbyGameMode::ALobbyGameMode()
{
    GameStateClass = ALobbyGameState::StaticClass();
    PlayerStateClass = ALobbyPlayerState::StaticClass();

    bUseSeamlessTravel = true;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("플레이어 접속: %s"), *NewPlayer->GetName());
    }

    if (ALobbyGameState* GS = GetGameState<ALobbyGameState>())
    {
        GS->RefreshPlayerNames();
    }
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    UE_LOG(LogTemp, Warning, TEXT("플레이어 퇴장: %s"), *Exiting->GetName());

    if (ALobbyGameState* GS = GetGameState<ALobbyGameState>())
    {
        GS->RefreshPlayerNames();
    }
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

}

void ALobbyGameMode::StartGame()
{
    if (!HasAuthority()) return;

    ALobbyGameState* GS = GetGameState<ALobbyGameState>();
    if (!GS || GS->PlayerArray.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lobby] 인원이 부족하여 게임을 시작할 수 없습니다. 현재 인원: %d"),
            GS ? GS->PlayerArray.Num() : 0);
        return;
    }

    GetWorld()->ServerTravel(TEXT("/Game/Developers/LSJae/Levels/TestPlayLevel"));
}
