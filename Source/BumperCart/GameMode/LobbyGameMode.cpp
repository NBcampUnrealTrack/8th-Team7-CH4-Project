// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameInstance/MainGameInstance.h"
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
        // 로비 입장 순서대로 아직 아무도 선택하지 않은 캐릭터를 자동 배정
        if (ALobbyPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ALobbyPlayerState>() : nullptr)
        {
            const int32 CharacterIndex = GS->GetNextAvailableCharacterIndex();
            if (CharacterIndex != INDEX_NONE)
            {
                PS->SelectCharacter(CharacterIndex);
                UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 에게 캐릭터 index %d 자동 배정"), *NewPlayer->GetName(), CharacterIndex);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 에게 배정 가능한 캐릭터가 없습니다."), *NewPlayer->GetName());
            }
        }

        GS->RefreshPlayerInfos();
    }
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    APlayerState* ExitingPS = Exiting ? Exiting->PlayerState : nullptr;

    Super::Logout(Exiting);
    UE_LOG(LogTemp, Warning, TEXT("플레이어 퇴장: %s"), *Exiting->GetName());

    if (ALobbyGameState* GS = GetGameState<ALobbyGameState>())
    {
        GS->RefreshPlayerInfos(ExitingPS);
    }
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

}

//게임 시작(게임 맵으로 이동)
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

    //모든 플레이어 준비완료 확인
    if (!GS->bIsAllPlayersReady())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lobby] 아직 모든 플레이어가 준비 완료 상태가 아니므로 게임을 시작할 수 없습니다."));
        return;
    }

    if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
    {
        GI->SetExpectedPlayerCount(GS->PlayerArray.Num());
    }

    UMainGameInstance* MainGI = Cast<UMainGameInstance>(GetGameInstance());
    if (!MainGI) return;

    const FString LevelPath = MainGI->GamePlayLevel.ToSoftObjectPath().GetLongPackageName();

    GetWorld()->ServerTravel(LevelPath);
}
